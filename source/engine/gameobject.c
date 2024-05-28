#include "gameobject.h"

#include "2dphysics.h"
#include <string.h>
#include "log.h"
#include "math.h"

#include "stb_ds.h"

transform go2t(gameobject *go)
{
  transform t = {0};
  t.pos.cp = cpBodyGetPosition(go->body);
  t.rotation = angle2rotation(cpBodyGetAngle(go->body));
  t.scale = go->t->scale;
  return t;
}

gameobject *body2go(cpBody *b)
{
  return cpBodyGetUserData(b);
}

gameobject *shape2go(cpShape *s)
{
  cpBody *b = cpShapeGetBody(s);
  return cpBodyGetUserData(b);
}

void gameobject_apply(gameobject *go) { *go->t = go2t(go); }

static void velocityFn(cpBody *body, cpVect gravity, cpFloat damping, cpFloat dt)
{
  gameobject *go = body2go(body);
  gameobject_apply(go);  
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
      .maxvelocity = INFINITY,
      .maxangularvelocity = INFINITY,
      .damping = INFINITY,
      .timescale = 1.0,
      .ref = JS_UNDEFINED,
      .warp_mask = ~0,
  };

  go.body = cpSpaceAddBody(space, cpBodyNew(1, 1));
  cpBodySetVelocityUpdateFunc(go.body, velocityFn);

  *ngo = go;
  cpBodySetUserData(go.body, ngo);
  phys2d_setup_handlers(ngo);
  return ngo;
}

void rm_body_shapes(cpBody *body, cpShape *shape, void *data) {
  cpSpaceRemoveShape(space, shape);
}

void rm_body_constraints(cpBody *body, cpConstraint *c, void *data)
{
  cpSpaceRemoveConstraint(space, c);
}

void gameobject_free(gameobject *go) {
  go->ref = JS_UNDEFINED;  
  cpBodyEachShape(go->body, rm_body_shapes, NULL);
  cpBodyEachConstraint(go->body, rm_body_constraints, NULL);
  cpSpaceRemoveBody(space, go->body);
  cpBodyFree(go->body);
  free(go);
}
