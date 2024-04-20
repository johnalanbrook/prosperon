#include "render.h"

#include "config.h"
#include "datastream.h"
#include "debugdraw.h"
#include "font.h"
#include "gameobject.h"
#include "log.h"
#include "sprite.h"
#include "particle.h"
#include "window.h"
#include "model.h"
#include "stb_ds.h"
#include "resources.h"
#include "yugine.h"
#include "sokol/sokol_app.h"
#define SOKOL_GLUE_IMPL
#include "sokol/sokol_glue.h"
#include "stb_image_write.h"

#include "box.sglsl.h"
#include "shadow.sglsl.h"

#include "sokol/sokol_gfx.h"
#include "sokol_gfx_ext.h"

#include "crt.sglsl.h"

#include "msf_gif.h"

HMM_Vec2 campos = {0,0};
float camzoom = 1;

static struct {
  sg_swapchain swap;
  sg_pipeline pipe;
  sg_bindings bind;
  sg_shader shader;
  sg_image img;
  sg_image depth;
} sg_gif;

static struct {
  sg_pipeline pipe;
  sg_bindings bind;
  sg_shader shader;
} sg_crt;

static struct {
  int w;
  int h;
  int cpf;
  int depth;
  double timer;
  double spf;
  int rec;
  uint8_t *buffer;
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
  gif.timer = apptime();
  if (gif.buffer) free(gif.buffer);
  gif.buffer = malloc(gif.w*gif.h*4);

  sg_destroy_image(sg_gif.img);
  sg_destroy_image(sg_gif.depth);

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
    .label = "gif depth",
  });

  sg_gif.swap = sglue_swapchain();
}

void gif_rec_end(const char *path)
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

void capture_screen(int x, int y, int w, int h, const char *path)
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

sg_pass_action pass_action = {0};

static struct {
  sg_pass_action pass_action;

  sg_pass pass;
  sg_pipeline pipe;
  sg_shader shader;
} sg_shadow;

void trace_apply_pipeline(sg_pipeline pip, void *data)
{
//  YughSpam("Applying pipeline %u %s.", pip, sg_query_pipeline_desc(pip).label);
}

void trace_begin_pass(sg_pass pass, const sg_pass_action *action, void *data)
{
//  YughSpam("Begin pass %s", pass.label);
}

#define SG_TRACE_SET(NAME) \
void trace_alloc_##NAME (sg_##NAME id, void *data) \
{ \
  sg_##NAME##_desc desc = sg_query_##NAME##_desc(id); \
  YughSpam("Alloc " #NAME " %d [%s]", id, desc.label); \
} \
\
void trace_dealloc_##NAME(sg_##NAME id, void *data) \
{ \
  sg_##NAME##_desc desc = sg_query_##NAME##_desc(id); \
  YughSpam("Dealloc " #NAME " %d [%s]", id, desc.label); \
} \
\
void trace_make_##NAME(sg_##NAME##_desc *desc, void *data) \
{ \
  YughSpam("Make " #NAME " [%s]", desc->label); \
} \
\
void trace_destroy_##NAME(sg_##NAME id, void *data) \
{ \
  sg_##NAME##_desc desc = sg_query_##NAME##_desc(id); \
  YughSpam("Destroy " #NAME " %d [%s]", id, desc.label); \
} \
\
void trace_init_##NAME(sg_##NAME id, sg_##NAME##_desc *desc, void *data) \
{ \
  YughSpam("Init " #NAME " %d [%s]", id, desc->label); \
} \
\
void trace_uninit_##NAME(sg_##NAME id, void *data) \
{ \
  sg_##NAME##_desc desc = sg_query_##NAME##_desc(id); \
  YughSpam("Init " #NAME " %d [%s]", id, desc.label); \
} \
\
void trace_fail_##NAME(sg_##NAME id, void *data) \
{ \
  sg_##NAME##_desc desc = sg_query_##NAME##_desc(id); \
  YughError("Failed " #NAME " %d: %s", id, desc.label); \
} \

SG_TRACE_SET(buffer)
SG_TRACE_SET(image)
SG_TRACE_SET(sampler)
SG_TRACE_SET(shader)
SG_TRACE_SET(pipeline)
SG_TRACE_SET(attachments)

#define SG_HOOK_SET(NAME) \
.alloc_##NAME = trace_alloc_##NAME, \
.dealloc_##NAME = trace_dealloc_##NAME, \
.init_##NAME = trace_init_##NAME, \
.uninit_##NAME = trace_uninit_##NAME, \
.fail_##NAME = trace_fail_##NAME, \
.destroy_##NAME = trace_destroy_##NAME, \
.make_##NAME = trace_make_##NAME \

void trace_append_buffer(sg_buffer id, sg_range *data, void *user)
{
  sg_buffer_desc desc = sg_query_buffer_desc(id);
//  YughSpam("Appending buffer %d [%s]", id, desc.label);
}

static sg_trace_hooks hooks = {
  .apply_pipeline = trace_apply_pipeline,
  .begin_pass = trace_begin_pass,
  SG_HOOK_SET(buffer),
  SG_HOOK_SET(image),
  SG_HOOK_SET(shader),
  SG_HOOK_SET(sampler),
  SG_HOOK_SET(pipeline),
  SG_HOOK_SET(attachments),
  .append_buffer = trace_append_buffer
};

void render_init() {
  mainwin.size = (HMM_Vec2){sapp_width(), sapp_height()};
  sg_setup(&(sg_desc){
    .environment = sglue_environment(),
    .logger = { .func = sg_logging },
    .buffer_pool_size = 1024
  });

#ifndef NDEBUG
  sg_trace_hooks hh = sg_install_trace_hooks(&hooks);
#endif

  font_init();
  debugdraw_init();
  sprite_initialize();

  model_init();
  
  sg_color c = (sg_color){0,0,0,1};
  pass_action = (sg_pass_action){
    .colors[0] = {.load_action = SG_LOADACTION_CLEAR, .clear_value = c},
  };
  
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

  float crt_quad[] = {
    -1, 1, 0, 1,
    -1, -1, 0, 0,
    1, -1, 1, 0,
    -1, 1, 0, 1,
    1, -1, 1, 0,
    1, 1, 1, 1
  };

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
    .label = "gif vert buffer",
  });
  sg_gif.bind.fs.samplers[0] = sg_make_sampler(&(sg_sampler_desc){});
  
  sg_crt.shader = sg_make_shader(crt_shader_desc(sg_query_backend()));
  sg_crt.pipe = sg_make_pipeline(&(sg_pipeline_desc){
    .shader = sg_crt.shader,
    .layout = {
      .attrs = {
        [0].format = SG_VERTEXFORMAT_FLOAT2,
        [1].format = SG_VERTEXFORMAT_FLOAT2
      }
    },
    .primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP,
  });
  
  sg_crt.bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
    .size = sizeof(crt_quad),
    .type = SG_BUFFERTYPE_VERTEXBUFFER,
    .usage = SG_USAGE_IMMUTABLE,
    .data = SG_RANGE(crt_quad),
    
    .label = "crt vert buffer",
  });
}

HMM_Mat4 projection = {0.f};
HMM_Mat4 hudproj = {0.f};
HMM_Mat4 useproj = {0};

#define MODE_STRETCH 0
#define MODE_KEEP 1
#define MODE_WIDTH 2
#define MODE_HEIGHT 3
#define MODE_EXPAND 4
#define MODE_FULL 5

void openglRender(struct window *window, gameobject *cam, float zoom) {
  sg_swapchain sch = sglue_swapchain();
  sg_begin_pass(&(sg_pass){
    .action = pass_action,
    .swapchain = sglue_swapchain(),
    .label =  "window pass"
  });

  HMM_Vec2 usesize = window->rendersize;

  switch(window->mode) {
    case MODE_STRETCH:
      sg_apply_viewportf(0,0,window->size.x,window->size.y,1);
      break;
    case MODE_WIDTH:
      sg_apply_viewportf(0, window->top, window->size.x, window->psize.y,1); // keep width
      break;
    case MODE_HEIGHT:
      sg_apply_viewportf(window->left,0,window->psize.x, window->size.y,1); // keep height
      break;
    case MODE_KEEP:
      sg_apply_viewportf(0,0,window->rendersize.x, window->rendersize.y, 1); // no scaling
      break;
    case MODE_EXPAND:
      if (window->aspect < window->raspect)
        sg_apply_viewportf(0, window->top, window->size.x, window->psize.y,1); // keep width
      else
        sg_apply_viewportf(window->left,0,window->psize.x, window->size.y,1); // keep height
      break;
    case MODE_FULL:
      usesize = window->size;
      break;
  }

  // 2D projection
  campos = go_pos(cam);
  camzoom = zoom;

  projection = HMM_Orthographic_RH_NO(
             campos.x - camzoom * usesize.x / 2,
             campos.x + camzoom * usesize.x / 2,
             campos.y - camzoom * usesize.y / 2,
             campos.y + camzoom * usesize.y / 2, -10000.f, 10000.f);

/*  if (gif.rec && (apptime() - gif.timer) > gif.spf) {
    sg_begin_pass(&(sg_pass){
      .action = pass_action,
      .swapchain = sg_gif.swap
    });
    sg_apply_pipeline(sg_gif.pipe);
    sg_apply_bindings(&sg_gif.bind);
    sg_draw(0,6,1);
    sg_end_pass();

    gif.timer = apptime();
    sg_query_image_pixels(sg_gif.img, crt_post.bind.fs.samplers[0], gif.buffer, gif.w*gif.h*4);
    msf_gif_frame(&gif_state, gif.buffer, gif.cpf, gif.depth, gif.w * -4);
  }
*/
}

struct boundingbox cwh2bb(HMM_Vec2 c, HMM_Vec2 wh) {
  struct boundingbox bb = {
    .t = c.Y + wh.Y/2,
    .b = c.Y - wh.Y/2,
    .r = c.X + wh.X/2,
    .l = c.X - wh.X/2
  };

  return bb;
}

float *rgba2floats(float *r, struct rgba c)
{
  r[0] = (float)c.r / RGBA_MAX;
  r[1] = (float)c.g / RGBA_MAX;
  r[2] = (float)c.b / RGBA_MAX;
  r[3] = (float)c.a / RGBA_MAX;
  return r;
}

sg_blend_state blend_trans = {
  .enabled = true,
  .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
  .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
  .src_factor_alpha = SG_BLENDFACTOR_SRC_ALPHA,
  .dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA
};
