#include "glad.h"
#include <tracy/Tracy.hpp>
#include <tracy/TracyOpenGL.hpp>

void trace_apply_uniforms(sg_shader_stage stage, int ub_index, const sg_range *data, void *user_data)
{
}

void trace_apply_pipeline(sg_pipeline pip, void *data)
{
}

void trace_begin_pass(const sg_pass *action, void *data)
{

}

void trace_apply_viewport(int x, int y, int width, int height, bool origin_top_left, void *user_data)
{
}

void trace_apply_bindings(const sg_bindings *bindings, void *data)
{

}

void trace_draw(int base_element, int num_elements, int num_instances, void *user_data)
{
}

void trace_alloc_buffer(sg_buffer result, void *data) {
  TracyMessageL("BUFFER ALLOC");
}
void trace_init_buffer(sg_buffer id, const sg_buffer_desc *desc, void *data){
  TracyMessageL("BUFFER INIT");
}
void trace_make_buffer(const sg_buffer_desc *desc, sg_buffer id, void *data){
  TracyAllocN((void*)id.id, desc->data.size, "buffer");
}
void trace_append_buffer(sg_buffer id, const sg_range *data, int result, void *user_data) {
}
void trace_update_buffer(sg_buffer buf, const sg_range *data, void *user_data){
  
  TracyMessageL("BUFFER UPDATED");
}
void trace_uninit_buffer(sg_buffer id, void *data){
  TracyMessageL("BUFFER UNINIT");
}
void trace_dealloc_buffer(sg_buffer id, void *data){
  TracyMessageL("BUFFER DEALLOC");
}
void trace_destroy_buffer(sg_buffer id, void *data){
  TracyFreeN((void*)id.id, "buffer");
}
void trace_fail_buffer(sg_buffer id, void *data){}

void trace_alloc_image(sg_image result, void *data) {
  TracyMessageL("IMAGE ALLOC");
}
void trace_init_image(sg_image id, const sg_image_desc *desc, void *data){
  TracyMessageL("IMAGE INIT");
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

//  TracyAllocN((void*)id.id, image_data_size(&desc->data), "tex_gpu");
}

void trace_update_image(sg_image id, const sg_image_data *data, void *user_data){
//  TracyFreeN((void*)id.id, "tex_gpu");
//  TracyAllocN((void*)id.id, image_data_size(data), "tex_gpu");
}
void trace_uninit_image(sg_image id, void *data){
  TracyMessageL("IMAGE UNINIT");
}
void trace_dealloc_image(sg_image id, void *data){
  TracyMessageL("IMAGE DEALLOC");
}
void trace_destroy_image(sg_image id, void *data){
  TracyFreeN((void*)id.id, "tex_gpu");
}

void trace_fail_image(sg_image id, void *data){}

#define SG_HOOK_SET(NAME) \
hooks.alloc_##NAME = trace_alloc_##NAME; \
hooks.dealloc_##NAME = trace_dealloc_##NAME; \
hooks.init_##NAME = trace_init_##NAME; \
hooks.uninit_##NAME = trace_uninit_##NAME; \
hooks.fail_##NAME = trace_fail_##NAME; \
hooks.destroy_##NAME = trace_destroy_##NAME; \
hooks.make_##NAME = trace_make_##NAME; \

static sg_trace_hooks hooks;

extern "C"{
void render_trace_init()
{
  return;
  hooks.apply_pipeline = trace_apply_pipeline;
  hooks.begin_pass = trace_begin_pass;
  SG_HOOK_SET(buffer);
  hooks.append_buffer = trace_append_buffer;
  SG_HOOK_SET(image);
//  SG_HOOK_SET(shader),
//  SG_HOOK_SET(sampler),
//  SG_HOOK_SET(pipeline),
//  SG_HOOK_SET(attachments),
  hooks.apply_uniforms = trace_apply_uniforms;
  hooks.draw = trace_draw;
  
  sg_trace_hooks hh = sg_install_trace_hooks(&hooks);
  
  TracyGpuContext;
}
}

/*static const JSCFunctionListEntry js_tracy_funcs[] = {
  JS_CFUNC_DEF("fiber", 1, js_tracy_fiber_enter),
  JS_CFUNC_DEF("fiber_leave", 1, js_tracy_fiber_leave),
  JS_CFUNC_DEF("gpu_zone_begin", 1, js_tracy_gpu_zone_begin),
  JS_CFUNC_DEF("gpu_zone_end", 1, js_tracy_gpu_zone_end),
  JS_CFUNC_DEF("gpu_collect", 0, js_tracy_gpu_collect),
  JS_CFUNC_DEF("end_frame", 0, js_tracy_frame_mark),
  JS_CFUNC_DEF("zone", 1, js_tracy_zone_begin),
  JS_CFUNC_DEF("message", 1, js_tracy_message),
  JS_CFUNC_DEF("plot", 2, js_tracy_plot),
};

JSValue js_tracy_use(JSContext *js)
{
  JSValue expy = JS_NewObject(js);
  JS_SetPropertyFunctionList(js, expy, js_tracy_funcs, sizeof(js_tracy_funcs)/sizeof(JSCFunctionListEntry));
  return expy;
}
*/
