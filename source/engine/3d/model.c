#include "model.h"

#include "log.h"
#include "resources.h"
#include "stb_ds.h"
#include "gameobject.h"

#include "render.h"

#include "HandmadeMath.h"

#include "math.h"
#include "time.h"

#include <cgltf.h>

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "yugine.h"

#include "jsffi.h"

#include "texture.h"

#include "sokol/sokol_gfx.h"

static void processnode();
static void processmesh();
static void processtexture();

static cgltf_data *cdata;
static char *cpath;

cgltf_attribute *get_attr_type(cgltf_primitive *p, cgltf_attribute_type t)
{
  for (int i = 0; i < p->attributes_count; i++) {
    if (p->attributes[i].type == t)
      return &p->attributes[i];
  }

  return NULL;
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
  
  prim->mat = calloc(sizeof(*prim->mat), 1);
  material *pmat = prim->mat;
  
  if (mat->has_pbr_metallic_roughness && mat->pbr_metallic_roughness.base_color_texture.texture) {
    cgltf_image *img = mat->pbr_metallic_roughness.base_color_texture.texture->image;
    if (img->buffer_view) {
      cgltf_buffer_view *buf = img->buffer_view;
      pmat->diffuse = texture_fromdata(buf->buffer->data, buf->size);
    } else {
      char *path = makepath(dirname(cpath), img->uri);
      pmat->diffuse = texture_from_file(path);
      free(path);
     }
   } else
     pmat->diffuse = texture_from_file("icons/moon.gif"); 
}

sg_buffer texcoord_floats(float *f, int n)
{
  unsigned short packed[n];
  for (int i = 0; i < n; i++) {
    float v = f[i];
    if (v < 0) v = 0;
    if (v > 1) v = 1;
    packed[i] = pack_short_tex(v);
  }

  return sg_make_buffer(&(sg_buffer_desc){
    .data = SG_RANGE(packed),
    .label = "tex coord vert buffer",
  });
}

sg_buffer par_idx_buffer(uint32_t *p, int v)
{
  uint16_t idx[v];
  for (int i = 0; i < v; i++) idx[i] = p[i];
  
  return sg_make_buffer(&(sg_buffer_desc){
    .data = SG_RANGE(idx),
    .type = SG_BUFFERTYPE_INDEXBUFFER
  });
}

sg_buffer float_buffer(float *f, int v)
{
  return sg_make_buffer(&(sg_buffer_desc){
    .data = (sg_range){
      .ptr = f,
      .size = sizeof(*f)*v
    }
  });
}

sg_buffer index_buffer(float *f, int verts)
{
  uint16_t idxs[verts];
  for (int i = 0; i < verts; i++)
    idxs[i] = f[i];
  
  return sg_make_buffer(&(sg_buffer_desc){
    .data = SG_RANGE(idxs),
    .type = SG_BUFFERTYPE_INDEXBUFFER,
  });
}

sg_buffer normal_floats(float *f, int n)
{
  uint32_t packed_norms[n/3];
  for (int v = 0, i = 0; v < n/3; v++, i+= 3)
    packed_norms[v] = pack_int10_n2(f+i);

  return sg_make_buffer(&(sg_buffer_desc){
    .data = SG_RANGE(packed_norms),
    .label = "normal vert buffer",
  });
}

sg_buffer ubyten_buffer(float *f, int v)
{
  unsigned char b[v];
  for (int i = 0; i < (v); i++)
    b[i] = f[i]*255;
    
  return sg_make_buffer(&(sg_buffer_desc){.data=SG_RANGE(b)});
}

sg_buffer ubyte_buffer(float *f, int v)
{
  unsigned char b[v];
  for (int i = 0; i < (v); i++)
    b[i] = f[i];
    
  return sg_make_buffer(&(sg_buffer_desc){.data=SG_RANGE(b)});
}

sg_buffer joint_buf(float *f, int v)
{
  char joints[v];
  for (int i = 0; i < (v); i++)
    joints[i] = f[i];
  
  return sg_make_buffer(&(sg_buffer_desc){ .data = SG_RANGE(joints)});
}

sg_buffer weight_buf(float *f, int v)
{
  unsigned char weights[v];
  for (int i = 0; i < (v); i++)
    weights[i] = f[i]*255;
  
  return sg_make_buffer(&(sg_buffer_desc){ .data = SG_RANGE(weights)});
}

HMM_Vec3 index_to_vert(uint32_t idx, float *f)
{
  return (HMM_Vec3){f[idx*3], f[idx*3+1], f[idx*3+2]};
}

void primitive_gen_indices(primitive *prim)
{
  if (prim->idx_count == 0) return;
  uint16_t *idxs = malloc(sizeof(*idxs)*prim->idx_count);
  
  for (int z = 0; z < prim->idx_count; z++)
    idxs[z] = z;
  
  prim->idx = sg_make_buffer(&(sg_buffer_desc){
    .data.ptr = idxs,
    .data.size = sizeof(uint16_t) * prim->idx_count,
    .type = SG_BUFFERTYPE_INDEXBUFFER});
    
  free(idxs);
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

    retp.idx = sg_make_buffer(&(sg_buffer_desc){
      .data.ptr = idxs,
      .data.size = sizeof(*idxs) * n,
      .type = SG_BUFFERTYPE_INDEXBUFFER,
      .label = "mesh index buffer",
    });

    retp.idx_count = n;
    free(idxs);
  } else {
    retp.idx_count = cgltf_accessor_unpack_floats(prim->attributes[0].data, NULL, 0);
    primitive_gen_indices(&retp);
  }
  
  printf("adding material\n");
  mesh_add_material(&retp, prim->material);

  for (int k = 0; k < prim->attributes_count; k++) {
    cgltf_attribute attribute = prim->attributes[k];

    int n = cgltf_accessor_unpack_floats(attribute.data, NULL, 0); /* floats per vertex x num elements. In other words, total floats pulled */
    int comp = cgltf_num_components(attribute.data->type);
    int verts = n/comp;
    float vs[n];
    cgltf_accessor_unpack_floats(attribute.data, vs, n);

    switch (attribute.type) {
      case cgltf_attribute_type_position:
      retp.pos = sg_make_buffer(&(sg_buffer_desc){
        .data.ptr = vs,
        .data.size = sizeof(float) * n,
        .label = "mesh vert buffer"
        });
      break;

    case cgltf_attribute_type_normal:
      retp.norm = normal_floats(vs, n);
      break;

    case cgltf_attribute_type_tangent:
      break;

    case cgltf_attribute_type_color:
      retp.color = ubyten_buffer(vs,n);
      break;

    case cgltf_attribute_type_weights:
      retp.weight = ubyten_buffer(vs, n);
      break;

    case cgltf_attribute_type_joints:
      retp.bone = ubyte_buffer(vs, n);
      break;

    case cgltf_attribute_type_texcoord:
      retp.uv = texcoord_floats(vs, n);
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

  if (!retp.bone.id) {
    char joints[retp.idx_count*4];
    memset(joints, 0, retp.idx_count*4);
    retp.bone = sg_make_buffer(&(sg_buffer_desc){ .data = SG_RANGE(joints)});
  }
  
  if (!retp.weight.id) {
    char weights[retp.idx_count*4];
    memset(weights,0,retp.idx_count*4);
    retp.weight = sg_make_buffer(&(sg_buffer_desc){ .data = SG_RANGE(weights)});
  }
  
  if (!retp.color.id) {
    char colors[retp.idx_count*4];
    memset(colors,0,retp.idx_count*4);
    retp.color = sg_make_buffer(&(sg_buffer_desc) { .data = SG_RANGE(colors) });
  }

  if (!retp.norm.id) {
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
     
     retp.norm = sg_make_buffer(&(sg_buffer_desc){
       .data.ptr = face_norms,
       .data.size = sizeof(uint32_t) * verts});
  }
  
  return retp;
}



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
    sk.binds[i] = MAT1;
  
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
  cpath = path;
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

sg_bindings primitive_bind(primitive *p)
{
  sg_bindings b = {0};
  b.vertex_buffers[MAT_POS] = p->pos;
  b.vertex_buffers[MAT_UV] = p->uv;
  b.vertex_buffers[MAT_NORM] = p->norm;
  b.vertex_buffers[MAT_BONE] = p->bone;
  b.vertex_buffers[MAT_WEIGHT] = p->weight;
  b.vertex_buffers[MAT_COLOR] = p->color;
  b.index_buffer = p->idx;
  b.fs.images[0] = p->mat->diffuse->id;
  b.fs.samplers[0] = tex_sampler;
  return b;
}

void model_draw_go(model *model, gameobject *go, gameobject *cam)
{
  HMM_Mat4 view = t3d_go2world(cam);
  HMM_Mat4 proj = HMM_Perspective_RH_NO(20, 1, 0.01, 10000);
  HMM_Mat4 vp = HMM_MulM4(proj, view);
  HMM_Mat4 gom = transform3d2mat(go2t3(go));
  
  animation_run(&model->anim, apptime());

  skin *sk = &model->skin;
  for (int i = 0; i < arrlen(sk->joints); i++) {
    md5joint *md = sk->joints[i];
    HMM_Mat4 local = HMM_M4TRS(md->pos.xyz, md->rot, md->scale.xyz);
    if (md->parent)
      local = HMM_MulM4(md->parent->t, local);
    md->t = local;
    sk->binds[i] = HMM_MulM4(md->t, sk->invbind[i]);
  }
  /*sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_skinv, &(sg_range){
    .ptr = sk->binds,
    .size = sizeof(*sk->binds)*50
  });
  */

  for (int i = 0; i < arrlen(model->meshes); i++) {
    HMM_Mat4 mod = *model->meshes[i].m;
    mod = HMM_MulM4(mod, gom);
    mesh msh = model->meshes[i];
    for (int j = 0; j < arrlen(msh.primitives); j++) {
      sg_bindings b = primitive_bind(msh.primitives+j);
      sg_apply_bindings(&b);    
      sg_draw(0, msh.primitives[j].idx_count, 1);    
    }
  }
}

int mat2type(int mat)
{
  switch(mat) {
    case MAT_POS:
      return SG_VERTEXFORMAT_FLOAT3;
    case MAT_PPOS:
    case MAT_WH:
    case MAT_ST:
      return SG_VERTEXFORMAT_FLOAT2;
    case MAT_UV:
    case MAT_TAN:
      return SG_VERTEXFORMAT_USHORT2N;
    case MAT_NORM:
      return SG_VERTEXFORMAT_UINT10_N2;
    case MAT_BONE:
      return SG_VERTEXFORMAT_UBYTE4;
    case MAT_WEIGHT:
    case MAT_COLOR:
      return SG_VERTEXFORMAT_UBYTE4N;
    case MAT_ANGLE:
    case MAT_SCALE:
      return SG_VERTEXFORMAT_FLOAT;
  };
  return 0;
}

int mat2step(int mat)
{
  switch(mat) {
    case MAT_POS:
    case MAT_UV:
    case MAT_TAN:
    case MAT_NORM:
    case MAT_BONE:
    case MAT_WEIGHT:
      return SG_VERTEXSTEP_PER_VERTEX;
  };
  return SG_VERTEXSTEP_PER_INSTANCE;
}

sg_buffer mat2buffer(int mat, primitive *p)
{
  switch(mat) {
    case MAT_POS: return p->pos;
    case MAT_NORM: return p->norm;
    case MAT_UV: return p->uv;
    case MAT_BONE: return p->bone;
    case MAT_WEIGHT: return p->weight;
    case MAT_COLOR: return p->color;
  };
  
  return p->pos;
}

sg_bindings primitive_bindings(primitive *p, JSValue v)
{
  sg_bindings b = {0};
  JSValue inputs = js_getpropstr(js_getpropstr(v, "vs"), "inputs");
  for (int i = 0; i < js_arrlen(inputs); i++) {
    JSValue attr = js_getpropidx(inputs, i);
    int mat = js2number(js_getpropstr(attr, "mat"));  
    int slot = js2number(js_getpropstr(attr, "slot"));
    sg_buffer buf = mat2buffer(mat,p);
    if (!buf.id) {
      // ERROR
    }
    b.vertex_buffers[slot] = buf;
  }
  
  b.index_buffer = p->idx;
  
  return b;
}

void primitive_free(primitive *prim)
{

}

void material_free(material *mat)
{

}
