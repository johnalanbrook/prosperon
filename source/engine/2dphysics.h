#ifndef TWODPHYSICS_H
#define TWODPHYSICS_H

#include "script.h"
#include <chipmunk/chipmunk.h>
#include "gameobject.h"
#include "render.h"
#include "transform.h"

extern float phys2d_gravity;
extern int physOn;
extern cpSpace *space;

extern struct rgba disabled_color;
extern struct rgba dynamic_color;
extern struct rgba kinematic_color;
extern struct rgba static_color;
extern struct rgba sleep_color;

struct phys2d_shape {
  cpShape *shape; /* user data is this phys2d_shape */
  transform2d t;
  gameobject *go;
  void *data; /* The specific subtype; phys2d_circle, etc */
  void (*debugdraw)(void *data);
  float (*moi)(void *data);
  void (*apply)(void *data);
  void (*free)(void *data);
};

void phys2d_shape_apply(struct phys2d_shape *s);

/* Circles are the fastest colldier type */
struct phys2d_circle {
  float radius;
  HMM_Vec2 offset;
  struct phys2d_shape shape;
};

/* A convex polygon; defined as the convex hull around the given set of points */
struct phys2d_poly {
  HMM_Vec2 *points;
  transform2d t;
  float radius;
  struct phys2d_shape shape;
};

/* An edge with no volume. Cannot collide with each other. Join to make levels. Static only. */
struct phys2d_edge {
  HMM_Vec2 *points; /* Points defined relative to the gameobject */
  float thickness;
  cpShape **shapes;
  struct phys2d_shape shape;
  int draws;
};

struct phys2d_circle *Make2DCircle(gameobject *go);
void phys2d_circledel(struct phys2d_circle *c);
void phys2d_applycircle(struct phys2d_circle *circle);
void phys2d_dbgdrawcircle(struct phys2d_circle *circle);
float phys2d_circle_moi(struct phys2d_circle *c);

struct phys2d_poly *Make2DPoly(gameobject *go);
void phys2d_poly_free(struct phys2d_poly *poly);
void phys2d_polydel(struct phys2d_poly *poly);
void phys2d_applypoly(struct phys2d_poly *poly);
void phys2d_dbgdrawpoly(struct phys2d_poly *poly);
void phys2d_polyaddvert(struct phys2d_poly *poly);
void phys2d_poly_setverts(struct phys2d_poly *poly, HMM_Vec2 *verts);
float phys2d_poly_moi(struct phys2d_poly *poly);

struct phys2d_edge *Make2DEdge(gameobject *go);
void phys2d_edge_free(struct phys2d_edge *edge);
void phys2d_edgedel(struct phys2d_edge *edge);
void phys2d_applyedge(struct phys2d_edge *edge);
void phys2d_dbgdrawedge(struct phys2d_edge *edge);
void phys2d_edgeaddvert(struct phys2d_edge *edge, HMM_Vec2 v);
void phys2d_edge_rmvert(struct phys2d_edge *edge, int index);
float phys2d_edge_moi(struct phys2d_edge *edge);

void phys2d_edge_setvert(struct phys2d_edge *edge, int index, cpVect val);
void phys2d_edge_clearverts(struct phys2d_edge *edge);
void phys2d_edge_rmvert(struct phys2d_edge *edge, int index);
void phys2d_edge_update_verts(struct phys2d_edge *edge, HMM_Vec2 *verts);
void phys2d_edge_addverts(struct phys2d_edge *edge, HMM_Vec2 *verts);
void phys2d_edge_set_sensor(struct phys2d_edge *edge, int sensor);
void phys2d_edge_set_enabled(struct phys2d_edge *edge, int enabled);

void phys2d_init();
void phys2d_update(float deltaT);
cpShape *phys2d_query_pos(cpVect pos);
gameobject **phys2d_query_box(HMM_Vec2 pos, HMM_Vec2 wh);

struct shape_cb {
  struct phys2d_shape *shape;
  struct phys_cbs cbs;
};

void fire_hits();

void phys2d_rm_go_handlers(gameobject *go);
void phys2d_set_gravity(cpVect v);

void shape_enabled(struct phys2d_shape *shape, int enabled);
int shape_is_enabled(struct phys2d_shape *shape);
void shape_set_sensor(struct phys2d_shape *shape, int sensor);
int shape_get_sensor(struct phys2d_shape *shape); 

struct rgba shape_color_s(cpShape *shape);

void shape_gui(struct phys2d_shape *shape);
void phys2d_setup_handlers(gameobject *go);
gameobject **phys2d_query_shape(struct phys2d_shape *shape);
int *phys2d_query_box_points(HMM_Vec2 pos, HMM_Vec2 wh, HMM_Vec2 *points, int n);
int query_point(HMM_Vec2 pos);

void flush_collide_cbs();

void phys2d_reindex_body(cpBody *body);
extern unsigned int category_masks[32];
void set_cat_mask(int cat, unsigned int mask);
int phys2d_in_air(cpBody *body);
#endif
