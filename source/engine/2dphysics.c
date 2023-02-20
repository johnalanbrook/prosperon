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

cpVect world2go(struct gameobject *go, cpVect worldpos)
{
  worldpos = cpvsub(worldpos, cpBodyGetPosition(go->body));
  worldpos = cpvmult(worldpos, 1/go->scale);
  return worldpos;
}

cpVect go2world(struct gameobject *go, cpVect gopos)
{
  cpVect pos = cpBodyGetPosition(go->body);
  float angle = cpBodyGetAngle(go->body);
  cpTransform T = {0};
  T.a = go->scale * go->flipx * cos(angle);
  T.b = -sin(angle) * go->scale;
  T.c = sin(angle) * go->scale;
  T.d = go->scale * go->flipy * cos(angle);
  T.tx = pos.x;
  T.ty = pos.y;
  return cpTransformPoint(T, gopos);
}

cpTransform body2transform(cpBody *body)
{
  cpTransform T = {0};
  cpVect pos = cpBodyGetPosition(body);
  float angle = cpBodyGetAngle(body);
  T.a = cos(angle);
  T.b = -sin(angle);
  T.c = sin(angle);
  T.d = cos(angle);
  T.tx = pos.x;
  T.ty = pos.y;
  
  return T;
}

cpVect gotransformpoint(struct gameobject *go, cpVect point)
{
  point.x *= go->scale * go->flipx;
  point.y *= go->scale * go->flipy;
  return point;
}

cpVect bodytransformpoint(cpBody *body, cpVect offset)
{
  cpVect pos = cpBodyGetPosition(body);
  float d = sqrt(pow(offset.x, 2.f) + pow(offset.y, 2.f));
  float a = atan2(offset.y, offset.x) + cpBodyGetAngle(body);
  pos.x += d*cos(a);
  pos.y += d*sin(a);
  return pos;
}

void phys2d_dbgdrawcpcirc(cpCircleShape *c)
{
    cpVect pos = bodytransformpoint(cpShapeGetBody(c), cpCircleShapeGetOffset(c));
    float radius = cpCircleShapeGetRadius(c);
    draw_circle(pos.x, pos.y, radius, 2, shape_color(c), 1);
}

void phys2d_dbgdrawcircle(struct phys2d_circle *circle)
{
    phys2d_dbgdrawcpcirc((cpCircleShape *)circle->shape.shape);
}

void phys2d_applycircle(struct phys2d_circle *circle)
{
    struct gameobject *go = id2go(circle->shape.go);

    float radius = circle->radius * go->scale;
    cpVect offset = gotransformpoint(go, circle->offset);

    cpCircleShapeSetRadius(circle->shape.shape, radius);
    cpCircleShapeSetOffset(circle->shape.shape, offset);
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

cpTransform go2transform(struct gameobject *go, cpVect offset, float angle)
{
  cpTransform T = {0};
  T.a = cos(angle) * go->scale * go->flipx;
  T.b = -sin(angle) * go->scale * go->flipx;
  T.c = sin(angle) * go->scale * go->flipy;
  T.d = cos(angle) * go->scale * go->flipy;
  T.tx = offset.x * go->scale;
  T.ty = offset.y * go->scale;
  return T;
}

void phys2d_boxdel(struct phys2d_box *box)
{
    phys2d_shape_del(&box->shape);
}

void phys2d_applybox(struct phys2d_box *box)
{
    phys2d_boxdel(box);
    struct gameobject *go = id2go(box->shape.go);
    cpVect off;
    off.x = box->offset[0];
    off.y = box->offset[1];
    cpTransform T = go2transform(id2go(box->shape.go), off, box->rotation);
    float hh = box->h / 2.f;
    float hw = box->w / 2.f;
    cpVect verts[4] = { { -hw, -hh }, { hw, -hh }, { hw, hh }, { -hw, hh } };
    box->shape.shape = cpSpaceAddShape(space, cpPolyShapeNew(go->body, 4, verts, T, box->r));
    init_phys2dshape(&box->shape, box->shape.go, box);
}

void phys2d_dbgdrawbox(struct phys2d_box *box)
{
    int n = cpPolyShapeGetCount(box->shape.shape);
    float points[n * 2];
    
    for (int i = 0; i < n; i++)
        points[i] = bodytransformpoint(cpShapeGetBody(box->shape.shape), cpPolyShapeGetVert(box->shape.shape, i));

    draw_poly(points, n, shape_color(box->shape.shape));
}
/************** POLYGON ************/

struct phys2d_poly *Make2DPoly(int go)
{
    struct phys2d_poly *new = malloc(sizeof(struct phys2d_poly));

    new->points = NULL;
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

void phys2d_poly_setverts(struct phys2d_poly *poly, cpVect *verts)
{
    if (!verts) return;
    arrfree(poly->points);
    poly->points = verts;
    phys2d_applypoly(poly);
}

void phys2d_applypoly(struct phys2d_poly *poly)
{
    if (arrlen(poly->points) <= 0) return;
    struct gameobject *go = id2go(poly->shape.go);
    
    cpTransform T = go2transform(go, cpvzero, 0);

    cpPolyShapeSetVerts(poly->shape.shape, arrlen(poly->points), poly->points, T);
    cpPolyShapeSetRadius(poly->shape.shape, poly->radius);
    cpSpaceReindexShapesForBody(space, cpShapeGetBody(poly->shape.shape));
}
void phys2d_dbgdrawpoly(struct phys2d_poly *poly)
{
    float *color = shape_color(poly->shape.shape);
    int n = arrlen(poly->points);

    if (arrlen(poly->points) >= 3) {
	int n = cpPolyShapeGetCount(poly->shape.shape);
	float points[n * 2];

	for (int i = 0; i < n; i++)
	  points[i*2] = bodytransformpoint(cpShapeGetBody(poly->shape.shape), cpPolyShapeGetVert(poly->shape.shape, i));

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
    struct gameobject *go = id2go(edge->shape.go);
    
    for (int i = 0; i < arrlen(edge->shapes); i++) {
        cpVect a = gotransformpoint(go, edge->points[i]);
        cpVect b = gotransformpoint(go, edge->points[i+1]);
	cpSegmentShapeSetEndpoints(edge->shapes[i], a, b);
	cpSegmentShapeSetRadius(edge->shapes[i], edge->thickness);
	go_shape_apply(NULL, edge->shapes[i], go);
	cpShapeSetUserData(edge->shapes[i], &edge->shape);
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

    cpVect drawpoints[arrlen(edge->points)];
    struct gameobject *go = id2go(edge->shape.go);

    for (int i = 0; i < arrlen(edge->points); i++) {
      drawpoints[i] = gotransformpoint(go, edge->points[i]);
      drawpoints[i] = bodytransformpoint(cpShapeGetBody(edge->shapes[0]), drawpoints[i]);
    }

    draw_edge(drawpoints, arrlen(edge->points), shape_color(edge->shapes[0]), edge->thickness*2);
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
    if (!shape->shape) {
      struct phys2d_edge *edge = shape->data;

      for (int i = 0; i < arrlen(edge->shapes); i++) {
        cpShapeSetSensor(edge->shapes[i], sensor);
      YughInfo("Setting shape %d sensor to %d", i, sensor);	
      }
    } else
      cpShapeSetSensor(shape->shape, sensor);
}

int shape_get_sensor(struct phys2d_shape *shape)
{
    if (!shape->shape) {
      return cpShapeGetSensor(((struct phys2d_edge *)(shape->data))->shapes[0]);
    }
    return cpShapeGetSensor(shape->shape);
}

void phys2d_reindex_body(cpBody *body) {
    cpSpaceReindexShapesForBody(space, body);
}


void register_collide(void *sym) {

}

void duk_call_phys_cb(cpArbiter *arb, struct callee c, int hit)
{
    duk_push_heapptr(duk, c.fn);
    duk_push_heapptr(duk, c.obj);

    int obj = duk_push_object(duk);

    vect2duk(cpArbiterGetNormal(arb));
    duk_put_prop_literal(duk, obj, "normal");

    duk_push_int(duk, hit);
    duk_put_prop_literal(duk, obj, "hit");

    duk_call_method(duk, 1);
    duk_pop(duk);
}

static cpBool script_phys_cb_begin(cpArbiter *arb, cpSpace *space, void *data) {

    cpBody *body1;
    cpBody *body2;
    cpArbiterGetBodies(arb, &body1, &body2);

    cpShape *shape1;
    cpShape *shape2;
    cpArbiterGetShapes(arb, &shape1, &shape2);

    int g1 = cpBodyGetUserData(body1);
    int g2 = cpBodyGetUserData(body2);
    struct gameobject *go = id2go(g1);

    for (int i = 0; i < arrlen(go->shape_cbs); i++) {
      if (go->shape_cbs[i].shape != shape1) continue;
      duk_call_phys_cb(arb, go->shape_cbs[i].cbs.begin, g2);
    }

    duk_call_phys_cb(arb, go->cbs.begin, g2);

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
