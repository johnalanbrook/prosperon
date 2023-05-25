#include "camera.h"

#include "gameobject.h"
#include "input.h"

const float CAMERA_MINSPEED = 1.f;
const float CAMERA_MAXSPEED = 300.f;
const float CAMERA_ROTATESPEED = 6.f;

void cam_goto_object(struct mCamera *cam, struct mTransform *transform) {
  cam->transform.pos = HMM_SubV3(transform->pos, HMM_MulV3F(trans_forward(transform), 10.0));
}

void cam_inverse_goto(struct mCamera *cam, struct mTransform *transform) {
  transform->pos = HMM_AddV3(cam->transform.pos, HMM_MulV3F(trans_forward(&cam->transform), 10.0));
}

HMM_Mat4 getviewmatrix(const struct mCamera *const camera)
{
  HMM_Vec3 lookvec = HMM_AddV3(camera->transform.pos, trans_forward(&camera->transform.rotation));
  return HMM_LookAt_RH(camera->transform.pos, lookvec, vY);
}
