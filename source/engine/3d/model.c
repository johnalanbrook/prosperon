#include "model.h"

#include "log.h"
#include "resources.h"
#include "stb_ds.h"
#include "gameobject.h"

//#include "diffuse.sglsl.h"
#include "unlit.sglsl.h"

#include "render.h"

#include "HandmadeMath.h"

#include "math.h"
#include "time.h"

#include <cgltf.h>

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "yugine.h"

#include "texture.h"

#include "sokol/sokol_gfx.h"

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

struct joints {
  char j1;
  char j2;
  char j3;
  char j4;
};

void model_init() {
  model_shader = sg_make_shader(unlit_shader_desc(sg_query_backend()));

  model_pipe = sg_make_pipeline(&(sg_pipeline_desc){
    .shader = model_shader,
    .layout = {
      .attrs = {
        [0].format = SG_VERTEXFORMAT_FLOAT3,
	      [1].format = SG_VERTEXFORMAT_USHORT2N,
	      [1].buffer_index = 1,
        [2].format = SG_VERTEXFORMAT_UINT10_N2,
        [2].buffer_index = 2,
        [3] = {
          .format = SG_VERTEXFORMAT_UBYTE4N,
          .buffer_index = 3
        },
        [4] = {
          .format = SG_VERTEXFORMAT_UBYTE4,
          .buffer_index = 4
        }
      },
    },
    .index_type = SG_INDEXTYPE_UINT16,
    .cull_mode = SG_CULLMODE_FRONT,
    .depth.write_enabled = true,
    .depth.compare = SG_COMPAREFUNC_LESS_EQUAL
  });
}

cgltf_attribute *get_attr_type(cgltf_primitive *p, cgltf_attribute_type t)
{
  for (int i = 0; i < p->attributes_count; i++) {
    if (p->attributes[i].type == t)
      return &p->attributes[i];
  }

  return NULL;
}

unsigned short pack_short_texcoord(float x, float y)
{
  unsigned short s;
  char xc = x*255;
  char yc = y*255;
  return (((unsigned short)yc) << 8) | xc;
}

unsigned short pack_short_tex(float c) { return c * USHRT_MAX; }

uint32_t pack_int10_n2(float *norm)
{
  uint32_t ret = 0;
  for (int i = 0; i < 3; i++) {
    int n = (norm[i]+1.0)*511;
    ret |= (n & 0x3ff) << (10*i);
  }
  return ret;
}

void mesh_add_material(primitive *prim, cgltf_material *mat)
{
  if (!mat) return;
  
  if (mat->has_pbr_metallic_roughness) {
    cgltf_image *img = mat->pbr_metallic_roughness.base_color_texture.texture->image;
     if (img->buffer_view) {
       cgltf_buffer_view *buf = img->buffer_view;
       prim->bind.fs.images[0] = texture_fromdata(buf->buffer->data, buf->size)->id;
     } else
       prim->bind.fs.images[0] = texture_from_file(img->uri)->id;
   } else
     prim->bind.fs.images[0] = texture_from_file("icons/moon.gif")->id; 
    
   prim->bind.fs.samplers[0] = std_sampler;
}

sg_buffer texcoord_floats(float *f, int verts, int comp)
{
  int n = verts*comp;
  unsigned short packed[n];
  for (int i = 0; i < n; i++)
    packed[i] = pack_short_tex(f[i]);

  return sg_make_buffer(&(sg_buffer_desc){
    .data = SG_RANGE(packed),
    .label = "tex coord vert buffer",
  });
}

sg_buffer normal_floats(float *f, int verts, int comp)
{
  uint32_t packed_norms[verts];
  for (int v = 0, i = 0; v < verts; v++, i+= comp)
    packed_norms[v] = pack_int10_n2(f+i);

  return sg_make_buffer(&(sg_buffer_desc){
    .data = SG_RANGE(packed_norms),
    .label = "normal vert buffer",
  });
}

sg_buffer joint_buf(float *f, int v, int c)
{
  char joints[v*c];
  for (int i = 0; i < (v*c); i++)
    joints[i] = f[i];
  
  return sg_make_buffer(&(sg_buffer_desc){ .data = SG_RANGE(joints)});
}

sg_buffer weight_buf(float *f, int v, int c)
{
  unsigned char weights[v*c];
  for (int i = 0; i < (v*c); i++)
    weights[i] = f[i]*255;
  
  return sg_make_buffer(&(sg_buffer_desc){ .data = SG_RANGE(weights)});
}

HMM_Vec3 index_to_vert(uint32_t idx, float *f)
{
  return (HMM_Vec3){f[idx*3], f[idx*3+1], f[idx*3+2]};
}

struct primitive mesh_add_primitive(cgltf_primitive *prim)
{
  primitive retp = (primitive){0};

  uint16_t *idxs;
  if (prim->indices) {
    int n = cgltf_accessor_unpack_floats(prim->indices, NULL, 0);
    float fidx[n];
    cgltf_accessor_unpack_floats(prim->indices, fidx, n);
    idxs = malloc(sizeof(*idxs)*n);
    for (int i = 0; i < n; i++)
      idxs[i] = fidx[i];

    retp.bind.index_buffer = sg_make_buffer(&(sg_buffer_desc){
      .data.ptr = idxs,
      .data.size = sizeof(*idxs) * n,
      .type = SG_BUFFERTYPE_INDEXBUFFER,
      .label = "mesh index buffer",
    });

    retp.idx_count = n;
  } else {
    YughWarn("Model does not have indices. Generating them.");
    int c = cgltf_accessor_unpack_floats(prim->attributes[0].data, NULL, 0);

    retp.idx_count = c;
    idxs = malloc(sizeof(*idxs)*c);
    
    for (int z = 0; z < c; z++)
      idxs[z] = z;

    retp.bind.index_buffer = sg_make_buffer(&(sg_buffer_desc){
	    .data.ptr = idxs,
	    .data.size = sizeof(uint16_t) * c,
	    .type = SG_BUFFERTYPE_INDEXBUFFER});
  }
  
  free(idxs);

  printf("adding material\n");
  mesh_add_material(&retp, prim->material);
  int has_norm = 0;  

  for (int k = 0; k < prim->attributes_count; k++) {
    cgltf_attribute attribute = prim->attributes[k];

    int n = cgltf_accessor_unpack_floats(attribute.data, NULL, 0); /* floats per vertex x num elements. In other words, total floats pulled */
    int comp = cgltf_num_components(attribute.data->type);
    int verts = n/comp;
    float vs[n];
    cgltf_accessor_unpack_floats(attribute.data, vs, n);

    switch (attribute.type) {
      case cgltf_attribute_type_position:
      retp.bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
        .data.ptr = vs,
        .data.size = sizeof(float) * n,
        .label = "mesh vert buffer"
        });
      break;

    case cgltf_attribute_type_normal:
      has_norm = 1;
      retp.bind.vertex_buffers[2] = normal_floats(vs, verts, comp);
      break;

    case cgltf_attribute_type_tangent:
      break;

    case cgltf_attribute_type_color:
      break;

    case cgltf_attribute_type_weights:
      retp.bind.vertex_buffers[3] = weight_buf(vs, verts, comp);
      break;

    case cgltf_attribute_type_joints:
      retp.bind.vertex_buffers[4] = joint_buf(vs, verts, comp);
      break;

    case cgltf_attribute_type_texcoord:
      retp.bind.vertex_buffers[1] = texcoord_floats(vs, verts, comp);
      break;
    case cgltf_attribute_type_invalid:
      YughWarn("Invalid type.");
      break;
      
    case cgltf_attribute_type_custom:
      break;
    case cgltf_attribute_type_max_enum:
      break;
    }
  }

  if (!has_norm) {
    YughInfo("Making normals.");
    cgltf_attribute *pa = get_attr_type(prim, cgltf_attribute_type_position);
    int n = cgltf_accessor_unpack_floats(pa->data, NULL,0);
    int comp = 3;
    int verts = n/comp;
    uint32_t face_norms[verts];
    float ps[n];
    cgltf_accessor_unpack_floats(pa->data,ps,n);

    for (int i = 0; i < verts; i+=3) {
      HMM_Vec3 a = index_to_vert(i,ps);
      HMM_Vec3 b = index_to_vert(i+1,ps);
      HMM_Vec3 c = index_to_vert(i+2,ps);
      HMM_Vec3 norm = HMM_NormV3(HMM_Cross(HMM_SubV3(b,a), HMM_SubV3(c,a)));
      uint32_t packed_norm = pack_int10_n2(norm.Elements);
      face_norms[i] = face_norms[i+1] = face_norms[i+2] = packed_norm;
     }
     
     retp.bind.vertex_buffers[2] = sg_make_buffer(&(sg_buffer_desc){
       .data.ptr = face_norms,
       .data.size = sizeof(uint32_t) * verts});
  }
  
  return retp;
}

static cgltf_data *cdata;

void model_add_cgltf_mesh(mesh *m, cgltf_mesh *gltf_mesh)
{
  printf("mesh has %d primitives\n", gltf_mesh->primitives_count);
  for (int i = 0; i < gltf_mesh->primitives_count; i++)
    arrput(m->primitives, mesh_add_primitive(gltf_mesh->primitives+i));
}

void packFloats(float *src, float *dest, int srcLength) {
    int i, j;
    for (i = 0, j = 0; i < srcLength; i += 3, j += 4) {
        dest[j] = src[i];
        dest[j + 1] = src[i + 1];
        dest[j + 2] = src[i + 2];
        dest[j + 3] = 0.0f;
    }
}

void model_add_cgltf_anim(model *model, cgltf_animation *anim)
{
  YughInfo("FOUND ANIM, using %d channels and %d samplers", anim->channels_count, anim->samplers_count);
  
  struct animation an = (struct animation){0};
  arrsetlen(an.samplers, anim->samplers_count);
  
  for (int i = 0; i < anim->samplers_count; i++) {
    cgltf_animation_sampler s = anim->samplers[i];
    sampler samp = (sampler){0};
    int n = cgltf_accessor_unpack_floats(s.input, NULL, 0);
    arrsetlen(samp.times, n);
    cgltf_accessor_unpack_floats(s.input, samp.times, n);

    n = cgltf_accessor_unpack_floats(s.output, NULL, 0);
    int comp = cgltf_num_components(s.output->type);
    arrsetlen(samp.data, n/comp);
    if (comp == 4)
      cgltf_accessor_unpack_floats(s.output, samp.data, n);
    else {
      float *out = malloc(sizeof(*out)*n);
      cgltf_accessor_unpack_floats(s.output, out, n);
      packFloats(out, samp.data, n);
      free(out);
    }
    
    samp.type = s.interpolation;
    
    if (samp.type == LINEAR && comp == 4)
      samp.type = SLERP;
    
    an.samplers[i] = samp;
  }
    
  for (int i = 0; i < anim->channels_count; i++) {
    cgltf_animation_channel ch = anim->channels[i];
    struct anim_channel ach = (struct anim_channel){0};
    md5joint *md = model->nodes+(ch.target_node-cdata->nodes);
    switch(ch.target_path) {
      case cgltf_animation_path_type_translation:
        ach.target = &md->pos;
        break;
      case cgltf_animation_path_type_rotation:
        ach.target = &md->rot;
        break;
      case cgltf_animation_path_type_scale:
        ach.target = &md->scale;
        break;
    }
    ach.sampler = an.samplers+(ch.sampler-anim->samplers);
    
    arrput(an.channels, ach);
  }
  
  model->anim = an;
  model->anim.time = apptime();
}

void model_add_cgltf_skin(model *model, cgltf_skin *skin)
{
  int n = cgltf_accessor_unpack_floats(skin->inverse_bind_matrices, NULL, 0);
  struct skin sk = (struct skin){0};
  arrsetlen(sk.invbind, n/16);
  cgltf_accessor_unpack_floats(skin->inverse_bind_matrices, sk.invbind, n);
  YughInfo("FOUND SKIN, of %d bones, and %d vert comps", skin->joints_count, n);
  
  cgltf_node *root = skin->skeleton;
  
  arrsetlen(sk.joints, skin->joints_count);
  sk.root = model->nodes+(skin->skeleton-cdata->nodes);
  
  for (int i = 0; i < 50; i++)
    sk.binds[i] = HMM_M4D(1);
  
  for (int i = 0; i < skin->joints_count; i++) {
    int offset = skin->joints[i]-cdata->nodes;
    sk.joints[i] = model->nodes+offset;
    md5joint *j = sk.joints[i];
    cgltf_node *n = skin->joints[i];
    for (int i = 0; i < 3; i++) {
      j->pos.e[i] = n->translation[i];
      j->scale.e[i] = n->scale[i];
    }
    for (int i = 0; i < 4; i++)
      j->rot.e[i] = n->rotation[i];
  }
  
  model->skin = sk;
}

void model_process_node(model *model, cgltf_node *node)
{
  int n = node-cdata->nodes;
  cgltf_node_transform_world(node, model->nodes[n].t.e);
  model->nodes[n].parent = model->nodes+(node->parent-cdata->nodes);
  
  if (node->mesh) {
    int meshn = node->mesh-cdata->meshes;
    arrsetlen(model->meshes, meshn+1);
    model->meshes[meshn].m = &model->nodes[n].t;
    model_add_cgltf_mesh(model->meshes+meshn, node->mesh);
  }
}

struct model *model_make(const char *path)
{
  YughInfo("Making the model from %s.", path);
  cgltf_options options = {0};
  cgltf_data *data = NULL;
  cgltf_result result = cgltf_parse_file(&options, path, &data);
  struct model *model = NULL;

  if (result) {
    YughError("CGLTF could not parse file %s, err %d.", path, result);
    goto CLEAN;
  }

  result = cgltf_load_buffers(&options, data, path);

  if (result) {
    YughError("CGLTF could not load buffers for file %s, err %d.", path, result);
    goto CLEAN;
  }
  
  cdata = data;

  model = calloc(1, sizeof(*model));
  
  arrsetlen(model->nodes, data->nodes_count);
  for (int i = 0; i < data->nodes_count; i++)
    model_process_node(model, data->nodes+i);

  for (int i = 0; i < data->animations_count; i++)
    model_add_cgltf_anim(model, data->animations+i);
    
  for (int i = 0; i < data->skins_count; i++)
    model_add_cgltf_skin(model, data->skins+i);
    
  CLEAN:
  cgltf_free(data);
  return model;
}

void model_free(model *m)
{

}

void model_draw_go(model *model, gameobject *go, gameobject *cam)
{
  animation_run(&model->anim, apptime());

  skin *sk = &model->skin;
  for (int i = 0; i < arrlen(sk->joints); i++) {
    md5joint *md = sk->joints[i];
    HMM_Mat4 local = HMM_M4TRS(md->pos.xyz, md->rot, md->scale.xyz);
    if (md->parent)
      local = HMM_MulM4(md->parent->t, local);
    md->t = local;
    sk->binds[i] = HMM_MulM4(md->t, sk->invbind[i]);
    
    //printf("TRANSLATION OF %d IS " HMMFMT_VEC3 "\n", i, HMMPRINT_VEC3(md->pos));
  }

  HMM_Mat4 view = t3d_go2world(cam);
  HMM_Mat4 proj = HMM_Perspective_RH_NO(20, 1, 0.01, 10000);
  HMM_Mat4 vp = HMM_MulM4(proj, view);
  HMM_Mat4 gom = transform3d2mat(go2t3(go));

  sg_apply_pipeline(model_pipe);
  sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_p, SG_RANGE_REF(vp.e));
  sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_skinv, &(sg_range){
    .ptr = sk->binds,
    .size = sizeof(*sk->binds)*50
  });
  float ambient[4] = {1.0,1.0,1.0,1.0};
  sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_lightf, SG_RANGE_REF(ambient));
  for (int i = 0; i < arrlen(model->meshes); i++) {
    HMM_Mat4 mod = *model->meshes[i].m;
    mod = HMM_MulM4(mod, gom);
    mesh msh = model->meshes[i];
    for (int j = 0; j < arrlen(msh.primitives); j++) {
      sg_apply_bindings(&(msh.primitives[j].bind));    
      sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vmodel, &(sg_range){
        .ptr = mod.em,
        .size = sizeof(mod)
      });
      sg_draw(0, model->meshes[i].primitives[j].idx_count, 1);    
    }
  }
}

void material_free(material *mat)
{

}
