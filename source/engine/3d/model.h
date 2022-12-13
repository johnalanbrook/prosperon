#ifndef MODEL_H
#define MODEL_H

struct mesh;
struct shader;

struct model {
    struct mesh *meshes;
    struct mesh *mp;
    char *directory;
    const char *path;
    char *name;
};

/* Get the model at a path, or create and return if it doesn't exist */
struct model *GetExistingModel(const char *path);

/* Make a Model struct */
struct model *MakeModel(const char *path);

/* Load a model from memory into the GPU */
void loadmodel(struct model *model);

void draw_model(struct model *model, struct shader *shader);

#endif
