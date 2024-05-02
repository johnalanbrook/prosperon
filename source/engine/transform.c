#include "transform.h"
#include <string.h>
#include <stdio.h>

const transform2d t2d_unit = {
  .pos = {0,0},
  .scale = {1,1},
  .angle = 0
};

transform3d *make_transform3d()
{
  transform3d *t = calloc(sizeof(transform3d),1);
  return t;
}

void transform3d_free(transform3d *t) { free(t); }

transform2d *make_transform2d()
{
  transform2d *t = calloc(sizeof(transform2d),1);
  t->scale = (HMM_Vec2){1,1};
  return t;
}

void transform2d_free(transform2d *t) { free(t); }

HMM_Vec3 trans_forward(const transform3d *const trans) { return HMM_QVRot(vFWD, trans->rotation); }
HMM_Vec3 trans_back(const transform3d *trans) { return HMM_QVRot(vBKWD, trans->rotation); }
HMM_Vec3 trans_up(const transform3d *trans) { return HMM_QVRot(vUP, trans->rotation); }
HMM_Vec3 trans_down(const transform3d *trans) { return HMM_QVRot(vDOWN, trans->rotation); }
HMM_Vec3 trans_right(const transform3d *trans) { return HMM_QVRot(vRIGHT, trans->rotation); }
HMM_Vec3 trans_left(const transform3d *trans) { return HMM_QVRot(vLEFT, trans->rotation); }

HMM_Vec2 mat_t_pos(HMM_Mat3 m, HMM_Vec2 pos) { return HMM_MulM3V3(m, (HMM_Vec3){pos.x, pos.y, 1}).xy; }

HMM_Vec2 mat_t_dir(HMM_Mat3 m, HMM_Vec2 dir)
{
  m.Columns[2] = (HMM_Vec3){0,0,1};
  return HMM_MulM3V3(m, (HMM_Vec3){dir.x, dir.y, 1}).XY;
}

HMM_Vec2 mat_up(HMM_Mat3 m) { return HMM_NormV2(m.Columns[1].XY); }
HMM_Vec2 mat_right(HMM_Mat3 m) { return HMM_NormV2(m.Columns[0].XY); }
float vec_angle(HMM_Vec2 a, HMM_Vec2 b) { return acos(HMM_DotV2(a,b)/(HMM_LenV2(a)*HMM_LenV2(b))); }
float vec_dirangle(HMM_Vec2 a, HMM_Vec2 b) { return atan2(b.x, b.y) - atan2(a.x, a.y); }


HMM_Vec3 mat3_t_pos(HMM_Mat4 m, HMM_Vec3 pos) { return HMM_MulM4V4(m, (HMM_Vec4){pos.X, pos.Y, pos.Z, 1}).XYZ; }

HMM_Vec3 mat3_t_dir(HMM_Mat4 m, HMM_Vec3 dir)
{
  m.Columns[3] = (HMM_Vec4){0,0,0,1};
  return mat3_t_pos(m, dir);
}

HMM_Mat3 transform2d2mat(transform2d trn) {
  return HMM_MulM3(HMM_Translate2D(trn.pos), HMM_MulM3(HMM_RotateM3(trn.angle), HMM_ScaleM3(trn.scale)));
}

HMM_Mat4 transform2d2mat4(transform2d *t)
{
  HMM_Mat4 T = {0};
  float c = cosf(t->angle);
  float s = sinf(t->angle);
  T.col[0].x = c*t->scale.x;
  T.col[0].y = s*t->scale.y;
  T.col[1].x = -s*t->scale.x;
  T.col[1].y = c*t->scale.y;
  T.col[3].xy = t->pos;
  T.col[2].z = 1;
  T.col[3].w = 1;
  return T;
}

transform2d mat2transform2d(HMM_Mat3 m)
{
  transform2d t;
  t.pos = m.Columns[2].xy;
  t.scale = (HMM_Vec2){HMM_LenV2(m.Columns[0].xy), HMM_LenV2(m.Columns[1].xy)};
  t.angle = acos(m.Columns[0].x/t.scale.x);
  return t;
}
  
HMM_Mat4 transform3d2mat(transform3d t) {
  return HMM_MulM4(HMM_Translate(t.pos),
          HMM_MulM4(HMM_QToM4(t.rotation),
          HMM_Scale(t.scale)));
}

transform3d mat2transform3d(HMM_Mat4 m)
{
  transform3d t;
  t.pos = m.Columns[3].xyz;
  for (int i = 0; i < 2; i++)
    t.scale.Elements[i] = HMM_LenV3(m.Columns[i].xyz);
//  for (int i = 0; i < 2; i++)
//    m.Columns[i].xyz = HMM_MulV3(m.Columns[i].xyz, t.scale.Elements[i]);
  t.rotation = HMM_M4ToQ_RH(m);
  return t;
}
