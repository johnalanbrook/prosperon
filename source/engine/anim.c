#include "anim.h"
#include "log.h"
#include "stb_ds.h"

HMM_Vec4 sample_linear(sampler *sampler, float time, int prev, int next)
{
  if (sampler->rotation)
    return (HMM_Vec4)HMM_SLerp(sampler->data[prev].quat, time, sampler->data[next].quat);
  else
    return HMM_LerpV4(sampler->data[prev], time, sampler->data[next]);
}

HMM_Vec4 sample_cubicspline(sampler *sampler, float t, int prev, int next)
{
  float t2 = t*t;
  float t3 = t2*t;
  float td = sampler->times[next]-sampler->times[prev];

  HMM_Vec4 v = HMM_MulV4F(sampler->data[prev*3+1], (2*t3-3*t2+1));
  v = HMM_AddV4(v, HMM_MulV4F(sampler->data[prev*3+2], td*(t3-2*t2+t)));
  v = HMM_AddV4(v, HMM_MulV4F(sampler->data[next*3+1], 3*t2-2*t3));
  v = HMM_AddV4(v, HMM_MulV4F(sampler->data[next*3], td*(t3-t2)));
  return v;
}

HMM_Vec4 sample_sampler(sampler *sampler, float time)
{
  int previous_time=0;
  int next_time=0;

  for (int i = 1; i < arrlen(sampler->times); i++) {
    if (time < sampler->times[i]) {
      previous_time = sampler->times[i-1];
      next_time = sampler->times[i];
      break;
    }
  }

  float td = sampler->times[next_time]-sampler->times[previous_time];
  float t = (time - sampler->times[previous_time])/td;

  switch(sampler->type) {
    case LINEAR:
      return sample_linear(sampler,t,previous_time,next_time);
      break;
    case STEP:
      return sampler->data[previous_time];
      break;
    case CUBICSPLINE:
      return sample_cubicspline(sampler,t, previous_time, next_time);
      break;
  }
  return sample_cubicspline(sampler,t, previous_time, next_time);  
}
