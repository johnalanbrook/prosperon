#ifndef MESH_H
#define MESH_H

#include "sokol/sokol_gfx.h"
#include <stdint.h>

struct shader;
struct Texture;

struct mesh {
  sg_bindings bind;
  uint32_t face_count;
};

struct mesh *MakeMesh(struct Vertex *vertices, struct Vertex *ve, uint32_t *indices, uint32_t *ie, struct Texture *textures, struct Texture *te);
void setupmesh(struct mesh *mesh); /* Loads mesh into the GPU */
void DrawMesh(struct mesh *mesh, struct shader *shader);
void DrawMeshAgain(struct mesh *mesh); /* Draws whatever mesh was drawn last */

#endif
