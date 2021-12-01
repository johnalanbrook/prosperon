#ifndef TEXTURE_H
#define TEXTURE_H

#include <SDL_timer.h>

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
    SDL_TimerID timer;
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
    char *type;
    unsigned int id;
    char *path;
    unsigned int width;
    unsigned int height;
    short flipy;
    unsigned char *data;

    struct TextureOptions opts;
    struct TexAnim anim;
};




struct Texture *texture_loadfromfile(struct Texture *tex,
				     const char *path);
void tex_gpu_load(struct Texture *tex);
void tex_reload(struct Texture *tex);
void tex_free(struct Texture *tex);
void tex_bind(struct Texture *tex);
unsigned int powof2(unsigned int num);
int ispow2(int num);

void anim_play(struct TexAnimation *anim);
void anim_stop(struct TexAnimation *anim);
void anim_pause(struct TexAnimation *anim);
void anim_fwd(struct TexAnimation *anim);
void anim_bkwd(struct TexAnimation *anim);
void anim_incr(struct TexAnimation  *anim);
void anim_decr(struct TexAnimation *anim);

Uint32 tex_incr_anim(Uint32 interval, struct TexAnimation *tex_anim);
void tex_anim_calc_uv(struct TexAnimation *anim);
void tex_anim_set(struct TexAnimation *anim);


#endif
