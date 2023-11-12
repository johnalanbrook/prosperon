#include "model.h"

#include "log.h"
#include "resources.h"
#include "shader.h"
#include "stb_ds.h"
#include "font.h"
#include "window.h"
#include "gameobject.h"
#include "libgen.h"

//#include "diffuse.sglsl.h"
#include "unlit.sglsl.h"

#include "render.h"

// #define HANDMADE_MATH_USE_TURNS
#include "HandmadeMath.h"

#include "math.h"
#include "time.h"

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>
#include <stdlib.h>
#include <string.h>

#include "texture.h"

#include "sokol/sokol_gfx.h"

static struct {
  char *key;
  struct model *value;
} *modelhash = NULL;

static void processnode();
static void processmesh();
static void processtexture();

static sg_shader model_shader;
static sg_pipeline model_pipe;

void model_init() {
/*  model_shader = sg_make_shader(diffuse_shader_desc(sg_query_backend()));

  model_pipe = sg_make_pipeline(&(sg_pipeline_desc){
      .shader = model_shader,
      .layout = {
          .attrs = {
              [0].format = SG_VERTEXFORMAT_FLOAT3,
              [1].format = SG_VERTEXFORMAT_USHORT2N,
          },
      },
      .index_type = SG_INDEXTYPE_UINT16,
      .cull_mode = SG_CULLMODE_FRONT,
      .depth.write_enabled = true,
      .depth.compare = SG_COMPAREFUNC_LESS_EQUAL
  });
*/

  model_shader = sg_make_shader(unlit_shader_desc(sg_query_backend()));

  model_pipe = sg_make_pipeline(&(sg_pipeline_desc){
    .shader = model_shader,
    .layout = {
      .attrs = {
        [0].format = SG_VERTEXFORMAT_FLOAT3,
	[1].format = SG_VERTEXFORMAT_USHORT2N,
	[1].buffer_index = 1,
      },
    },
    .index_type = SG_INDEXTYPE_UINT16,
    .cull_mode = SG_CULLMODE_FRONT,
    .depth.write_enabled = true,
    .depth.compare = SG_COMPAREFUNC_LESS_EQUAL
  });
}

struct model *GetExistingModel(const char *path) {
  if (!path || path[0] == '\0') return NULL;

  int index = shgeti(modelhash, path);
  if (index != -1) return modelhash[index].value;

  return MakeModel(path);
}

cgltf_attribute *get_attr_type(cgltf_primitive p, cgltf_attribute_type t)
{
  for (int i = 0; i < p.attributes_count; i++) {
    if (p.attributes[i].type == t)
      return &p.attributes[i];
  }

  return NULL;
}

unsigned short pack_short_texcoord(float c)
{
  return c * USHRT_MAX;
}

uint32_t pack_int10_n2(float *norm)
{
  uint32_t ni[3];

  for (int i = 0; i < 3; i++) {
    ni[i] = fabs(norm[i]) * 511.0 + 0.5;
    ni[i] = (ni[i] > 511) ? 511 : ni[i];
    ni[i] = ( norm[i] < 0.0 ) ? -ni[i] : ni[i];
  }

  return (ni[0] & 0x3FF) | ( (ni[1] & 0x3FF) << 10) | ( (ni[2] & 0x3FF) << 20) | ( (0 & 0x3) << 30);
}

struct model *MakeModel(const char *path) {
  YughInfo("Making the model from %s.", path);
  cgltf_options options = {0};
  cgltf_data *data = NULL;
  cgltf_result result = cgltf_parse_file(&options, path, &data);

  if (result) {
    YughError("CGLTF could not parse file %s, err %d.", path, result);
    return NULL;
  }

  result = cgltf_load_buffers(&options, data, path);

  if (result) {
    YughError("CGLTF could not load buffers for file %s, err %d.", path, result);
    return NULL;
  }

  struct model *model = calloc(1, sizeof(*model));
  /* TODO: Optimize by grouping by material. One material per draw. */
  YughInfo("Model has %d materials.", data->materials_count);
  const char *dir = dirname(path);

  float vs[65535*3];
  uint16_t idxs[65535];

  for (int i = 0; i < data->meshes_count; i++) {
    cgltf_mesh *mesh = &data->meshes[i];
    struct mesh newmesh = {0};
    arrput(model->meshes,newmesh);

    YughInfo("Making mesh %d. It has %d primitives.", i, mesh->primitives_count);

    for (int j = 0; j < mesh->primitives_count; j++) {
      cgltf_primitive primitive = mesh->primitives[j];

      if (primitive.indices) {
        int c = primitive.indices->count;
        memcpy(idxs, cgltf_buffer_view_data(primitive.indices->buffer_view), sizeof(uint16_t) * c);

        model->meshes[j].bind.index_buffer = sg_make_buffer(&(sg_buffer_desc){
            .data.ptr = idxs,
            .data.size = sizeof(uint16_t) * c,
            .type = SG_BUFFERTYPE_INDEXBUFFER});

        model->meshes[j].face_count = c;
      } else {
        YughWarn("Model does not have indices. Generating them.");
        int c = primitive.attributes[0].data->count;
        model->meshes[j].face_count = c;
        for (int z = 0; z < c; z++)
          idxs[z] = z;

        model->meshes[j].bind.index_buffer = sg_make_buffer(&(sg_buffer_desc){
            .data.ptr = idxs,
            .data.size = sizeof(uint16_t) * c,
            .type = SG_BUFFERTYPE_INDEXBUFFER});
      }

      if (primitive.material->has_pbr_metallic_roughness) {// && primitive.material->pbr_metallic_roughness.base_color_texture.texture) {
        cgltf_image *img = primitive.material->pbr_metallic_roughness.base_color_texture.texture->image;
	if (img->buffer_view) {
	  cgltf_buffer_view *buf = img->buffer_view;
	  model->meshes[j].bind.fs.images[0] = texture_fromdata(buf->buffer->data, buf->size)->id;
	} else {
          const char *imp = seprint("%s/%s", dir, img->uri);
          model->meshes[j].bind.fs.images[0] = texture_pullfromfile(imp)->id;
	  free(imp);
  	}
      } else
        model->meshes[j].bind.fs.images[0] = texture_pullfromfile("k")->id;

      model->meshes[j].bind.fs.samplers[0] = sg_make_sampler(&(sg_sampler_desc){});

      cgltf_texture *tex;
//      if (tex = primitive.material->normal_texture.texture) {
//        model->meshes[j].bind.fs.images[1] = texture_pullfromfile(tex->image->uri)->id;
//      }// else
//        model->meshes[j].bind.fs.images[1] = texture_pullfromfile("k")->id;

      int has_norm = 0;

      for (int k = 0; k < primitive.attributes_count; k++) {
        cgltf_attribute attribute = primitive.attributes[k];

        int n = cgltf_accessor_unpack_floats(attribute.data, NULL, 0); /* floats per element x num elements */

        cgltf_accessor_unpack_floats(attribute.data, vs, n);

	uint32_t *packed_norms;
	unsigned short *packed_coords;

        switch (attribute.type) {
        case cgltf_attribute_type_position:

          model->meshes[j].bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
              .data.ptr = vs,
              .data.size = sizeof(float) * n});
          break;

        case cgltf_attribute_type_normal:
	  has_norm = 1;
	  packed_norms = malloc(model->meshes[j].face_count * sizeof(uint32_t));;
	  for (int i = 0; i < model->meshes[j].face_count; i++)
	    packed_norms[i] = pack_int10_n2(vs + i*3);

//          model->meshes[j].bind.vertex_buffers[2] = sg_make_buffer(&(sg_buffer_desc){
//              .data.ptr = packed_norms,
//              .data.size = sizeof(uint32_t) * model->meshes[j].face_count});

	  free (packed_norms);
          break;

        case cgltf_attribute_type_tangent:
          break;

        case cgltf_attribute_type_texcoord:
	  packed_coords = malloc(model->meshes[j].face_count * 2 * sizeof(unsigned short));
	  for (int i = 0; i < model->meshes[j].face_count*2; i++)
	    packed_coords[i] = pack_short_texcoord(vs[i]);

          model->meshes[j].bind.vertex_buffers[1] = sg_make_buffer(&(sg_buffer_desc){
              .data.ptr = packed_coords,
              .data.size = sizeof(unsigned short) * 2 * model->meshes[j].face_count});

	  free(packed_coords);
          break;
        }
      }

      if (!has_norm) {
      YughInfo("Model does not have normals. Generating them.");
      uint32_t norms[model->meshes[j].face_count];


      cgltf_attribute *pa = get_attr_type(primitive, cgltf_attribute_type_position);
      int n = cgltf_accessor_unpack_floats(pa->data, NULL,0);
      float ps[n];
      cgltf_accessor_unpack_floats(pa->data,ps,n);

      for (int i = 0, face=0; i < model->meshes[j].face_count/3; i++, face+=9) {
        int o = face;
        HMM_Vec3 a = {ps[o], ps[o+1],ps[o+2]};
	o += 3;
	HMM_Vec3 b = {ps[o], ps[o+1],ps[o+2]};
	o += 3;
	HMM_Vec3 c = {ps[o], ps[o+1],ps[o+2]};
	HMM_Vec3 norm = HMM_NormV3(HMM_Cross(HMM_SubV3(b,a), HMM_SubV3(c,a)));

	uint32_t packed_norm = pack_int10_n2(norm.Elements);
	for (int j = 0; j < 3; j++)
          norms[i*3+j] = packed_norm;
      }

//      model->meshes[j].bind.vertex_buffers[2] = sg_make_buffer(&(sg_buffer_desc){
//        .data.ptr = norms,
//        .data.size = sizeof(uint32_t) * model->meshes[j].face_count
//      });
      }
    }    
  }

  shput(modelhash, path, model);

  return model;
}

HMM_Vec3 eye = {50,10,5};

void draw_model(struct model *model, HMM_Mat4 amodel) {
  HMM_Mat4 proj = HMM_Perspective_RH_ZO(45, (float)mainwin.width / mainwin.height, 0.1, 10000);
  HMM_Vec3 center = {0.f, 0.f, 0.f};
  HMM_Vec3 up = {0.f, 1.f, 0.f};
  HMM_Mat4 view = HMM_LookAt_RH(eye, center, up);

  HMM_Mat4 vp = HMM_MulM4(proj, view);
  HMM_Mat4 mvp = HMM_MulM4(vp, amodel);

  HMM_Vec3 lp = {1, 1, 1};
  HMM_Vec3 dir_dir = HMM_NormV3(HMM_SubV3(center, dirl_pos));

  vs_p_t vs_p;
  memcpy(vs_p.vp, view.Elements, sizeof(float)*16);
  memcpy(vs_p.model, amodel.Elements, sizeof(float)*16);
  memcpy(vs_p.proj, proj.Elements, sizeof(float)*16);  

  sg_apply_pipeline(model_pipe);
  sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_p, SG_RANGE_REF(vs_p));

  for (int i = 0; i < arrlen(model->meshes); i++) {
    sg_apply_bindings(&model->meshes[i].bind);
    sg_draw(0, model->meshes[i].face_count, 1);    
  }
}

struct drawmodel *make_drawmodel(int go)
{
  struct drawmodel *dm = malloc(sizeof(struct drawmodel));
  dm->model = NULL;
  dm->amodel = HMM_M4D(1.f);
  dm->go = go;
  return dm;
}

void draw_drawmodel(struct drawmodel *dm)
{
  if (!dm->model) return;
  struct gameobject *go = id2go(dm->go);
  cpVect pos = cpBodyGetPosition(go->body);
  HMM_Mat4 scale = HMM_Scale(id2go(dm->go)->scale);
  HMM_Mat4 trans = HMM_M4D(1.f);
  trans.Elements[3][2] = -pos.x;
  trans.Elements[3][1] = pos.y;
  HMM_Mat4 rot = HMM_Rotate_RH(cpBodyGetAngle(go->body), vUP);
  /* model matrix = trans * rot * scale */
  
  draw_model(dm->model, HMM_MulM4(trans, HMM_MulM4(rot, scale)));
}

void free_drawmodel(struct drawmodel *dm)
{
  free(dm);
}

