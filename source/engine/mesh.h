#ifndef MESH_H
#define MESH_H

#include "mathc.h"
#include <stdint.h>

struct shader;
struct Texture;

#define MAX_BONE_INFLUENCE 4

struct Vertex {
    mfloat_t Position[3];
    mfloat_t Normal[3];
    mfloat_t TexCoords[2];
    mfloat_t Tangent[3];
    mfloat_t Bitangent[3];

    int m_BoneIDs[MAX_BONE_INFLUENCE];

    float m_Weights[MAX_BONE_INFLUENCE];
};

struct mesh {
    struct Vertex *vertices;
    struct Vertex *ve;
    uint32_t *indices;
    uint32_t *ie;
    struct Texture *textures;
    struct Texture *te;
    uint32_t VAO, VBO, EBO;
};

struct mesh *MakeMesh(struct Vertex *vertices, struct Vertex *ve,
		       uint32_t * indices, uint32_t * ie,
		       struct Texture *textures, struct Texture *te);
void setupmesh(struct mesh *mesh);	/* Loads mesh into the GPU */
void DrawMesh(struct mesh *mesh, struct shader *shader);
void DrawMeshAgain(struct mesh *mesh);	/* Draws whatever mesh was drawn last */

#endif
