#ifndef MODEL_H
#define MODEL_H

struct mMesh;
struct mShader;

struct mModel {
    struct mMesh *meshes;
    struct mMesh *mp;
    char *directory;
    const char *path;
    char *name;
};

/* Get the model at a path, or create and return if it doesn't exist */
struct mModel *GetExistingModel(const char *path);

/* Make a Model struct */
struct mModel *MakeModel(const char *path);

/* Load a model from memory into the GPU */
void loadmodel(struct mModel *model);

void draw_model(struct mModel *model, struct mShader *shader);

#endif
