#ifndef WARP_H
#define WARP_H

#include "stdint.h"
#include "transform.h"

typedef uint32_t warpmask;

#define gravmask 1U

typedef struct {
  transform t;
  float strength;
  float decay;
  int spherical;
  HMM_Vec3 planar_force;
  warpmask mask;
} warp_gravity;

typedef struct {
  transform t;
  int unlimited_range;
  HMM_Vec3 range;
  HMM_Vec3 falloff;
  HMM_Vec3 damp;
  warpmask mask;
} warp_damp;
  
typedef struct {
  transform t;
  float strength;
  float decay;
  float pulse; /* strength of random variance in the wind effect */
  float frequency; /* when 0, pulse is smooth. Increase for very pulse over time */
  float turbulence; /* When 0, pulsing is smooth and regular. Increase for more chaos. */
  int spherical;
} warp_wind;

/* returns the total force for an object at pos */
HMM_Vec3 warp_force(HMM_Vec3 pos, warpmask mask);

warp_gravity *warp_gravity_make();
warp_damp *warp_damp_make();
void warp_gravity_free(warp_gravity *g);
void warp_damp_free(warp_damp *d);

#endif
