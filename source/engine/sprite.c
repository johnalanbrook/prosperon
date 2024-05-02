#include "sprite.h"

#include "gameobject.h"
#include "log.h"
#include "render.h"
#include "stb_ds.h"
#include "texture.h"
#include "HandmadeMath.h"

sprite *sprite_make()
{
  sprite *sp = calloc(sizeof(*sp), 1);
  sp->color = color_white;
  sp->emissive = color_clear;
  sp->rect = (HMM_Vec4){0,0,1,1};
  return sp;
}

void sprite_free(sprite *sprite) { free(sprite); }