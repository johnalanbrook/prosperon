#include "render.h"

#include "config.h"
#include "datastream.h"
#include "font.h"
#include "gameobject.h"
#include "log.h"
#include "window.h"
#include "model.h"
#include "stb_ds.h"
#include "resources.h"
#include "yugine.h"
#include "sokol/sokol_app.h"
#define SOKOL_GLUE_IMPL
#include "sokol/sokol_glue.h"
#include "stb_image_write.h"

#include "sokol/sokol_gfx.h"
#include "sokol/util/sokol_gl.h"
#include "gui.h"

static HMM_Vec2 lastuse = {0};

HMM_Vec2 campos = {0,0};
float camzoom = 1;

viewstate globalview = {0};

sg_sampler std_sampler;
sg_sampler nofilter_sampler;
sg_sampler tex_sampler;

int TOPLEFT = 0;

sg_pass offscreen;

#include "sokol/sokol_app.h"

#include "HandmadeMath.h"

sg_pass_action pass_action = {0};
sg_pass_action off_action = {0};
sg_image screencolor = {0};
sg_image screendepth = {0};

struct rfd {
  int num_passes;
  int num_apply_viewport;
  int num_apply_scissor_rect;
  int num_apply_pipeline;
  int num_apply_uniforms;
  int size_uniforms;
};

static struct rfd rfd = {0};

void trace_apply_uniforms(sg_shader_stage stage, int ub_index, const sg_range *data, void *user_data)
{
  rfd.num_apply_uniforms++;
  rfd.size_uniforms += data->size;
}

void trace_draw(int base_e, int num_e, int num_inst, void *data)
{
  
}

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
} \
\
void trace_dealloc_##NAME(sg_##NAME id, void *data) \
{ \
  sg_##NAME##_desc desc = sg_query_##NAME##_desc(id); \
} \
\
void trace_make_##NAME(sg_##NAME##_desc *desc, void *data) \
{ \
} \
\
void trace_destroy_##NAME(sg_##NAME id, void *data) \
{ \
  sg_##NAME##_desc desc = sg_query_##NAME##_desc(id); \
} \
\
void trace_init_##NAME(sg_##NAME id, sg_##NAME##_desc *desc, void *data) \
{ \
} \
\
void trace_uninit_##NAME(sg_##NAME id, void *data) \
{ \
  sg_##NAME##_desc desc = sg_query_##NAME##_desc(id); \
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
  sg_setup(&(sg_desc){
    .environment = sglue_environment(),
    .logger = { .func = sg_logging },
    .buffer_pool_size = 1024,
    .image_pool_size = 1024,
  });
  
#ifndef NDEBUG
  sg_trace_hooks hh = sg_install_trace_hooks(&hooks);
#endif

  std_sampler = sg_make_sampler(&(sg_sampler_desc){});
  tex_sampler = sg_make_sampler(&(sg_sampler_desc){
    .min_filter = SG_FILTER_LINEAR,
    .mag_filter = SG_FILTER_LINEAR,
    .mipmap_filter = SG_FILTER_LINEAR,
    .wrap_u = SG_WRAP_REPEAT,
    .wrap_v = SG_WRAP_REPEAT
  });

  sg_features feat = sg_query_features();
  TOPLEFT = feat.origin_top_left;

  sg_color c = (sg_color){0,0,0,1};
  pass_action = (sg_pass_action){
    .colors[0] = {.load_action = SG_LOADACTION_CLEAR, .clear_value = c},
  };
  
  sg_color oc = (sg_color){35.0/255,60.0/255,92.0/255,1};
  off_action = (sg_pass_action){
    .colors[0] = {.load_action = SG_LOADACTION_CLEAR, .clear_value = oc},
    .depth = {
      .load_action = SG_LOADACTION_CLEAR,
      .clear_value = 1
    },
    .stencil = {
      .clear_value = 1
    }
  };
  
  screencolor = sg_make_image(&(sg_image_desc){
    .render_target = true,
    .width = 500,
    .height = 500,
    .pixel_format = sapp_color_format(),
    .sample_count = 1,
  });
  screendepth = sg_make_image(&(sg_image_desc){
    .render_target = true,
    .width =500,
    .height=500,
    .pixel_format = sapp_depth_format(),
    .sample_count = 1
  });
  
  offscreen = (sg_pass){
    .attachments = sg_make_attachments(&(sg_attachments_desc){
      .colors[0].image = screencolor,
      .depth_stencil.image = screendepth,
    }),
    .action = off_action,
  };
}

HMM_Mat4 projection = {0.f};
HMM_Mat4 hudproj = {0.f};
HMM_Mat4 useproj = {0};

void openglRender(HMM_Vec2 usesize) {
 if (usesize.x != lastuse.x || usesize.y != lastuse.y) {
  sg_destroy_image(screencolor);
  sg_destroy_image(screendepth);
  sg_destroy_attachments(offscreen.attachments);
  screencolor = sg_make_image(&(sg_image_desc){
    .render_target = true,
    .width = usesize.x,
    .height = usesize.y,
    .pixel_format = sapp_color_format(),
    .sample_count = 1,
  });
  screendepth = sg_make_image(&(sg_image_desc){
    .render_target = true,
    .width =usesize.x,
    .height=usesize.y,
    .pixel_format = sapp_depth_format(),
    .sample_count = 1
  });
  offscreen = (sg_pass){
    .attachments = sg_make_attachments(&(sg_attachments_desc){
      .colors[0].image = screencolor,
      .depth_stencil.image = screendepth
    }),
    .action = off_action,
  };
 }
 lastuse = usesize;
 
 sg_begin_pass(&offscreen);
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
