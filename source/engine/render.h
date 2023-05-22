#ifndef RENDER_H
#define RENDER_H

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "sokol/sokol_gfx.h"

struct uv_n {
  unsigned short u;
  unsigned short v;
};

struct st_n {
  struct uv_n s;
  struct uv_n t;
};

static sg_blend_state blend_trans = {
  .enabled = true,
  .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
  .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
  .src_factor_alpha = SG_BLENDFACTOR_SRC_ALPHA,
  .src_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA
};


#endif
