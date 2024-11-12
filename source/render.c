#include "render.h"

#include "sokol/sokol_gfx.h"
#include "sokol/sokol_glue.h"
#include "sokol/util/sokol_gl.h"
#include "HandmadeMath.h"

sg_sampler std_sampler;
sg_sampler tex_sampler;

viewstate globalview = {0};

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
//    .logger = { .func = sg_logging },
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
}

float *rgba2floats(float *r, struct rgba c)
{
  r[0] = (float)c.r / RGBA_MAX;
  r[1] = (float)c.g / RGBA_MAX;
  r[2] = (float)c.b / RGBA_MAX;
  r[3] = (float)c.a / RGBA_MAX;
  return r;
}
