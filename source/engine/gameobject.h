#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include "quickjs/quickjs.h"
#include "HandmadeMath.h"
#include "transform.h"
#include "script.h"
#include "warp.h"

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

struct gameobject {
  cpBodyType bodytype;
  cpBody *body; /* NULL if this object is dead; has 2d position and rotation, relative to global 0 */  
  HMM_Vec3 scale; /* local */  
  int next;
  float mass;
  float f;   /* friction */
  float e;   /* elasticity */
  float timescale;
  float maxvelocity;
  float maxangularvelocity;
  HMM_Vec2 gravity; /* its own gravity */
  float damping;
  unsigned int layer;
  cpShapeFilter filter;
//  warpmask warpmask;
  struct phys_cbs cbs;
  struct shape_cb *shape_cbs;
  JSValue ref;
  HMM_Mat4 world;
  float drawlayer;
};

typedef struct gameobject gameobject;

gameobject *MakeGameobject();
int go_count();
void gameobject_apply(gameobject *go);
void gameobject_free(gameobject *go);

transform2d go2t(gameobject *go);
transform3d go2t3(gameobject *go);

HMM_Vec2 go2world(gameobject *go, HMM_Vec2 pos);
HMM_Vec2 world2go(gameobject *go, HMM_Vec2 pos);

HMM_Mat3 t_go2world(gameobject *go);
HMM_Mat3 t_world2go(gameobject *go);
HMM_Mat4 t3d_go2world(gameobject *go);
HMM_Mat4 t3d_world2go(gameobject *go);

HMM_Vec2 go_pos(gameobject *go);
void gameobject_setpos(gameobject *go, cpVect vec);
float go_angle(gameobject *go);
void gameobject_setangle(gameobject *go, float angle);

gameobject *body2go(cpBody *body);
gameobject *shape2go(cpShape *shape);

void go_shape_apply(cpBody *body, cpShape *shape, gameobject *go);

/* Tries a few methods to select a gameobject; if none is selected returns -1 */
gameobject *pos2gameobject(HMM_Vec2 pos, float give);

void gameobject_draw_debug(gameobject *go);
void gameobject_draw_debugs();
#endif
