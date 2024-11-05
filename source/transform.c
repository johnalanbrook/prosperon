#include "transform.h"
#include <string.h>
#include <stdio.h>

transform *make_transform()
{
  transform *t = calloc(sizeof(transform),1);
    
  t->scale = (HMM_Vec3){1,1,1};
  t->rotation = (HMM_Quat){0,0,0,1};
  t->dirty = 1;
  return t;
}

void transform_free(transform *t) { free(t); }

void transform_apply(transform *t) { t->dirty = 1; }

void transform_move(transform *t, HMM_Vec3 v)
{
  t->pos = HMM_AddV3(t->pos, v);
  t->dirty = 1;
}

HMM_Vec3 transform_direction(transform *t, HMM_Vec3 dir)
{
  return HMM_QVRot(dir, t->rotation);
}

HMM_Vec3 trans_forward(const transform *const trans) { return HMM_QVRot(vFWD, trans->rotation); }
HMM_Vec3 trans_back(const transform *trans) { return HMM_QVRot(vBKWD, trans->rotation); }
HMM_Vec3 trans_up(const transform *trans) { return HMM_QVRot(vUP, trans->rotation); }
HMM_Vec3 trans_down(const transform *trans) { return HMM_QVRot(vDOWN, trans->rotation); }
HMM_Vec3 trans_right(const transform *trans) { return HMM_QVRot(vRIGHT, trans->rotation); }
HMM_Vec3 trans_left(const transform *trans) { return HMM_QVRot(vLEFT, trans->rotation); }

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

HMM_Mat4 transform2mat(transform *t) {
  return HMM_M4TRS(t->pos, t->rotation, t->scale);
  HMM_Mat4 scale = HMM_Scale(t->scale);
  HMM_Mat4 rot = HMM_QToM4(t->rotation);
  HMM_Mat4 pos = HMM_Translate(t->pos);
  return HMM_MulM4(pos, HMM_MulM4(rot, scale));

  
  if (t->dirty) {
    t->cache = HMM_M4TRS(t->pos, t->rotation, t->scale);
    t->dirty = 0;
  }
  
  return t->cache;
}

HMM_Quat angle2rotation(float angle)
{
  return HMM_QFromAxisAngle_RH(vBKWD, angle);
}

transform mat2transform(HMM_Mat4 m)
{
  transform t;
  t.pos = m.Columns[3].xyz;
  for (int i = 0; i < 2; i++)
    t.scale.Elements[i] = HMM_LenV3(m.Columns[i].xyz);
//  for (int i = 0; i < 2; i++)
//    m.Columns[i].xyz = HMM_MulV3(m.Columns[i].xyz, t.scale.Elements[i]);
  t.rotation = HMM_M4ToQ_RH(m);
  return t;
}
