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
  transform2d t;
  struct rgba color;
  struct rgba emissive;
  gameobject *go;
  struct Texture *tex;
  struct glrect frame;
  int enabled;
  int next;
  int drawmode;
  float parallax;
};

int make_sprite(gameobject *go);
struct sprite *id2sprite(int id);
void sprite_delete(int id);
void sprite_enabled(int id, int e);
void sprite_loadtex(struct sprite *sprite, const char *path, struct glrect rect);
void sprite_settex(struct sprite *sprite, struct Texture *tex);
void sprite_setframe(struct sprite *sprite, struct glrect *frame);
void sprite_initialize();
void sprite_draw(struct sprite *sprite);
void sprite_draw_all();
unsigned int incrementAnimFrame(unsigned int interval, struct sprite *sprite);
void sprite_flush();

void gui_draw_img(const char *img, transform2d t, int wrap, HMM_Vec2 wrapoffset, float wrapscale, struct rgba color);

#endif
