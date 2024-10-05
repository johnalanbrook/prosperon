#ifndef TEXTURE_H
#define TEXTURE_H

#include "sokol/sokol_gfx.h"
#include "HandmadeMath.h"
#include "render.h"

#include "stb_rect_pack.h";

#include "sokol_app.h"
#include "sokol/util/sokol_imgui.h"

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
  sg_image id; /* ID reference for the GPU memory location of the texture */
  simgui_image_t simgui;
  int width;
  int height;
  HMM_Vec3 dimensions;
  unsigned char *data;
  int frames;
  int *delays;
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

texture *texture_from_file(const char *path);
texture *texture_fromdata(void *raw, long size);
texture *texture_empty(int width, int height); // Make an empty texture
texture *texture_dup(texture *tex); // return an identical texture
texture *texture_scale(texture *tex, int width, int height); // dup and scale the texture

void texture_free(texture *tex);
void texture_offload(texture *tex); // Remove the data from this texture
void texture_load_gpu(texture *tex); // Upload this data to the GPU if it isn't already there. Replace it if it is.

int texture_write_pixel(texture *tex, int x, int y, struct rgba color);
int texture_fill(texture *tex, struct rgba color);
int texture_fill_rect(texture *tex, struct rect rect, struct rgba color);
int texture_blit(texture *dst, texture *src, struct rect dstrect, struct rect srcrect, int tile); // copies src into dst, using their respective squares, scaling if necessary
int texture_flip(texture *tex, int y);

void texture_save(texture *tex, const char *file); // save the texture data to the given file

#endif
