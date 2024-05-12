#ifndef TRANSFORM_H
#define TRANSFORM_H

#include "HandmadeMath.h"

typedef struct transform {
    HMM_Vec3 pos;
    HMM_Vec3 scale;
    HMM_Quat rotation;
} transform;

transform *make_transform();
void transform_free(transform *t);

#define VEC2_FMT "[%g,%g]"
#define VEC2_MEMS(s) (s).x, (s).y

HMM_Vec3 trans_forward(const transform *const trans);
HMM_Vec3 trans_back(const transform *trans);
HMM_Vec3 trans_up(const transform *trans);
HMM_Vec3 trans_down(const transform *trans);
HMM_Vec3 trans_right(const transform *trans);
HMM_Vec3 trans_left(const transform *trans);

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

HMM_Mat4 transform2mat(transform t);
transform mat2transform(HMM_Mat4 m);

HMM_Quat angle2rotation(float angle);

#endif
