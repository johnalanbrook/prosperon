#include "2dphysics.h"

#include "gameobject.h"
#include <string.h>
#include "mathc.h"
#include "nuke.h"

#include "debugdraw.h"
#include "gameobject.h"
#include <math.h>
#include <chipmunk/chipmunk_unsafe.h>

cpBody *ballBody = NULL;
cpSpace *space = NULL;
float phys2d_gravity = -50.f;

void phys2d_init()
{
    space = cpSpaceNew();
    cpSpaceSetGravity(space, cpv(0, phys2d_gravity));
}

void phys2d_update(float deltaT)
{
    cpSpaceStep(space, deltaT);
}

void phys2d_apply()
{
    cpSpaceSetGravity(space, cpv(0, phys2d_gravity));
}

void phys2d_shape_apply(struct phys2d_shape *shape)
{
    cpShapeSetFriction(shape->shape, shape->go->f);
    cpShapeSetElasticity(shape->shape, shape->go->e);
}

void init_phys2dshape(struct phys2d_shape *shape, struct mGameObject *go)
{
    shape->go = go;
    phys2d_shape_apply(shape);
}

void phys2d_shape_del(struct phys2d_shape *shape)
{
    cpSpaceRemoveShape(space, shape->shape);
}

struct phys2d_circle *Make2DCircle(struct mGameObject *go)
{
    struct phys2d_circle *new = malloc(sizeof(struct phys2d_circle));

    new->radius = 10.f;
    new->offset[0] = 0.f;
    new->offset[1] = 0.f;
    phys2d_circleinit(new, go);

    return new;
}

void phys2d_circleinit(struct phys2d_circle *circle, struct mGameObject *go)
{
    printf("Initing a circle\n");
    circle->shape.shape = cpSpaceAddShape(space, cpCircleShapeNew(go->body, circle->radius, cpvzero));
    init_phys2dshape(&circle->shape, go);
}

void phys2d_circledel(struct phys2d_circle *c)
{
    phys2d_shape_del(&c->shape);
}

void circle_gui(struct phys2d_circle *circle)
{
    nk_property_float(ctx, "Radius", 1.f, &circle->radius, 10000.f, 1.f, 1.f);
    nk_property_float2(ctx, "Offset", 0.f, circle->offset, 1.f, 0.01f, 0.01f);

    phys2d_applycircle(circle);
}

void phys2d_dbgdrawcircle(struct phys2d_circle *circle)
{
    cpVect p = cpBodyGetPosition(circle->shape.go->body);
    cpVect o = cpCircleShapeGetOffset(circle->shape.shape);
    float d = sqrt(pow(o.x, 2.f) + pow(o.y, 2.f));
    float a = atan2(o.y, o.x) + cpBodyGetAngle(circle->shape.go->body);
    draw_circle(p.x + (d * cos(a)), p.y + (d * sin(a)), cpCircleShapeGetRadius(circle->shape.shape), 1);
}

struct phys2d_segment *Make2DSegment(struct mGameObject *go)
{
    struct phys2d_segment *new = malloc(sizeof(struct phys2d_segment));

    new->thickness = 1.f;
    new->a[0] = 0.f;
    new->a[1] = 0.f;
    new->b[0] = 0.f;
    new->b[1] = 0.f;
    phys2d_seginit(new, go);

    return new;
}

void phys2d_seginit(struct phys2d_segment *seg, struct mGameObject *go)
{
    seg->shape.shape = cpSpaceAddShape(space, cpSegmentShapeNew(go->body, cpvzero, cpvzero, seg->thickness));
    init_phys2dshape(&seg->shape, go);
}

void phys2d_segdel(struct phys2d_segment *seg)
{
    phys2d_shape_del(&seg->shape);
}

void segment_gui(struct phys2d_segment *seg)
{
    nk_property_float2(ctx, "a", 0.f, seg->a, 1.f, 0.01f, 0.01f);
    nk_property_float2(ctx, "b", 0.f, seg->b, 1.f, 0.01f, 0.01f);

    phys2d_applyseg(seg);
}

struct phys2d_box *Make2DBox(struct mGameObject *go)
{
    struct phys2d_box *new = malloc(sizeof(struct phys2d_box));

    new->w = 50.f;
    new->h = 50.f;
    new->r = 0.f;
    new->offset[0] = 0.f;
    new->offset[1] = 0.f;

    phys2d_boxinit(new, go);

    return new;
}

void phys2d_boxinit(struct phys2d_box *box, struct mGameObject *go)
{
    box->shape.shape =
	cpSpaceAddShape(space,
			cpBoxShapeNew(go->body, box->w, box->h, box->r));
    init_phys2dshape(&box->shape, go);
    phys2d_applybox(box);
}

void phys2d_boxdel(struct phys2d_box *box)
{
    phys2d_shape_del(&box->shape);
}

void box_gui(struct phys2d_box *box)
{
    nk_property_float(ctx, "Width", 0.f, &box->w, 1000.f, 1.f, 1.f);
    nk_property_float(ctx, "Height", 0.f, &box->h, 1000.f, 1.f, 1.f);
    nk_property_float2(ctx, "Offset", 0.f, &box->offset, 1.f, 0.01f, 0.01f);

    phys2d_applybox(box);
}


struct phys2d_poly *Make2DPoly(struct mGameObject *go)
{
    struct phys2d_poly *new = malloc(sizeof(struct phys2d_poly));

    new->n = 0;
    new->points = NULL;
    new->radius = 0.f;

    phys2d_polyinit(new, go);

    return new;
}

void phys2d_polyinit(struct phys2d_poly *poly, struct mGameObject *go)
{
    cpTransform T = { 0 };
    poly->shape.shape =
	cpSpaceAddShape(space,
			cpPolyShapeNew(go->body, 0, NULL, T,
				       poly->radius));
    init_phys2dshape(&poly->shape, go);
    phys2d_applypoly(poly);
}

void phys2d_polydel(struct phys2d_poly *poly)
{
    phys2d_shape_del(&poly->shape);
}

void phys2d_polyaddvert(struct phys2d_poly *poly)
{
    poly->n++;
    float *oldpoints = poly->points;
    poly->points = calloc(2 * poly->n, sizeof(float));
    memcpy(poly->points, oldpoints, sizeof(float) * 2 * (poly->n - 1));
    free(oldpoints);
}

void poly_gui(struct phys2d_poly *poly)
{

    if (nk_button_label(ctx, "Add Poly Vertex")) phys2d_polyaddvert(poly);

    for (int i = 0; i < poly->n; i++) {
	nk_property_float2(ctx, "#P", 0.f, &poly->points[i*2], 1.f, 0.1f, 0.1f);
    }

    nk_property_float(ctx, "Radius", 0.01f, &poly->radius, 1000.f, 1.f, 0.1f);

    phys2d_applypoly(poly);
}

struct phys2d_edge *Make2DEdge(struct mGameObject *go)
{
    struct phys2d_edge *new = malloc(sizeof(struct phys2d_edge));

    new->n = 2;
    new->points = calloc(2 * 2, sizeof(float));
    new->thickness = 0.f;
    new->shapes = malloc(sizeof(cpShape *));

    phys2d_edgeinit(new, go);

    return new;
}

void phys2d_edgeinit(struct phys2d_edge *edge, struct mGameObject *go)
{
    edge->shapes[0] =
	cpSpaceAddShape(space,
			cpSegmentShapeNew(go->body, cpvzero, cpvzero,
					  edge->thickness));
    edge->shape.go = go;
    phys2d_edgeshapeapply(&edge->shape, edge->shapes[0]);



    phys2d_applyedge(edge);
}

void phys2d_edgedel(struct phys2d_edge *edge)
{
    phys2d_shape_del(&edge->shape);
}

void phys2d_edgeshapeapply(struct phys2d_shape *mshape, cpShape * shape)
{
    cpShapeSetFriction(shape, mshape->go->f);
    cpShapeSetElasticity(shape, mshape->go->e);
}

void phys2d_edgeaddvert(struct phys2d_edge *edge)
{
    edge->n++;
    float *oldp = edge->points;
    edge->points = calloc(edge->n * 2, sizeof(float));
    memcpy(edge->points, oldp, sizeof(float) * 2 * (edge->n - 1));

    cpShape **oldshapes = edge->shapes;
    edge->shapes = malloc(sizeof(cpShape *) * (edge->n - 1));
    memcpy(edge->shapes, oldshapes, sizeof(cpShape *) * (edge->n - 2));
    cpVect a =
	{ edge->points[(edge->n - 2) * 2],
     edge->points[(edge->n - 2) * 2 + 1] };
    cpVect b =
	{ edge->points[(edge->n - 1) * 2],
     edge->points[(edge->n - 1) * 2 + 1] };
    edge->shapes[edge->n - 2] =
	cpSpaceAddShape(space,
			cpSegmentShapeNew(edge->shape.go->body, a, b,
					  edge->thickness));
    phys2d_edgeshapeapply(&edge->shape, edge->shapes[edge->n - 2]);

    free(oldp);
    free(oldshapes);
}

void edge_gui(struct phys2d_edge *edge)
{
    if (nk_button_label(ctx, "Add Edge Vertex")) phys2d_edgeaddvert(edge);

    for (int i = 0; i < edge->n; i++) {
	//ImGui::PushID(i);
	nk_property_float2(ctx, "E", 0.f, &edge->points[i*2], 1.f, 0.01f, 0.01f);
	//ImGui::PopID();
    }

    nk_property_float(ctx, "Thickness", 0.01f, &edge->thickness, 1.f, 0.01f, 0.01f);

    phys2d_applyedge(edge);
}



void phys2d_applycircle(struct phys2d_circle *circle)
{
    float radius = circle->radius * circle->shape.go->scale;
    float s = circle->shape.go->scale;
    cpVect offset = { circle->offset[0] * s, circle->offset[1] * s };

    cpCircleShapeSetRadius(circle->shape.shape, radius);
    cpCircleShapeSetOffset(circle->shape.shape, offset);
    cpBodySetMoment(circle->shape.go->body,
		    cpMomentForCircle(circle->shape.go->mass, 0, radius,
				      offset));
}

void phys2d_applyseg(struct phys2d_segment *seg)
{
    float s = seg->shape.go->scale;
    cpVect a = { seg->a[0] * s, seg->a[1] * s };
    cpVect b = { seg->b[0] * s, seg->b[1] * s };
    cpSegmentShapeSetEndpoints(seg->shape.shape, a, b);
    cpSegmentShapeSetRadius(seg->shape.shape, seg->thickness * s);
}

void phys2d_applybox(struct phys2d_box *box)
{
    float s = box->shape.go->scale;
    cpTransform T = { 0 };
    T.a = s;
    T.d = s;
    T.tx = box->offset[0] * s;
    T.ty = box->offset[1] * s;
    float hh = box->h / 2.f;
    float hw = box->w / 2.f;
    cpVect verts[4] =
	{ { -hw, -hh }, { hw, -hh }, { hw, hh }, { -hw, hh } };
    cpPolyShapeSetVerts(box->shape.shape, 4, verts, T);
    cpPolyShapeSetRadius(box->shape.shape, box->r);
}

void phys2d_applypoly(struct phys2d_poly *poly)
{
    cpVect verts[poly->n];

    for (int i = 0; i < poly->n; i++) {
	verts[i].x = poly->points[i * 2];
	verts[i].y = poly->points[i * 2 + 1];
    }

    CP_CONVEX_HULL(poly->n, verts, hullCount, hullVerts);

    float s = poly->shape.go->scale;
    cpTransform T = { 0 };
    T.a = s;
    T.d = s;

    cpPolyShapeSetVerts(poly->shape.shape, hullCount, hullVerts, T);
    cpPolyShapeSetRadius(poly->shape.shape, poly->radius);
}

void phys2d_applyedge(struct phys2d_edge *edge)
{
    float s = edge->shape.go->scale;

    for (int i = 0; i < edge->n - 1; i++) {
	cpVect a =
	    { edge->points[i * 2] * s, edge->points[i * 2 + 1] * s };
	cpVect b =
	    { edge->points[i * 2 + 2] * s, edge->points[i * 2 + 3] * s };
	cpSegmentShapeSetEndpoints(edge->shapes[i], a, b);
	cpSegmentShapeSetRadius(edge->shapes[i], edge->thickness);
    }
}



void phys2d_dbgdrawseg(struct phys2d_segment *seg)
{
    cpVect p = cpBodyGetPosition(seg->shape.go->body);
    cpVect a = cpSegmentShapeGetA(seg->shape.shape);
    cpVect b = cpSegmentShapeGetB(seg->shape.shape);

    float angle = cpBodyGetAngle(seg->shape.go->body);
    float ad = sqrt(pow(a.x, 2.f) + pow(a.y, 2.f));
    float bd = sqrt(pow(b.x, 2.f) + pow(b.y, 2.f));
    float aa = atan2(a.y, a.x) + angle;
    float ba = atan2(b.y, b.x) + angle;
    draw_line(ad * cos(aa) + p.x, ad * sin(aa) + p.y, bd * cos(ba) + p.x,
	      bd * sin(ba) + p.y);
}

void phys2d_dbgdrawbox(struct phys2d_box *box)
{
    int n = cpPolyShapeGetCount(box->shape.shape);
    cpVect b = cpBodyGetPosition(box->shape.go->body);
    float angle = cpBodyGetAngle(box->shape.go->body);
    float points[n * 2];

    for (int i = 0; i < n; i++) {
	cpVect p = cpPolyShapeGetVert(box->shape.shape, i);
	float d = sqrt(pow(p.x, 2.f) + pow(p.y, 2.f));
	float a = atan2(p.y, p.x) + angle;
	points[i * 2] = d * cos(a) + b.x;
	points[i * 2 + 1] = d * sin(a) + b.y;
    }

    draw_poly(points, n);
}

void phys2d_dbgdrawpoly(struct phys2d_poly *poly)
{
    cpVect b = cpBodyGetPosition(poly->shape.go->body);
    float angle = cpBodyGetAngle(poly->shape.go->body);

    float s = poly->shape.go->scale;
    for (int i = 0; i < poly->n; i++) {
	float d =
	    sqrt(pow(poly->points[i * 2] * s, 2.f) +
		 pow(poly->points[i * 2 + 1] * s, 2.f));
	float a =
	    atan2(poly->points[i * 2 + 1], poly->points[i * 2]) + angle;
	draw_point(b.x + d * cos(a), b.y + d * sin(a), 3);
    }

    if (poly->n >= 3) {
	int n = cpPolyShapeGetCount(poly->shape.shape);
	float points[n * 2];

	for (int i = 0; i < n; i++) {
	    cpVect p = cpPolyShapeGetVert(poly->shape.shape, i);
	    float d = sqrt(pow(p.x, 2.f) + pow(p.y, 2.f));
	    float a = atan2(p.y, p.x) + angle;
	    points[i * 2] = d * cos(a) + b.x;
	    points[i * 2 + 1] = d * sin(a) + b.y;
	}

	draw_poly(points, n);
    }

}

void phys2d_dbgdrawedge(struct phys2d_edge *edge)
{
    cpVect p = cpBodyGetPosition(edge->shape.go->body);
    float s = edge->shape.go->scale;
    float angle = cpBodyGetAngle(edge->shape.go->body);

    for (int i = 0; i < edge->n; i++) {
	float d =
	    sqrt(pow(edge->points[i * 2] * s, 2.f) +
		 pow(edge->points[i * 2 + 1] * s, 2.f));
	float a =
	    atan2(edge->points[i * 2 + 1], edge->points[i * 2]) + angle;
	draw_point(p.x + d * cos(a), p.y + d * sin(a), 3);
    }

    for (int i = 0; i < edge->n - 1; i++) {
	cpVect a = cpSegmentShapeGetA(edge->shapes[i]);
	cpVect b = cpSegmentShapeGetB(edge->shapes[i]);
	float ad = sqrt(pow(a.x, 2.f) + pow(a.y, 2.f));
	float bd = sqrt(pow(b.x, 2.f) + pow(b.y, 2.f));
	float aa = atan2(a.y, a.x) + angle;
	float ba = atan2(b.y, b.x) + angle;
	draw_line(ad * cos(aa) + p.x, ad * sin(aa) + p.y,
		  bd * cos(ba) + p.x, bd * sin(ba) + p.y);
    }
}
