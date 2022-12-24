#ifndef TEXTURE_H
#define TEXTURE_H

#include "timer.h"

#define TEX_SPEC 0
#define TEX_NORM 1
#define TEX_HEIGHT 2
#define TEX_DIFF 3


struct Rect {
    float x;
    float y;
    float w;
    float h;
};

struct TexAnimation {
    int frame;
    int playing;
    int pausetime;
    struct timer *timer;
    struct Rect uv;
    struct Texture *tex;
};

struct TexAnim {
    int frames;
    int dimensions[2];
    int ms;
    int loop;
};

struct TextureOptions {
    int sprite;
    unsigned int gamma:1;
    int animation;
};

struct Texture {
    int type;
    unsigned int id;
    int width;
    int height;
    short flipy;
    unsigned char *data;   // Pixel data of the texture, loaded in at runtime

    struct TextureOptions opts;
    struct TexAnim anim;
};

struct Texture *texture_pullfromfile(const char *path);   // Create texture from image
struct Texture *texture_loadfromfile(const char *path);    // Create texture & load to gpu
void tex_gpu_load(struct Texture *tex);       // Send texture data to gpu
void tex_gpu_reload(struct Texture *tex);    // gpu_free then gpu_load
void tex_gpu_free(struct Texture *tex);     // Remove texture data from gpu
void tex_free(struct Texture *tex);    // Delete struct
void tex_flush(struct Texture *tex);     // Remove pixel data from struct
void tex_pull(struct Texture *tex);    // Pull pixel data from image
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
void tex_anim_calc_uv(struct TexAnimation *anim);
void tex_anim_set(struct TexAnimation *anim);




#endif
