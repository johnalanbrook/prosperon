#include "camera.h"

#include "gameobject.h"
#include "input.h"

const float CAMERA_MINSPEED = 1.f;
const float CAMERA_MAXSPEED = 300.f;
const float CAMERA_ROTATESPEED = 6.f;

void cam_goto_object(struct mCamera *cam, struct mTransform *transform) {
  mfloat_t fwd[3] = {0.f};
  vec3_subtract(cam->transform.position, transform->position,
                vec3_multiply_f(fwd, trans_forward(fwd, transform),
                                10.f));
}

void cam_inverse_goto(struct mCamera *cam, struct mTransform *transform) {
  mfloat_t fwd[3] = {0.f};
  vec3_add(transform->position, cam->transform.position,
           vec3_multiply_f(fwd, trans_forward(fwd, &cam->transform),
                           10.f));
}

mfloat_t *getviewmatrix(mfloat_t view[16],
                        const struct mCamera *const camera) {
  mfloat_t fwd[3] = {0.f};
  mfloat_t look[3] = {0.f};
  vec3_rotate_quat(fwd, FORWARD, camera->transform.rotation);
  vec3_add(look, camera->transform.position, fwd);
  mat4_look_at(view, camera->transform.position, look, UP);
  return view;
  /*return mat4_look_at(view, ncam.transform.position,
     vec3_add(look, ncam.transform.position,
     trans_forward(fwd, &ncam.transform)),
     UP); */
}

void camera_2d_update(struct mCamera *camera, float deltaT) {
  static mfloat_t frame[3];
  vec3_zero(frame);
  if (action_down(GLFW_KEY_W))
    vec3_add(frame, frame, UP);
  if (action_down(GLFW_KEY_S))
    vec3_add(frame, frame, DOWN);
  if (action_down(GLFW_KEY_A))
    vec3_add(frame, frame, LEFT);
  if (action_down(GLFW_KEY_D))
    vec3_add(frame, frame, RIGHT);

  float speedMult = action_down(GLFW_KEY_LEFT_SHIFT) ? 2.f : 1.f;

  if (!vec3_is_zero(frame)) {
    vec3_normalize(frame, frame);
    vec3_add(camera->transform.position, camera->transform.position,
             vec3_multiply_f(frame, frame,
                             camera->speed * speedMult * deltaT));
  }
}

/*
void camera_update(struct mCamera * camera, float mouseX, float mouseY,
                   const uint8_t * keystate, int32_t mouseWheelY,
                   float deltaTime)
{
    //    if (SDL_GetRelativeMouseMode()) vec3_zero(camera->frame_move);

    static mfloat_t holdvec[VEC3_SIZE];
    vec3_zero(camera->frame_move);

    if (currentKeystates[SDL_SCANCODE_W])
        vec3_add(camera->frame_move, camera->frame_move,
                trans_forward(holdvec, &camera->transform));

    if (currentKeystates[SDL_SCANCODE_S])
        vec3_subtract(camera->frame_move, camera->frame_move,
                      trans_forward(holdvec, &camera->transform));

    if (currentKeystates[SDL_SCANCODE_A])
        vec3_subtract(camera->frame_move, camera->frame_move,
                      trans_right(holdvec, &camera->transform));

    if (currentKeystates[SDL_SCANCODE_D])
        vec3_add(camera->frame_move, camera->frame_move,
                 trans_right(holdvec, &camera->transform));

    if (currentKeystates[SDL_SCANCODE_E])
        vec3_add(camera->frame_move, camera->frame_move,
                 trans_up(holdvec, &camera->transform));

    if (currentKeystates[SDL_SCANCODE_Q])
        vec3_subtract(camera->frame_move, camera->frame_move,
                      trans_up(holdvec, &camera->transform));

    camera->speedMult = currentKeystates[SDL_SCANCODE_LSHIFT] ? 2.f : 1.f;


    if (!vec3_is_zero(camera->frame_move)) {
        vec3_normalize(camera->frame_move, camera->frame_move);
        vec3_add(camera->transform.position, camera->transform.position,
                 vec3_multiply_f(camera->frame_move, camera->frame_move,
                                 camera->speed * camera->speedMult *
                                 deltaTime));
    }
    // Adjust speed based on mouse wheel
    camera->speed =
        clampf(camera->speed + mouseWheelY, CAMERA_MINSPEED,
               CAMERA_MAXSPEED);


    // TODO: Handle this as additive quaternions

    camera->yaw -= mouseX * CAMERA_ROTATESPEED * deltaTime;
    camera->pitch -= mouseY * CAMERA_ROTATESPEED * deltaTime;


    if (camera->pitch > 89.f)
        camera->pitch = 89.f;
    if (camera->pitch < -89.f)
        camera->pitch = -89.f;

    mfloat_t qyaw[4] = {0.f};
    mfloat_t qpitch[4] = {0.f};

    quat_from_axis_angle(qyaw, UP, camera->yaw);
    quat_from_axis_angle(qpitch, RIGHT, camera->pitch);
    quat_multiply(camera->transform.rotation, qyaw, qpitch);

}
*/
