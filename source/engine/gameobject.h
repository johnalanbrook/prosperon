#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include "2dphysics.h"
#include "config.h"
#include <chipmunk/chipmunk.h>
#include <stdbool.h>
#include <stdio.h>
#include "quickjs/quickjs.h"
#include "HandmadeMath.h"
#include "transform.h"

#define dag_rm(p,c) do{\
 for (int i = arrlen(p->children)-1; i--; i >=0) {\
  if (p->children[i] == c) { \
  arrdelswap(p->children,i);\
  c->parent=NULL;\
  break;\
}}}while(0)

#define dag_set(p,c) do{\
  arrpush(p->children,c);\
  if(c->parent) dag_rm(c->parent,c);\
  c->parent=p;\
}while(0)

#define dag_clip(p) do{\
  if (p->parent)\
    dag_rm(p->parent,p);\
}while(0)

struct shader;
struct sprite;
struct component;

typedef struct gameobject {
  cpBodyType bodytype;
  int next;
  HMM_Vec3 scale; /* local */
  float mass;
  float f;   /* friction */
  float e;   /* elasticity */
  float timescale;
  float maxvelocity;
  float maxangularvelocity;
  int gravity;
  HMM_Vec2 cgravity;
  float damping;
  unsigned int layer;
  cpShapeFilter filter;
  cpBody *body; /* NULL if this object is dead */
  int id;
  struct phys_cbs cbs;
  struct shape_cb *shape_cbs;
  JSValue ref;
  struct gameobject *master;
  HMM_Mat4 world;
  transform2d t; /* The local transformation of this object */
  float drawlayer;
  struct gameobject *parent;
  struct gameobject **children;
} gameobject;

extern struct gameobject *gameobjects;

int MakeGameobject();
void gameobject_apply(struct gameobject *go);
void gameobject_delete(int id);
void gameobject_free(int id);
void gameobjects_cleanup();

void gameobject_traverse(struct gameobject *start, HMM_Mat4 p);

transform2d go2t(gameobject *go);
transform3d go2t3(gameobject *go);

HMM_Vec2 go2world(struct gameobject *go, HMM_Vec2 pos);
HMM_Vec2 world2go(struct gameobject *go, HMM_Vec2 pos);

HMM_Mat3 t_go2world(struct gameobject *go);
HMM_Mat3 t_world2go(struct gameobject *go);
HMM_Mat4 t3d_go2world(struct gameobject *go);
HMM_Mat4 t3d_world2go(struct gameobject *go);

HMM_Vec2 go2pos(struct gameobject *go);
float go2angle(struct gameobject *go);

HMM_Vec2 goscale(struct gameobject *go, HMM_Vec2 pos);
HMM_Vec2 gotpos(struct gameobject *go, HMM_Vec2 pos);

HMM_Mat3 mt_rst(transform2d t);

struct gameobject *id2go(int id);
int go2id(struct gameobject *go);
int body2id(cpBody *body);
cpBody *id2body(int id);
int shape2gameobject(cpShape *shape);
struct gameobject *shape2go(cpShape *shape);

void go_shape_apply(cpBody *body, cpShape *shape, struct gameobject *go);

/* Tries a few methods to select a gameobject; if none is selected returns -1 */
int pos2gameobject(HMM_Vec2 pos);

void gameobject_move(struct gameobject *go, HMM_Vec2 vec);
void gameobject_rotate(struct gameobject *go, float as);
void gameobject_setangle(struct gameobject *go, float angle);
void gameobject_setpos(struct gameobject *go, cpVect vec);

void gameobject_draw_debugs();
void gameobject_draw_debug(int go);
#endif
