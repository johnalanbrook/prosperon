#include "model.h"

#include "mesh.h"
#include "resources.h"
#include "shader.h"
#include <cgltf.h>
#include <string.h>

static struct mModel *lastRendered;
static struct mModel *loadedModels[100];
static struct mModel **lastModel = loadedModels;

static void processnode();
static void processmesh();
static void processtexture();


struct mModel *GetExistingModel(const char *path)
{
    struct mModel **model = loadedModels;

    while (model++ != lastModel) {
	if (!strcmp(path, (*model)->path))
	    goto end;

	return MakeModel(path);

    }

  end:
    return NULL;
}

/* TODO: Make this a hash compare for speedup */
struct mModel *MakeModel(const char *path)
{
    char *modelPath =
	(char *) malloc(sizeof(char) *
			(strlen(DATA_PATH) + strlen(path) + 1));
    modelPath[0] = '\0';
    strcat(modelPath, DATA_PATH);
    strcat(modelPath, path);
    printf
	("Created new model with modelPath %s, from data_path %s and path %s\n",
	 modelPath, DATA_PATH, path);

    struct mModel *newmodel =
	(struct mModel *) malloc(sizeof(struct mModel));
    newmodel->path = path;

    loadmodel(newmodel);
    *lastModel++ = newmodel;
    return newmodel;
}

// TODO: Come back to this; simple optimization
void draw_model(struct mModel *model, struct mShader *shader)
{
    if (lastRendered != model) {
	lastRendered = model;
	for (uint32_t i = 0; i < (model->mp - model->meshes); i++)
	    DrawMesh(&model->meshes[i], shader);
    } else {
	for (uint32_t i = 0; i < (model->mp - model->meshes); i++)
	    DrawMeshAgain(&model->meshes[i]);
    }
}

void loadmodel(struct mModel *model)
{
    printf("Loading model at path %s\n", model->path);
/*
    // Load model with cgltf
    cgltf_options options = {0};
    cgltf_data *data = NULL;
    cgltf_result result = cgltf_parse_file(&options, model->path, &data);

    meshes = (struct mMesh*)malloc(sizeof(Mesh)*cgltf_data->meshes_count);

    directory = get_directory_from_path(model->path);

    for (int i = 0; i < data->nodes_count; i++) {
        if (data->nodes[i]->mesh) {
            for (int j = 0; j < data->nodes[i]->mesh->primatives_count; j++) {


                for (int k = 0; k < data->nodes[i]->mesh->primatives[j]->attributes_count; k++) {
                    switch(data->nodes[i]->mesh->primatives[j]->attributes[k]->type) {
                        case cgltf_attribute_type_position:
                            Vertex *vs = (Vertex*)malloc(sizeof(Vertex) * cgltf_accessor_unpack_floats(:::attributes[k]->accesor, NULL, attributes[k]->accessor.count);
                            cgltf_accessor_unpack_floats(:::attributes[k]->accessor, vs, attributes[k]->accessor.count);
                            break;

                        case cgltf_attribute_type_normal:

                            break;

                        case cgltf_attribute_type_tangent:
                            break;

                        case cgltf_attribute_type_texcoord:
                            break;

                        }
                    }
                }
            }
        }
    }
    */

    /* TODO: DELETE



       // read file via ASSIMP
       Assimp::Importer importer;
       const aiScene *scene = importer.ReadFile(path,
       aiProcess_Triangulate |
       aiProcess_GenSmoothNormals |
       aiProcess_FlipUVs |
       aiProcess_CalcTangentSpace);
       // check for errors
       if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE
       || !scene->mRootNode) {
       YughLog(0, SDL_LOG_PRIORITY_ERROR,
       "ASSIMP error: %s",
       importer.GetErrorString());
       return;
       }

       directory = get_directory_from_path(path);

       meshes = (Mesh *) malloc(sizeof(Mesh) * 100);
       mp = meshes;
       // process ASSIMP's root node recursively
       processNode(scene->mRootNode, scene); */
}


static void processnode()
{
/*
    for (uint32_t i = 0; i < node->mNumMeshes; i++) {
	aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
	*mp = processMesh(mesh, scene);
	mp++;
    }

    for (uint32_t i = 0; i < node->mNumChildren; i++) {
	processnode(node->mChildren[i], scene);
    }
*/
}

static void processmesh()
{
/*
    Vertex *vertices =
	(Vertex *) malloc(sizeof(Vertex) * mesh->mNumVertices);
    Vertex *vp = vertices + mesh->mNumVertices;
    Vertex *p = vertices;
    for (int i = 0; i < mesh->mNumVertices; i++) {
	// positions
	(p + i)->Position.x = mesh->mVertices[i][0];
	(p + i)->Position.y = mesh->mVertices[i][1];
	(p + i)->Position.z = mesh->mVertices[i][2];


	// normals
	if (mesh->HasNormals()) {
	    (p + i)->Normal.x = mesh->mNormals[i][0];
	    (p + i)->Normal.y = mesh->mNormals[i].y;
	    (p + i)->Normal.z = mesh->mNormals[i].z;
	}

	// texture coordinates
	if (mesh->mTextureCoords[0]) {
	    glm::vec2 vec;
	    // a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't
	    // use models where a vertex can have multiple texture coordinates so we always take the first set (0).
	    (p + i)->TexCoords.x = mesh->mTextureCoords[0][i].x;
	    (p + i)->TexCoords.y = mesh->mTextureCoords[0][i].y;

	    // tangent
	    (p + i)->Tangent.x = mesh->mTangents[i].x;
	    (p + i)->Tangent.y = mesh->mTangents[i].y;
	    (p + i)->Tangent.z = mesh->mTangents[i].z;

	    // bitangent
	    (p + i)->Bitangent.x = mesh->mBitangents[i].x;
	    (p + i)->Bitangent.y = mesh->mBitangents[i].y;
	    (p + i)->Bitangent.z = mesh->mBitangents[i].z;

	} else
	    (p + i)->TexCoords = glm::vec2(0.0f, 0.0f);
    }



    // TODO: Done quickly, find better way. Go through for loop twice!
    int numindices = 0;
    // now walk through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
    for (uint32_t i = 0; i < mesh->mNumFaces; i++) {
	numindices += mesh->mFaces[i].mNumIndices;
    }

    uint32_t *indices = (uint32_t *) malloc(sizeof(uint32_t) * numindices);
    uint32_t *ip = indices;

    for (uint32_t i = 0; i < mesh->mNumFaces; i++) {
	for (uint32_t j = 0; j < mesh->mFaces[i].mNumIndices; j++) {
	    *ip = mesh->mFaces[i].mIndices[j];
	    ip++;
	}
    }


    //  std::vector<Texture> textures;
    aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];

    // TODO: Allocating 100 to be safe, can probably be way less
    textures_loaded = (Texture *) malloc(sizeof(Texture) * 100);
    tp = textures_loaded;
    // we assume a convention for sampler names in the shaders. Each diffuse texture should be named
    // as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER.
    // Same applies to other texture as the following list summarizes:
    // diffuse: texture_diffuseN
    // specular: texture_specularN
    // normal: texture_normalN

    // 1. diffuse maps
    loadMaterialTextures(material, aiTextureType_DIFFUSE,
			 "texture_diffuse");

    // 2. specular maps
    loadMaterialTextures(material, aiTextureType_SPECULAR,
			 "texture_specular");

    // 3. normal maps
    loadMaterialTextures(material, aiTextureType_NORMALS,
			 "texture_normal");

    // 4. height maps
    loadMaterialTextures(material, aiTextureType_AMBIENT,
			 "texture_height");


    // return a mesh object created from the extracted mesh data
    return Mesh(vertices, vp, indices, ip, textures_loaded, tp);

    */

}

// TODO:  This routine mallocs inside the function
static void processtexture()
{
/*
    for (uint32_t i = 0; i < mat->GetTextureCount(type); i++) {
	aiString str;
	mat->GetTexture(type, i, &str);
	for (Texture * tpp = textures_loaded; tpp != tp; tpp++) {
	    if (strcmp(tpp->path, str.data) == 0)
		goto next;	// Check if we already have this texture
	}

	tp->id = TextureFromFile(str.data, this->directory);
	tp->type = (char *) malloc(sizeof(char) * strlen(typeName));
	strcpy(tp->type, typeName);
	tp->path = (char *) malloc(sizeof(char) * strlen(str.data));
	strcpy(tp->path, str.data);

	tp++;

      next:;

    }
    */
}
