#ifndef SPRITE_H
#define SPRITE_H

#include <stdio.h>
#include "timer.h"
#include "mathc.h"
#include "texture.h"

struct datastream;
struct gameobject;

struct sprite {
    mfloat_t pos[2];
    mfloat_t size[2];
    float rotation;
    mfloat_t color[3];
    int go;
    int id;
    struct anim2d anim;
    struct Texture *tex;
    int next;
    int enabled;
};


int make_sprite(int go);
struct sprite *id2sprite(int id);
void sprite_delete(int id);
void sprite_enabled(int id, int e);
void sprite_io(struct sprite *sprite, FILE *f, int read);
void sprite_loadtex(struct sprite *sprite, const char *path);
void sprite_settex(struct sprite *sprite, struct Texture *tex);
void sprite_initialize();
void sprite_draw(struct sprite *sprite);
void video_draw(struct datastream *ds, mfloat_t pos[2], mfloat_t size[2], float rotate, mfloat_t color[3]);
void sprite_draw_all();
unsigned int incrementAnimFrame(unsigned int interval, struct sprite *sprite);


void gui_draw_img(const char *img, float x, float y);


#endif
