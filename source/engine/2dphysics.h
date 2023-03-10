#ifndef TWODPHYSICS_H
#define TWODPHYSICS_H

#include <chipmunk/chipmunk.h>
#include "script.h"

struct gameobject;

extern float phys2d_gravity;
extern int physOn;
extern cpSpace *space;

extern float dbg_color[3];
extern float trigger_color[3];
extern float disabled_color[3];
extern float dynamic_color[3];
extern float kinematic_color[3];
extern float static_color[3];

struct phys2d_shape {
    cpShape *shape;
    int go;
    void *data;
    void (*debugdraw)(void *data);
    float (*moi)(void *data, float mass);
};

/* Circles are the fastest collier type */
struct phys2d_circle {
    float radius;
    cpVect offset;
    struct phys2d_shape shape;
};

/* A single segment */
struct phys2d_segment {
    float a[2];
    float b[2];
    float thickness;
    struct phys2d_shape shape;
};

/* A convex polygon; defined as the convex hull around the given set of points */
struct phys2d_poly {
    cpVect *points;
    float radius;
    struct phys2d_shape shape;
};

/* A box shape; a type of a polygon collider */
struct phys2d_box {
    float w;
    float h;
    float offset[2];
    float rotation;
    float r;
    struct phys2d_shape shape;
};

/* An edge with no volume. Cannot collide with each other. Join to make levels. Static only. */
struct phys2d_edge {
    cpVect *points;
    float thickness;
    cpShape **shapes;
    int closed; /* True if the first and last points should be connected */
    struct phys2d_shape shape;
    int draws;
};

struct phys2d_circle *Make2DCircle(int go);
void phys2d_circledel(struct phys2d_circle *c);
void phys2d_applycircle(struct phys2d_circle *circle);
void phys2d_dbgdrawcircle(struct phys2d_circle *circle);
float phys2d_circle_moi(struct phys2d_circle *c, float m);

struct phys2d_box *Make2DBox(int go);
void phys2d_boxdel(struct phys2d_box *box);
void phys2d_applybox(struct phys2d_box *box);
void phys2d_dbgdrawbox(struct phys2d_box *box);
float phys2d_box_moi(struct phys2d_box *box, float m);

struct phys2d_poly *Make2DPoly(int go);
void phys2d_polydel(struct phys2d_poly *poly);
void phys2d_applypoly(struct phys2d_poly *poly);
void phys2d_dbgdrawpoly(struct phys2d_poly *poly);
void phys2d_polyaddvert(struct phys2d_poly *poly);
void phys2d_poly_setverts(struct phys2d_poly *poly, cpVect *verts);
float phys2d_poly_moi(struct phys2d_poly *poly, float m);

struct phys2d_edge *Make2DEdge(int go);
void phys2d_edgedel(struct phys2d_edge *edge);
void phys2d_applyedge(struct phys2d_edge *edge);
void phys2d_dbgdrawedge(struct phys2d_edge *edge);
void phys2d_edgeaddvert(struct phys2d_edge *edge);
void phys2d_edge_rmvert(struct phys2d_edge *edge, int index);
float phys2d_edge_moi(struct phys2d_edge *edge, float m);

void phys2d_edge_setvert(struct phys2d_edge *edge, int index, cpVect val);
void phys2d_edge_clearverts(struct phys2d_edge *edge);
void phys2d_edge_addverts(struct phys2d_edge *edge, cpVect *verts);
void phys2d_edge_set_sensor(struct phys2d_edge *edge, int sensor);
void phys2d_edge_set_enabled(struct phys2d_edge *edge, int enabled);

void phys2d_init();
void phys2d_update(float deltaT);
cpShape *phys2d_query_pos(cpVect pos);
int *phys2d_query_box(cpVect pos, cpVect wh);

struct phys_cbs {
    struct callee begin;
    struct callee separate;
};

struct shape_cb {
  struct phys2d_shape *shape;
  struct phys_cbs cbs;
};

void fire_hits();

void phys2d_add_handler_type(int cmd, int go,  struct callee c);
void register_collide(void *sym);
void phys2d_rm_go_handlers(int go);
void phys2d_set_gravity(cpVect v);

void shape_enabled(struct phys2d_shape *shape, int enabled);
int shape_is_enabled(struct phys2d_shape *shape);
void shape_set_sensor(struct phys2d_shape *shape, int sensor);
int shape_get_sensor(struct phys2d_shape *shape);

struct color {
    unsigned char r;
    unsigned char g;
    unsigned char b;
};

void color2float(struct color, float *fcolor);
struct color float2color(float *fcolor);

void shape_gui(struct phys2d_shape *shape);
void phys2d_setup_handlers(int go);

void phys2d_reindex_body(cpBody *body);
cpVect world2go(struct gameobject *go, cpVect worldpos);
cpVect go2world(struct gameobject *go, cpVect gopos);
extern unsigned int category_masks[32];
void set_cat_mask(int cat, unsigned int mask);

#endif
