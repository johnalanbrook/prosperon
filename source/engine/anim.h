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

typedef struct samplerf {
  float *times;
  float *data;
  int type;
} samplerf;

typedef struct sampler {
  float *times;
  HMM_Vec4 *data;
  int type;
} sampler;

struct anim_channel {
  sampler *sampler;
};

struct animation {
  char *name;
  double time;
  struct anim_channel *channels;
};

void sampler_add(sampler *s, float time, HMM_Vec4 val);
HMM_Vec4 sample_sampler(sampler *sampler, float time);


#endif
