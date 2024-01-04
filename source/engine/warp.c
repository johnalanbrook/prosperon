#include "warp.h"
#include "stb_ds.h"
#include "log.h"

static warp_gravity **warps = NULL;

warp_gravity *warp_gravity_make()
{
  warp_gravity *n = calloc(sizeof(*n),1);
  n->t.pos = (HMM_Vec3){0,0,0};
  n->strength = 9.8;
  n->t.scale = (HMM_Vec3){0,-1,0};
  n->planar_force = HMM_MulV3F(n->t.scale, n->strength);
  n->mask = gravmask;
  n->spherical = 0;
  arrput(warps, n);
  return n;
}

HMM_Vec3 warp_gravity_force(warp_gravity *g, HMM_Vec3 pos)
{

  HMM_Vec3 f = (HMM_Vec3){0,0,0};
  if (g->strength == 0) return f;
  if (g->spherical) {
    HMM_Vec3 dir = HMM_SubV3(g->t.pos, pos);
    float len = HMM_LenV3(dir);
    if (len == 0) return f;
    HMM_Vec3 norm = HMM_NormV3(HMM_SubV3(g->t.pos, pos));
    return HMM_MulV3F(norm,g->strength);
  } else {
    return g->planar_force;
  }
}

HMM_Vec3 warp_force(HMM_Vec3 pos, warpmask mask)
{
  HMM_Vec3 f = (HMM_Vec3){0,0,0};
  for (int i = 0; i < arrlen(warps); i++) {
//    if (!(mask & warps[i]->mask)) continue;
    f = HMM_AddV3(f, warp_gravity_force(warps[i], pos));
  }
  return f;
}
