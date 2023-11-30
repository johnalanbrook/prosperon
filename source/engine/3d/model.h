#ifndef MODEL_H
#define MODEL_H

#include "HandmadeMath.h"
#include "transform.h"
#include "sokol/sokol_gfx.h"

extern HMM_Vec3 eye;
struct shader;

/* A single mesh */
struct mesh {
  sg_bindings bind;
  uint32_t face_count;
};

/* A collection of meshes which create a full figure */
struct model {
  struct mesh *meshes;
};

/* A model with draw information */ 
struct drawmodel {
  struct model *model;
  HMM_Mat4 amodel;
  int go;
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

void draw_model(struct model *model, HMM_Mat4 amodel);
void draw_models(struct model *model, struct shader *shader);

struct drawmodel *make_drawmodel(int go);
void draw_drawmodel(struct drawmodel *dm);
void free_drawmodel(struct drawmodel *dm);

#endif
