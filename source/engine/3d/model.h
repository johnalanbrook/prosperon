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
  struct model *model;
} mesh;

/* A collection of meshes which create a full figure */
typedef struct model {
  struct mesh *meshes;
  const char *path;
  HMM_Mat4 matrix;
} model;

/* A model with draw information */ 
struct drawmodel {
  struct model *model;
  HMM_Mat4 amodel;
  gameobject *go;
};

typedef struct bone {
  transform3d t;
  struct bone *children;
} bone;

/* Get the model at a path, or create and return if it doesn't exist */
struct model *GetExistingModel(const char *path);

/* Make a Model struct */
struct model *MakeModel(const char *path);

/* Load a model from memory into the GPU */
void loadmodel(struct model *model);

void model_init();

struct drawmodel *make_drawmodel(gameobject *go);
void draw_drawmodel(struct drawmodel *dm);
void model_draw_all();
void free_drawmodel(struct drawmodel *dm);

#endif
