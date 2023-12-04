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

typedef struct sampler {
  float *times;
  HMM_Vec4 *data;
  int type;
  int rotation;
} sampler;

struct anim_channel {
  sampler *sampler;
};

struct animation {
  char *name;
  double time;
  struct anim_channel *channels;
};

HMM_Vec4 sample_sampler(sampler *sampler, float time);

#endif
