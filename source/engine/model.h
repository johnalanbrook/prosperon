#ifndef MODEL_H
#define MODEL_H

#include "HandmadeMath.h"
#include "transform.h"
#include "sokol/sokol_gfx.h"
#include "gameobject.h"
#include "anim.h"
#include "texture.h"

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

typedef struct material {
  struct texture *diffuse;
  struct texture *metalrough;
  float metal;
  float rough;
  struct texture *normal;
  float nrm;
  struct texture *occlusion;
  float occl;
  struct texture *emissive;
  HMM_Vec3 emis;
} material;

struct model;

typedef struct primitive {
  sg_buffer pos;
  sg_buffer norm;
  sg_buffer uv;
  sg_buffer bone;
  sg_buffer weight;
  sg_buffer color;
  sg_buffer index;
  material *mat;
  uint32_t idx_count;
} primitive;

/* A single mesh */
typedef struct mesh {
  primitive *primitives;
  HMM_Mat4 *m;
} mesh;

typedef struct joint {
  int me;
  struct joint *children;
} joint_t;

typedef struct md5joint {
  struct md5joint *parent;
  HMM_Vec4 pos;
  HMM_Quat rot;
  HMM_Vec4 scale;
  HMM_Mat4 t;
} md5joint;

typedef struct skin {
  md5joint **joints;
  HMM_Mat4 *invbind;
  HMM_Mat4 binds[50]; /* binds = joint * invbind */
  md5joint *root;
} skin;

/* A collection of meshes which create a full figure */
typedef struct model {
  struct mesh *meshes;
  md5joint *nodes;
  material *mats;
  skin skin;
  struct animation anim;
} model;

/* Make a Model struct */
struct model *model_make(const char *path);
void model_free(model *m);

void model_draw_go(model *m, transform *go);
sg_bindings primitive_bindings(primitive *p, JSValue pipe);
void primitive_gen_indices(primitive *prim);
int mat2type(int mat);

sg_buffer float_buffer(float *f, int v);
sg_buffer index_buffer(float *f, int verts);
sg_buffer texcoord_floats(float *f, int n);
sg_buffer par_idx_buffer(uint32_t *i, int v);
sg_buffer normal_floats(float *f, int n);
sg_buffer ubyten_buffer(float *f, int v);
sg_buffer ubyte_buffer(float *f, int v);
sg_buffer joint_buf(float *f, int v);
sg_buffer weight_buf(float *f, int v);
void primitive_free(primitive *prim);

material *material_make();
void material_free(material *mat);

#endif
