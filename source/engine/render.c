#include "render.h"

#include "camera.h"
#include "config.h"
#include "datastream.h"
#include "debugdraw.h"
#include "font.h"
#include "gameobject.h"
#include "log.h"
#include "nuke.h"
#include "shader.h"
#include "sprite.h"
#include "window.h"
#include "model.h"
#include "stb_ds.h"
#include "resources.h"
#include "yugine.h"

#include "crt.sglsl.h"
#include "box.sglsl.h"
#include "shadow.sglsl.h"

#define SOKOL_TRACE_HOOKS
#define SOKOL_GFX_IMPL
#include "sokol/sokol_gfx.h"

#define SOKOL_GFX_EXT_IMPL
#include "sokol/sokol_gfx_ext.h"

#define MSF_GIF_IMPL
#include "msf_gif.h"

static struct {
  sg_pass pass;
  sg_pass_action pa;
  sg_pipeline pipe;
  sg_bindings bind;
  sg_shader shader;
  sg_image img;
  sg_image depth;
} sg_gif;

static struct {
  int w;
  int h;
  int cpf;
  int depth;
  double timer;
  double spf;
  int rec;
  char *buffer;
} gif;

MsfGifState gif_state = {};
void gif_rec_start(int w, int h, int cpf, int bitdepth)
{
  gif.w = w;
  gif.h = h;
  gif.depth = bitdepth;
  msf_gif_begin(&gif_state, gif.w, gif.h);
  gif.cpf = cpf;
  gif.spf = cpf/100.0;
  gif.rec = 1;
  gif.timer = appTime;
  if (gif.buffer) free(gif.buffer);
  gif.buffer = malloc(gif.w*gif.h*4);

  sg_destroy_image(sg_gif.img);
  sg_destroy_image(sg_gif.depth);
  sg_destroy_pass(sg_gif.pass);

  sg_gif.img = sg_make_image(&(sg_image_desc){
    .render_target = true,
    .width = gif.w,
    .height = gif.h,
  });

  sg_gif.depth = sg_make_image(&(sg_image_desc){
    .render_target = true,
    .width = gif.w,
    .height = gif.h,
    .pixel_format = SG_PIXELFORMAT_DEPTH_STENCIL
  });
  
  sg_gif.pass = sg_make_pass(&(sg_pass_desc){
    .color_attachments[0].image = sg_gif.img,
    .depth_stencil_attachment.image = sg_gif.depth
  });
}

void gif_rec_end(char *path)
{
  if (!gif.rec) return;
  
  MsfGifResult gif_res = msf_gif_end(&gif_state);
  if (gif_res.data) {
    FILE *f = fopen(path, "wb");
    fwrite(gif_res.data, gif_res.dataSize, 1, f);
    fclose(f);
  }
  msf_gif_free(gif_res);
  gif.rec = 0;
}

#include "sokol/sokol_app.h"

#include "HandmadeMath.h"

int renderMode = LIT;

struct shader *spriteShader = NULL;
struct shader *wireframeShader = NULL;
struct shader *animSpriteShader = NULL;
static struct shader *textShader;

struct rgba editorClearColor = {35,60,92,255};

float shadowLookahead = 8.5f;

struct rgba gridSmallColor = {
  .r = 255 * 0.35f,
  .g = 255,
  .b = 255 * 0.9f
};

struct rgba gridBigColor = {
  .r = 255 * 0.92f,
  .g = 255 * 0.92f,
  .b = 255 * 0.68f
};

float gridScale = 500.f;
float smallGridUnit = 1.f;
float bigGridUnit = 10.f;
float gridSmallThickness = 2.f;
float gridBigThickness = 7.f;
float gridOpacity = 0.3f;

// Debug render modes
bool renderGizmos = false;
bool showGrid = true;
bool debugDrawPhysics = false;
bool renderNav = false;

// Lighting effect flags
bool renderAO = true;
bool renderDynamicShadows = true;
bool renderRefraction = true;
bool renderReflection = true;

///// for editing
struct gameobject *selectedobject = NULL;
char objectName[200] = {'\0'}; // object name buffer

sg_image ddimg;

void debug_draw_phys(int draw) {
  debugDrawPhysics = draw;
}

void opengl_rendermode(enum RenderMode r) {
  renderMode = r;
}

sg_pipeline mainpip;
sg_pass_action pass_action = {0};

static struct {
  sg_pass_action pass_action;
  sg_pass pass;
  sg_pipeline pipe;
  sg_shader shader;
} sg_shadow;

static struct {
  sg_shader shader;
  sg_pipeline pipe;
  sg_bindings bind;
  sg_pass pass;
  sg_image img;
  sg_image depth_img;
} crt_post;


void trace_make_shader(sg_shader_desc *d, sg_shader result, void *data)
{
  if (sg_query_shader_state(result) == SG_RESOURCESTATE_FAILED)
    YughError("FAILED MAKING A SHADER: %s\n%s\n%s", d->label);
}

void trace_fail_shader(sg_shader id, void *data)
{
  YughWarn("SHADER DID NOT COMPILE");
}

void trace_destroy_shader(sg_shader shd, void *data)
{
  YughWarn("DESTROYED SHADER");
}

static sg_trace_hooks hooks = {
  .fail_shader = trace_fail_shader,
  .make_shader = trace_make_shader,
  .destroy_shader = trace_destroy_shader,
};

void render_init() {
  mainwin.width = sapp_width();
  mainwin.height = sapp_height();

  sg_setup(&(sg_desc){
      .logger = {
          .func = sg_logging,
          .user_data = NULL,
      },
 
      .buffer_pool_size = 1024,
      .context.sample_count = 1,
  });

  sg_trace_hooks hh = sg_install_trace_hooks(&hooks);

  font_init();
  debugdraw_init();
  sprite_initialize();

  #ifndef NO_EDITOR
  nuke_init(&mainwin);
  #endif
  
  model_init();
  sg_color c;
  rgba2floats(&c, editorClearColor);
  pass_action = (sg_pass_action){
    .colors[0] = {.load_action = SG_LOADACTION_CLEAR, .clear_value = c}
  };

  crt_post.shader = sg_make_shader(crt_shader_desc(sg_query_backend()));
  sg_gif.shader = sg_make_shader(box_shader_desc(sg_query_backend()));

  sg_gif.pipe = sg_make_pipeline(&(sg_pipeline_desc){
    .shader = sg_gif.shader,
    .layout = {
      .attrs = {
        [0].format = SG_VERTEXFORMAT_FLOAT2,
	[1].format = SG_VERTEXFORMAT_FLOAT2
      }
    }
  });

  crt_post.pipe = sg_make_pipeline(&(sg_pipeline_desc){
    .shader = crt_post.shader,
    .layout = {
      .attrs = {
        [0].format = SG_VERTEXFORMAT_FLOAT2,
	[1].format = SG_VERTEXFORMAT_FLOAT2
      }
    }
  });

  crt_post.img = sg_make_image(&(sg_image_desc){
    .render_target = true,
    .width = mainwin.width,
    .height = mainwin.height,
  });

  crt_post.depth_img = sg_make_image(&(sg_image_desc){
    .render_target = true,
    .width = mainwin.width,
    .height = mainwin.height,
    .pixel_format = SG_PIXELFORMAT_DEPTH_STENCIL
  });

  crt_post.pass = sg_make_pass(&(sg_pass_desc){
    .color_attachments[0].image = crt_post.img,
    .depth_stencil_attachment.image = crt_post.depth_img,
  });

  float crt_quad[] = {
    -1, 1, 0, 1,
    -1, -1, 0, 0,
    1, -1, 1, 0,
    -1, 1, 0, 1,
    1, -1, 1, 0,
    1, 1, 1, 1
  };

  crt_post.bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
    .size = sizeof(crt_quad),
    .data = crt_quad
  });

  crt_post.bind.fs.images[0] = crt_post.img;
  crt_post.bind.fs.samplers[0] = sg_make_sampler(&(sg_sampler_desc){});
/*
  sg_image_desc shadow_desc = {
    .render_target = true,
    .width = 1024,
    .height = 1024,
    .pixel_format = SG_PIXELFORMAT_R32F,
  };
  sg_image depth_img = sg_make_image(&shadow_desc);
  shadow_desc.pixel_format = SG_PIXELFORMAT_DEPTH;
  ddimg = sg_make_image(&shadow_desc);

  sg_shadow.pass = sg_make_pass(&(sg_pass_desc){
    .color_attachments[0].image = depth_img,
    .depth_stencil_attachment.image = ddimg,
  });
  
  sg_shadow.pass_action = (sg_pass_action) {
    .colors[0] = { .action=SG_ACTION_CLEAR, .value = {1,1,1,1} } };

  sg_shadow.shader = sg_make_shader(shadow_shader_desc(sg_query_backend()));

  sg_shadow.pipe = sg_make_pipeline(&(sg_pipeline_desc){
    .shader = sg_shadow.shader,
    .layout = {
      .attrs = {
        [0].format = SG_VERTEXFORMAT_FLOAT3,
      }
    },
    .depth = {
      .compare = SG_COMPAREFUNC_LESS_EQUAL,
      .write_enabled = true,
      .pixel_format = SG_PIXELFORMAT_DEPTH
    },
    .colors[0].pixel_format = SG_PIXELFORMAT_R32F,
    .index_type = SG_INDEXTYPE_UINT16,
    .cull_mode = SG_CULLMODE_BACK,
  });
    
*/

}

void render_winsize()
{
  sg_destroy_image(crt_post.img);
  sg_destroy_image(crt_post.depth_img);
  sg_destroy_pass(crt_post.pass);
  
  crt_post.img = sg_make_image(&(sg_image_desc){
    .render_target = true,
    .width = mainwin.width,
    .height = mainwin.height
  });

  crt_post.depth_img = sg_make_image(&(sg_image_desc){
    .render_target = true,
    .width = mainwin.width,
    .height = mainwin.height,
    .pixel_format = SG_PIXELFORMAT_DEPTH_STENCIL
  });

  crt_post.pass = sg_make_pass(&(sg_pass_desc){
    .color_attachments[0].image = crt_post.img,
    .depth_stencil_attachment.image = crt_post.depth_img,
  });

  crt_post.bind.fs.images[0] = crt_post.img;  
}

static cpBody *camera = NULL;
void set_cam_body(cpBody *body) {
  camera = body;
}

cpVect cam_pos() {
  return camera ? cpBodyGetPosition(camera) : cpvzero;
}

static float zoom = 1.f;
float cam_zoom() { return zoom; }

void add_zoom(float val) { zoom = val; }

HMM_Mat4 projection = {0.f};
HMM_Mat4 hudproj = {0.f};

HMM_Vec3 dirl_pos = {4, 100, 20};

void full_2d_pass(struct window *window)
{
  //////////// 2D projection
  cpVect pos = cam_pos();

  projection = HMM_Orthographic_RH_NO(
             pos.x - zoom * window->width / 2,
             pos.x + zoom * window->width / 2,
             pos.y - zoom * window->height / 2,
             pos.y + zoom * window->height / 2, -1.f, 1.f);

  hudproj = HMM_Orthographic_RH_NO(0, window->width, 0, window->height, -1.f, 1.f);


  sprite_draw_all();
  call_draw();

  //// DEBUG
  if (debugDrawPhysics) {
    gameobject_draw_debugs();
    call_debugs();    
  }
  debug_flush(&projection);
  text_flush(&projection);

  ////// TEXT && GUI
  debug_nextpass();
  #ifndef NO_EDITOR
  nuke_start();
  #endif
  
  call_gui();
  debug_flush(&hudproj);
  text_flush(&hudproj);
  sprite_flush();

  #ifndef NO_EDITOR
  call_nk_gui();
  nuke_end();
  #endif
}

void full_3d_pass(struct window *window)
{
  HMM_Mat4 model = HMM_M4D(1.f);
  float scale = 0.08;
  model = HMM_MulM4(model, HMM_Scale((HMM_Vec3){scale,scale,scale}));

  // Shadow pass
  sg_begin_pass(sg_shadow.pass, &sg_shadow.pass_action);
  sg_apply_pipeline(sg_shadow.pipe);

  HMM_Mat4 light_proj = HMM_Orthographic_RH_ZO(-100.f, 100.f, -100.f, 100.f, 1.f, 100.f);
  HMM_Mat4 light_view = HMM_LookAt_RH(dirl_pos, (HMM_Vec3){0,0,0}, (HMM_Vec3){0,1,0});

  HMM_Mat4 lsm = HMM_MulM4(light_proj, light_view);

  HMM_Mat4 subo[2];
  subo[0] = lsm;
  subo[1] = model;

  sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, SG_RANGE_REF(subo));
}

void openglRender(struct window *window) {
  sg_begin_pass(crt_post.pass, &pass_action);
  full_2d_pass(window);
  sg_end_pass();


  if (gif.rec && (appTime - gif.timer) > gif.spf) {
    sg_begin_pass(sg_gif.pass, &pass_action);
    sg_apply_pipeline(sg_gif.pipe);
    sg_apply_bindings(&crt_post.bind);
    sg_draw(0,6,1);
    sg_end_pass();

    gif.timer = appTime;
    sg_query_image_pixels(sg_gif.img, crt_post.bind.fs.samplers[0], gif.buffer, gif.w*gif.h*4);
    msf_gif_frame(&gif_state, gif.buffer, gif.cpf, gif.depth, gif.w * -4);
  }

  sg_begin_default_pass(&pass_action, window->width, window->height);
  sg_apply_pipeline(crt_post.pipe);
  sg_apply_bindings(&crt_post.bind);
  sg_draw(0,6,1);

  sg_end_pass();  

  sg_commit();

  debug_newframe();
}

sg_shader sg_compile_shader(const char *v, const char *f, sg_shader_desc *d)
{
  YughInfo("Making shader with %s and %s", v, f);
  char *vs = slurp_text(v, NULL);
  char *fs = slurp_text(f, NULL);

  d->vs.source = vs;
  d->fs.source = fs;
  d->label = v;

  sg_shader ret = sg_make_shader(d);
  
  free(vs);
  free(fs);
  return ret;
}
