#ifndef SPRITE_H
#define SPRITE_H

#include "texture.h"
#include "HandmadeMath.h"
#include "render.h"
#include "transform.h"
#include "gameobject.h"

struct sprite {
  HMM_Vec2 pos;
  HMM_Vec2 scale;
  float angle;
  struct rgba color;
  struct rgba emissive;
  HMM_Vec2 spritesize;
  HMM_Vec2 spriteoffset;
};

struct spriteuni {
  HMM_Vec4 color;
  HMM_Vec4 emissive;
  HMM_Vec2 size;
  HMM_Vec2 offset;
  HMM_Mat4 model;
};

typedef struct sprite sprite;

extern sg_bindings bind_sprite;
extern sg_pipeline pip_sprite;

sprite *sprite_make();
void sprite_free(sprite *sprite);
void sprite_tex(texture *t);
void sprite_initialize();
void tex_draw(texture *tex, gameobject *go);
void sprite_draw(struct sprite *sprite, gameobject *go);
void sprite_pipe();
void sprite_draw_all();
void sprite_setpipe(sg_pipeline p);

void gui_draw_img(texture *tex, transform2d t, int wrap, HMM_Vec2 wrapoffset, float wrapscale, struct rgba color);

#endif
