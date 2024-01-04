#ifndef WARP_H
#define WARP_H

#include "stdint.h"
#include "transform.h"

typedef uint32_t warpmask;

#define gravmask 1U

typedef struct {
  transform3d t;
  float strength;
  float decay;
  int spherical;
  HMM_Vec3 planar_force;
  warpmask mask;
} warp_gravity;

/* returns the total force for an object at pos */
HMM_Vec3 warp_force(HMM_Vec3 pos, warpmask mask);

warp_gravity *warp_gravity_make();



#endif
