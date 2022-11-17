#ifndef SPRITE_H
#define SPRITE_H

#include <stdio.h>
#include "timer.h"
#include "mathc.h"

struct datastream;
struct mGameObject;
struct Texture;

struct timer;

struct Anim2D {
    int frames;
    int frame;
    int dimensions[2];
    struct timer *timer;
    int ms;
};

struct sprite {
    mfloat_t pos[2];
    mfloat_t size[2];
    float rotation;
    mfloat_t color[3];
    int index;

    struct Anim2D anim;
    struct mGameObject *go;
    struct Texture *tex;
};

struct sprite *make_sprite(struct mGameObject *go);
void sprite_delete(struct sprite *sprite);
void sprite_init(struct sprite *sprite, struct mGameObject *go);
void sprite_io(struct sprite *sprite, FILE *f, int read);
void sprite_loadtex(struct sprite *sprite, const char *path);
void sprite_loadanim(struct sprite *sprite, const char *path, struct Anim2D anim);
void sprite_settex(struct sprite *sprite, struct Texture *tex);
void sprite_initialize();
void sprite_draw(struct sprite *sprite);
void spriteanim_draw(struct sprite *sprite);
void video_draw(struct datastream *ds, mfloat_t pos[2], mfloat_t size[2], float rotate, mfloat_t color[3]);
void sprite_draw_all();
unsigned int incrementAnimFrame(unsigned int interval, struct sprite *sprite);


#endif
