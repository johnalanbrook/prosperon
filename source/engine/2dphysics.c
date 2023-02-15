#include "2dphysics.h"

#include "gameobject.h"
#include <string.h>
#include "mathc.h"
#include "nuke.h"
#include "debug.h"

#include "debugdraw.h"
#include "gameobject.h"
#include <math.h>
#include <chipmunk/chipmunk_unsafe.h>
#include "stb_ds.h"
#include <assert.h>

#include "tinyspline.h"

#include "script.h"
#include "ffi.h"

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

cpShape *phys2d_query_pos(cpVect pos)
{
  cpShapeFilter filter;
  filter.group = 0;
  filter.mask = CP_ALL_CATEGORIES;
  filter.categories = CP_ALL_CATEGORIES;
  cpShape *find = cpSpacePointQueryNearest(space, pos, 0.f, filter, NULL);

  return find;
}

int *qhits;

void querylist(cpShape *shape, cpContactPointSet *points, void *data)
{
  int go = shape2gameobject(shape);
  int in = 0;
  for (int i = 0; i < arrlen(qhits); i++) {
    if (qhits[i] == go) {
      in = 1;
      break;
    }
  }

  if (!in) arrput(qhits, go);
}

void querylistbodies(cpBody *body, void *data)
{
  cpBB *bbox = data;
  if (cpBBContainsVect(*bbox, cpBodyGetPosition(body))) {
    int go = body2id(body);
    if (go < 0) return;
    
    int in = 0;
    for (int i = 0; i < arrlen(qhits); i++) {
      if (qhits[i] == go) {
        in = 1;
	break;
      }
    }
    
    if (!in) arrput(qhits, go);
  }
}

int *phys2d_query_box(cpVect pos, cpVect wh)
{
  cpShape *box = cpBoxShapeNew(NULL, wh.x, wh.y, 0.f);
  cpTransform T = {0};
  T.a = 1;
  T.d = 1;
  T.tx = pos.x;
  T.ty = pos.y;
  cpShapeUpdate(box, T);
  
  cpBB bbox = cpShapeGetBB(box);

  if (qhits) arrfree(qhits);
  
  cpSpaceShapeQuery(space, box, querylist, NULL);

  cpSpaceEachBody(space, querylistbodies, &bbox);
  
  cpShapeFree(box);
  
  return qhits;
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

void init_phys2dshape(struct phys2d_shape *shape, int go, void *data)
{
    shape->go = go;
    shape->data = data;
    go_shape_apply(id2go(go)->body, shape->shape, id2go(go));
    cpShapeSetCollisionType(shape->shape, go);
    cpShapeSetUserData(shape->shape, shape);
}

void phys2d_shape_del(struct phys2d_shape *shape)
{
    if (!shape->shape) return;
    cpSpaceRemoveShape(space, shape->shape);
    cpShapeFree(shape->shape);
}

/***************** CIRCLE2D *****************/
struct phys2d_circle *Make2DCircle(int go)
{
    struct phys2d_circle *new = malloc(sizeof(struct phys2d_circle));

    new->radius = 10.f;
    new->offset = cpvzero;

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
    //nuke_property_float2("Offset", 0.f, circle->offset, 1.f, 0.01f, 0.01f);

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

void phys2d_applycircle(struct phys2d_circle *circle)
{
    struct gameobject *go = id2go(circle->shape.go);

    float radius = circle->radius * go->scale;
    float s = go->scale;
    cpVect offset = { circle->offset.x * s, circle->offset.y * s };

    cpCircleShapeSetRadius(circle->shape.shape, radius);
    cpCircleShapeSetOffset(circle->shape.shape, offset);
    //cpBodySetMoment(go->body, cpMomentForCircle(go->mass, 0, radius, offset));
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
    new->shape.go = go;
    phys2d_applybox(new);
    new->shape.debugdraw = phys2d_dbgdrawbox;

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

void phys2d_applybox(struct phys2d_box *box)
{
    phys2d_boxdel(box);
    struct gameobject *go = id2go(box->shape.go);
    float s = id2go(box->shape.go)->scale;
    cpTransform T = { 0 };
    T.a = s * cos(box->rotation);
    T.b = s * -sin(box->rotation);
    T.c = s * sin(box->rotation);
    T.d = s * cos(box->rotation);
    T.tx = box->offset[0] * s * go->flipx;
    T.ty = box->offset[1] * s * go->flipy;
    float hh = box->h / 2.f;
    float hw = box->w / 2.f;
    cpVect verts[4] = { { -hw, -hh }, { hw, -hh }, { hw, hh }, { -hw, hh } };
    box->shape.shape = cpSpaceAddShape(space, cpPolyShapeNew(go->body, 4, verts, T, box->r));
    init_phys2dshape(&box->shape, box->shape.go, box);
//    cpPolyShapeSetVerts(box->shape.shape, 4, verts, T);
//    cpPolyShapeSetRadius(box->shape.shape, box->r);
    
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
/************** POLYGON ************/

struct phys2d_poly *Make2DPoly(int go)
{
    struct phys2d_poly *new = malloc(sizeof(struct phys2d_poly));

    arrsetlen(new->points, 0);
    new->radius = 0.f;

    new->shape.shape = cpSpaceAddShape(space, cpPolyShapeNewRaw(id2go(go)->body, 0, new->points, new->radius));
    new->shape.debugdraw = phys2d_dbgdrawpoly;
    init_phys2dshape(&new->shape, go, new);
    return new;
}

void phys2d_polydel(struct phys2d_poly *poly)
{
    arrfree(poly->points);
    phys2d_shape_del(&poly->shape);
}

void phys2d_polyaddvert(struct phys2d_poly *poly)
{
    arrput(poly->points, cpvzero);
}

void poly_gui(struct phys2d_poly *poly)
{
}

void phys2d_poly_setverts(struct phys2d_poly *poly, cpVect *verts)
{
    arrfree(poly->points);
    poly->points = verts;
    phys2d_applypoly(poly);
}

void phys2d_applypoly(struct phys2d_poly *poly)
{
    if (arrlen(poly->points) <= 0) return;
    float s = id2go(poly->shape.go)->scale;
    cpTransform T = { 0 };
    T.a = s;
    T.d = s;

    cpPolyShapeSetVerts(poly->shape.shape, arrlen(poly->points), poly->points, T);
    cpPolyShapeSetRadius(poly->shape.shape, poly->radius);
}
void phys2d_dbgdrawpoly(struct phys2d_poly *poly)
{
    float *color = shape_color(poly->shape.shape);
    int n = arrlen(poly->points);

    cpVect b = cpBodyGetPosition(cpShapeGetBody(poly->shape.shape));
    float angle = cpBodyGetAngle(cpShapeGetBody(poly->shape.shape));

    float s = id2go(poly->shape.go)->scale;
    for (int i = 0; i < n; i++) {
	float d = sqrt(pow(poly->points[i * 2].x * s, 2.f) + pow(poly->points[i * 2].y* s, 2.f));
	float a = atan2(poly->points[i * 2].y, poly->points[i * 2].x) + angle;
	draw_point(b.x + d * cos(a), b.y + d * sin(a), 3, color);
    }

    if (arrlen(poly->points) >= 3) {
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
/****************** EDGE 2D**************/

struct phys2d_edge *Make2DEdge(int go)
{
    struct phys2d_edge *new = malloc(sizeof(struct phys2d_edge));
    new->points = NULL;
    arrsetlen(new->points, 0);
    new->thickness = 0.f;
    new->shapes = NULL;
    arrsetlen(new->shapes, 0);
    new->shape.go = go;
    new->shape.data = new;
    new->shape.debugdraw = phys2d_dbgdrawedge;
    phys2d_applyedge(new);

    return new;
}

void phys2d_edgedel(struct phys2d_edge *edge)
{
    phys2d_shape_del(&edge->shape);
}

void phys2d_edgeaddvert(struct phys2d_edge *edge)
{
    arrput(edge->points, cpvzero);
    if (arrlen(edge->points) > 1)
        arrput(edge->shapes, cpSpaceAddShape(space, cpSegmentShapeNew(id2go(edge->shape.go)->body, cpvzero, cpvzero, edge->thickness)));

    phys2d_applyedge(edge);
}

void phys2d_edge_rmvert(struct phys2d_edge *edge, int index)
{
    assert(arrlen(edge->points) > index && index >= 0);

    arrdel(edge->points, index);

    if (arrlen(edge->points) == 0) return;


    if (index == 0) {
      cpSpaceRemoveShape(space, edge->shapes[index]);
      arrdel(edge->shapes, index);
      phys2d_applyedge(edge);
      return;
    }

    if (index != arrlen(edge->points)) {
      cpSegmentShapeSetEndpoints(edge->shapes[index-1], edge->points[index-1], edge->points[index]);
    }

    cpSpaceRemoveShape(space, edge->shapes[index-1]);
    arrdel(edge->shapes, index-1);

    phys2d_applyedge(edge);
}

void phys2d_edge_setvert(struct phys2d_edge *edge, int index, cpVect val)
{
    assert(arrlen(edge->points) > index && index >= 0);
    edge->points[index] = val;

    phys2d_applyedge(edge);
}

void phys2d_edge_clearverts(struct phys2d_edge *edge)
{
  for (int i = arrlen(edge->points)-1; i>=0; i--) {
    phys2d_edge_rmvert(edge, i);
  }
}

void phys2d_edge_addverts(struct phys2d_edge *edge, cpVect *verts)
{
  for (int i = 0; i < arrlen(verts); i++) {
    phys2d_edgeaddvert(edge);
    phys2d_edge_setvert(edge, i, verts[i]);
  }
}

void phys2d_applyedge(struct phys2d_edge *edge)
{
    float s = id2go(edge->shape.go)->scale;

    for (int i = 0; i < arrlen(edge->shapes); i++) {
        cpVect a = edge->points[i];
        cpVect b = edge->points[i+1];
        a.x *= s;
        a.y *= s;
        b.x *= s;
        b.y *= s;
	cpSegmentShapeSetEndpoints(edge->shapes[i], a, b);
	cpSegmentShapeSetRadius(edge->shapes[i], edge->thickness);
	cpShapeSetUserData(edge->shapes[i], &edge->shape);
	cpShapeSetCollisionType(edge->shapes[i], edge->shape.go);
	cpShapeSetFriction(edge->shapes[i], id2go(edge->shape.go)->f);
	cpShapeSetElasticity(edge->shapes[i], id2go(edge->shape.go)->e);
    }

    cpSpaceReindexShapesForBody(space, id2go(edge->shape.go)->body);
}

void phys2d_dbgdrawedge(struct phys2d_edge *edge)
{
    edge->draws++;
    if (edge->draws > 1) {
        if (edge->draws >= arrlen(edge->shapes))
            edge->draws = 0;

        return;
    }

    if (arrlen(edge->shapes) < 1) return;

    cpVect p = cpBodyGetPosition(cpShapeGetBody(edge->shapes[0]));
    float s = id2go(edge->shape.go)->scale;
    float angle = cpBodyGetAngle(cpShapeGetBody(edge->shapes[0]));

    cpVect drawpoints[arrlen(edge->points)];

    for (int i = 0; i < arrlen(edge->points); i++) {
        float d = sqrt(pow(edge->points[i].x*s, 2.f) + pow(edge->points[i].y*s, 2.f));
        float a = atan2(edge->points[i].y, edge->points[i].x) + angle;
        drawpoints[i].x = p.x + d*cos(a);
        drawpoints[i].y = p.y + d*sin(a);
    }

    draw_edge(drawpoints, arrlen(edge->points), trigger_color, edge->thickness*2);
    draw_points(drawpoints, arrlen(edge->points), 2, kinematic_color);
}


/************ COLLIDER ****************/
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
