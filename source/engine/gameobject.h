#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include "2dphysics.h"
#include "config.h"
#include <chipmunk/chipmunk.h>
#include <stdbool.h>
#include <stdio.h>
#include "quickjs/quickjs.h"
#include "HandmadeMath.h"

struct shader;
struct sprite;
struct component;

typedef struct transform2d {
  HMM_Vec2 pos;
  HMM_Vec2 scale;
  double angle;
} transform2d;

transform2d mat2transform2d(HMM_Mat3 mat);
HMM_Mat3 transform2d2mat(transform2d t);

typedef struct gameobject {
  cpBodyType bodytype;
  int next;
  float scale;
  HMM_Vec3 scale3;
  float mass;
  float f;   /* friction */
  float e;   /* elasticity */
  float maxvelocity;
  float maxangularvelocity;
  int gravity;
  float damping;
  int flipx; /* 1 or -1 */
  int flipy;
  int sensor;
  unsigned int layer;
  cpShapeFilter filter;
  cpBody *body; /* NULL if this object is dead */
  int id;
  struct phys_cbs cbs;
  struct shape_cb *shape_cbs;
  JSValue ref;
  HMM_Mat3 transform;
  struct gameobject *master;
  transform2d t; /* The local transformation of this object */
} gameobject;

extern struct gameobject *gameobjects;

int MakeGameobject();
void gameobject_apply(struct gameobject *go);
void gameobject_delete(int id);
void gameobjects_cleanup();

void gameobject_set_sensor(int id, int sensor);

HMM_Vec2 go2pos(struct gameobject *go);
float go2angle(struct gameobject *go);
transform2d go2t(gameobject *go);

struct gameobject *get_gameobject_from_id(int id);
struct gameobject *id2go(int id);
int id_from_gameobject(struct gameobject *go);
int go2id(struct gameobject *go);
int body2id(cpBody *body);
cpBody *id2body(int id);
int shape2gameobject(cpShape *shape);
struct gameobject *shape2go(cpShape *shape);
uint32_t go2category(struct gameobject *go);
uint32_t go2mask(struct gameobject *go);

void go_shape_apply(cpBody *body, cpShape *shape, struct gameobject *go);

/* Tries a few methods to select a gameobject; if none is selected returns -1 */
int pos2gameobject(cpVect pos);

void gameobject_move(struct gameobject *go, cpVect vec);
void gameobject_rotate(struct gameobject *go, float as);
void gameobject_setangle(struct gameobject *go, float angle);
void gameobject_setpos(struct gameobject *go, cpVect vec);

void gameobject_draw_debugs();
void gameobject_draw_debug(int go);

void object_gui(struct gameobject *go);

void gameobject_saveall();
void gameobject_loadall();
int gameobjects_saved();

#endif
