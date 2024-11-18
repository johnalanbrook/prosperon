#include "render.h"

#include "sokol/sokol_gfx.h"
#include "sokol/sokol_glue.h"
#include "HandmadeMath.h"
#include <stdio.h>

sg_sampler std_sampler;
sg_sampler tex_sampler;

viewstate globalview = {0};

#ifdef TRACY_ENABLE
#include <tracy/TracyC.h>

#define DUMPSTAT(NAME) TracyCPlotI(#NAME, stats.NAME);

void render_dump_trace()
{
  sg_frame_stats stats = sg_query_frame_stats();
  DUMPSTAT(num_passes)
  DUMPSTAT(num_draw)
  DUMPSTAT(num_tris)
  DUMPSTAT(num_verts)
  
  DUMPSTAT(num_apply_pipeline)
  DUMPSTAT(num_apply_bindings)
  DUMPSTAT(num_apply_uniforms)
  DUMPSTAT(size_apply_uniforms)  


  DUMPSTAT(size_update_buffer)
  DUMPSTAT(size_append_buffer)
  DUMPSTAT(size_update_image)

  DUMPSTAT(num_apply_viewport)
  DUMPSTAT(num_apply_scissor_rect)
}

#endif

void render_trace_init();

void render_init() {
  sg_setup(&(sg_desc){
    .environment = sglue_environment(),
//    .logger = { .func = sg_logging },
    .buffer_pool_size = 1024,
    .image_pool_size = 1024,
  });
  
#ifdef TRACY_ENABLE
  sg_enable_frame_stats();
  TracyCPlotConfig("size_apply_uniforms", TracyPlotFormatMemory, 1, 1, 5474985);
  TracyCPlotConfig("size_update_buffer", TracyPlotFormatMemory, 1, 1, 547498235);
  TracyCPlotConfig("size_append_buffer", TracyPlotFormatMemory, 1, 1, 12345615);
  TracyCPlotConfig("size_update_image", TracyPlotFormatMemory, 1, 1, 49831049);
//  render_trace_init();
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
