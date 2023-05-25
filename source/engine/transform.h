#ifndef TRANSFORM_H
#define TRANSFORM_H

#include "HandmadeMath.h"

struct mTransform {
    HMM_Vec3 pos;
    HMM_Quat rotation;
    float scale;
};


struct mTransform MakeTransform(HMM_Vec3 pos, HMM_Quat rotation, float scale);

HMM_Vec3 trans_forward(const struct mTransform *const trans);
HMM_Vec3 trans_back(const struct mTransform *trans);
HMM_Vec3 trans_up(const struct mTransform *trans);
HMM_Vec3 trans_down(const struct mTransform *trans);
HMM_Vec3 trans_right(const struct mTransform *trans);
HMM_Vec3 trans_left(const struct mTransform *trans);
void trans_drawgui(struct mTransform *T);

//extern Serialize *make_transform();

#endif
