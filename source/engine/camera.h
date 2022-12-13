#ifndef CAMERA_H
#define CAMERA_H

#include "transform.h"

extern const float CAMERA_MINSPEED;
extern const float CAMERA_MAXSPEED;
extern const float CAMERA_ROTATESPEED;

struct mCamera {
    struct mTransform transform;
    float speed;
    float speedMult;
    mfloat_t frame_move[VEC3_SIZE];
};

void camera_2d_update(struct mCamera *camera, float deltaT);

mfloat_t *getviewmatrix(mfloat_t view[MAT4_SIZE], const struct mCamera *const camera);
void cam_goto_object(struct mCamera *cam, struct mTransform *transform);
void cam_inverse_goto(struct mCamera *cam, struct mTransform *transform);

#endif
