#include "gameobject.h"

#include "2dphysics.h"
#include <string.h>
#include "log.h"
#include "math.h"

#include "stb_ds.h"

gameobject *body2go(cpBody *body) { return cpBodyGetUserData(body); }
gameobject *shape2go(cpShape *shape) {
  struct phys2d_shape *pshape = cpShapeGetUserData(shape);
  if (!pshape) return NULL;
  return pshape->go;
}

HMM_Vec3 go_pos(gameobject *go)
{
  cpVect p = cpBodyGetPosition(go->body);
  return (HMM_Vec3){p.x, p.y, 0};
}
float go_angle(gameobject *go) { return cpBodyGetAngle(go->body); }

transform go2t(gameobject *go)
{
  transform t = {0};
  t.pos.cp = cpBodyGetPosition(go->body);
  t.rotation = angle2rotation(cpBodyGetAngle(go->body));
  t.scale = go->scale;
  if (!isfinite(t.scale.X)) t.scale.X = 1;
  if (!isfinite(t.scale.Y)) t.scale.Y = 1;
  return t;
}

void go_shape_apply(cpBody *body, cpShape *shape, gameobject *go) {
  cpShapeSetFriction(shape, go->friction);
  cpShapeSetElasticity(shape, go->elasticity);
  cpShapeSetCollisionType(shape, (cpCollisionType)go);

  cpShapeFilter filter;
  filter.group = (cpCollisionType)go;
  filter.categories = go->categories;
  filter.mask = go->mask;
//  filter.mask = CP_ALL_CATEGORIES;
  cpShapeSetFilter(shape, filter);

  struct phys2d_shape *ape = cpShapeGetUserData(shape);
  if (ape && ape->apply)
    ape->apply(ape->data);
}

void go_shape_moi(cpBody *body, cpShape *shape, gameobject *go) {
  float moment = cpBodyGetMoment(body);
  struct phys2d_shape *s = cpShapeGetUserData(shape);
  if (!s) {
    cpBodySetMoment(body, moment + 1);
    return;
  }

  moment += s->moi(s->data);
  if (moment < 0) moment = 0;
  cpBodySetMoment(body, moment);
}

void gameobject_apply(gameobject *go) {
  YughSpam("Applying gameobject %p", go);
  cpBodySetType(go->body, go->phys);
  cpBodyEachShape(go->body, go_shape_apply, go);

  if (go->phys == CP_BODY_TYPE_DYNAMIC) {
    cpBodySetMass(go->body, go->mass);
    cpBodySetMoment(go->body, 0.f);
    cpBodyEachShape(go->body, go_shape_moi, go);

    if (cpBodyGetMoment(go->body) <= 0.f)
      cpBodySetMoment(go->body, 1.f);
  }
}

static void velocityFn(cpBody *body, cpVect gravity, cpFloat damping, cpFloat dt)
{
  gameobject *go = body2go(body);
  cpVect pos = cpBodyGetPosition(body);  
  HMM_Vec2 g = warp_force((HMM_Vec3){pos.x, pos.y, 0}, go->warp_mask).xy;
  if (!go) {
    cpBodyUpdateVelocity(body,g.cp,damping,dt);
    return;
  }

//  cpFloat d = isfinite(go->damping) ? go->damping : damping;
  cpFloat d = damping;
  
  cpBodyUpdateVelocity(body,g.cp,d,dt*go->timescale);

  if (isfinite(go->maxvelocity))
    cpBodySetVelocity(body, cpvclamp(cpBodyGetVelocity(body), go->maxvelocity));

  if (isfinite(go->maxangularvelocity)) {
    float av = cpBodyGetAngularVelocity(body);
    if (fabs(av) > go->maxangularvelocity)
      cpBodySetAngularVelocity(body, copysignf(go->maxangularvelocity, av));
  }
}

gameobject *MakeGameobject() {
  gameobject *ngo = malloc(sizeof(*ngo));
  gameobject go = {
      .scale = (HMM_Vec3){1.f,1.f,1.f},
      .phys = CP_BODY_TYPE_STATIC,
      .maxvelocity = INFINITY,
      .maxangularvelocity = INFINITY,
      .mass = 1.f,
      .damping = INFINITY,
      .timescale = 1.0,
      .ref = JS_UNDEFINED,
      .mask = ~0,
      .categories = 1,
      .warp_mask = ~0,
  };

  go.body = cpSpaceAddBody(space, cpBodyNew(go.mass, 1.f));
  cpBodySetVelocityUpdateFunc(go.body, velocityFn);

  *ngo = go;
  cpBodySetUserData(go.body, ngo);
  phys2d_setup_handlers(ngo);
  return ngo;
}

void rm_body_shapes(cpBody *body, cpShape *shape, void *data) {
  struct phys2d_shape *s = cpShapeGetUserData(shape);
  
  if (s) {
    JS_FreeValue(js, s->ref);
    s->ref = JS_UNDEFINED;
    if (s->free)
      s->free(s->data);
    else
      free(s->data);
  }

  cpShapeSetFilter(shape, CP_SHAPE_FILTER_NONE);
  
  cpSpaceRemoveShape(space, shape);
  cpShapeFree(shape);
}

void rm_body_constraints(cpBody *body, cpConstraint *constraint, void *data)
{
  constraint_break(cpConstraintGetUserData(constraint));
}

void gameobject_free(gameobject *go) {
  go->ref = JS_UNDEFINED;  
  cpBodyEachShape(go->body, rm_body_shapes, NULL);
  cpBodyEachConstraint(go->body, rm_body_constraints, NULL);
  cpSpaceRemoveBody(space, go->body);
  cpBodyFree(go->body);
  free(go);
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
