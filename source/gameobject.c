#include "gameobject.h"

#include "math.h"
#include <chipmunk/chipmunk.h>

#include "stb_ds.h"

static void velocityFn(cpBody *body, cpVect gravity, cpFloat damping, cpFloat dt)
{
/*  gameobject *go = body2go(body);
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
*/
}
