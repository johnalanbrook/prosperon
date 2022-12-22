#ifndef TRANSFORM_H
#define TRANSFORM_H

#include "mathc.h"

struct mTransform {
    mfloat_t position[3];
    mfloat_t rotation[4];
    float scale;
};


struct mTransform MakeTransform(mfloat_t pos[3], mfloat_t rotation[3], float scale);

mfloat_t *trans_forward(mfloat_t * res, const struct mTransform *const trans);
mfloat_t *trans_back(mfloat_t * res, const struct mTransform *trans);
mfloat_t *trans_up(mfloat_t * res, const struct mTransform *trans);
mfloat_t *trans_down(mfloat_t * res, const struct mTransform *trans);
mfloat_t *trans_right(mfloat_t * res, const struct mTransform *trans);
mfloat_t *trans_left(mfloat_t * res, const struct mTransform *trans);
void trans_drawgui(struct mTransform *T);

//extern Serialize *make_transform();

#endif
