#include "openglrender.h"

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

#include "HandmadeMath.h"

int renderMode = LIT;

struct shader *spriteShader = NULL;
struct shader *wireframeShader = NULL;
struct shader *animSpriteShader = NULL;
static struct shader *textShader;

mfloat_t editorClearColor[4] = {0.2f, 0.4f, 0.3f, 1.f};

float shadowLookahead = 8.5f;

mfloat_t gridSmallColor[3] = {0.35f, 1.f, 0.9f};

mfloat_t gridBigColor[3] = {0.92f, 0.92f, 0.68f};

float gridScale = 500.f;
float smallGridUnit = 1.f;
float bigGridUnit = 10.f;
float gridSmallThickness = 2.f;
float gridBigThickness = 7.f;
float gridOpacity = 0.3f;

mfloat_t proj[16];

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

struct sprite *tsprite = NULL;

const char *donquixote;

static struct model *duck;

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


void openglInit() {
  if (!mainwin) {
    YughError("No window to init OpenGL on.", 1);
    exit(1);
  }

  font_init(NULL);
  debugdraw_init();
  sprite_initialize();
  nuke_init(mainwin);

  model_init();
//  duck = MakeModel("sponza.glb");

  pass_action = (sg_pass_action){
      .colors[0] = {.action = SG_ACTION_CLEAR, .value = {editorClearColor[1], editorClearColor[2], editorClearColor[3], 1.f}},
  };

  crt_post.shader = sg_make_shader(&(sg_shader_desc){
    .vs.source = slurp_text("shaders/postvert.glsl"),
    .fs.source = slurp_text("shaders/crtfrag.glsl"),

    .fs.images[0] = {
      .name = "diffuse_texture",
      .image_type = SG_IMAGETYPE_2D,
      .sampler_type = SG_SAMPLERTYPE_FLOAT
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
    .width = 1200,
    .height = 720,
  });

  crt_post.depth_img = sg_make_image(&(sg_image_desc){
    .render_target = true,
    .width = 1200,
    .height = 720,
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

  crt_post.bind.fs_images[0] = crt_post.img;

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

//    duck = MakeModel("sponza.glb");  
  
  sg_shadow.pass = sg_make_pass(&(sg_pass_desc){
    .color_attachments[0].image = depth_img,
    .depth_stencil_attachment.image = ddimg,
  });
  
  sg_shadow.pass_action = (sg_pass_action) {
    .colors[0] = { .action=SG_ACTION_CLEAR, .value = {1,1,1,1} } };

  sg_shadow.shader = sg_make_shader(&(sg_shader_desc){
    .vs.source = slurp_text("shaders/shadowvert.glsl"),
    .fs.source = slurp_text("shaders/shadowfrag.glsl"),
    .vs.uniform_blocks[0] = {
    .size = sizeof(float) * 16 * 2,
    .uniforms = {
      [0] = {.name = "lightSpaceMatrix", .type = SG_UNIFORMTYPE_MAT4},
      [1] = {.name = "model", .type = SG_UNIFORMTYPE_MAT4},
      }
    }
    });

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

mfloat_t projection[16] = {0.f};

HMM_Vec3 dirl_pos = {4, 100, 20};

void openglRender(struct window *window) {
/*
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
  
  for (int i = 0; i < arrlen(duck->meshes); i++) {
    sg_bindings sbind = {0};
    sbind.vertex_buffers[0] = duck->meshes[i].bind.vertex_buffers[0];
    sbind.index_buffer = duck->meshes[i].bind.index_buffer;
    sg_apply_bindings(&sbind);
    sg_draw(0,duck->meshes[i].face_count,1);
  }
  sg_end_pass();

  draw_model(duck,model, lsm);  
*/  

//  sg_begin_default_pass(&pass_action, window->width, window->height);
  sg_begin_pass(crt_post.pass, &pass_action);

  //////////// 2D projection
  cpVect pos = cam_pos();

  mat4_ortho(projection, pos.x - zoom * window->width / 2,
             pos.x + zoom * window->width / 2,
             pos.y - zoom * window->height / 2,
             pos.y + zoom * window->height / 2, -1.f, 1.f);

  mfloat_t ui_projection[16] = {0.f};
  mat4_ortho(ui_projection, 0, window->width, 0, window->height, -1.f, 1.f);
  
  sprite_draw_all();
  sprite_flush();
  
  //// DEBUG
  if (debugDrawPhysics)
    gameobject_draw_debugs();
  
  call_debugs();
  debug_flush();
 
  ////// TEXT && GUI
  call_gui();

  text_flush();

  nuke_start();
  call_nk_gui();
  nuke_end();

  sg_end_pass();

  sg_begin_default_pass(&pass_action, window->width, window->height);
  sg_apply_pipeline(crt_post.pipe);
  sg_apply_bindings(&crt_post.bind);
  sg_draw(0,6,1);
  sg_end_pass();

  
  sg_commit();
}
