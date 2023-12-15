#include "render.h"

#include "config.h"
#include "datastream.h"
#include "debugdraw.h"
#include "font.h"
#include "gameobject.h"
#include "log.h"
#include "sprite.h"
#include "window.h"
#include "model.h"
#include "stb_ds.h"
#include "resources.h"
#include "yugine.h"
#include "sokol/sokol_app.h"
#include "stb_image_write.h"

#include "crt.sglsl.h"
#include "box.sglsl.h"
#include "shadow.sglsl.h"

#include "sokol/sokol_gfx.h"
#include "sokol/sokol_gfx_ext.h"

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

static struct {
  sg_shader shader;
  sg_pipeline pipe;
  sg_bindings bind;
  sg_pass pass;
  sg_image img;
  sg_image depth_img;
} crt_post;


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
  gif.timer = apptime();
  if (gif.buffer) free(gif.buffer);
  gif.buffer = malloc(gif.w*gif.h*4);

  sg_destroy_image(sg_gif.img);
  sg_destroy_image(sg_gif.depth);
  sg_destroy_pass(sg_gif.pass);

  sg_gif.img = sg_make_image(&(sg_image_desc){
    .render_target = true,
    .width = gif.w,
    .height = gif.h,
    .pixel_format = SG_PIXELFORMAT_RGBA8,
    .label = "gif rt",
  });

  sg_gif.depth = sg_make_image(&(sg_image_desc){
    .render_target = true,
    .width = gif.w,
    .height = gif.h,
    .pixel_format = SG_PIXELFORMAT_DEPTH,
    .label = "gif depth",
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

void capture_screen(int x, int y, int w, int h, char *path)
{
  int n = 4;
  void *data = malloc(w*h*n);
  sg_query_pixels(0,0,w,h,1,data,w*h*sizeof(char)*n);
//  sg_query_image_pixels(crt_post.img, crt_post.bind.fs.samplers[0], data, w*h*4);  
  stbi_write_png("cap.png", w, h, n, data, n*w);
//  stbi_write_bmp("cap.bmp", w, h, n, data);
  free(data);
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


void trace_make_image(const sg_image_desc *d, sg_image result, void *data)
{
  YughInfo("Making image %s", d->label);
}

void trace_init_image(sg_image id, const sg_image_desc *d, void *data)
{
  YughInfo("Init image %s", d->label);
}

void trace_make_shader(const sg_shader_desc *d, sg_shader result, void *data)
{
  YughInfo("Making shader %s", d->label);
  if (sg_query_shader_state(result) == SG_RESOURCESTATE_FAILED)
    YughError("FAILED MAKING A SHADER: %s\n%s\n%s", d->label);
}

void trace_fail_shader(sg_shader id, void *data)
{
  YughError("SHADER DID NOT COMPILE");
}

void trace_destroy_shader(sg_shader shd, void *data)
{
  YughInfo("DESTROYED SHADER");
}

void trace_fail_image(sg_image id, void *data)
{
  sg_image_desc desc = sg_query_image_desc(id);
  YughError("Failed to make image %s", desc.label);
}

void trace_make_pipeline(const sg_pipeline_desc *d, sg_pipeline result, void *data)
{
  YughInfo("Making pipeline %s, id %d", d->label, result);
}

void trace_apply_pipeline(sg_pipeline pip, void *data)
{
  YughInfo("Applying pipeline %d", pip);
}

void trace_fail_pipeline(sg_pipeline pip, void *data)
{
  YughError("Failed pipeline %s", sg_query_pipeline_desc(pip).label);
}

void trace_make_pass(const sg_pass_desc *d, sg_pass result, void *data)
{
  YughInfo("Making pass %s", d->label);
}

void trace_begin_pass(sg_pass pass, const sg_pass_action *action, void *data)
{
  
}

static sg_trace_hooks hooks = {
  .fail_shader = trace_fail_shader,
  .make_shader = trace_make_shader,
  .destroy_shader = trace_destroy_shader,
  .fail_image = trace_fail_image,
  .make_image = trace_make_image,
  .init_image = trace_init_image,
  .make_pipeline = trace_make_pipeline,
  .fail_pipeline = trace_fail_pipeline,
  .apply_pipeline = trace_apply_pipeline,
  .begin_pass = trace_begin_pass,
  .make_pass = trace_make_pass,
};

void render_init() {
  mainwin.width = sapp_width();
  mainwin.height = sapp_height();

  sg_setup(&(sg_desc){
      .context.d3d11.device = sapp_d3d11_get_device(),
      .context.d3d11.device_context = sapp_d3d11_get_device_context(),
      .context.d3d11.render_target_view_cb = sapp_d3d11_get_render_target_view,
      .context.d3d11.depth_stencil_view_cb = sapp_d3d11_get_depth_stencil_view,
      .context.metal.device = sapp_metal_get_device(),
      .context.metal.renderpass_descriptor_cb = sapp_metal_get_renderpass_descriptor,
      .context.metal.drawable_cb = sapp_metal_get_drawable,
      .context.color_format = sapp_color_format(),
      .context.depth_format = SG_PIXELFORMAT_DEPTH,
      .context.sample_count = sapp_sample_count(),
      .context.wgpu.device = sapp_wgpu_get_device(),
      .context.wgpu.render_view_cb = sapp_wgpu_get_render_view,
      .context.wgpu.resolve_view_cb = sapp_wgpu_get_resolve_view,
      .context.wgpu.depth_stencil_view_cb = sapp_wgpu_get_depth_stencil_view,
      .mtl_force_managed_storage_mode = 1,
      .logger = {
          .func = sg_logging,
      },
      .buffer_pool_size = 1024,
  });

  sg_trace_hooks hh = sg_install_trace_hooks(&hooks);

  font_init();
  debugdraw_init();
  sprite_initialize();

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
    },
    .colors[0].pixel_format = SG_PIXELFORMAT_RGBA8,
    .label = "gif pipe",
  });

  crt_post.pipe = sg_make_pipeline(&(sg_pipeline_desc){
    .shader = crt_post.shader,
    .layout = {
      .attrs = {
        [0].format = SG_VERTEXFORMAT_FLOAT2,
	[1].format = SG_VERTEXFORMAT_FLOAT2
      }
    },
    .label = "crt post pipeline",
  });

  crt_post.img = sg_make_image(&(sg_image_desc){
    .render_target = true,
    .width = mainwin.width,
    .height = mainwin.height,
    .label = "crt rt",
  });

  crt_post.depth_img = sg_make_image(&(sg_image_desc){
    .render_target = true,
    .width = mainwin.width,
    .height = mainwin.height,
    .pixel_format = SG_PIXELFORMAT_DEPTH,
    .label = "crt depth",
  });

  crt_post.pass = sg_make_pass(&(sg_pass_desc){
    .color_attachments[0].image = crt_post.img,
    .depth_stencil_attachment.image = crt_post.depth_img,
    .label = "crt post pass",
  });

#if defined SOKOL_GLCORE33 || defined SOKOL_GLES3
  float crt_quad[] = {
    -1, 1, 0, 1,
    -1, -1, 0, 0,
    1, -1, 1, 0,
    -1, 1, 0, 1,
    1, -1, 1, 0,
    1, 1, 1, 1
  };
#else
  float crt_quad[] = {
    -1, 1, 0, 0,
    -1, -1, 0, 1,
    1, -1, 1, 1,
    -1, 1, 0, 0,
    1, -1, 1, 1,
    1, 1, 1, 0
  };
#endif
  float gif_quad[] = {
    -1, 1, 0, 1,
    -1, -1, 0, 0,
    1, -1, 1, 0,
    -1, 1, 0, 1,
    1, -1, 1, 0,
    1, 1, 1, 1
  };

  sg_gif.bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
    .size = sizeof(gif_quad),
    .data = gif_quad,
  });
  sg_gif.bind.fs.images[0] = crt_post.img;
  sg_gif.bind.fs.samplers[0] = sg_make_sampler(&(sg_sampler_desc){});

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
  shadow_desc.pixel_format = sapp_depth_format();
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
      .pixel_format = sapp_depth_format()
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
    .height = mainwin.height,
    .label = "crt img resize",
  });

  crt_post.depth_img = sg_make_image(&(sg_image_desc){
    .render_target = true,
    .width = mainwin.width,
    .height = mainwin.height,
    .pixel_format = SG_PIXELFORMAT_DEPTH,
    .label = "crt depth resize",
  });

  crt_post.pass = sg_make_pass(&(sg_pass_desc){
    .color_attachments[0].image = crt_post.img,
    .depth_stencil_attachment.image = crt_post.depth_img,
    .label = "crt pass resize",
  });

  crt_post.bind.fs.images[0] = crt_post.img;
  sg_gif.bind.fs.images[0] = crt_post.img;
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

HMM_Vec2 world2screen(HMM_Vec2 pos)
{
  pos = HMM_SubV2(pos, HMM_V2(cam_pos().x, cam_pos().y));
  pos = HMM_ScaleV2(pos, 1.0/zoom);
  pos = HMM_AddV2(pos, HMM_V2(mainwin.rwidth/2.0, mainwin.rheight/2.0));
  return pos;
}

HMM_Vec2 screen2world(HMM_Vec2 pos)
{
  pos = HMM_ScaleV2(pos, 1/mainwin.dpi);
  pos = HMM_SubV2(pos, HMM_V2(mainwin.rwidth/2.0, mainwin.rheight/2.0));
  pos = HMM_ScaleV2(pos, zoom);
  pos = HMM_AddV2(pos, HMM_V2(cam_pos().x, cam_pos().y));
  return pos;
}

HMM_Mat4 projection = {0.f};
HMM_Mat4 hudproj = {0.f};

HMM_Vec3 dirl_pos = {4, 100, 20};

void full_2d_pass(struct window *window)
{
  //////////// 2D projection
  cpVect pos = cam_pos();

  projection = HMM_Orthographic_LH_NO(
             pos.x - zoom * window->rwidth / 2,
             pos.x + zoom * window->rwidth / 2,
             pos.y - zoom * window->rheight / 2,
             pos.y + zoom * window->rheight / 2, -1000.f, 1000.f);

  hudproj = HMM_Orthographic_LH_ZO(0, window->rwidth, 0, window->rheight, -1.f, 1.f);

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
  call_gui();
  debug_flush(&hudproj);
  text_flush(&hudproj);
  sprite_flush();
}

void full_3d_pass(struct window *window)
{
  HMM_Mat4 model = HMM_M4D(1.f);
  float scale = 0.08;
  model = HMM_MulM4(model, HMM_Scale((HMM_Vec3){scale,scale,scale}));

  // Shadow pass
//  sg_begin_pass(sg_shadow.pass, &sg_shadow.pass_action);
//  sg_apply_pipeline(sg_shadow.pipe);

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

  if (gif.rec && (apptime() - gif.timer) > gif.spf) {
    sg_begin_pass(sg_gif.pass, &pass_action);
    sg_apply_pipeline(sg_gif.pipe);
    sg_apply_bindings(&sg_gif.bind);
    sg_draw(0,6,1);
    sg_end_pass();

    gif.timer = apptime();
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
