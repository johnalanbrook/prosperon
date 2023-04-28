#include "model.h"

#include "log.h"
#include "mesh.h"
#include "resources.h"
#include "shader.h"
#include "stb_ds.h"
#include <cgltf.h>
#include <stdlib.h>
#include <string.h>

static struct {
  char *key;
  struct Texture *value;
} *modelhash = NULL;

static void processnode();
static void processmesh();
static void processtexture();

struct model *GetExistingModel(const char *path) {
  if (!path || path[0] == '\0') return NULL;

  int index = shgeti(modelhash, path);
  if (index != -1) return modelhash[index].value;

  return MakeModel(path);
}

/* TODO: Make this a hash compare for speedup */
struct model *MakeModel(const char *path) {
  cgltf_options options = {0};
  cgltf_data *data = NULL;
  cgltf_result result = cgltf_parse_file(&options, path, &data);

  if (!result == cgltf_result_success) {
    YughError("Could not read file %s.", path);
    return NULL;
  }

  result = cgltf_load_buffers(&options, data, path);

  if (!result == cgltf_result_success) {
    YughError("Could not load buffers for file %s.", path);
    return NULL;
  }

  struct model *model = malloc(sizeof(struct model));

  model->meshes = malloc(sizeof(struct mesh) * data->meshes_count);

  for (int i = 0; i < data->nodes_count; i++) {
    
    if (data->nodes[i].mesh) {
      cgltf_mesh *mesh = data->nodes[i].mesh;
      
      for (int j = 0; j < mesh->primitives_count; j++) {
        cgltf_primitive primitive = mesh->primitives[j];
	
        for (int k = 0; k < primitive.attributes_count; k++) {
	  cgltf_attribute attribute = primitive.attributes[k];
	  
          switch (attribute.type) {
	  
          case cgltf_attribute_type_position:
//          float *vs = malloc(sizeof(float) * cgltf_accessor_unpack_floats(attribute.accessor, NULL, attribute.accessor.count);
//          cgltf_accessor_unpack_floats(attribute.accessor, vs, attribute.accessor.count);
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

/* TODO: DELETE
static void processnode() {

      for (uint32_t i = 0; i < node->mNumMeshes; i++) {
          aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
          *mp = processMesh(mesh, scene);
          mp++;
      }

      for (uint32_t i = 0; i < node->mNumChildren; i++) {
          processnode(node->mChildren[i], scene);
      }

}
*/
static void processmesh() {
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
static void processtexture() {
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

void draw_models(struct model *model, struct shader *shader)
{
  
}

// TODO: Come back to this; simple optimization
void draw_model(struct model *model, struct shader *shader) {

}
