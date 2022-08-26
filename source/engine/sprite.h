#ifndef SPRITE_H
#define SPRITE_H

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

struct mSprite {
    struct Texture *tex;
    mfloat_t pos[2];
    mfloat_t size[2];
    float rotation;
    mfloat_t color[3];
    int index;

    struct Anim2D anim;
    struct mGameObject *go;
};

struct mSprite *MakeSprite(struct mGameObject *go);
void sprite_delete(struct mSprite *sprite);
void sprite_init(struct mSprite *sprite, struct mGameObject *go);
void sprite_loadtex(struct mSprite *sprite, const char *path);
void sprite_loadanim(struct mSprite *sprite, const char *path, struct Anim2D anim);
void sprite_settex(struct mSprite *sprite, struct Texture *tex);
void sprite_initialize();
void sprite_draw(struct mSprite *sprite);
void spriteanim_draw(struct mSprite *sprite);
void video_draw(struct datastream *ds, mfloat_t pos[2], mfloat_t size[2], float rotate, mfloat_t color[3]);
void sprite_draw_all();
unsigned int incrementAnimFrame(unsigned int interval, struct mSprite *sprite);


#endif
