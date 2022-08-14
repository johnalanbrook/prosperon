#include "mesh.h"

#include "render.h"
#include "shader.h"
#include "texture.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void DrawMesh(struct mMesh *mesh, struct mShader *shader)
{
    // bind appropriate textures
    uint32_t diffuseNr = 1;
    uint32_t specularNr = 1;
    uint32_t normalNr = 1;
    uint32_t heightNr = 1;

    for (int i = 0; i < (mesh->te - mesh->textures); i++) {
	glActiveTexture(GL_TEXTURE0 + i);	// active proper texture unit before binding
	// retrieve texture number (the N in diffuse_textureN)
	char number = 0;
	// TODO: malloc every single frame ... nope! Change to stack
	/*char *name =
	    (char *) malloc(sizeof(char) *
			    (strlen(mesh->textures[i].type) + 2));*/
	if (mesh->textures[i].type == TEX_DIFF)
	    number = diffuseNr++;
	else if (mesh->textures[i].type == TEX_SPEC)
	    number = specularNr++;
	else if (mesh->textures[i].type == TEX_NORM)
	    number = normalNr++;
	else if (mesh->textures[i].type == TEX_HEIGHT)
	    number = heightNr++;


/*
	glUniform1i(glGetUniformLocation(shader->id, name), i);
	glBindTexture(GL_TEXTURE_2D, mesh->textures[i].id);
	free(name);
	*/
    }

    // draw mesh
    glBindVertexArray(mesh->VAO);
    DrawMeshAgain(mesh);

    // DEBUG
    //    triCount += indices.size() / 3;
}


void DrawMeshAgain(struct mMesh *mesh)
{
    glDrawElements(GL_TRIANGLES, (mesh->ie - mesh->indices),
		   GL_UNSIGNED_INT, 0);
}

struct mMesh *MakeMesh(struct Vertex *vertices, struct Vertex *ve,
		       uint32_t * indices, uint32_t * ie,
		       struct Texture *textures, struct Texture *te)
{
    struct mMesh *newmesh = (struct mMesh *) malloc(sizeof(struct mMesh));
    newmesh->vertices = vertices;
    newmesh->ve = ve;
    newmesh->indices = indices;
    newmesh->ie = ie;
    newmesh->textures = textures;
    newmesh->te = te;

    setupmesh(newmesh);
    return newmesh;
}

void setupmesh(struct mMesh *mesh)
{
    // create buffers/arrays
    glGenVertexArrays(1, &mesh->VAO);
    glGenBuffers(1, &mesh->VBO);
    glGenBuffers(1, &mesh->EBO);

    glBindVertexArray(mesh->VAO);
    // load data into vertex buffers
    glBindBuffer(GL_ARRAY_BUFFER, mesh->VBO);


    // The effect is that we can simply pass a pointer to the struct and it translates perfectly to vevc array which
    // again translates to 3/2 floats which translates to a byte array.
    glBufferData(GL_ARRAY_BUFFER,
		 (mesh->ve - mesh->vertices) * sizeof(struct Vertex),
		 &mesh->vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		 (mesh->ie - mesh->indices) * sizeof(uint32_t),
		 &mesh->indices[0], GL_STATIC_DRAW);

    // set the vertex attribute pointers
    // vertex Positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(struct Vertex),
			  (void *) 0);
    // vertex normals
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(struct Vertex),
			  (void *) offsetof(struct Vertex, Normal[3]));
    // vertex texture coords
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(struct Vertex),
			  (void *) offsetof(struct Vertex, TexCoords[2]));
    // vertex tangent
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(struct Vertex),
			  (void *) offsetof(struct Vertex, Tangent[3]));
    // vertex bitangent
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(struct Vertex),
			  (void *) offsetof(struct Vertex, Bitangent[3]));

    // Bone ids
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 4, GL_INT, GL_FALSE, sizeof(struct Vertex),
			  (void *) offsetof(struct Vertex,
					    m_BoneIDs
					    [MAX_BONE_INFLUENCE]));

    // Weights
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(struct Vertex),
			  (void *) offsetof(struct Vertex, m_Weights));

    glBindVertexArray(0);
}
