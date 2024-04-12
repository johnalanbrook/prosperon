#include "2dphysics.h"

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
struct rgba color_clear = {0,0,0,0};

struct rgba disabled_color = {148,148,148,255};
struct rgba sleep_color = {255,140,228,255};
struct rgba dynamic_color = {255,70,46,255};
struct rgba kinematic_color = {255, 194, 64, 255};
struct rgba static_color = {73,209,80,255};

static JSValue fns[100];
static JSValue hits[100];
static int cb_idx = 0;

static const unsigned char col_alpha = 40;
static const float sensor_seg = 10;

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
  return cpSpacePointQueryNearest(space, pos, 0.f, CP_SHAPE_FILTER_ALL, NULL);
}


static int qhit;
void qpoint(cpShape *shape, cpFloat dist, cpVect point, int *data)
{
  qhit++;
}

void bbhit(cpShape *shape, int *data)
{
  qhit++;
}

int query_point(HMM_Vec2 pos)
{
  qhit = 0;
//  cpSpacePointQuery(space, pos.cp, 0, filter, qpoint, &qhit);
  cpSpaceBBQuery(space, cpBBNewForCircle(pos.cp, 2), CP_SHAPE_FILTER_ALL, bbhit, &qhit);
  return qhit;
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

static warp_gravity *space_gravity;

void phys2d_init()
{
  space = cpSpaceNew();
  cpSpaceSetSleepTimeThreshold(space, 1);
  cpSpaceSetCollisionSlop(space, 0.01);
  cpSpaceSetCollisionBias(space, cpfpow(1.0-0.5, 165.f));
  space_gravity = warp_gravity_make();
}

void phys2d_set_gravity(HMM_Vec2 v)
{
  float str = HMM_LenV2(v);
  HMM_Vec2 dir = HMM_NormV2(v);
  space_gravity->strength = str;
  space_gravity->t.scale = (HMM_Vec3){v.x,v.y, 0};
  space_gravity->planar_force = (HMM_Vec3){v.x,v.y,0};
}

constraint *constraint_make(cpConstraint *c)
{
  constraint *cp = malloc(sizeof(*cp));
  cp->c = c;
  cp->break_cb = JS_UNDEFINED;
  cp->remove_cb = JS_UNDEFINED;
  cpSpaceAddConstraint(space,c);
  cpConstraintSetUserData(c, cp);
  return cp;
}

void constraint_break(constraint *constraint)
{
  if (!constraint->c) return;
  cpSpaceRemoveConstraint(space, constraint->c);
  cpConstraintFree(constraint->c);
  constraint->c = NULL;
  script_call_sym(constraint->break_cb,0,NULL);
}

void constraint_free(constraint *constraint)
{
  constraint_break(constraint);
  free(constraint);
}

void constraint_test(cpConstraint *constraint, float *dt)
{
  float max = cpConstraintGetMaxForce(constraint);
  if (!isfinite(max)) return;
  float force = cpConstraintGetImpulse(constraint)/ *dt;
  if (force > max)
    constraint_break(cpConstraintGetUserData(constraint));
}

void phys2d_update(float deltaT) {
  cpSpaceStep(space, deltaT);
  cpSpaceEachConstraint(space, constraint_test, &deltaT);
  cb_idx = 0;
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

float phys2d_circle_moi(struct phys2d_circle *c) {
  float m = c->shape.go->mass;
  return cpMomentForCircle(m, 0, cpCircleShapeGetRadius(c->shape.shape), cpCircleShapeGetOffset(c->shape.shape));
}

void phys2d_circledel(struct phys2d_circle *c) { phys2d_shape_del(&c->shape); }
void circle2d_free(circle2d *c) { phys2d_circledel(c); }

void phys2d_dbgdrawcpcirc(cpShape *c) {
  HMM_Vec2 pos = mat_t_pos(t_go2world(shape2go(c)), (HMM_Vec2)cpCircleShapeGetOffset(c));
  float radius = cpCircleShapeGetRadius(c);
  struct rgba color = shape_color(c);
  float seglen = cpShapeGetSensor(c) ? 5 : -1;
  draw_circle(pos, radius, 1, color, seglen);
  color.a = col_alpha;
  draw_circle(pos,radius,radius,color,-1);
}

void phys2d_shape_apply(struct phys2d_shape *s)
{
  float moment = cpBodyGetMoment(s->go->body);
  float moi = s->moi(s->data);

  s->apply(s->data);
  float newmoi = s->moi(s->data);
  moment-=moi;
  moment += newmoi;
  if (moment < 0) moment = 0;
  cpBodySetMoment(s->go->body, moment);
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

float phys2d_poly_moi(struct phys2d_poly *poly) {
  float m = poly->shape.go->mass;
  int len = cpPolyShapeGetCount(poly->shape.shape);
  if (!len) {
    YughWarn("Cannot evaluate the MOI of a polygon of length %d.", len);
    return 0;
  }
  cpVect points[len];
  for (int i = 0; i < len; i++)
    points[i] = cpPolyShapeGetVert(poly->shape.shape, i);

  float moi = cpMomentForPoly(m, len, points, cpvzero, poly->radius);
  if (!isfinite(moi))
    return 0;

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

  phys2d_shape_apply(&poly->shape);
}

void phys2d_applypoly(struct phys2d_poly *poly) {
  if (arrlen(poly->points) <= 0) return;
  assert(sizeof(poly->points[0]) == sizeof(cpVect));
  struct gameobject *go = poly->shape.go;
  transform2d t = go2t(shape2go(poly->shape.shape));
  t.pos.cp = cpvzero;
  t.angle = 0;
  cpTransform T = m3_to_cpt(transform2d2mat(t));
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
    transform2d t = go2t(shape2go(poly->shape.shape));
    t.scale = (HMM_Vec2){1,1};
    HMM_Mat3 rt = transform2d2mat(t);
    for (int i = 0; i < n; i++)
      points[i] = mat_t_pos(rt, (HMM_Vec2)cpPolyShapeGetVert(poly->shape.shape, i));

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

float phys2d_edge_moi(struct phys2d_edge *edge) {
  float m = edge->shape.go->mass;
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
  cpShapeFilter set = enabled ? CP_SHAPE_FILTER_ALL : CP_SHAPE_FILTER_NONE;
  if (!shape->shape) {
    struct phys2d_edge *edge = shape->data;
    for (int i = 0; i < arrlen(edge->shapes[i]); i++)
      cpShapeSetFilter(edge->shapes[i], set);
  } else
    cpShapeSetFilter(shape->shape, set);
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

void phys2d_reindex_body(cpBody *body) { cpSpaceReindexShapesForBody(space, body); }

JSValue arb2js(cpArbiter *arb)
{
  cpBody *body1;
  cpBody *body2;
  cpArbiterGetBodies(arb, &body1, &body2);
  
  cpShape *shape1;
  cpShape *shape2;
  cpArbiterGetShapes(arb, &shape1, &shape2);

  struct phys2d_shape *pshape = cpShapeGetUserData(shape2);
  gameobject *go2 = cpBodyGetUserData(body2);
  
  JSValue obj = JS_NewObject(js);
  JS_SetPropertyStr(js, obj, "normal", vec22js((HMM_Vec2)cpArbiterGetNormal(arb)));
  JS_SetPropertyStr(js, obj, "obj", JS_DupValue(js,go2->ref));
  JS_SetPropertyStr(js, obj, "shape", JS_DupValue(js, pshape->ref));
//  JS_SetPropertyStr(js, obj, "point", vec22js((HMM_Vec2)cpArbiterGetPointA(arb, 0)));
  
  HMM_Vec2 srfv;
  srfv.cp = cpArbiterGetSurfaceVelocity(arb);
  JS_SetPropertyStr(js, obj, "velocity", vec22js(srfv));

  return obj;
}

void phys_run_post(cpSpace *space, JSValue *fn, JSValue *hit)
{
  script_call_sym(*fn, 1, hit);
  JS_FreeValue(js, *hit);
  JS_FreeValue(js, *fn);
}

void register_hit(cpArbiter *arb, gameobject *go, const char *name)
{
  if (JS_IsUndefined(go->ref)) return;
  JSValue cb = JS_GetPropertyStr(js, go->ref, name);
  if (!JS_IsUndefined(cb)) {
    JSValue jarb = arb2js(arb);
    fns[cb_idx] = JS_DupValue(js, cb);
    hits[cb_idx] = jarb;
    cpSpaceAddPostStepCallback(space, phys_run_post, &fns[cb_idx], &hits[cb_idx]);
    cb_idx++;
  }
  
  cpShape *s1, *s2;
  cpArbiterGetShapes(arb, &s1, &s2);  
  gameobject *g1, *g2;
  g1 = shape2go(s1);
  g2 = shape2go(g2);
  if (!g1) return;
  if (!g2) return;
  if (JS_IsUndefined(g1->ref)) return;
  if (JS_IsUndefined(g2->ref)) return;
  struct phys2d_shape *pshape1 = cpShapeGetUserData(s1);
  
  if (JS_IsUndefined(pshape1->ref)) return;
  cb = JS_GetPropertyStr(js, pshape1->ref, name);
  if (!JS_IsUndefined(cb)) {
    JSValue jarb = arb2js(arb);
    fns[cb_idx] = JS_DupValue(js,cb);
    hits[cb_idx] = jarb;
    cpSpaceAddPostStepCallback(space, phys_run_post, &fns[cb_idx], &hits[cb_idx]);
    cb_idx++;
  }
}

int script_phys_cb_begin(cpArbiter *arb, cpSpace *space, gameobject *go) { register_hit(arb, go, "collide"); return 1; }
void script_phys_cb_separate(cpArbiter *arb, cpSpace *space, gameobject *go) { register_hit(arb, go, "separate"); }

void phys2d_setup_handlers(gameobject *go) {
  cpCollisionHandler *handler = cpSpaceAddWildcardHandler(space, (cpCollisionType)go);
  handler->userData = go;
  handler->beginFunc = script_phys_cb_begin;
  handler->separateFunc = script_phys_cb_separate;
}
