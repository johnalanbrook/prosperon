#include "gameobject.h"

#include "2dphysics.h"
#include <chipmunk/chipmunk.h>
#include <string.h>
#include "debugdraw.h"
#include "log.h"

#include "stb_ds.h"

static gameobject **gameobjects;

gameobject *body2go(cpBody *body) { return cpBodyGetUserData(body); }
gameobject *shape2go(cpShape *shape) { return ((struct phys2d_shape *)cpShapeGetUserData(shape))->go; }

HMM_Vec2 go_pos(gameobject *go)
{
  cpVect p = cpBodyGetPosition(go->body);
  return (HMM_Vec2){p.x, p.y};
}
float go_angle(gameobject *go) { return cpBodyGetAngle(go->body); }

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

HMM_Vec2 go2world(gameobject *go, HMM_Vec2 pos) { return mat_t_pos(t_go2world(go), pos); }
HMM_Vec2 world2go(gameobject *go, HMM_Vec2 pos) { return mat_t_pos(t_world2go(go), pos); }
HMM_Mat3 t_go2world(gameobject *go) { return transform2d2mat(go2t(go)); }
HMM_Mat3 t_world2go(gameobject *go) { return HMM_InvGeneralM3(t_go2world(go)); }
HMM_Mat4 t3d_go2world(gameobject *go) { return transform3d2mat(go2t3(go)); }
HMM_Mat4 t3d_world2go(gameobject *go) { return HMM_InvGeneralM4(t3d_go2world(go)); }

gameobject *pos2gameobject(HMM_Vec2 pos, float give) {
  cpShape *hit = phys2d_query_pos(pos.cp);

  if (hit)
    return shape2go(hit);

  for (int i = 0; i < arrlen(gameobjects); i++) {
    if (!gameobjects[i]->body) continue;
    HMM_Vec2 gpos = go_pos(gameobjects[i]);
    float dist = HMM_DistV2(gpos,pos);

    if (dist <= give) return gameobjects[i];
  }

  return NULL;
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

unsigned int editor_cat = 1<<31;

void go_shape_apply(cpBody *body, cpShape *shape, gameobject *go) {
  cpShapeSetFriction(shape, go->f);
  cpShapeSetElasticity(shape, go->e);
  cpShapeSetCollisionType(shape, (int)go);

  cpShapeFilter filter;
  filter.group = (int)go;
  filter.categories = 1<<go->layer | editor_cat;
//  filter.mask = CP_ALL_CATEGORIES;
  filter.mask = category_masks[go->layer] | editor_cat;
//  filter.mask = CP_ALL_CATEGORIES;
  cpShapeSetFilter(shape, filter);

  struct phys2d_shape *ape = cpShapeGetUserData(shape);
  if (ape && ape->apply)
    ape->apply(ape->data);
}

void go_shape_moi(cpBody *body, cpShape *shape, gameobject *go) {
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

void gameobject_apply(gameobject *go) {
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
  gameobject *go = body2go(body);
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

gameobject *MakeGameobject() {
  gameobject *ngo = malloc(sizeof(*ngo));
  gameobject go = {
      .scale = (HMM_Vec3){1.f,1.f,1.f},
      .bodytype = CP_BODY_TYPE_STATIC,
      .maxvelocity = INFINITY,
      .maxangularvelocity = INFINITY,
      .mass = 1.f,
      .next = -1,
      .drawlayer = 0,
      .shape_cbs = NULL,
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

  *ngo = go;
  cpBodySetUserData(go.body, ngo);
  phys2d_setup_handlers(ngo);
  arrpush(gameobjects, ngo);
  return ngo;
}

void rm_body_shapes(cpBody *body, cpShape *shape, void *data) {
  struct phys2d_shape *s = cpShapeGetUserData(shape);
  cpSpaceRemoveShape(space, shape);
  cpShapeFree(shape);
}

gameobject **go_toclean = NULL;

/* Free this gameobject */
void gameobject_clean(gameobject *go) {
  arrfree(go->shape_cbs);
  cpBodyEachShape(go->body, rm_body_shapes, NULL);
  cpSpaceRemoveBody(space, go->body);
  cpBodyFree(go->body);
  go->body = NULL;

  free(go);
}

/* Really more of a "mark for deletion" ... */
void gameobject_free(gameobject *go) {
  if (!go) return;

  for (int i = arrlen(gameobjects)-1; i >= 0; i--)
    if (gameobjects[i] == go) {
      arrdelswap(gameobjects, i);
      break;
    }

  if (cpSpaceIsLocked(space))
    arrpush(go_toclean, go);
  else
    gameobject_clean(go);
}

void gameobjects_cleanup() {
  for (int i = 0; i < arrlen(go_toclean); i++)
    gameobject_clean(go_toclean[i]);

  arrsetlen(go_toclean, 0);
}

void gameobject_setangle(gameobject *go, float angle) {
  cpBodySetAngle(go->body, angle);
  phys2d_reindex_body(go->body);
}

void gameobject_setpos(gameobject *go, cpVect vec) {
  if (!go || !go->body) return;
  cpBodySetPosition(go->body, vec);
  phys2d_reindex_body(go->body);
}

void body_draw_shapes_dbg(cpBody *body, cpShape *shape, void *data) {
  struct phys2d_shape *s = cpShapeGetUserData(shape);
  s->debugdraw(s->data);
}

void gameobject_draw_debug(gameobject *go) {
  if (!go || !go->body) return;

  cpVect pos = cpBodyGetPosition(go->body);
  cpBodyEachShape(go->body, body_draw_shapes_dbg, NULL);
}

void gameobject_draw_debugs()
{
  for (int i = 0; i < arrlen(gameobjects); i++)
    gameobject_draw_debug(gameobjects[i]);
}
