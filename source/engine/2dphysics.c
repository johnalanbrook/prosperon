#include "2dphysics.h"

#include "debug.h"
#include "gameobject.h"
#include "nuke.h"
#include <string.h>

#include "debugdraw.h"
#include "stb_ds.h"
#include <assert.h>
#include <chipmunk/chipmunk_unsafe.h>
#include <math.h>

#include "2dphysics.h"

#include "tinyspline.h"

#include "ffi.h"
#include "script.h"

#include "log.h"

cpSpace *space = NULL;

struct rgba color_white = {255,255,255,255};
struct rgba color_black = {0,0,0,255};

struct rgba disabled_color = {148,148,148,255};
struct rgba sleep_color = {255,140,228,255};
struct rgba dynamic_color = {255,70,46,255};
struct rgba kinematic_color = {255, 194, 64, 255};
struct rgba static_color = {73,209,80,255};

static const unsigned char col_alpha = 40;
static const float sensor_seg = 10;

unsigned int category_masks[32];

void set_cat_mask(int cat, unsigned int mask) {
  category_masks[cat] = mask;
}

cpShape *phys2d_query_pos(cpVect pos) {
  cpShapeFilter filter;
  filter.group = CP_NO_GROUP;
  filter.mask = CP_ALL_CATEGORIES;
  filter.categories = CP_ALL_CATEGORIES;
  cpShape *find = cpSpacePointQueryNearest(space, pos, 0.f, filter, NULL);
  //  cpShape *find = cpSpaceSegmentQueryFirst(space, pos, pos, 0.f, filter, NULL);

  return find;
}

int *qhits;

void querylist(cpShape *shape, cpContactPointSet *points, void *data) {
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

void querylistbodies(cpBody *body, void *data) {
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

int *phys2d_query_box_points(cpVect pos, cpVect wh, cpVect *points, int n) {
  cpShape *box = cpBoxShapeNew(NULL, wh.x, wh.y, 0.f);
  cpTransform T = {0};
  T.a = 1;
  T.d = 1;
  T.tx = pos.x;
  T.ty = pos.y;
  cpShapeUpdate(box, T);

  cpBB bbox = cpShapeGetBB(box);

  if (qhits) arrfree(qhits);

  for (int i = 0; i < n; i++) {
    if (cpBBContainsVect(bbox, points[i]))
      arrpush(qhits, i);
  }

  return qhits;
}

int *phys2d_query_box(cpVect pos, cpVect wh) {
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

int *phys2d_query_shape(struct phys2d_shape *shape) {
  if (qhits) arrfree(qhits);

  cpSpaceShapeQuery(space, shape->shape, querylist, NULL);

  return qhits;
}

int cpshape_enabled(cpShape *c) {
  cpShapeFilter filter = cpShapeGetFilter(c);
  if (filter.categories == ~CP_ALL_CATEGORIES && filter.mask == ~CP_ALL_CATEGORIES)
    return 0;

  return 1;
}

struct rgba shape_color(cpShape *shape) {
  switch (cpBodyGetType(cpShapeGetBody(shape))) {
  case CP_BODY_TYPE_DYNAMIC:
//    cpBodySleep(cpShapeGetBody(shape));
    if (cpBodyIsSleeping(cpShapeGetBody(shape)))
      return sleep_color;
    return dynamic_color;

  case CP_BODY_TYPE_KINEMATIC:
    return kinematic_color;

  case CP_BODY_TYPE_STATIC:
    return static_color;
  }

  return static_color;
}

void phys2d_init()
{
  space = cpSpaceNew();
  cpSpaceSetSleepTimeThreshold(space, 1);
  cpSpaceSetCollisionSlop(space, 0.01);
  cpSpaceSetCollisionBias(space, cpfpow(1.0-0.5, 165.f));
}

void phys2d_set_gravity(cpVect v) {
  cpSpaceSetGravity(space, v);
}

void phys2d_update(float deltaT) {
  cpSpaceStep(space, deltaT);
  flush_collide_cbs();
}

void init_phys2dshape(struct phys2d_shape *shape, int go, void *data) {
  shape->go = go;
  shape->data = data;
  go_shape_apply(id2go(go)->body, shape->shape, id2go(go));
  cpShapeSetCollisionType(shape->shape, go);
  cpShapeSetUserData(shape->shape, shape);
}

void phys2d_shape_del(struct phys2d_shape *shape) {
  if (!shape->shape) return;
  cpSpaceRemoveShape(space, shape->shape);
  cpShapeFree(shape->shape);
}

/***************** CIRCLE2D *****************/
struct phys2d_circle *Make2DCircle(int go) {
  struct phys2d_circle *new = malloc(sizeof(struct phys2d_circle));

  new->radius = 10.f;
  new->offset = cpvzero;

  new->shape.shape = cpSpaceAddShape(space, cpCircleShapeNew(id2go(go)->body, new->radius, cpvzero));
  new->shape.debugdraw = phys2d_dbgdrawcircle;
  new->shape.moi = phys2d_circle_moi;
  init_phys2dshape(&new->shape, go, new);

  return new;
}

float phys2d_circle_moi(struct phys2d_circle *c, float m) {
  return cpMomentForCircle(m, 0, c->radius, c->offset);
}

void phys2d_circledel(struct phys2d_circle *c) {
  phys2d_shape_del(&c->shape);
}

cpVect world2go(struct gameobject *go, cpVect worldpos) {
  cpTransform T = {0};
  cpVect pos = cpBodyGetPosition(go->body);
  worldpos.x -= pos.x;
  worldpos.y -= pos.y;
  //  worldpos.x /= go->scale;
  //  worldpos.y /= go->scale;
  float angle = cpBodyGetAngle(go->body);
  T.a = go->flipx * cos(-angle) / go->scale;
  T.b = sin(-angle) / go->scale;
  T.c = -sin(-angle) / go->scale;
  T.d = go->flipy * cos(-angle) / go->scale;
  worldpos = cpTransformPoint(T, worldpos);

  return worldpos;
}

cpVect go2world(struct gameobject *go, cpVect gopos) {
  cpVect pos = cpBodyGetPosition(go->body);
  float angle = cpBodyGetAngle(go->body);
  cpTransform T = {0};
  T.a = go->scale * go->flipx * cos(angle);
  T.b = sin(angle) * go->scale * go->flipx;
  T.c = -sin(angle) * go->scale * go->flipy;
  T.d = go->scale * go->flipy * cos(angle);
  T.tx = pos.x;
  T.ty = pos.y;
  return cpTransformPoint(T, gopos);
}

cpVect gotransformpoint(struct gameobject *go, cpVect point) {
  point.x *= go->scale * go->flipx;
  point.y *= go->scale * go->flipy;
  return point;
}

cpVect bodytransformpoint(cpBody *body, cpVect offset) {
  cpVect pos = cpBodyGetPosition(body);
  float d = sqrt(pow(offset.x, 2.f) + pow(offset.y, 2.f));
  float a = atan2(offset.y, offset.x) + cpBodyGetAngle(body);
  pos.x += d * cos(a);
  pos.y += d * sin(a);
  return pos;
}

void phys2d_dbgdrawcpcirc(cpCircleShape *c) {
  cpVect pos = bodytransformpoint(cpShapeGetBody(c), cpCircleShapeGetOffset(c));
  float radius = cpCircleShapeGetRadius(c);
  struct rgba color = shape_color(c);
  float seglen = cpShapeGetSensor(c) ? 5 : -1;
  draw_circle(pos, radius, 1, color, seglen);
  color.a = col_alpha;
  draw_circle(pos,radius,radius,color,-1);
}

void phys2d_dbgdrawcircle(struct phys2d_circle *circle) {
  phys2d_dbgdrawcpcirc((cpCircleShape *)circle->shape.shape);
}

void phys2d_applycircle(struct phys2d_circle *circle) {
  struct gameobject *go = id2go(circle->shape.go);

  float radius = circle->radius * go->scale;
  cpVect offset = gotransformpoint(go, circle->offset);

  cpCircleShapeSetRadius(circle->shape.shape, radius);
  cpCircleShapeSetOffset(circle->shape.shape, offset);
}

/************* BOX2D ************/

struct phys2d_box *Make2DBox(int go) {
  struct phys2d_box *new = malloc(sizeof(struct phys2d_box));

  new->w = 50.f;
  new->h = 50.f;
  new->r = 0.f;
  new->offset[0] = 0.f;
  new->offset[1] = 0.f;
  new->shape.go = go;
  phys2d_applybox(new);
  new->shape.debugdraw = phys2d_dbgdrawbox;
  new->shape.moi = phys2d_box_moi;

  return new;
}

float phys2d_box_moi(struct phys2d_box *box, float m) {
  return cpMomentForBox(m, box->w, box->h);
}

cpTransform go2transform(struct gameobject *go, cpVect offset, float angle) {
  cpTransform T = {0};
  T.a = cos(angle) * go->scale * go->flipx;
  T.b = -sin(angle) * go->scale * go->flipx;
  T.c = sin(angle) * go->scale * go->flipy;
  T.d = cos(angle) * go->scale * go->flipy;
  T.tx = offset.x * go->scale;
  T.ty = offset.y * go->scale;
  return T;
}

void phys2d_boxdel(struct phys2d_box *box) {
  phys2d_shape_del(&box->shape);
}

void phys2d_applybox(struct phys2d_box *box) {
  phys2d_boxdel(box);
  struct gameobject *go = id2go(box->shape.go);
  cpVect off;
  off.x = box->offset[0];
  off.y = box->offset[1];
  cpTransform T = go2transform(id2go(box->shape.go), off, box->rotation);
  float hh = box->h / 2.f;
  float hw = box->w / 2.f;
  cpVect verts[4] = {{-hw, -hh}, {hw, -hh}, {hw, hh}, {-hw, hh}};
  box->shape.shape = cpSpaceAddShape(space, cpPolyShapeNew(go->body, 4, verts, T, box->r));
  init_phys2dshape(&box->shape, box->shape.go, box);
}

void phys2d_dbgdrawbox(struct phys2d_box *box) {
  int n = cpPolyShapeGetCount(box->shape.shape);
  cpVect points[n * 2];

  for (int i = 0; i < n; i++)
    points[i] = bodytransformpoint(cpShapeGetBody(box->shape.shape), cpPolyShapeGetVert(box->shape.shape, i));

  struct rgba c = shape_color(box->shape.shape);
  struct rgba cl = c;
  cl.a = col_alpha;
  float seglen = cpShapeGetSensor(box->shape.shape) ? sensor_seg : 0;  
  draw_line(points, n, cl,seglen, 1, 0);
  draw_poly(points, n, c);
}
/************** POLYGON ************/

struct phys2d_poly *Make2DPoly(int go) {
  struct phys2d_poly *new = malloc(sizeof(struct phys2d_poly));

  new->points = NULL;
  arrsetlen(new->points, 0);
  new->radius = 0.f;

  new->shape.shape = cpSpaceAddShape(space, cpPolyShapeNewRaw(id2go(go)->body, 0, new->points, new->radius));
  new->shape.debugdraw = phys2d_dbgdrawpoly;
  new->shape.moi = phys2d_poly_moi;
  init_phys2dshape(&new->shape, go, new);
  return new;
}

float phys2d_poly_moi(struct phys2d_poly *poly, float m) {
  float moi = cpMomentForPoly(m, arrlen(poly->points), poly->points, cpvzero, poly->radius);
  if (isnan(moi)) {
//    YughError("Polygon MOI returned an error. Returning 0.");
    return 0;
  }

  return moi;
}

void phys2d_polydel(struct phys2d_poly *poly) {
  arrfree(poly->points);
  phys2d_shape_del(&poly->shape);
}

void phys2d_polyaddvert(struct phys2d_poly *poly) {
  arrput(poly->points, cpvzero);
}

void phys2d_poly_setverts(struct phys2d_poly *poly, cpVect *verts) {
  if (!verts) return;
  arrfree(poly->points);
  poly->points = verts;
  phys2d_applypoly(poly);
}

void phys2d_applypoly(struct phys2d_poly *poly) {
  if (arrlen(poly->points) <= 0) return;
  struct gameobject *go = id2go(poly->shape.go);

  cpTransform T = go2transform(go, cpvzero, 0);

  cpPolyShapeSetVerts(poly->shape.shape, arrlen(poly->points), poly->points, T);
  cpPolyShapeSetRadius(poly->shape.shape, poly->radius);
  cpSpaceReindexShapesForBody(space, cpShapeGetBody(poly->shape.shape));
}
void phys2d_dbgdrawpoly(struct phys2d_poly *poly) {
  struct rgba color = shape_color(poly->shape.shape);
  struct rgba line_color = color;
  color.a = col_alpha;

  if (arrlen(poly->points) >= 3) {
    int n = cpPolyShapeGetCount(poly->shape.shape);
    cpVect points[n];

    for (int i = 0; i < n; i++)
      points[i] = bodytransformpoint(cpShapeGetBody(poly->shape.shape), cpPolyShapeGetVert(poly->shape.shape, i));

    draw_poly(points, n, color);
    float seglen = cpShapeGetSensor(poly->shape.shape) ? sensor_seg : 0;
    draw_line(points, n, line_color, seglen, 1, 0);
  }
}
/****************** EDGE 2D**************/

struct phys2d_edge *Make2DEdge(int go) {
  struct phys2d_edge *new = malloc(sizeof(struct phys2d_edge));
  new->points = NULL;
  arrsetlen(new->points, 0);
  new->thickness = 0.f;
  new->shapes = NULL;
  arrsetlen(new->shapes, 0);
  new->shape.go = go;
  new->shape.data = new;
  new->shape.debugdraw = phys2d_dbgdrawedge;
  new->shape.moi = phys2d_edge_moi;
  new->shape.shape = NULL;
  new->draws = 0;
  new->closed = 0;
  phys2d_applyedge(new);

  return new;
}

float phys2d_edge_moi(struct phys2d_edge *edge, float m) {
  float moi = 0;
  for (int i = 0; i < arrlen(edge->points) - 1; i++)
    moi += cpMomentForSegment(m, edge->points[i], edge->points[i + 1], edge->thickness);

  return moi;
}

void phys2d_edgedel(struct phys2d_edge *edge) {
  phys2d_shape_del(&edge->shape);
}

void phys2d_edgeaddvert(struct phys2d_edge *edge) {
  arrput(edge->points, cpvzero);
  if (arrlen(edge->points) > 1)
    arrput(edge->shapes, cpSpaceAddShape(space, cpSegmentShapeNew(id2go(edge->shape.go)->body, cpvzero, cpvzero, edge->thickness)));

  phys2d_applyedge(edge);
}

void phys2d_edge_rmvert(struct phys2d_edge *edge, int index) {
  assert(arrlen(edge->points) > index && index >= 0);

  arrdel(edge->points, index);

  if (arrlen(edge->points) == 0) return;

  if (index == 0) {
    cpSpaceRemoveShape(space, edge->shapes[index]);
    cpShapeFree(edge->shapes[index]);
    arrdel(edge->shapes, index);
    phys2d_applyedge(edge);
    return;
  }

  if (index != arrlen(edge->points)) {
    cpSegmentShapeSetEndpoints(edge->shapes[index - 1], edge->points[index - 1], edge->points[index]);
  }

  cpSpaceRemoveShape(space, edge->shapes[index - 1]);
  cpShapeFree(edge->shapes[index - 1]);
  arrdel(edge->shapes, index - 1);

  phys2d_applyedge(edge);
}

void phys2d_edge_setvert(struct phys2d_edge *edge, int index, cpVect val) {
  assert(arrlen(edge->points) > index && index >= 0);
  edge->points[index] = val;

  phys2d_applyedge(edge);
}

void phys2d_edge_clearverts(struct phys2d_edge *edge) {
  for (int i = arrlen(edge->points) - 1; i >= 0; i--) {
    phys2d_edge_rmvert(edge, i);
  }
}

void phys2d_edge_addverts(struct phys2d_edge *edge, cpVect *verts) {
  for (int i = 0; i < arrlen(verts); i++) {
    phys2d_edgeaddvert(edge);
    phys2d_edge_setvert(edge, i, verts[i]);
  }
}

void phys2d_applyedge(struct phys2d_edge *edge) {
  struct gameobject *go = id2go(edge->shape.go);

  for (int i = 0; i < arrlen(edge->shapes); i++) {
    cpVect a = gotransformpoint(go, edge->points[i]);
    cpVect b = gotransformpoint(go, edge->points[i + 1]);
    cpSegmentShapeSetEndpoints(edge->shapes[i], a, b);
    cpSegmentShapeSetRadius(edge->shapes[i], edge->thickness);
    if (i > 0 && i < arrlen(edge->shapes) - 1)
      cpSegmentShapeSetNeighbors(edge->shapes[i], gotransformpoint(go, edge->points[i - 1]), gotransformpoint(go, edge->points[i + 2]));
    go_shape_apply(NULL, edge->shapes[i], go);
    cpShapeSetUserData(edge->shapes[i], &edge->shape);
  }

  cpSpaceReindexShapesForBody(space, id2go(edge->shape.go)->body);
}

void phys2d_dbgdrawedge(struct phys2d_edge *edge) {
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

  float seglen = cpShapeGetSensor(edge->shapes[0]) ? sensor_seg : 0;
  struct rgba color = shape_color(edge->shapes[0]);
  struct rgba line_color = color;
  color.a = col_alpha;
  draw_edge(drawpoints, arrlen(edge->points), color, edge->thickness * 2, 0,0, line_color, seglen);
  draw_points(drawpoints, arrlen(edge->points), 2, kinematic_color);
}

/************ COLLIDER ****************/
void shape_enabled(struct phys2d_shape *shape, int enabled) {
  if (enabled)
    cpShapeSetFilter(shape->data, CP_SHAPE_FILTER_ALL);
  else
    cpShapeSetFilter(shape->data, CP_SHAPE_FILTER_NONE);
}

int shape_is_enabled(struct phys2d_shape *shape) {
  if (cpshape_enabled(shape->shape))
    return 1;

  return 0;
}

void shape_set_sensor(struct phys2d_shape *shape, int sensor) {
  if (!shape->shape) {
    struct phys2d_edge *edge = shape->data;

    for (int i = 0; i < arrlen(edge->shapes); i++)
      cpShapeSetSensor(edge->shapes[i], sensor);
  } else
    cpShapeSetSensor(shape->shape, sensor);
}

int shape_get_sensor(struct phys2d_shape *shape) {
  if (!shape->shape) {
    struct phys2d_edge *edge = shape->data;
    if (arrlen(edge->shapes) > 0) return cpShapeGetSensor(edge->shapes[0]);
	
    return 0;
  }

  return cpShapeGetSensor(shape->shape);
}

void phys2d_reindex_body(cpBody *body) {
  cpSpaceReindexShapesForBody(space, body);
}

struct postphys_cb {
  struct callee c;
  JSValue send;
};

static struct postphys_cb begins[512];
static uint32_t bptr;

void flush_collide_cbs() {
  for (int i = 0; i < bptr; i++)
    script_callee(begins[i].c, 1, &begins[i].send);

  bptr = 0;
}

void duk_call_phys_cb(cpVect norm, struct callee c, int hit, cpArbiter *arb) {
  cpShape *shape1;
  cpShape *shape2;
  cpArbiterGetShapes(arb, &shape1, &shape2);

  JSValue obj = JS_NewObject(js);
  JS_SetPropertyStr(js, obj, "normal", vec2js(norm));
  JS_SetPropertyStr(js, obj, "hit", JS_NewInt32(js, hit));
  JS_SetPropertyStr(js, obj, "sensor", JS_NewBool(js, cpShapeGetSensor(shape2)));
  JS_SetPropertyStr(js, obj, "velocity", vec2js(cpArbiterGetSurfaceVelocity(arb)));
  JS_SetPropertyStr(js, obj, "pos", vec2js(cpArbiterGetPointA(arb, 0)));
  JS_SetPropertyStr(js,obj,"depth", num2js(cpArbiterGetDepth(arb,0)));
  JS_SetPropertyStr(js, obj, "id", JS_NewInt32(js,hit));
  JS_SetPropertyStr(js,obj,"obj", JS_DupValue(js,id2go(hit)->ref));

  begins[bptr].c = c;
  begins[bptr].send = obj;
  bptr++;
  return;
}

#define CTYPE_BEGIN 0
#define CTYPE_SEP 1

static cpBool handle_collision(cpArbiter *arb, int type) {
  cpBody *body1;
  cpBody *body2;
  cpArbiterGetBodies(arb, &body1, &body2);
  int g1 = (int)cpBodyGetUserData(body1);
  int g2 = (int)cpBodyGetUserData(body2);
  struct gameobject *go = id2go(g1);
  struct gameobject *go2 = id2go(g2);

  cpShape *shape1;
  cpShape *shape2;
  cpArbiterGetShapes(arb, &shape1, &shape2);
  struct phys2d_shape *pshape1 = cpShapeGetUserData(shape1);
  struct phys2d_shape *pshape2 = cpShapeGetUserData(shape2);

  cpVect norm1 = cpArbiterGetNormal(arb);

  switch (type) {
  case CTYPE_BEGIN:
    for (int i = 0; i < arrlen(go->shape_cbs); i++)
      if (go->shape_cbs[i].shape == pshape1)
        duk_call_phys_cb(norm1, go->shape_cbs[i].cbs.begin, g2, arb);

    if (JS_IsObject(go->cbs.begin.obj))
      duk_call_phys_cb(norm1, go->cbs.begin, g2, arb);

    break;

  case CTYPE_SEP:
    if (JS_IsObject(go->cbs.separate.obj)) {
      YughWarn("Made it here; separate.");
      duk_call_phys_cb(norm1, go->cbs.separate, g2, arb);
    }

    break;
  }

  return 1;
}

static cpBool script_phys_cb_begin(cpArbiter *arb, cpSpace *space, void *data) {
  return handle_collision(arb, CTYPE_BEGIN);
}

static cpBool script_phys_cb_separate(cpArbiter *arb, cpSpace *space, void *data) {
  return handle_collision(arb, CTYPE_SEP);
}

void phys2d_rm_go_handlers(int go) {
  cpCollisionHandler *handler = cpSpaceAddWildcardHandler(space, go);
  handler->userData = NULL;
  handler->beginFunc = NULL;
  handler->separateFunc = NULL;
}

void phys2d_setup_handlers(int go) {
  cpCollisionHandler *handler = cpSpaceAddWildcardHandler(space, go);
  handler->userData = (void *)go;
  handler->beginFunc = script_phys_cb_begin;
  handler->separateFunc = script_phys_cb_separate;
}

static int airborne = 0;

void inair(cpBody *body, cpArbiter *arbiter, void *data) {
  airborne = 0;
}

int phys2d_in_air(cpBody *body) {
  airborne = 1;
  cpBodyEachArbiter(body, inair, NULL);

  return airborne;
}
