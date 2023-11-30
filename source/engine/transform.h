#ifndef TRANSFORM_H
#define TRANSFORM_H

#include "HandmadeMath.h"

typedef struct {
    HMM_Vec3 pos;
    HMM_Vec3 scale;
    HMM_Quat rotation;
} transform3d;

typedef struct {
  HMM_Vec2 pos;
  HMM_Vec2 scale;
  float angle;
} transform2d;

extern const transform2d t2d_unit;

HMM_Vec3 trans_forward(const transform3d *const trans);
HMM_Vec3 trans_back(const transform3d *trans);
HMM_Vec3 trans_up(const transform3d *trans);
HMM_Vec3 trans_down(const transform3d *trans);
HMM_Vec3 trans_right(const transform3d *trans);
HMM_Vec3 trans_left(const transform3d *trans);

/* Transform a position via the matrix */
HMM_Vec2 mat_t_pos(HMM_Mat3 m, HMM_Vec2 pos);
/* Transform a direction via the matrix - does not take into account translation of matrix */
HMM_Vec2 mat_t_dir(HMM_Mat3 m, HMM_Vec2 dir);

HMM_Vec2 mat_up(HMM_Mat3 m);
HMM_Vec2 mat_right(HMM_Mat3 m);
float vec_angle(HMM_Vec2 a, HMM_Vec2 b);
float vec_dirangle(HMM_Vec2 a, HMM_Vec2 b);

HMM_Vec3 mat3_t_pos(HMM_Mat4 m, HMM_Vec3 pos);
HMM_Vec3 mat3_t_dir(HMM_Mat4 m, HMM_Vec3 dir);

HMM_Mat3 transform2d2mat(transform2d t);
HMM_Mat4 transform3d2mat(transform3d t);

#endif
