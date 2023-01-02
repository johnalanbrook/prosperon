#ifndef TEXTURE_H
#define TEXTURE_H

#include "timer.h"

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

#define ST_UNIT (struct glrect) { 0.f, 1.f, 0.f, 1.f }

/* Pixel U,V coordiantes */
struct uvrect {
    int u0;
    int u1;
    int v0;
    int v1;
};

/* Tracks a playing animation */
struct TexAnimation {
    int frame;
    int playing;
    int pausetime;
    struct timer *timer;
    struct TexAnim *anim;
    int loop;
};

/* Describes an animation on a particular texture */
struct TexAnim {
    struct glrect *st_frames; /* Dynamic array of frames of animation */
    int ms;
    struct Texture *tex;
};

struct TextureOptions {
    int sprite;
    unsigned int gamma:1;
    int animation;
};

struct Texture {
    unsigned int id; /* ID reference for the GPU memory location of the texture */
    int width;
    int height;

    struct TextureOptions opts;
    struct TexAnim anim;
};

struct Texture *texture_pullfromfile(const char *path);   // Create texture from image
struct Texture *texture_loadfromfile(const char *path);    // Create texture & load to gpu
void tex_gpu_reload(struct Texture *tex);    // gpu_free then gpu_load
void tex_gpu_free(struct Texture *tex);     // Remove texture data from gpu
void tex_bind(struct Texture *tex);    // Bind to gl context

char * tex_get_path(struct Texture *tex);   // Get image path for texture

void anim_play(struct TexAnimation *anim);
void anim_setframe(struct TexAnimation *anim, int frame);
void anim_stop(struct TexAnimation *anim);
void anim_pause(struct TexAnimation *anim);
void anim_fwd(struct TexAnimation *anim);
void anim_bkwd(struct TexAnimation *anim);
void anim_incr(struct TexAnimation  *anim);
void anim_decr(struct TexAnimation *anim);

void tex_incr_anim(struct TexAnimation *tex_anim);
void tex_anim_set(struct TexAnimation *anim);

struct glrect tex_get_rect(struct Texture *tex);
struct glrect anim_get_rect(struct TexAnimation *anim);

int anim_frames(struct TexAnim *a);

#endif
