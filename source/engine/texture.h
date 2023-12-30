#ifndef TEXTURE_H
#define TEXTURE_H

#include "sokol/sokol_gfx.h"
#include "HandmadeMath.h"
#include "render.h"

#define TEX_SPEC 0
#define TEX_NORM 1
#define TEX_HEIGHT 2
#define TEX_DIFF 3

float st_s_w(struct glrect st);
float st_s_h(struct glrect st);

extern struct glrect ST_UNIT;

struct TextureOptions {
    int sprite;
    int mips;
    unsigned int gamma:1;
    int wrapx;
    int wrapy;
};

/* Represents an actual texture on the GPU */
struct Texture {
    sg_image id; /* ID reference for the GPU memory location of the texture */
    int width;
    int height;
    unsigned char *data;
    struct TextureOptions opts;
  int frames;
  int *delays;
};

typedef struct Texture texture;

struct Image {
  struct Texture *tex;
  struct glrect frame;
};

struct Texture *texture_pullfromfile(const char *path);   // Create texture from image
struct Texture *texture_fromdata(void *raw, long size);

/* Hot reloads a texture, if needed */
void texture_sync(const char *path);

char * tex_get_path(struct Texture *tex);   // Get image path for texture

int gif_nframes(const char *path);
int *gif_delays(const char *path);

struct glrect tex_get_rect(struct Texture *tex);
HMM_Vec2 tex_get_dimensions(struct Texture *tex);

#endif
