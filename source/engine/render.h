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

struct rgba {
  unsigned char r;
  unsigned char g;
  unsigned char b;
  unsigned char a;
};

static float *rgba2floats(float *r, struct rgba c)
{
  r[0] = c.r / 255.0;
  r[1] = c.g / 255.0;
  r[2] = c.b / 255.0;
  r[3] = c.a / 255.0;
  return r;
}

static sg_blend_state blend_trans = {
  .enabled = true,
  .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
  .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
  .src_factor_alpha = SG_BLENDFACTOR_SRC_ALPHA,
  .src_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA
};

#endif
