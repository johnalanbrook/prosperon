#include "render.h"

#include "sokol/sokol_gfx.h"
#include "sokol/sokol_glue.h"
#include "sokol/util/sokol_gl.h"
#include "HandmadeMath.h"
#include <stdio.h>

#include <tracy/TracyC.h>

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

void trace_apply_pipeline(sg_pipeline pip, void *data)
{
//  YughSpam("Applying pipeline %u %s.", pip, sg_query_pipeline_desc(pip).label);
}

void trace_begin_pass(sg_pass pass, const sg_pass_action *action, void *data)
{
//  YughSpam("Begin pass %s", pass.label);
}

void trace_apply_viewport(int x, int y, int width, int height, bool origin_top_left, void *user_data)
{

}

void trace_draw(int base_element, int num_elements, int num_instances, void *user_data)
{
  TracyCPlotI("draw", num_elements);
}

void trace_alloc_buffer(sg_buffer result, void *data) {
  TracyCMessageL("BUFFER ALLOC");
}
void trace_init_buffer(sg_buffer id, const sg_buffer_desc *desc, void *data){
  TracyCMessageL("BUFFER INIT");
}
void trace_make_buffer(const sg_buffer_desc *desc, sg_buffer id, void *data){
  TracyCAllocN((void*)id.id, desc->data.size, "buffer");
}
void trace_append_buffer(sg_buffer id, const sg_range *data, int result, void *user_data) {
  sg_buffer_desc desc = sg_query_buffer_desc(id);  
}
void trace_update_buffer(sg_buffer buf, const sg_range *data, void *user_data){
  TracyCMessageL("BUFFER UPDATED");
}
void trace_uninit_buffer(sg_buffer id, void *data){
  TracyCMessageL("BUFFER UNINIT");
}
void trace_dealloc_buffer(sg_buffer id, void *data){
  TracyCMessageL("BUFFER DEALLOC");
}
void trace_destroy_buffer(sg_buffer id, void *data){
  TracyCFreeN((void*)id.id, "buffer");
}
void trace_fail_buffer(sg_buffer id, void *data){}

void trace_alloc_image(sg_image result, void *data) {
  TracyCMessageL("IMAGE ALLOC");
}
void trace_init_image(sg_image id, const sg_image_desc *desc, void *data){
  TracyCMessageL("IMAGE INIT");
}

int image_data_size(sg_image_data *data)
{
  int total_size = 0;
  for (int i = 0; i < SG_CUBEFACE_NUM; i++)
    for (int j = 0; j < SG_MAX_MIPMAPS; j++)
      total_size += data->subimage[i][j].size;

  return total_size;
}

void trace_make_image(const sg_image_desc *desc, sg_image id, void *data){
  int total_size = 0;
  for (int i = 0; i < desc->num_mipmaps; i++)
    total_size += desc->data.subimage[i]->size;

  TracyCAllocN((void*)id.id, image_data_size(&desc->data), "tex_gpu");
}

void trace_update_image(sg_image id, const sg_image_data *data, void *user_data){
  TracyCFreeN((void*)id.id, "tex_gpu");
  TracyCAllocN((void*)id.id, image_data_size(data), "tex_gpu");
}
void trace_uninit_image(sg_image id, void *data){
  TracyCMessageL("IMAGE UNINIT");
}
void trace_dealloc_image(sg_image id, void *data){
  TracyCMessageL("IMAGE DEALLOC");
}
void trace_destroy_image(sg_image id, void *data){
  TracyCFreeN((void*)id.id, "tex_gpu");
}

void trace_fail_image(sg_image id, void *data){}


#define SG_HOOK_SET(NAME) \
.alloc_##NAME = trace_alloc_##NAME, \
.dealloc_##NAME = trace_dealloc_##NAME, \
.init_##NAME = trace_init_##NAME, \
.uninit_##NAME = trace_uninit_##NAME, \
.fail_##NAME = trace_fail_##NAME, \
.destroy_##NAME = trace_destroy_##NAME, \
.make_##NAME = trace_make_##NAME \

static sg_trace_hooks hooks = {
  .apply_pipeline = trace_apply_pipeline,
  .begin_pass = trace_begin_pass,
  SG_HOOK_SET(buffer),
  .append_buffer = trace_append_buffer,
  SG_HOOK_SET(image),
//  SG_HOOK_SET(shader),
//  SG_HOOK_SET(sampler),
//  SG_HOOK_SET(pipeline),
//  SG_HOOK_SET(attachments),

  .draw = trace_draw,
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
