#ifndef SPRITE_H
#define SPRITE_H

#include "texture.h"
#include "HandmadeMath.h"
#include "render.h"
#include "transform.h"
#include "gameobject.h"

typedef struct sprite {
  struct rgba color;
  struct rgba emissive;
  HMM_Vec4 rect;
  HMM_Vec2 spriteoffset;
} sprite;

sprite *sprite_make();
void sprite_free(sprite *sprite);

#endif
