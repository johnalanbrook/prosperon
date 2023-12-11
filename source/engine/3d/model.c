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

struct bone_weights {
  char b1;
  char b2;
  char b3;
  char b4;
};

struct mesh_v {
  HMM_Vec3 pos;
  struct uv_n uv;
  uint32_t norm;
  struct bone_weights bones;
};

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

cgltf_attribute *get_attr_type(cgltf_primitive *p, cgltf_attribute_type t)
{
  for (int i = 0; i < p->attributes_count; i++) {
    if (p->attributes[i].type == t)
      return &p->attributes[i];
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

void mesh_add_material(mesh *mesh, cgltf_material *mat)
{
  if (!mat) return;
  
  if (mat && mat->has_pbr_metallic_roughness) {
    cgltf_image *img = mat->pbr_metallic_roughness.base_color_texture.texture->image;
     if (img->buffer_view) {
       cgltf_buffer_view *buf = img->buffer_view;
       mesh->bind.fs.images[0] = texture_fromdata(buf->buffer->data, buf->size)->id;
     } else {
       const char *imp = seprint("%s/%s", dirname(mesh->model->path), img->uri);
       mesh->bind.fs.images[0] = texture_pullfromfile(imp)->id;
       free(imp);
     }
   } else
     // Get "no texture" tex
     mesh->bind.fs.images[0] = texture_pullfromfile("k")->id;
     mesh->bind.fs.samplers[0] = sg_make_sampler(&(sg_sampler_desc){});
     
     cgltf_texture *tex;
     if (tex = mat->normal_texture.texture)
       mesh->bind.fs.images[1] = texture_pullfromfile(tex->image->uri)->id;
     else
       mesh->bind.fs.images[1] = texture_pullfromfile("k")->id;
}

void mesh_add_primitive(mesh *mesh, cgltf_primitive *prim)
{
  uint16_t *idxs;
  if (prim->indices) {
    int c = prim->indices->count;
    idxs = malloc(sizeof(*idxs)*c);
    memcpy(idxs, cgltf_buffer_view_data(prim->indices->buffer_view), sizeof(uint16_t) * c);

    mesh->bind.index_buffer = sg_make_buffer(&(sg_buffer_desc){
	.data.ptr = idxs,
	.data.size = sizeof(uint16_t) * c,
	.type = SG_BUFFERTYPE_INDEXBUFFER});

    mesh->face_count = c;
  } else {
    YughWarn("Model does not have indices. Generating them.");
    int c = prim->attributes[0].data->count;
    mesh->face_count = c;
    idxs = malloc(sizeof(*idxs)*c);
    
    for (int z = 0; z < c; z++)
      idxs[z] = z;

    mesh->bind.index_buffer = sg_make_buffer(&(sg_buffer_desc){
	.data.ptr = idxs,
	.data.size = sizeof(uint16_t) * c,
	.type = SG_BUFFERTYPE_INDEXBUFFER});
  }
  free(idxs);

  mesh_add_material(mesh, prim->material);
  int has_norm = 0;  

  for (int k = 0; k < prim->attributes_count; k++) {
    cgltf_attribute attribute = prim->attributes[k];

    int n = cgltf_accessor_unpack_floats(attribute.data, NULL, 0); /* floats per element x num elements */
    float *vs = malloc(sizeof(float)*n);
    cgltf_accessor_unpack_floats(attribute.data, vs, n);

    uint32_t *packed_norms;
    unsigned short *packed_coords;


    switch (attribute.type) {
      case cgltf_attribute_type_position:

      mesh->bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
	  .data.ptr = vs,
	  .data.size = sizeof(float) * n});
      break;

    case cgltf_attribute_type_normal:
      has_norm = 1;
      packed_norms = malloc(mesh->face_count * sizeof(uint32_t));
      for (int i = 0; i < mesh->face_count; i++)
	packed_norms[i] = pack_int10_n2(vs + i*3);

        mesh->bind.vertex_buffers[2] = sg_make_buffer(&(sg_buffer_desc){
          .data.ptr = packed_norms,
          .data.size = sizeof(uint32_t) * mesh->face_count});

      free (packed_norms);
      break;

    case cgltf_attribute_type_tangent:
      break;

    case cgltf_attribute_type_color:
      break;

    case cgltf_attribute_type_weights:
      break;

    case cgltf_attribute_type_joints:
      break;

    case cgltf_attribute_type_texcoord:
      packed_coords = malloc(mesh->face_count * 2 * sizeof(unsigned short));
      for (int i = 0; i < mesh->face_count*2; i++)
	packed_coords[i] = pack_short_texcoord(vs[i]);

      mesh->bind.vertex_buffers[1] = sg_make_buffer(&(sg_buffer_desc){
        .data.ptr = packed_coords,
	.data.size = sizeof(unsigned short) * 2 * mesh->face_count});

      free(packed_coords);
      break;
    }
    free(vs);
  }

  if (!has_norm) {
    uint32_t norms[mesh->face_count];

    cgltf_attribute *pa = get_attr_type(prim, cgltf_attribute_type_position);
    int n = cgltf_accessor_unpack_floats(pa->data, NULL,0);
    float ps[n];
    cgltf_accessor_unpack_floats(pa->data,ps,n);

    for (int i = 0, face=0; i < mesh->face_count/3; i++, face+=9) {
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
     mesh->bind.vertex_buffers[2] = sg_make_buffer(&(sg_buffer_desc){
       .data.ptr = norms,
       .data.size = sizeof(uint32_t) * mesh->face_count
     });
  }
}

void model_add_cgltf_mesh(model *model, cgltf_mesh *gltf_mesh)
{
  mesh mesh = {0};
  mesh.model = model;

  for (int i = 0; i < gltf_mesh->primitives_count; i++)
    mesh_add_primitive(&mesh, &gltf_mesh->primitives[i]);

  arrput(model->meshes,mesh);
}

void model_add_cgltf_anim(model *model, cgltf_animation *anim)
{

}

void model_add_cgltf_skin(model *model, cgltf_skin *skin)
{
  
}

void model_process_node(model *model, cgltf_node *node)
{
  if (node->has_matrix)
    memcpy(model->matrix.Elements, node->matrix, sizeof(float)*16);

  if (node->mesh)
    model_add_cgltf_mesh(model, node->mesh);

  if (node->skin)
    model_add_cgltf_skin(model, node->skin);
}

void model_process_scene(model *model, cgltf_scene *scene)
{
  for (int i = 0; i < scene->nodes_count; i++)
    model_process_node(model, scene->nodes[i]);
}

struct model *MakeModel(const char *path)
{
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
  
  model->path = path;

  if (data->scenes_count == 0 || data->scenes_count > 1) return NULL;
  model_process_scene(model, data->scene);

  for (int i = 0; i < data->meshes_count; i++)
    model_add_cgltf_mesh(model, &data->meshes[i]);

  for (int i = 0; i < data->animations_count; i++)
    model_add_cgltf_anim(model, &data->animations[i]);

  shput(modelhash, path, model);

  return model;
}

/* eye position */
HMM_Vec3 eye = {0,0,100};

void draw_model(struct model *model, HMM_Mat4 amodel) {
  HMM_Mat4 proj = projection;//HMM_Perspective_RH_ZO(45, (float)mainwin.width / mainwin.height, 0.1, 10000);
  HMM_Vec3 center = {0.f, 0.f, 0.f};
  HMM_Vec3 up = {0.f, 1.f, 0.f};
  HMM_Mat4 view = HMM_LookAt_RH(eye, center, vUP);

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
  struct gameobject *go = dm->go;
  HMM_Mat4 rst = t3d_go2world(go);
  draw_model(dm->model, rst);
}

void free_drawmodel(struct drawmodel *dm)
{
  free(dm);
}

