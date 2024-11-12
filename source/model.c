#include "model.h"

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

#include "jsffi.h"

unsigned short pack_short_tex(float c) { return c * USHRT_MAX; }

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

uint32_t pack_int10_n2(float *norm)
{
  uint32_t ret = 0;
  for (int i = 0; i < 3; i++) {
    int n = (norm[i]+1.0)*511;
    ret |= (n & 0x3ff) << (10*i);
  }
  return ret;
}

// Pack an array of normals into 
sg_buffer normal_floats(float *f, int n)
{
  return float_buffer(f, n);
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

sg_buffer accessor2buffer(cgltf_accessor *a, int type)
{
  int n = cgltf_accessor_unpack_floats(a, NULL, 0);
  float vs[n];
  cgltf_accessor_unpack_floats(a, vs, n);

  switch(type) {
    case MAT_POS:
      return sg_make_buffer(&(sg_buffer_desc){
        .data.ptr = vs,
	.data.size = sizeof(float)*n
      });
    case MAT_NORM:
      return normal_floats(vs,n);
    case MAT_TAN:
      return normal_floats(vs,n); // TODO: MAKE A TANGENT READER
    case MAT_COLOR:
      return ubyten_buffer(vs,n);
    case MAT_WEIGHT:
      return ubyten_buffer(vs,n);
    case MAT_BONE:
      return ubyte_buffer(vs,n);
    case MAT_UV:
      return texcoord_floats(vs,n);
    case MAT_INDEX:
      return index_buffer(vs,n);
  }

  return sg_make_buffer(&(sg_buffer_desc) {
  .data.size = 4,
  .usage = SG_USAGE_STREAM
  });
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

static md5joint *node2joint(skin *sk, cgltf_node *n, cgltf_skin *skin)
{
  int k = 0;
  while (skin->joints[k] != n && k < skin->joints_count) k++;
  return sk->joints+k;
}

animation *gltf_anim(cgltf_animation *anim, skin *sk, cgltf_skin *skin)
{
  animation *ret = calloc(sizeof(*ret), 1);
  animation an = *ret;
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
    md5joint *md = node2joint(sk, ch.target_node, skin);
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
      default: break;
    }
    ach.sampler = an.samplers+(ch.sampler-anim->samplers);
    
    arrput(an.channels, ach);
  }

  an.time = 0;

  *ret = an;
  return ret;
}

skin *make_gltf_skin(cgltf_skin *skin, cgltf_data *data)
{
  int n = cgltf_accessor_unpack_floats(skin->inverse_bind_matrices, NULL, 0);
  struct skin *sk = NULL;
  sk = calloc(sizeof(*sk),1);
  arrsetlen(sk->invbind, n/16);
  cgltf_accessor_unpack_floats(skin->inverse_bind_matrices, sk->invbind, n);
  
  arrsetlen(sk->joints, skin->joints_count);
  
  for (int i = 0; i < 50; i++)
    sk->binds[i] = MAT1;
  
  for (int i = 0; i < skin->joints_count; i++) {
    cgltf_node *n = skin->joints[i];
    md5joint *j = sk->joints+i;

    if (n == skin->skeleton)
      j->parent = NULL;
    else
      j->parent = node2joint(sk, n->parent, skin);

    for (int i = 0; i < 3; i++) {
      j->pos.e[i] = n->translation[i];
      j->scale.e[i] = n->scale[i];
    }
    for (int i = 0; i < 4; i++)
      j->rot.e[i] = n->rotation[i];
  }

  sk->anim = gltf_anim(data->animations+0, sk, skin);

  return sk;
}

void skin_calculate(skin *sk)
{
//  animation_run(sk->anim, apptime());
  for (int i = 0; i < arrlen(sk->joints); i++) {
    md5joint *md = sk->joints+i;
    md->t = HMM_M4TRS(md->pos.xyz, md->rot, md->scale.xyz);
    if (md->parent)
      md->t = HMM_MulM4(md->parent->t, md->t);

    sk->binds[i] = HMM_MulM4(md->t, sk->invbind[i]);
  }
}
