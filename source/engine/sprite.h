#ifndef SPRITE_H
#define SPRITE_H

#include <stdio.h>
#include "timer.h"
#include "texture.h"
#include "HandmadeMath.h"
#include "render.h"


struct datastream;
struct gameobject;

struct sprite {
    HMM_Vec2 pos;
    HMM_Vec2 size;
    float rotation;
    struct rgba color;
    int go;
    int id;
    struct Texture *tex;
    struct glrect frame;
    int next;
    int enabled;
    int layer;
};

int make_sprite(int go);
struct sprite *id2sprite(int id);
void sprite_delete(int id);
void sprite_enabled(int id, int e);
void sprite_io(struct sprite *sprite, FILE *f, int read);
void sprite_loadtex(struct sprite *sprite, const char *path, struct glrect rect);
void sprite_settex(struct sprite *sprite, struct Texture *tex);
void sprite_setanim(struct sprite *sprite, struct TexAnim *anim, int frame);
void sprite_setframe(struct sprite *sprite, struct glrect *frame);
void sprite_initialize();
void sprite_draw(struct sprite *sprite);
void video_draw(struct datastream *ds, HMM_Vec2 pos, HMM_Vec2 size, float rotate, struct rgba color);
void sprite_draw_all();
unsigned int incrementAnimFrame(unsigned int interval, struct sprite *sprite);
void sprite_flush();

void gui_draw_img(const char *img, float x, float y);


#endif
