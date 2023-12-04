#include "gameobject.h"

#include "2dphysics.h"
#include "log.h"
#include <chipmunk/chipmunk.h>
#include <string.h>
#include "debugdraw.h"
#include "freelist.h"

#include "stb_ds.h"

struct gameobject *gameobjects = NULL;

struct gameobject *id2go(int id) {
  if (id < 0) return NULL;

  return &gameobjects[id];
}

int body2id(cpBody *body) { return (int)cpBodyGetUserData(body); }

cpBody *id2body(int id) {
  struct gameobject *go = id2go(id);

  if (go)
    return go->body;

  return NULL;
}

int shape2gameobject(cpShape *shape) {
  struct phys2d_shape *s = cpShapeGetUserData(shape);
  return s->go;
}

struct gameobject *shape2go(cpShape *shape)
{
  return id2go(shape2gameobject(shape));
}

HMM_Vec2 go_pos(struct gameobject *go)
{
  cpVect p = cpBodyGetPosition(go->body);
  return (HMM_Vec2){p.x, p.y};
}

HMM_Vec2 go_worldpos(struct gameobject *go)
{
  HMM_Vec2 ret;
  ret.cp = cpBodyGetPosition(go->body);
  return ret;
}

float go_angle(struct gameobject *go) { return go_worldangle(go); }

float go_worldangle(struct gameobject *go) { return cpBodyGetAngle(go->body); }

float go2angle(struct gameobject *go) { return cpBodyGetAngle(go->body); }

transform3d go2t3(gameobject *go)
{
  transform3d t;
  HMM_Vec2 p = go_pos(go);
  t.pos.X = p.X;  
  t.pos.Y = p.Y;
  t.pos.Z = go->drawlayer;
  t.scale = go->scale;
  t.scale.Z = go->scale.X;
  t.rotation = HMM_QFromAxisAngle_RH(vFWD, go_angle(go));
  t.rotation = HMM_MulQ(HMM_QFromAxisAngle_RH(vRIGHT, -t.pos.Y/100), t.rotation);
  t.rotation = HMM_MulQ(HMM_QFromAxisAngle_RH(vUP, t.pos.X/100), t.rotation);
  return t;
}

HMM_Vec2 go2world(struct gameobject *go, HMM_Vec2 pos) { return mat_t_pos(t_go2world(go), pos); }

HMM_Vec2 world2go(struct gameobject *go, HMM_Vec2 pos) { return mat_t_pos(t_world2go(go), pos); }

HMM_Mat3 t_go2world(struct gameobject *go) { return transform2d2mat(go2t(go)); }

HMM_Mat3 t_world2go(struct gameobject *go) { return HMM_InvGeneralM3(t_go2world(go)); }

HMM_Mat4 t3d_go2world(struct gameobject *go) { return transform3d2mat(go2t3(go)); }
HMM_Mat4 t3d_world2go(struct gameobject *go) { return HMM_InvGeneralM4(t3d_go2world(go)); }

int pos2gameobject(HMM_Vec2 pos) {
  cpShape *hit = phys2d_query_pos(pos.cp);

  if (hit)
    return shape2gameobject(hit);

  for (int i = 0; i < arrlen(gameobjects); i++) {
    if (!gameobjects[i].body) continue;
    cpVect gpos = cpBodyGetPosition(gameobjects[i].body);
    float dist = cpvlength(cpvsub(gpos, pos.cp));

    if (dist <= 25) return i;
  }
  
  return -1;
}

transform2d go2t(gameobject *go)
{
  transform2d t;
  t.pos.cp = cpBodyGetPosition(go->body);
  t.angle = cpBodyGetAngle(go->body);
  t.scale = go->scale.XY;
  if (isnan(t.scale.X)) t.scale.X = 1;
  if (isnan(t.scale.Y)) t.scale.Y = 1;
  return t;
}

int go2id(struct gameobject *go) {
  for (int i = 0; i < arrlen(gameobjects); i++)
    if (&gameobjects[i] == go) return i;

  return -1;
}

unsigned int editor_cat = 1<<31;

void go_shape_apply(cpBody *body, cpShape *shape, struct gameobject *go) {
  cpShapeSetFriction(shape, go->f);
  cpShapeSetElasticity(shape, go->e);
  cpShapeSetCollisionType(shape, go2id(go));

  cpShapeFilter filter;
  filter.group = go2id(go);
  filter.categories = 1<<go->layer | editor_cat;
//  filter.mask = CP_ALL_CATEGORIES;
  filter.mask = category_masks[go->layer] | editor_cat;
//  filter.mask = CP_ALL_CATEGORIES;
  cpShapeSetFilter(shape, filter);

  struct phys2d_shape *ape = cpShapeGetUserData(shape);
  if (ape && ape->apply)
    ape->apply(ape->data);
}

void go_shape_moi(cpBody *body, cpShape *shape, struct gameobject *go) {
  float moment = cpBodyGetMoment(go->body);
  struct phys2d_shape *s = cpShapeGetUserData(shape);
  if (!s) {
    cpBodySetMoment(go->body, moment + 1);
    return;
  }

  moment += s->moi(s->data, go->mass);
  if (moment < 0) moment = 1;
  cpBodySetMoment(go->body, 1);
}

void gameobject_apply(struct gameobject *go) {
  cpBodySetType(go->body, go->bodytype);
  cpBodyEachShape(go->body, go_shape_apply, go);

  if (go->bodytype == CP_BODY_TYPE_DYNAMIC) {
    cpBodySetMass(go->body, go->mass);
    cpBodySetMoment(go->body, 0.f);
    cpBodyEachShape(go->body, go_shape_moi, go);

    if (cpBodyGetMoment(go->body) <= 0.f)
      cpBodySetMoment(go->body, 1.f);

    return;
  }
}

static void velocityFn(cpBody *body, cpVect gravity, cpFloat damping, cpFloat dt)
{
  struct gameobject *go = id2go((int)cpBodyGetUserData(body));
  if (!go) {
    cpBodyUpdateVelocity(body,gravity,damping,dt);
    return;
  }

  cpFloat d = isnan(go->damping) ? damping : d;
  cpVect g = go->gravity ? gravity : go->cgravity.cp;
  
  cpBodyUpdateVelocity(body,g,d,dt*go->timescale);

  if (!isinf(go->maxvelocity))
    cpBodySetVelocity(body, cpvclamp(cpBodyGetVelocity(body), go->maxvelocity));

  if (!isinf(go->maxangularvelocity)) {
    float av = cpBodyGetAngularVelocity(body);
    if (fabs(av) > go->maxangularvelocity)
      cpBodySetAngularVelocity(body, copysignf(go->maxangularvelocity, av));
  }
}

int MakeGameobject() {
  struct gameobject go = {
      .scale = (HMM_Vec3){1.f,1.f,1.f},
      .bodytype = CP_BODY_TYPE_STATIC,
      .maxvelocity = INFINITY,
      .maxangularvelocity = INFINITY,
      .mass = 1.f,
      .next = -1,
      .drawlayer = 0,
      .shape_cbs = NULL,
      .children = NULL,
      .gravity = 1,
      .cgravity = (HMM_Vec2){0,0},
      .damping = NAN,
      .timescale = 1.0,
      .ref = JS_UNDEFINED,
  };

  go.cbs.begin.obj = JS_UNDEFINED;
  go.cbs.separate.obj = JS_UNDEFINED;

  go.body = cpSpaceAddBody(space, cpBodyNew(go.mass, 1.f));
  cpBodySetVelocityUpdateFunc(go.body, velocityFn);

  int id;
  if (!gameobjects) freelist_size(gameobjects,500);
  freelist_grab(id, gameobjects);
  cpBodySetUserData(go.body, (void*)id);
  phys2d_setup_handlers(id);
  gameobjects[id] = go;
  return id;
}

void gameobject_traverse(struct gameobject *go, HMM_Mat4 p)
{
  HMM_Mat4 local = transform3d2mat(go2t3(go));
  go->world = HMM_MulM4(local, p);

  for (int i = 0; i < arrlen(go->children); i++)
    gameobject_traverse(go->children[i], go->world);
}

void rm_body_shapes(cpBody *body, cpShape *shape, void *data) {
  struct phys2d_shape *s = cpShapeGetUserData(shape);
  if (s->data) {
    free(s->data);
    s->data = NULL;
  }
  cpSpaceRemoveShape(space, shape);
  cpShapeFree(shape);
}

int *go_toclean = NULL;

/* Free this gameobject */
void gameobject_clean(int id) {
  struct gameobject *go = id2go(id);
  arrfree(go->shape_cbs);
  cpBodyEachShape(go->body, rm_body_shapes, NULL);
  cpSpaceRemoveBody(space, go->body);
  cpBodyFree(go->body);
  go->body = NULL;
}

/* Really more of a "mark for deletion" ... */
void gameobject_delete(int id) {
  gameobject *go = id2go(id);
  JS_FreeValue(js, go->ref);

  if (cpSpaceIsLocked(space))
    arrpush(go_toclean, id);
  else
    gameobject_clean(id);

  dag_clip(go);
  freelist_kill(gameobjects,id);  
}

void gameobject_free(int id)
{
  if (id >= 0)
    gameobject_delete(id);
}

void gameobjects_cleanup() {
  for (int i = 0; i < arrlen(go_toclean); i++)
    gameobject_clean(go_toclean[i]);

  arrsetlen(go_toclean, 0);
}

void gameobject_setangle(struct gameobject *go, float angle) {
  cpBodySetAngle(go->body, angle);
  phys2d_reindex_body(go->body);
}

void gameobject_setpos(struct gameobject *go, cpVect vec) {
  if (!go || !go->body) return;
  cpBodySetPosition(go->body, vec);
  phys2d_reindex_body(go->body);
}

void body_draw_shapes_dbg(cpBody *body, cpShape *shape, void *data) {
  struct phys2d_shape *s = cpShapeGetUserData(shape);
  s->debugdraw(s->data);
}

void gameobject_draw_debug(int go) {
  struct gameobject *g = id2go(go);
  if (!g || !g->body) return;

  cpVect pos = cpBodyGetPosition(g->body);
  cpBodyEachShape(g->body, body_draw_shapes_dbg, NULL);
}

void gameobject_draw_debugs() {
  for (int i = 0; i < arrlen(gameobjects); i++)
    gameobject_draw_debug(i);
}
