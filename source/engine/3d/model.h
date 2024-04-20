#ifndef MODEL_H
#define MODEL_H

#include "HandmadeMath.h"
#include "transform.h"
#include "sokol/sokol_gfx.h"
#include "gameobject.h"

extern HMM_Vec3 eye;

typedef struct material {

} material;

struct model;

/* A single mesh */
typedef struct mesh {
  sg_bindings bind; /* Encapsulates material, norms, etc */
  uint32_t idx_count;
} mesh;

/* A collection of meshes which create a full figure */
typedef struct model {
  struct mesh *meshes;
  HMM_Mat4 matrix;
} model;

typedef struct bone {
  transform3d t;
  struct bone *children;
} bone;

/* Make a Model struct */
struct model *model_make(const char *path);
void model_free(model *m);

void model_draw_go(model *m, gameobject *go, gameobject *cam);

void model_init();

material *material_make();
void material_free(material *mat);

#endif
