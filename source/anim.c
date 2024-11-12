#include "anim.h"
#include "stb_ds.h"

void animation_run(struct animation *anim, float now)
{
  float elapsed = now - anim->time;
  elapsed = fmod(elapsed,2);
  if (!anim->channels) return;  
  
  for (int i = 0; i < arrlen(anim->channels); i++) {
    struct anim_channel *ch = anim->channels+i;
    HMM_Vec4 s = sample_sampler(ch->sampler, elapsed);
    *(ch->target) = s;
  }
}

HMM_Vec4 sample_cubicspline(sampler *sampler, float t, int prev, int next)
{
  return (HMM_Vec4)HMM_SLerp(HMM_QV4(sampler->data[prev]), t, HMM_QV4(sampler->data[next]));
}

HMM_Vec4 sample_sampler(sampler *sampler, float time)
{
  if (arrlen(sampler->data) == 0) return v4zero;
  if (arrlen(sampler->data) == 1) return sampler->data[0];
  int previous_time=0;
  int next_time=0;

  for (int i = 1; i < arrlen(sampler->times); i++) {
    if (time < sampler->times[i]) {
      previous_time = i-1;
      next_time = i;
      break;
    }
  }
  
  float td = sampler->times[next_time]-sampler->times[previous_time];
  float t = (time - sampler->times[previous_time])/td;

  switch(sampler->type) {
    case LINEAR:
      return HMM_LerpV4(sampler->data[previous_time],time,sampler->data[next_time]);
      break;
    case STEP:
      return sampler->data[previous_time];
      break;
    case CUBICSPLINE:
      return sample_cubicspline(sampler,t, previous_time, next_time);
      break;
    case SLERP:
      return (HMM_Vec4)HMM_SLerp(sampler->data[previous_time].quat, time, sampler->data[next_time].quat);
      break;
  }
  return sample_cubicspline(sampler,t, previous_time, next_time);  
}
