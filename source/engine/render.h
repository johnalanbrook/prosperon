#ifndef RENDER_H
#define RENDER_H

#include "sokol/sokol_gfx.h"

#include "HandmadeMath.h"

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

struct boundingbox {
  float t;
  float b;
  float r;
  float l;
};

static struct boundingbox cwh2bb(HMM_Vec2 c, HMM_Vec2 wh) {
  struct boundingbox bb = {
    .t = c.Y + wh.Y/2,
    .b = c.Y - wh.Y/2,
    .r = c.X + wh.X/2,
    .l = c.X - wh.X/2
  };

  return bb;
}

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
  .dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA
};

#endif
