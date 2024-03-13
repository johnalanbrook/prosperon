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
  gameobject *go;
  texture *tex;
  struct rect frame;
  int enabled;
  int drawmode;
  float parallax;
  unsigned int next;
};

typedef struct sprite sprite;

sprite *sprite_make();
int make_sprite(gameobject *go);
void sprite_free(sprite *sprite);
void sprite_delete(int id);
void sprite_initialize();
void sprite_draw(struct sprite *sprite);
void sprite_draw_all();
void sprite_flush();

void gui_draw_img(const char *img, transform2d t, int wrap, HMM_Vec2 wrapoffset, float wrapscale, struct rgba color);

#endif
