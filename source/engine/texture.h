#ifndef TEXTURE_H
#define TEXTURE_H

#include "sokol/sokol_gfx.h"
#include "HandmadeMath.h"
#include "render.h"

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
  int width;
  int height;
  unsigned char *data;
  int frames;
  int *delays;
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

struct texture *texture_from_file(const char *path);   // Create texture from image
struct texture *texture_fromdata(void *raw, long size);

void texture_free(texture *tex);

/* Hot reloads a texture, if needed */
void texture_sync(const char *path);

char * tex_get_path(struct texture *tex);   // Get image path for texture

int gif_nframes(const char *path);
int *gif_delays(const char *path);

struct glrect tex_get_rect(struct texture *tex);
HMM_Vec2 tex_get_dimensions(struct texture *tex);

double perlin(double x, double y, double z);

#endif
