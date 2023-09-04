#ifndef MODEL_H
#define MODEL_H

#include "HandmadeMath.h"

#include "sokol/sokol_gfx.h"

extern HMM_Vec3 eye;
struct shader;

struct mesh {
  sg_bindings bind;
  uint32_t face_count;
};

struct model {
  struct mesh *meshes;
};

/* Get the model at a path, or create and return if it doesn't exist */
struct model *GetExistingModel(const char *path);

/* Make a Model struct */
struct model *MakeModel(const char *path);

/* Load a model from memory into the GPU */
void loadmodel(struct model *model);

void model_init();

void draw_model(struct model *model, HMM_Mat4 amodel, HMM_Mat4 lsm);
void draw_models(struct model *model, struct shader *shader);

#endif
