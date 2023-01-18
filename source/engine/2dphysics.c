#include "2dphysics.h"

#include "gameobject.h"
#include <string.h>
#include "mathc.h"
#include "nuke.h"

#include "debugdraw.h"
#include "gameobject.h"
#include <math.h>
#include <chipmunk/chipmunk_unsafe.h>
#include "stb_ds.h"

#include "script.h"

#include "log.h"

cpSpace *space = NULL;
float phys2d_gravity = -50.f;

float dbg_color[3] = {0.836f, 1.f, 0.45f};
float trigger_color[3] = {0.278f, 0.953f, 1.f};
float disabled_color[3] = {0.58f, 0.58f, 0.58f};
float dynamic_color[3] = {255/255, 70/255, 46/255};
float kinematic_color[3] = {255/255, 206/255,71/255};
float static_color[3] = {0.22f, 0.271f, 1.f};

void color2float(struct color color, float *fcolor)
{
    fcolor[0] = (float)color.r/255;
    fcolor[1] = (float)color.b/255;
    fcolor[2] = (float)color.g/255;
}

struct color float2color(float *fcolor)
{
    struct color new;
    new.r = fcolor[0]*255;
    new.b = fcolor[1]*255;
    new.g = fcolor[2]*255;
    return new;
}

int cpshape_enabled(cpShape *c)
{
    cpShapeFilter filter = cpShapeGetFilter(c);
    if (filter.categories == ~CP_ALL_CATEGORIES && filter.mask == ~CP_ALL_CATEGORIES)
        return 0;

    return 1;
}

float *shape_outline_color(cpShape *shape)
{
    switch (cpBodyGetType(cpShapeGetBody(shape))) {
        case CP_BODY_TYPE_DYNAMIC:
            return dynamic_color;

        case CP_BODY_TYPE_KINEMATIC:
            return kinematic_color;

        case CP_BODY_TYPE_STATIC:
            return static_color;
    }

    return static_color;
}

float *shape_color(cpShape *shape)
{
    if (!cpshape_enabled(shape)) return disabled_color;

    if (cpShapeGetSensor(shape)) return trigger_color;

    return dbg_color;
}

void phys2d_init()
{
    space = cpSpaceNew();
    cpVect grav = {0, phys2d_gravity};
    phys2d_set_gravity(grav);
    cpSpaceSetGravity(space, cpv(0, phys2d_gravity));
}

void phys2d_set_gravity(cpVect v) {
    cpSpaceSetGravity(space, v);
}

void phys2d_update(float deltaT)
{
    cpSpaceStep(space, deltaT);
}

void phys2d_shape_apply(struct phys2d_shape *shape)
{
    cpShapeSetFriction(shape->shape, id2go(shape->go)->f);
    cpShapeSetElasticity(shape->shape, id2go(shape->go)->e);
}

void init_phys2dshape(struct phys2d_shape *shape, int go, void *data)
{
    shape->go = go;
    shape->data = data;
    cpShapeSetCollisionType(shape->shape, go);
    cpShapeSetUserData(shape->shape, shape);
    phys2d_shape_apply(shape);
}

void phys2d_shape_del(struct phys2d_shape *shape)
{
    cpSpaceRemoveShape(space, shape->shape);
}


/***************** CIRCLE2D *****************/
struct phys2d_circle *Make2DCircle(int go)
{
    struct phys2d_circle *new = malloc(sizeof(struct phys2d_circle));

    new->radius = 10.f;
    new->offset[0] = 0.f;
    new->offset[1] = 0.f;

    new->shape.shape = cpSpaceAddShape(space, cpCircleShapeNew(id2go(go)->body, new->radius, cpvzero));
    new->shape.debugdraw = phys2d_dbgdrawcircle;
    init_phys2dshape(&new->shape, go, new);

    return new;
}

void phys2d_circledel(struct phys2d_circle *c)
{
    phys2d_shape_del(&c->shape);
}

void circle_gui(struct phys2d_circle *circle)
{
    nuke_property_float("Radius", 1.f, &circle->radius, 10000.f, 1.f, 1.f);
    nuke_property_float2("Offset", 0.f, circle->offset, 1.f, 0.01f, 0.01f);

    phys2d_applycircle(circle);
}

void phys2d_dbgdrawcpcirc(cpCircleShape *c)
{
    cpVect pos = cpBodyGetPosition(cpShapeGetBody(c));
    cpVect offset = cpCircleShapeGetOffset(c);
    float radius = cpCircleShapeGetRadius(c);
    float d = sqrt(pow(offset.x, 2.f) + pow(offset.y, 2.f));
    float a = atan2(offset.y, offset.x) + cpBodyGetAngle(cpShapeGetBody(c));


    draw_circle(pos.x + (d * cos(a)), pos.y + (d*sin(a)), radius, 2, shape_color(c), 1);
}

void phys2d_dbgdrawcircle(struct phys2d_circle *circle)
{
    phys2d_dbgdrawcpcirc((cpCircleShape *)circle->shape.shape);
}


/*********** SEGMENT2D  **************/

struct phys2d_segment *Make2DSegment(int go)
{
    struct phys2d_segment *new = malloc(sizeof(struct phys2d_segment));

    new->thickness = 1.f;
    new->a[0] = 0.f;
    new->a[1] = 0.f;
    new->b[0] = 0.f;
    new->b[1] = 0.f;

    new->shape.shape = cpSpaceAddShape(space, cpSegmentShapeNew(id2go(go)->body, cpvzero, cpvzero, new->thickness));
    new->shape.debugdraw = phys2d_dbgdrawseg;
    init_phys2dshape(&new->shape, go, new);

    return new;
}

void phys2d_segdel(struct phys2d_segment *seg)
{
    phys2d_shape_del(&seg->shape);
}

void segment_gui(struct phys2d_segment *seg)
{
    nuke_property_float2("a", 0.f, seg->a, 1.f, 0.01f, 0.01f);
    nuke_property_float2("b", 0.f, seg->b, 1.f, 0.01f, 0.01f);

    phys2d_applyseg(seg);
}


/************* BOX2D ************/

struct phys2d_box *Make2DBox(int go)
{
    struct phys2d_box *new = malloc(sizeof(struct phys2d_box));

    new->w = 50.f;
    new->h = 50.f;
    new->r = 0.f;
    new->offset[0] = 0.f;
    new->offset[1] = 0.f;

    new->shape.shape = cpSpaceAddShape(space, cpBoxShapeNew(id2go(go)->body, new->w, new->h, new->r));
    new->shape.debugdraw = phys2d_dbgdrawbox;
    init_phys2dshape(&new->shape, go, new);
    phys2d_applybox(new);

    return new;
}

void phys2d_boxdel(struct phys2d_box *box)
{
    phys2d_shape_del(&box->shape);
}

void box_gui(struct phys2d_box *box)
{
    nuke_property_float("Width", 0.f, &box->w, 1000.f, 1.f, 1.f);
    nuke_property_float("Height", 0.f, &box->h, 1000.f, 1.f, 1.f);
    nuke_property_float2("Offset", 0.f, box->offset, 1.f, 0.01f, 0.01f);

    phys2d_applybox(box);
}

/************** POLYGON ************/

struct phys2d_poly *Make2DPoly(int go)
{
    struct phys2d_poly *new = malloc(sizeof(struct phys2d_poly));

    new->n = 0;
    new->points = NULL;
    new->radius = 0.f;

    cpTransform T = { 0 };
    new->shape.shape = cpSpaceAddShape(space, cpPolyShapeNew(id2go(go)->body, 0, NULL, T, new->radius));
    init_phys2dshape(&new->shape, go, new);
    new->shape.debugdraw = phys2d_dbgdrawpoly;
    phys2d_applypoly(new);

    return new;
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

    if (nuke_btn("Add Poly Vertex")) phys2d_polyaddvert(poly);

    for (int i = 0; i < poly->n; i++) {
	nuke_property_float2("#P", 0.f, &poly->points[i*2], 1.f, 0.1f, 0.1f);
    }

    nuke_property_float("Radius", 0.01f, &poly->radius, 1000.f, 1.f, 0.1f);

    phys2d_applypoly(poly);
}



/****************** EDGE 2D**************/

struct phys2d_edge *Make2DEdge(int go)
{
    struct phys2d_edge *new = malloc(sizeof(struct phys2d_edge));

    new->n = 2;
    new->points = calloc(2 * 2, sizeof(float));
    new->thickness = 0.f;
    new->shapes = malloc(sizeof(cpShape *));

    new->shapes[0] = cpSpaceAddShape(space, cpSegmentShapeNew(id2go(go)->body, cpvzero, cpvzero, new->thickness));
    new->shape.go = go;
    phys2d_edgeshapeapply(&new->shape, new->shapes[0]);

    phys2d_applyedge(new);

    return new;
}

void phys2d_edgedel(struct phys2d_edge *edge)
{
    phys2d_shape_del(&edge->shape);
}

void phys2d_edgeshapeapply(struct phys2d_shape *mshape, cpShape * shape)
{
    cpShapeSetFriction(shape, id2go(mshape->go)->f);
    cpShapeSetElasticity(shape, id2go(mshape->go)->e);
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
    edge->shapes[edge->n - 2] = cpSpaceAddShape(space, cpSegmentShapeNew(id2go(edge->shape.go)->body, a, b, edge->thickness));
    phys2d_edgeshapeapply(&edge->shape, edge->shapes[edge->n - 2]);

    free(oldp);
    free(oldshapes);
}

void edge_gui(struct phys2d_edge *edge)
{
    if (nuke_btn("Add Edge Vertex")) phys2d_edgeaddvert(edge);

    for (int i = 0; i < edge->n; i++)
	nuke_property_float2("E", 0.f, &edge->points[i*2], 1.f, 0.01f, 0.01f);

    nuke_property_float("Thickness", 0.01f, &edge->thickness, 1.f, 0.01f, 0.01f);

    phys2d_applyedge(edge);
}



void phys2d_applycircle(struct phys2d_circle *circle)
{
    struct gameobject *go = id2go(circle->shape.go);

    float radius = circle->radius * go->scale;
    float s = go->scale;
    cpVect offset = { circle->offset[0] * s, circle->offset[1] * s };

    cpCircleShapeSetRadius(circle->shape.shape, radius);
    cpCircleShapeSetOffset(circle->shape.shape, offset);
    cpBodySetMoment(go->body, cpMomentForCircle(go->mass, 0, radius, offset));
}

void phys2d_applyseg(struct phys2d_segment *seg)
{
    float s = id2go(seg->shape.go)->scale;
    cpVect a = { seg->a[0] * s, seg->a[1] * s };
    cpVect b = { seg->b[0] * s, seg->b[1] * s };
    cpSegmentShapeSetEndpoints(seg->shape.shape, a, b);
    cpSegmentShapeSetRadius(seg->shape.shape, seg->thickness * s);
}

void phys2d_applybox(struct phys2d_box *box)
{
    float s = id2go(box->shape.go)->scale;
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

    float s = id2go(poly->shape.go)->scale;
    cpTransform T = { 0 };
    T.a = s;
    T.d = s;

    cpPolyShapeSetVerts(poly->shape.shape, hullCount, hullVerts, T);
    cpPolyShapeSetRadius(poly->shape.shape, poly->radius);
}

void phys2d_applyedge(struct phys2d_edge *edge)
{
    float s = id2go(edge->shape.go)->scale;

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
    cpVect p = cpBodyGetPosition(cpShapeGetBody(seg->shape.shape));
    cpVect a = cpSegmentShapeGetA(seg->shape.shape);
    cpVect b = cpSegmentShapeGetB(seg->shape.shape);

    float angle = cpBodyGetAngle(cpShapeGetBody(seg->shape.shape));
    float ad = sqrt(pow(a.x, 2.f) + pow(a.y, 2.f));
    float bd = sqrt(pow(b.x, 2.f) + pow(b.y, 2.f));
    float aa = atan2(a.y, a.x) + angle;
    float ba = atan2(b.y, b.x) + angle;
    draw_line(ad * cos(aa) + p.x, ad * sin(aa) + p.y, bd * cos(ba) + p.x, bd * sin(ba) + p.y, shape_color(seg->shape.shape));
}

void phys2d_dbgdrawbox(struct phys2d_box *box)
{
    int n = cpPolyShapeGetCount(box->shape.shape);
    cpVect b = cpBodyGetPosition(cpShapeGetBody(box->shape.shape));
    float angle = cpBodyGetAngle(cpShapeGetBody(box->shape.shape));
    float points[n * 2];

    for (int i = 0; i < n; i++) {
	cpVect p = cpPolyShapeGetVert(box->shape.shape, i);
	float d = sqrt(pow(p.x, 2.f) + pow(p.y, 2.f));
	float a = atan2(p.y, p.x) + angle;
	points[i * 2] = d * cos(a) + b.x;
	points[i * 2 + 1] = d * sin(a) + b.y;
    }

    draw_poly(points, n, shape_color(box->shape.shape));
}

void phys2d_dbgdrawpoly(struct phys2d_poly *poly)
{
    float *color = shape_color(poly->shape.shape);

    cpVect b = cpBodyGetPosition(cpShapeGetBody(poly->shape.shape));
    float angle = cpBodyGetAngle(cpShapeGetBody(poly->shape.shape));

    float s = id2go(poly->shape.go)->scale;
    for (int i = 0; i < poly->n; i++) {
	float d = sqrt(pow(poly->points[i * 2] * s, 2.f) + pow(poly->points[i * 2 + 1] * s, 2.f));
	float a = atan2(poly->points[i * 2 + 1], poly->points[i * 2]) + angle;
	draw_point(b.x + d * cos(a), b.y + d * sin(a), 3, color);
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

	draw_poly(points, n, color);
    }

}

void phys2d_dbgdrawedge(struct phys2d_edge *edge)
{
    float *color =shape_color(edge->shape.shape);

    cpVect p = cpBodyGetPosition(cpShapeGetBody(edge->shape.shape));
    float s = id2go(edge->shape.go)->scale;
    float angle = cpBodyGetAngle(cpShapeGetBody(edge->shape.shape));

    for (int i = 0; i < edge->n; i++) {
	float d = sqrt(pow(edge->points[i * 2] * s, 2.f) + pow(edge->points[i * 2 + 1] * s, 2.f));
	float a = atan2(edge->points[i * 2 + 1], edge->points[i * 2]) + angle;
	draw_point(p.x + d * cos(a), p.y + d * sin(a), 3, color);
    }

    for (int i = 0; i < edge->n - 1; i++) {
	cpVect a = cpSegmentShapeGetA(edge->shapes[i]);
	cpVect b = cpSegmentShapeGetB(edge->shapes[i]);
	float ad = sqrt(pow(a.x, 2.f) + pow(a.y, 2.f));
	float bd = sqrt(pow(b.x, 2.f) + pow(b.y, 2.f));
	float aa = atan2(a.y, a.x) + angle;
	float ba = atan2(b.y, b.x) + angle;
	draw_line(ad * cos(aa) + p.x, ad * sin(aa) + p.y, bd * cos(ba) + p.x, bd * sin(ba) + p.y, color);
    }
}

void shape_enabled(struct phys2d_shape *shape, int enabled)
{
    if (enabled)
      cpShapeSetFilter(shape->shape, CP_SHAPE_FILTER_ALL);
    else
      cpShapeSetFilter(shape->shape, CP_SHAPE_FILTER_NONE);
}

int shape_is_enabled(struct phys2d_shape *shape)
{
    if (cpshape_enabled(shape->shape))
        return 1;

    return 0;
}

void shape_set_sensor(struct phys2d_shape *shape, int sensor)
{
    cpShapeSetSensor(shape->shape, sensor);
}

int shape_get_sensor(struct phys2d_shape *shape)
{
    return cpShapeGetSensor(shape->shape);
}

void phys2d_reindex_body(cpBody *body) {
    cpSpaceReindexShapesForBody(space, body);
}


void register_collide(void *sym) {

}

static cpBool script_phys_cb_begin(cpArbiter *arb, cpSpace *space, void *data) {
    cpBody *body1;
    cpBody *body2;
    cpArbiterGetBodies(arb, &body1, &body2);

    int g1 = cpBodyGetUserData(body1);
    int g2 = cpBodyGetUserData(body2);

    duk_push_heapptr(duk, id2go(g1)->cbs.begin.fn);
    duk_push_heapptr(duk, id2go(g1)->cbs.begin.obj);

    int obj = duk_push_object(duk);

    vect2duk(cpArbiterGetNormal(arb));
    duk_put_prop_literal(duk, obj, "normal");

    duk_push_int(duk, g2);
    duk_put_prop_literal(duk, obj, "hit");

    duk_call_method(duk, 1);
    duk_pop(duk);

    return 1;
}

void phys2d_add_handler_type(int cmd, int go, struct callee c) {
    cpCollisionHandler *handler = cpSpaceAddWildcardHandler(space, go);

    handler->userData = go;

    switch (cmd) {
        case 0:
            handler->beginFunc = script_phys_cb_begin;
            id2go(go)->cbs.begin = c;
            break;

        case 1:
            break;

        case 2:
            break;

        case 3:
            //handler->separateFunc = s7_phys_cb_separate;
            //go->cbs->separate = cb;
            break;
    }

}