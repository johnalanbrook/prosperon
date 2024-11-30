#ifndef OPENGL_RENDER_H
#define OPENGL_RENDER_H

#include "HandmadeMath.h"
#include <SDL3/SDL.h>

#define RGBA_MAX 255

extern struct rgba color_white;
extern struct rgba color_black;
extern struct rgba color_clear;
extern int TOPLEFT;

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

typedef struct rgba rgba;

static inline rgba vec2rgba(HMM_Vec4 v) {
  return (rgba){v.e[0]*255,v.e[1]*255,v.e[2]*255,v.e[3]*255};
}

// rectangles are always defined with [x,y] in the bottom left
struct rect {
  float x,y,w,h;
};

float *rgba2floats(float *r, struct rgba c);

static inline float lerp(float f, float a, float b)
{
  return a * (1.0-f)+(b*f);
}

#endif
