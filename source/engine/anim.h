#ifndef ANIM_H
#define ANIM_H

#include "HandmadeMath.h"

struct keyframe {
  double time;
  double val;
};

#define LINEAR 0
#define STEP 1
#define CUBICSPLINE 2
#define SLERP 3

typedef struct sampler {
  float *times;
  HMM_Vec4 *data;
  int type;
} sampler;

struct anim_channel {
  HMM_Vec4 *target;
  int comps;
  sampler *sampler;
};

typedef struct animation {
  double time;
  struct anim_channel *channels;
  sampler *samplers;
} animation;

void animation_run(struct animation *anim, float now);
HMM_Vec4 sample_sampler(sampler *sampler, float time);

#endif
