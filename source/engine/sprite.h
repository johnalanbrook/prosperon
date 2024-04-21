#ifndef SPRITE_H
#define SPRITE_H

#include "texture.h"
#include "HandmadeMath.h"
#include "render.h"
#include "transform.h"
#include "gameobject.h"

#define DRAW_SIMPLE 0
#define DRAW_TILE 1

struct sprite {
  HMM_Vec2 pos;
  HMM_Vec2 scale;
  float angle;
  struct rgba color;
  struct rgba emissive;
  struct rect frame;
  int drawmode;
  float parallax;
};

typedef struct sprite sprite;

extern sg_bindings bind_sprite;

sprite *sprite_make();
void sprite_free(sprite *sprite);
void sprite_tex(texture *t);
void sprite_initialize();
void sprite_draw(struct sprite *sprite, gameobject *go);
void sprite_pipe();
void sprite_draw_all();
void sprite_flush();
void sprite_newframe();

void gui_draw_img(texture *tex, transform2d t, int wrap, HMM_Vec2 wrapoffset, float wrapscale, struct rgba color);

#endif
