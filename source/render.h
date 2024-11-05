#ifndef OPENGL_RENDER_H
#define OPENGL_RENDER_H

#include "config.h"

#include "sokol/sokol_gfx.h"
#include "HandmadeMath.h"
#include "gameobject.h"
#include "transform.h"
#include "model.h"

#define RGBA_MAX 255

#include "window.h"

extern struct rgba color_white;
extern struct rgba color_black;
extern struct rgba color_clear;
extern int TOPLEFT;

extern HMM_Vec3 dirl_pos;

extern HMM_Mat4 projection;
extern HMM_Mat4 hudproj;
extern HMM_Mat4 useproj;
extern sg_pass_action pass_action;

extern sg_sampler std_sampler;
extern sg_sampler tex_sampler;
extern sg_image screencolor;
extern sg_image screendepth;

typedef struct viewstate {
  HMM_Mat4 v;
  HMM_Mat4 p;
  HMM_Mat4 vp;
} viewstate;

extern viewstate globalview;

void render_init();

void openglRender(HMM_Vec2 usesize);

void capture_screen(int x, int y, int w, int h, const char *path);

void gif_rec_start(int w, int h, int cpf, int bitdepth);
void gif_rec_end(const char *path);

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

typedef struct rgba rgba;

static inline rgba vec2rgba(HMM_Vec4 v) {
  return (rgba){v.e[0]*255,v.e[1]*255,v.e[2]*255,v.e[3]*255};
}

struct boundingbox {
  float t;
  float b;
  float r;
  float l;
};

// rectangles are always defined with [x,y] in the bottom left
struct rect {
  float x,y,w,h;
};
typedef struct rect rect;

struct boundingbox cwh2bb(HMM_Vec2 c, HMM_Vec2 wh);
float *rgba2floats(float *r, struct rgba c);
extern sg_blend_state blend_trans;

static inline float lerp(float f, float a, float b)
{
  return a * (1.0-f)+(b*f);
}

#endif
