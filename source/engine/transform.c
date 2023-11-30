#include "transform.h"
#include <string.h>

const transform2d t2d_unit = {
  .pos = {0,0},
  .scale = {1,1},
  .angle = 0
};

HMM_Vec3 trans_forward(const transform3d *const trans)
{
  return HMM_QVRot(vFWD, trans->rotation);
}

HMM_Vec3 trans_back(const transform3d *trans)
{
  return HMM_QVRot(vBKWD, trans->rotation);
}

HMM_Vec3 trans_up(const transform3d *trans)
{
  return HMM_QVRot(vUP, trans->rotation);
}

HMM_Vec3 trans_down(const transform3d *trans)
{
  return HMM_QVRot(vDOWN, trans->rotation);
}

HMM_Vec3 trans_right(const transform3d *trans)
{
  return HMM_QVRot(vRIGHT, trans->rotation);
}

HMM_Vec3 trans_left(const transform3d *trans)
{
  return HMM_QVRot(vLEFT, trans->rotation);
}

HMM_Vec2 mat_t_pos(HMM_Mat3 m, HMM_Vec2 pos)
{
  return HMM_MulM3V3(m, (HMM_Vec3){pos.x, pos.y, 0}).XY;
}

HMM_Vec2 mat_t_dir(HMM_Mat3 m, HMM_Vec2 dir)
{
  m.Columns[2] = (HMM_Vec3){0,0,1};
  return HMM_MulM3V3(m, (HMM_Vec3){dir.x, dir.y, 1}).XY;
}

HMM_Vec3 mat3_t_pos(HMM_Mat4 m, HMM_Vec3 pos)
{
  return HMM_MulM4V4(m, (HMM_Vec4){pos.X, pos.Y, pos.Z, 1}).XYZ;
}

HMM_Vec3 mat3_t_dir(HMM_Mat4 m, HMM_Vec3 dir)
{
  m.Columns[4] = (HMM_Vec4){0,0,0,1};
  return mat3_t_pos(m, dir);
}

HMM_Mat3 transform2d2mat(transform2d t) {
  HMM_Mat2 m = HMM_MulM2(HMM_RotateM2(t.angle), HMM_ScaleM2(t.scale));
  return HMM_M2BasisPos(m, t.pos);
}
  
HMM_Mat4 transform3d2mat(transform3d t) { return HMM_MulM4(HMM_Translate(t.pos), HMM_MulM4(HMM_QToM4(t.rotation), HMM_Scale(t.scale))); }
