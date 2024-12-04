#ifndef MODEL_H
#define MODEL_H

#include "HandmadeMath.h"
#include "transform.h"
#include "gameobject.h"
#include "anim.h"
#include "cgltf.h"

#define MAT_POS 0
#define MAT_UV 1
#define MAT_NORM 2
#define MAT_BONE 3
#define MAT_WEIGHT 4
#define MAT_COLOR 5
#define MAT_TAN 6
#define MAT_ANGLE 7
#define MAT_WH 8
#define MAT_ST 9
#define MAT_PPOS 10
#define MAT_SCALE 11
#define MAT_INDEX 100

typedef struct md5joint {
  struct md5joint *parent;
  HMM_Vec4 pos;
  HMM_Quat rot;
  HMM_Vec4 scale;
  HMM_Mat4 t;
} md5joint;

typedef struct skin {
  md5joint *joints;
  HMM_Mat4 *invbind;
  HMM_Mat4 binds[50]; /* binds = joint * invbind */
  animation *anim;
} skin;

//sg_buffer accessor2buffer(cgltf_accessor *a, int type);
skin *make_gltf_skin(cgltf_skin *skin, cgltf_data *data);
void skin_calculate(skin *sk);

/*sg_buffer float_buffer(float *f, int v);
sg_buffer index_buffer(float *f, int verts);
sg_buffer texcoord_floats(float *f, int n);
sg_buffer par_idx_buffer(uint32_t *i, int v);
sg_buffer normal_floats(float *f, int n);
sg_buffer ubyten_buffer(float *f, int v);
sg_buffer ubyte_buffer(float *f, int v);
sg_buffer joint_buf(float *f, int v);
sg_buffer weight_buf(float *f, int v);
*/
#endif
