#ifndef TEXTURE_H
#define TEXTURE_H

#include "HandmadeMath.h"
#include "render.h"
#include <quickjs.h>
#include "stb_rect_pack.h"
#include <SDL3/SDL_render.h>

#define TEX_SPEC 0
#define TEX_NORM 1
#define TEX_HEIGHT 2
#define TEX_DIFF 3

#define FILTER_NEAREST SG_FILTER_NEAREST
#define FILTER_NONE SG_FILTER_NONE
#define FILTER_LINEAR SG_FILTER_LINEAR

extern struct rect ST_UNIT;

/* Represents an actual texture on the GPU */
struct texture {
  SDL_Texture *id;
  SDL_Surface *surface;
  int width;
  int height;
  HMM_Vec3 dimensions;
  unsigned char *data;
  int vram;
};

typedef struct texture texture;

typedef struct img_sampler{
  int wrap_u;
  int wrap_v;
  int wrap_w;
  int min_filter;
  int mag_filter;
  int mip_filter;
} img_sampler;

texture *texture_fromdata(void *raw, long size);
texture *texture_empty(int width, int height); // Make an empty texture
texture *texture_dup(texture *tex); // return an identical texture
texture *texture_scale(texture *tex, int width, int height); // dup and scale the texture

void texture_free(JSRuntime *rt,texture *tex);
void texture_offload(texture *tex); // Remove the data from this texture

void texture_save(texture *tex, const char *file); // save the texture data to the given file

#endif
