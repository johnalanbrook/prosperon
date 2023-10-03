#include "transform.h"
#include <string.h>

struct mTransform MakeTransform(HMM_Vec3 pos, HMM_Quat rotation, float scale)
{
  struct mTransform newT = {
    .pos = pos,
    .rotation = rotation,
    .scale = scale
  };
  return newT;
}

HMM_Vec3 trans_forward(const struct mTransform *const trans)
{
  return HMM_QVRot(vFWD, trans->rotation);
}

HMM_Vec3 trans_back(const struct mTransform *trans)
{
  return HMM_QVRot(vBKWD, trans->rotation);
}

HMM_Vec3 trans_up(const struct mTransform *trans)
{
  return HMM_QVRot(vUP, trans->rotation);
}

HMM_Vec3 trans_down(const struct mTransform *trans)
{
  return HMM_QVRot(vDOWN, trans->rotation);
}

HMM_Vec3 trans_right(const struct mTransform *trans)
{
  return HMM_QVRot(vRIGHT, trans->rotation);
}

HMM_Vec3 trans_left(const struct mTransform *trans)
{
  return HMM_QVRot(vLEFT, trans->rotation);
}
