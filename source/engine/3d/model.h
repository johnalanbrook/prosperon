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

typedef struct material {
  texture *diffuse;
  texture *metalrough;
  float metal;
  float rough;
  texture *normal;
  float nrm;
  texture *occlusion;
  float occl;
  texture *emissive;
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
  sg_buffer idx;
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

void model_draw_go(model *m, gameobject *go, gameobject *cam);

material *material_make();
void material_free(material *mat);

#endif
