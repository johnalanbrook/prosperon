#ifndef TWODPHYSICS_H
#define TWODPHYSICS_H

#include <chipmunk/chipmunk.h>


struct mGameObject;

extern cpBody *ballBody;
extern float phys2d_gravity;
extern int physOn;
extern cpSpace *space;

struct phys2d_shape {
    cpShape *shape;
    struct mGameObject *go;
};

struct phys2d_circle {
    float radius;
    float offset[2];
    struct phys2d_shape shape;
};

struct phys2d_segment {
    float a[2];
    float b[2];
    float thickness;
    struct phys2d_shape shape;
};

struct phys2d_box {
    float w;
    float h;
    float offset[2];
    float r;
    struct phys2d_shape shape;
};

struct phys2d_edge {
    int n;
    float *points;
    float thickness;
    cpShape **shapes;
    struct phys2d_shape shape;
};

struct phys2d_poly {
    int n;
    float *points;
    float radius;
    struct phys2d_shape shape;
};

struct phys2d_circle *Make2DCircle(struct mGameObject *go);
void phys2d_circleinit(struct phys2d_circle *circle,
		       struct mGameObject *go);
void phys2d_applycircle(struct phys2d_circle *circle);
void phys2d_dbgdrawcircle(struct phys2d_circle *circle);
void circle_gui(struct phys2d_circle *circle);

struct phys2d_segment *Make2DSegment(struct mGameObject *go);
void phys2d_seginit(struct phys2d_segment *seg, struct mGameObject *go);
void phys2d_applyseg(struct phys2d_segment *seg);
void phys2d_dbgdrawseg(struct phys2d_segment *seg);
void segment_gui(struct phys2d_segment *seg);

struct phys2d_box *Make2DBox(struct mGameObject *go);
void phys2d_boxinit(struct phys2d_box *box, struct mGameObject *go);
void phys2d_applybox(struct phys2d_box *box);
void phys2d_dbgdrawbox(struct phys2d_box *box);
void box_gui(struct phys2d_box *box);

struct phys2d_poly *Make2DPoly(struct mGameObject *go);
void phys2d_polyinit(struct phys2d_poly *poly, struct mGameObject *go);
void phys2d_applypoly(struct phys2d_poly *poly);
void phys2d_dbgdrawpoly(struct phys2d_poly *poly);
void phys2d_polyaddvert(struct phys2d_poly *poly);
void poly_gui(struct phys2d_poly *poly);

struct phys2d_edge *Make2DEdge(struct mGameObject *go);
void phys2d_edgeinit(struct phys2d_edge *edge, struct mGameObject *go);
void phys2d_applyedge(struct phys2d_edge *edge);
void phys2d_edgeshapeapply(struct phys2d_shape *mshape, cpShape * shape);
void phys2d_dbgdrawedge(struct phys2d_edge *edge);
void phys2d_edgeaddvert(struct phys2d_edge *edge);
void edge_gui(struct phys2d_edge *edge);


void phys2d_init();
void phys2d_update(float deltaT);
void phys2d_apply();

void shape_gui(struct phys2d_shape *shape);

#endif
