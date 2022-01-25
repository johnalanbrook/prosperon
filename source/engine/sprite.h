#ifndef SPRITE_H
#define SPRITE_H

#include <SDL2/SDL_timer.h>
#include "mathc.h"

struct datastream;
struct mGameObject;
struct Texture;

struct Anim2D {
    int frames;
    int frame;
    int dimensions[2];
    SDL_TimerID timer;
    int ms;
};

struct mSprite {
    struct Texture *tex;
    mfloat_t pos[2];
    mfloat_t size[2];
    float rotation;
    mfloat_t color[3];

    struct Anim2D anim;
    struct mGameObject *go;
};

extern struct mSprite *sprites[100];
extern int numSprites;

struct mSprite *MakeSprite(struct mGameObject *go);
void sprite_init(struct mSprite *sprite, struct mGameObject *go);
void sprite_loadtex(struct mSprite *sprite, const char *path);
void sprite_loadanim(struct mSprite *sprite, const char *path,
		     struct Anim2D anim);
void sprite_settex(struct mSprite *sprite, struct Texture *tex);
void sprite_initalize();
void sprite_draw(struct mSprite *sprite);
void spriteanim_draw(struct mSprite *sprite);
void video_draw(struct datastream *ds, mfloat_t pos[2], mfloat_t size[2],
		float rotate, mfloat_t color[3]);
Uint32 incrementAnimFrame(Uint32 interval, struct mSprite *sprite);

struct mSprite *gui_makesprite();
void gui_init();

void sprite_draw_all();


#endif
