#ifndef OPENGL_RENDER_H
#define OPENGL_RENDER_H

#if defined __linux__
  #define SOKOL_GLCORE
#elif __EMSCRIPTEN__
  #define SOKOL_WGPU
#elif __WIN32
  #define SOKOL_D3D11
#elif __APPLE__
  #define SOKOL_METAL
#endif

#include "sokol/sokol_gfx.h"
#include "HandmadeMath.h"

#define RGBA_MAX 255

extern struct rgba color_white;
extern struct rgba color_black;
extern struct rgba color_clear;
extern int TOPLEFT;

extern sg_sampler std_sampler;
extern sg_sampler tex_sampler;

typedef struct viewstate {
  HMM_Mat4 v;
  HMM_Mat4 p;
  HMM_Mat4 vp;
} viewstate;

extern viewstate globalview;

void render_init();

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

void render_dump_trace();

typedef struct rgba rgba;

static inline rgba vec2rgba(HMM_Vec4 v) {
  return (rgba){v.e[0]*255,v.e[1]*255,v.e[2]*255,v.e[3]*255};
}

// rectangles are always defined with [x,y] in the bottom left
struct rect {
  float x,y,w,h;
};
typedef struct rect rect;

float *rgba2floats(float *r, struct rgba c);

static inline float lerp(float f, float a, float b)
{
  return a * (1.0-f)+(b*f);
}

#endif
