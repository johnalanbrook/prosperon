#include "2dphysics.h"

#include "debug.h"
#include "gameobject.h"
#include <string.h>

#include "debugdraw.h"
#include "stb_ds.h"
#include <assert.h>
#include <chipmunk/chipmunk_unsafe.h>
#include <math.h>

#include "2dphysics.h"

#include "jsffi.h"
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

void set_cat_mask(int cat, unsigned int mask) { category_masks[cat] = mask; }

cpTransform m3_to_cpt(HMM_Mat3 m)
{
  cpTransform t;
  t.a = m.Columns[0].x;
  t.b = m.Columns[0].y;
  t.tx = m.Columns[2].x;
  t.c = m.Columns[1].x;
  t.d = m.Columns[1].y;
  t.ty = m.Columns[2].y;
  return t;
}

cpShape *phys2d_query_pos(cpVect pos) {
  cpShapeFilter filter;
  filter.group = CP_NO_GROUP;
  filter.mask = CP_ALL_CATEGORIES;
  filter.categories = CP_ALL_CATEGORIES;
  cpShape *find = cpSpacePointQueryNearest(space, pos, 0.f, filter, NULL);

  return find;
}

int p_compare(void *a, void *b)
{
  if (a > b) return 1;
  if (a < b) return -1;
  return 0;
}

gameobject **clean_ids(gameobject **ids)
{
  qsort(ids, arrlen(ids), sizeof(*ids), p_compare);
  gameobject *curid = NULL;
  for (int i = arrlen(ids)-1; i >= 0; i--)
    if (ids[i] == curid)
      arrdelswap(ids, i);
    else
      curid = ids[i];

  return ids;
}

typedef struct querybox {
  cpBB bb;
  gameobject **ids;
} querybox;

void querylist(cpShape *shape, cpContactPointSet *points, querybox *qb) {
  arrput(qb->ids, shape2go(shape));
}

void querylistbodies(cpBody *body, querybox *qb) {
  if (cpBBContainsVect(qb->bb, cpBodyGetPosition(body)))
    arrput(qb->ids,body2go(body));
}

/* Return all points from a list of points in the given boundingbox */
int *phys2d_query_box_points(HMM_Vec2 pos, HMM_Vec2 wh, HMM_Vec2 *points, int n) {
  cpBB bbox;
  bbox = cpBBExpand(bbox, cpvadd(pos.cp, cpvmult(wh.cp,0.5)));
  bbox = cpBBExpand(bbox, cpvsub(pos.cp, cpvmult(wh.cp,0.5)));
  int *hits = NULL;

  for (int i = 0; i < n; i++)
    if (cpBBContainsVect(bbox, points[i].cp))
      arrpush(hits, i);

  return hits;
}

/* Return all gameobjects within the given box */
gameobject **phys2d_query_box(HMM_Vec2 pos, HMM_Vec2 wh) {
  cpShape *box = cpBoxShapeNew(NULL, wh.x, wh.y, 0.f);
  cpTransform T = {0};
  T.a = 1;
  T.d = 1;
  T.tx = pos.x;
  T.ty = pos.y;
  cpShapeUpdate(box, T);

  cpBB bbox = cpShapeGetBB(box);

  querybox qb;
  qb.bb = bbox;
  qb.ids = NULL;

  cpSpaceShapeQuery(space, box, querylist, &qb);
  cpSpaceEachBody(space, querylistbodies, &qb);

  cpShapeFree(box);

  return clean_ids(qb.ids);
}

gameobject **phys2d_query_shape(struct phys2d_shape *shape)
{
  gameobject **ids = NULL;
  cpSpaceShapeQuery(space, shape->shape, querylist, ids);
  return clean_ids(ids);
}

int cpshape_enabled(cpShape *c) {
  cpShapeFilter filter = cpShapeGetFilter(c);
  if (filter.categories == ~CP_ALL_CATEGORIES && filter.mask == ~CP_ALL_CATEGORIES)
    return 0;

  return 1;
}

struct rgba shape_color(cpShape *shape) {
  if (!cpshape_enabled(shape)) return disabled_color;
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

void init_phys2dshape(struct phys2d_shape *shape, gameobject *go, void *data) {
  shape->go = go;
  shape->data = data;
  shape->t.scale = (HMM_Vec2){1.0,1.0};
  go_shape_apply(go->body, shape->shape, go);
  cpShapeSetCollisionType(shape->shape, (cpCollisionType)go);
  cpShapeSetUserData(shape->shape, shape);
}

void phys2d_shape_del(struct phys2d_shape *shape) {
  if (!shape->shape) return;
  cpSpaceRemoveShape(space, shape->shape);
  cpShapeFree(shape->shape);
}

/***************** CIRCLE2D *****************/
struct phys2d_circle *Make2DCircle(gameobject *go) {
  struct phys2d_circle *new = malloc(sizeof(struct phys2d_circle));

  new->radius = 10.f;
  new->offset = v2zero;

  new->shape.shape = cpSpaceAddShape(space, cpCircleShapeNew(go->body, new->radius, cpvzero));
  new->shape.debugdraw = phys2d_dbgdrawcircle;
  new->shape.moi = phys2d_circle_moi;
  new->shape.apply = phys2d_applycircle;
  new->shape.free = NULL;
  init_phys2dshape(&new->shape, go, new);
  phys2d_applycircle(new);

  return new;
}

float phys2d_circle_moi(struct phys2d_circle *c, float m) {
  return 1;
  //TODO: Calculate correctly
  //return cpMomentForCircle(m, 0, c->radius, c->offset);
}

void phys2d_circledel(struct phys2d_circle *c) {
  phys2d_shape_del(&c->shape);
}

void phys2d_dbgdrawcpcirc(cpShape *c) {
  HMM_Vec2 pos = mat_t_pos(t_go2world(shape2go(c)), (HMM_Vec2)cpCircleShapeGetOffset(c));
  float radius = cpCircleShapeGetRadius(c);
  struct rgba color = shape_color(c);
  float seglen = cpShapeGetSensor(c) ? 5 : -1;
  draw_circle(pos, radius, 1, color, seglen);
  color.a = col_alpha;
  draw_circle(pos,radius,radius,color,-1);
}

void phys2d_dbgdrawcircle(struct phys2d_circle *circle) {
  phys2d_dbgdrawcpcirc(circle->shape.shape);
}

void phys2d_applycircle(struct phys2d_circle *circle) {
  gameobject *go = circle->shape.go;
  float radius = circle->radius * HMM_MAX(HMM_ABS(go->scale.X), HMM_ABS(go->scale.Y));
  cpCircleShapeSetRadius(circle->shape.shape, radius);
  cpCircleShapeSetOffset(circle->shape.shape, circle->offset.cp);
}

/************** POLYGON ************/

struct phys2d_poly *Make2DPoly(gameobject *go) {
  struct phys2d_poly *new = malloc(sizeof(struct phys2d_poly));

  new->points = NULL;
  arrsetlen(new->points, 0);
  new->radius = 0.f;

  new->shape.shape = cpSpaceAddShape(space, cpPolyShapeNewRaw(go->body, 0, (cpVect*)new->points, new->radius));
  new->shape.debugdraw = phys2d_dbgdrawpoly;
  new->shape.moi = phys2d_poly_moi;
  new->shape.free = phys2d_poly_free;
  new->shape.apply = phys2d_applypoly;
  init_phys2dshape(&new->shape, go, new);
  return new;
}

void phys2d_poly_free(struct phys2d_poly *poly)
{
  arrfree(poly->points);
  free(poly);
}

float phys2d_poly_moi(struct phys2d_poly *poly, float m) {
  float moi = cpMomentForPoly(m, arrlen(poly->points), (cpVect*)poly->points, cpvzero, poly->radius);
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
  arrput(poly->points, v2zero);
}

void phys2d_poly_setverts(struct phys2d_poly *poly, HMM_Vec2 *verts) {
  if (!verts) return;
  if (poly->points)
    arrfree(poly->points);
    
  arrsetlen(poly->points, arrlen(verts));
  
  for (int i = 0; i < arrlen(verts); i++)
    poly->points[i] = verts[i];
    
  phys2d_applypoly(poly);
}

void phys2d_applypoly(struct phys2d_poly *poly) {
  if (arrlen(poly->points) <= 0) return;
  assert(sizeof(poly->points[0]) == sizeof(cpVect));
  struct gameobject *go = poly->shape.go;
//  cpTransform T = m3_to_cpt(transform2d2mat(poly->t));
  cpTransform T = m3_to_cpt(transform2d2mat(poly->shape.go->t));
  cpPolyShapeSetVerts(poly->shape.shape, arrlen(poly->points), (cpVect*)poly->points, T);
  cpPolyShapeSetRadius(poly->shape.shape, poly->radius);
  cpSpaceReindexShapesForBody(space, cpShapeGetBody(poly->shape.shape));
}
void phys2d_dbgdrawpoly(struct phys2d_poly *poly) {
  struct rgba color = shape_color(poly->shape.shape);
  struct rgba line_color = color;
  color.a = col_alpha;

  if (arrlen(poly->points) >= 3) {
    int n = cpPolyShapeGetCount(poly->shape.shape);
    HMM_Vec2 points[n+1];
    HMM_Mat3 rt = t_go2world(shape2go(poly->shape.shape));
    for (int i = 0; i < n; i++)
      points[i] = (HMM_Vec2)cpPolyShapeGetVert(poly->shape.shape, i);

    points[n] = points[0];

    draw_poly(points, n, color);
    float seglen = cpShapeGetSensor(poly->shape.shape) ? sensor_seg : 0;
    draw_line(points, n, line_color, seglen, 0);
  }
}
/****************** EDGE 2D**************/

struct phys2d_edge *Make2DEdge(gameobject *go) {
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
  new->shape.apply = NULL;
  new->shape.free = phys2d_edge_free;
  new->draws = 0;
  phys2d_applyedge(new);

  return new;
}

void phys2d_edge_free(struct phys2d_edge *edge)
{
  for (int i = 0; i < arrlen(edge->shapes); i++)
    cpShapeSetUserData(edge->shapes[i], NULL);
  arrfree(edge->points);
  arrfree(edge->shapes);
  free(edge);
}

float phys2d_edge_moi(struct phys2d_edge *edge, float m) {
  float moi = 0;
  for (int i = 0; i < arrlen(edge->points) - 1; i++)
    moi += cpMomentForSegment(m, edge->points[i].cp, edge->points[i + 1].cp, edge->thickness);

  return moi;
}

void phys2d_edgedel(struct phys2d_edge *edge) { phys2d_shape_del(&edge->shape); }

void phys2d_edgeaddvert(struct phys2d_edge *edge, HMM_Vec2 v) {
  arrput(edge->points, v);
  if (arrlen(edge->points) > 1)
    arrput(edge->shapes, cpSpaceAddShape(space, cpSegmentShapeNew(edge->shape.go->body, cpvzero, cpvzero, edge->thickness)));
}

void phys2d_edge_rmvert(struct phys2d_edge *edge, int index) {
  if (index>arrlen(edge->points) || index < 0) return;

  arrdel(edge->points, index);

  if (arrlen(edge->points) == 0) return;

  if (index == 0) {
    cpSpaceRemoveShape(space, edge->shapes[index]);
    cpShapeFree(edge->shapes[index]);
    arrdel(edge->shapes, index);
    return;
  }

  if (index != arrlen(edge->points))
    cpSegmentShapeSetEndpoints(edge->shapes[index - 1], edge->points[index - 1].cp, edge->points[index].cp);

  cpSpaceRemoveShape(space, edge->shapes[index - 1]);
  cpShapeFree(edge->shapes[index - 1]);
  arrdel(edge->shapes, index - 1);
}

void phys2d_edge_setvert(struct phys2d_edge *edge, int index, cpVect val) {
  assert(arrlen(edge->points) > index && index >= 0);
  edge->points[index].cp = val;
}

void phys2d_edge_update_verts(struct phys2d_edge *edge, HMM_Vec2 *verts)
{
  if (arrlen(edge->points) == arrlen(verts)) {
    for (int i = 0; i < arrlen(verts); i++)
      phys2d_edge_setvert(edge,i,verts[i].cp);
  } else {
    int vertchange = arrlen(verts)-arrlen(edge->points);
    phys2d_edge_clearverts(edge);
    phys2d_edge_addverts(edge,verts);
  }

  phys2d_applyedge(edge);
}

void phys2d_edge_clearverts(struct phys2d_edge *edge) {
  for (int i = arrlen(edge->points) - 1; i >= 0; i--)
    phys2d_edge_rmvert(edge, i);
}

void phys2d_edge_addverts(struct phys2d_edge *edge, HMM_Vec2 *verts) {
  for (int i = 0; i < arrlen(verts); i++)
    phys2d_edgeaddvert(edge, verts[i]);
}

/* Calculates all true positions of verts, links them up, and so on */
void phys2d_applyedge(struct phys2d_edge *edge) {
  struct gameobject *go = edge->shape.go;

  for (int i = 0; i < arrlen(edge->shapes); i++) {
    /* Points must be scaled with gameobject, */
    HMM_Vec2 a = HMM_MulV2(go->scale.xy, edge->points[i]);
    HMM_Vec2 b = HMM_MulV2(go->scale.xy, edge->points[i+1]);
    cpSegmentShapeSetEndpoints(edge->shapes[i], a.cp, b.cp);
    cpSegmentShapeSetRadius(edge->shapes[i], edge->thickness);
    if (i > 0 && i < arrlen(edge->shapes) - 1)
      cpSegmentShapeSetNeighbors(edge->shapes[i], HMM_MulV2(go->scale.xy,edge->points[i-1]).cp, HMM_MulV2(go->scale.xy,edge->points[i+2]).cp);
    go_shape_apply(NULL, edge->shapes[i], go);
    cpShapeSetUserData(edge->shapes[i], &edge->shape);
  }

  cpSpaceReindexShapesForBody(space, edge->shape.go->body);
}

void phys2d_dbgdrawedge(struct phys2d_edge *edge) {
  edge->draws++;
  if (edge->draws > 1) {
    if (edge->draws >= arrlen(edge->shapes))
      edge->draws = 0;

    return;
  }

  if (arrlen(edge->shapes) < 1) return;

  HMM_Vec2 drawpoints[arrlen(edge->points)];
  struct gameobject *go = edge->shape.go;

  HMM_Mat3 g2w = t_go2world(go);
  for (int i = 0; i < arrlen(edge->points); i++) 
    drawpoints[i] = mat_t_pos(g2w, edge->points[i]);

  float seglen = cpShapeGetSensor(edge->shapes[0]) ? sensor_seg : 0;
  struct rgba color = shape_color(edge->shapes[0]);
  struct rgba line_color = color;
  color.a = col_alpha;
  draw_edge(drawpoints, arrlen(edge->points), color, edge->thickness * 2, 0, line_color, seglen);
  draw_points(drawpoints, arrlen(edge->points), 2, kinematic_color);
}

/************ COLLIDER ****************/
void shape_enabled(struct phys2d_shape *shape, int enabled) {
  if (enabled)
    cpShapeSetFilter(shape->shape, CP_SHAPE_FILTER_ALL);
  else
    cpShapeSetFilter(shape->shape, CP_SHAPE_FILTER_NONE);
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
    YughInfo("Attempted to get the sensor of an edge with no shapes. It has %d points.", arrlen(edge->points));
    return 0;
  }

  return cpShapeGetSensor(shape->shape);
}

void phys2d_reindex_body(cpBody *body) { cpSpaceReindexShapesForBody(space, body); }

struct postphys_cb {
  struct callee c;
  JSValue send;
};

static struct postphys_cb *begins = NULL;

void flush_collide_cbs() {
  for (int i = 0; i < arrlen(begins); i++) {
    script_callee(begins[i].c, 1, &begins[i].send);
    JS_FreeValue(js, begins[i].send);
  }

  arrsetlen(begins,0);
}

void duk_call_phys_cb(HMM_Vec2 norm, struct callee c, gameobject *hit, cpArbiter *arb) {
  cpShape *shape1;
  cpShape *shape2;
  cpArbiterGetShapes(arb, &shape1, &shape2);

  JSValue obj = JS_NewObject(js);
  JS_SetPropertyStr(js, obj, "normal", vec2js(norm));
  JS_SetPropertyStr(js, obj, "obj", hit->ref);
  JS_SetPropertyStr(js, obj, "sensor", JS_NewBool(js, cpShapeGetSensor(shape2)));
  HMM_Vec2 srfv;
  srfv.cp = cpArbiterGetSurfaceVelocity(arb);
  JS_SetPropertyStr(js, obj, "velocity", vec2js(srfv));
//  srfv.cp = cpArbiterGetPointA(arb,0);
//  JS_SetPropertyStr(js, obj, "pos", vec2js(srfv));
//  JS_SetPropertyStr(js,obj,"depth", num2js(cpArbiterGetDepth(arb,0)));

  struct postphys_cb cb;
  cb.c = c;
  cb.send = obj;
  arrput(begins, cb);  
}

#define CTYPE_BEGIN 0
#define CTYPE_SEP 1

static cpBool handle_collision(cpArbiter *arb, int type) {
  cpBody *body1;
  cpBody *body2;
  cpArbiterGetBodies(arb, &body1, &body2);
  gameobject *go = cpBodyGetUserData(body1);
  gameobject *go2 = cpBodyGetUserData(body2);
  cpShape *shape1;
  cpShape *shape2;
  cpArbiterGetShapes(arb, &shape1, &shape2);
  struct phys2d_shape *pshape1 = cpShapeGetUserData(shape1);
  struct phys2d_shape *pshape2 = cpShapeGetUserData(shape2);

  HMM_Vec2 norm1;
  norm1.cp = cpArbiterGetNormal(arb);

  switch (type) {
  case CTYPE_BEGIN:
    for (int i = 0; i < arrlen(go->shape_cbs); i++)
      if (go->shape_cbs[i].shape == pshape1)
        duk_call_phys_cb(norm1, go->shape_cbs[i].cbs.begin, go2, arb);

    if (JS_IsObject(go->cbs.begin.obj))
      duk_call_phys_cb(norm1, go->cbs.begin, go2, arb);

    break;

  case CTYPE_SEP:
    if (JS_IsObject(go->cbs.separate.obj))
      duk_call_phys_cb(norm1, go->cbs.separate, go2, arb);

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

void phys2d_rm_go_handlers(gameobject *go) {
  cpCollisionHandler *handler = cpSpaceAddWildcardHandler(space, (cpCollisionType)go);
  handler->userData = NULL;
  handler->beginFunc = NULL;
  handler->separateFunc = NULL;
}

void phys2d_setup_handlers(gameobject *go) {
  cpCollisionHandler *handler = cpSpaceAddWildcardHandler(space, (cpCollisionType)go);
  handler->userData = go;
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
