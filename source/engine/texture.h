#ifndef TEXTURE_H
#define TEXTURE_H

#include "timer.h"
#include <chipmunk/chipmunk.h>
#include "sokol/sokol_gfx.h"


#define TEX_SPEC 0
#define TEX_NORM 1
#define TEX_HEIGHT 2
#define TEX_DIFF 3

/* Normalized S,T coordinates for rendering */
struct glrect {
    float s0;
    float s1;
    float t0;
    float t1;
};

float st_s_w(struct glrect st);
float st_s_h(struct glrect st);

extern struct glrect ST_UNIT;

/* Pixel U,V coordiantes */
struct uvrect {
    int u0;
    int u1;
    int v0;
    int v1;
};

/* Tracks a playing animation */
/* Objects should keep this, and just change what TexAnim they are pointing to */
struct anim2d {
    int frame;
    int playing;
    int pausetime;
    struct timer *timer;
    struct TexAnim *anim;
    float size[2]; /* Current size of animation in pixels*/
};

/* Describes an animation on a particular texture */
struct TexAnim {
    struct Texture *tex;
    struct glrect *st_frames; /* Dynamic array of frames of animation */
    int ms;
    int loop;
};

struct TextureOptions {
    int sprite;
    int mips;
    unsigned int gamma:1;
    int animation;
    int wrapx;
    int wrapy;
};

/* Represents an actual texture on the GPU */
struct Texture {
    sg_image id; /* ID reference for the GPU memory location of the texture */
    uint16_t width;
    uint16_t height;
    unsigned char *data;
    struct TextureOptions opts;
    struct TexAnim anim;
};

struct Image {
  struct Texture *tex;
  struct glrect frame;
};

struct Texture *texture_pullfromfile(const char *path);   // Create texture from image
struct Texture *texture_loadfromfile(const char *path);    // Create texture & load to gpu
void texture_sync(const char *path);
struct Texture *str2tex(const char *path);
void tex_gpu_reload(struct Texture *tex);    // gpu_free then gpu_load
void tex_gpu_free(struct Texture *tex);     // Remove texture data from gpu
void tex_bind(struct Texture *tex);    // Bind to gl context

char * tex_get_path(struct Texture *tex);   // Get image path for texture

struct TexAnim *anim2d_from_tex(const char *path, int frames, int fps);
void texanim_fromframes(struct TexAnim *anim, int frames);

void anim_load(struct anim2d *anim, const char *path); /* Load and start new animation */
void anim_calc(struct anim2d *anim);
void anim_play(struct anim2d *anim);
void anim_setframe(struct anim2d *anim, int frame);
void anim_stop(struct anim2d *anim);
void anim_pause(struct anim2d *anim);
void anim_fwd(struct anim2d *anim);
void anim_bkwd(struct anim2d *anim);
void anim_incr(struct anim2d  *anim);
void anim_decr(struct anim2d *anim);

struct glrect tex_get_rect(struct Texture *tex);
cpVect tex_get_dimensions(struct Texture *tex);
struct glrect anim_get_rect(struct anim2d *anim);

int anim_frames(struct TexAnim *a);

#endif
