#include "sprite.h"

#include "datastream.h"
#include "font.h"
#include "gameobject.h"
#include "log.h"
#include "render.h"
#include "shader.h"
#include "stb_ds.h"
#include "texture.h"
#include "timer.h"
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include "HandmadeMath.h"
#include "freelist.h"

#include "sprite.sglsl.h"
#include "9slice.sglsl.h"

struct TextureOptions TEX_SPRITE = {1, 0, 0};

static struct sprite *sprites = NULL;

static sg_shader shader_sprite;
static sg_pipeline pip_sprite;
static sg_bindings bind_sprite;

struct sprite_vert {
  HMM_Vec2 pos;
  HMM_Vec2 uv;
  struct rgba color;
  struct rgba emissive;
};

static int num_spriteverts = 5000;

static sg_shader slice9_shader;
static sg_pipeline slice9_pipe;
static sg_bindings slice9_bind;
static float slice9_points[8] = {
  0.0, 0.0,
  0.0, 1.0,
  1.0, 0.0,
  1.0, 1.0
};
struct slice9_vert {
  HMM_Vec2 pos;
  struct uv_n uv;
  unsigned short border[4];
  HMM_Vec2 scale;
  struct rgba color;
};

int make_sprite(int go) {
  YughWarn("Attaching sprite to gameobject %d", go);
  struct sprite sprite = {
      .color = color_white,
      .emissive = {0,0,0,0},
      .size = {1.f, 1.f},
      .tex = texture_loadfromfile(NULL),
      .go = go,
      .layer = 0,
      .next = -1,
      .enabled = 1};

  int id;
  freelist_grab(id, sprites);
  sprites[id] = sprite;
  return id;
}

void sprite_delete(int id) {
  struct sprite *sp = id2sprite(id);
  sp->go = -1;
  sp->enabled = 0;
  freelist_kill(sprites,id);
}

void sprite_enabled(int id, int e) {
  sprites[id].enabled = e;
}

struct sprite *id2sprite(int id) {
  if (id < 0) return NULL;
  return &sprites[id];
}

static int sprite_count = 0;

void sprite_flush() {
  sprite_count = 0;
}

void sprite_io(struct sprite *sprite, FILE *f, int read) {
  char path[100];
  if (read) {
    // fscanf(f, "%s", &path);
    for (int i = 0; i < 100; i++) {
      path[i] = fgetc(f);

      if (path[i] == '\0') break;
    }
    fread(sprite, sizeof(*sprite), 1, f);
    sprite_loadtex(sprite, path, ST_UNIT);
  } else {
    fputs(tex_get_path(sprite->tex), f);
    fputc('\0', f);
    fwrite(sprite, sizeof(*sprite), 1, f);
  }
}

int sprite_sort(int *a, int *b)
{
  struct gameobject *goa = id2go(sprites[*a].go);
  struct gameobject *gob = id2go(sprites[*b].go);
  if (goa->drawlayer == gob->drawlayer) return 0;
  if (goa->drawlayer > gob->drawlayer) return 1;
  return -1;
}

void sprite_draw_all() {
  sg_apply_pipeline(pip_sprite);
  sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, SG_RANGE_REF(projection));
  static int *layers;
  if (layers) arrfree(layers);

  for (int i = 0; i < freelist_len(sprites); i++)
    if (sprites[i].next == -1 && sprites[i].go >= 0 && sprites[i].enabled)
      arrpush(layers, i);

  if (arrlen(layers) == 0) return;
  if (arrlen(layers) > 1)
    qsort(layers, arrlen(layers), sizeof(*layers), sprite_sort);

  for (int i = 0; i < arrlen(layers); i++)
    sprite_draw(&sprites[layers[i]]);
}

void sprite_loadtex(struct sprite *sprite, const char *path, struct glrect frame) {
  if (!sprite) {
    YughWarn("NO SPRITE!");
    return;
  }
  sprite->tex = texture_loadfromfile(path);
  sprite_setframe(sprite, &frame);
}

void sprite_settex(struct sprite *sprite, struct Texture *tex) {
  sprite->tex = tex;
  sprite_setframe(sprite, &ST_UNIT);
}

void sprite_initialize() {
  freelist_size(sprites, 500);
  
  shader_sprite = sg_make_shader(sprite_shader_desc(sg_query_backend()));

  pip_sprite = sg_make_pipeline(&(sg_pipeline_desc){
      .shader = shader_sprite,
      .layout = {
          .attrs = {
              [0].format = SG_VERTEXFORMAT_FLOAT2,
	      [1].format = SG_VERTEXFORMAT_FLOAT2,
	      [2].format = SG_VERTEXFORMAT_UBYTE4N,
	      [3].format = SG_VERTEXFORMAT_UBYTE4N}},
      .primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP,
      .label = "sprite pipeline",
      .colors[0].blend = blend_trans,
      .depth = {
        .write_enabled = true,
	.compare = SG_COMPAREFUNC_LESS_EQUAL,
	.pixel_format = SG_PIXELFORMAT_DEPTH
      }
  });

  bind_sprite.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
      .size = sizeof(struct sprite_vert) * num_spriteverts,
      .type = SG_BUFFERTYPE_VERTEXBUFFER,
      .usage = SG_USAGE_STREAM,
      .label = "sprite vertex buffer",
  });
  bind_sprite.fs.samplers[0] = sg_make_sampler(&(sg_sampler_desc){});

  slice9_shader = sg_make_shader(slice9_shader_desc(sg_query_backend()));

  slice9_pipe = sg_make_pipeline(&(sg_pipeline_desc){
    .shader = slice9_shader,
    .layout = {
      .attrs = {
        [0].format = SG_VERTEXFORMAT_FLOAT2,
	[1].format = SG_VERTEXFORMAT_FLOAT2,
	[2].format = SG_VERTEXFORMAT_USHORT4N,
	[3].format = SG_VERTEXFORMAT_FLOAT2,
	[4].format = SG_VERTEXFORMAT_UBYTE4N
      }},
    .primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP,
  });

  slice9_bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
    .size = sizeof(struct slice9_vert) * 100,
    .type = SG_BUFFERTYPE_VERTEXBUFFER,
    .usage = SG_USAGE_STREAM,
  });
}

/* offset given in texture offset, so -0.5,-0.5 results in it being centered */
void tex_draw(struct Texture *tex, HMM_Mat3 m, struct glrect r, struct rgba color, int wrap, HMM_Vec2 wrapoffset, float wrapscale, struct rgba emissive) {
  struct sprite_vert verts[4];
  
  HMM_Vec2 sposes[4] = {
    {0.0,0.0},
    {1.0,0.0},
    {0.0,1.0},
    {1.0,1.0},
  };

  HMM_Vec2 t_scale = {
    tex->width * st_s_w(r), //*size.X;
    tex->height * st_s_h(r) // * size.Y
  };

  for (int i = 0; i < 4; i++) {
    verts[i].pos = mat_t_pos(m, sposes[i]);
    verts[i].color = color;
    verts[i].emissive = emissive;
  }
  
  if (!wrap) {
    verts[0].uv.X = r.s0;
    verts[0].uv.Y = r.t1;
    verts[1].uv.X = r.s1;
    verts[1].uv.Y = r.t1;
    verts[2].uv.X = r.s0;
    verts[2].uv.Y = r.t0;
    verts[3].uv.X = r.s1;
    verts[3].uv.Y = r.t0;
  } else {
//    verts[0].uv = HMM_MulV2((HMM_Vec2){0,0}, size);
//    verts[1].uv = HMM_MulV2((HMM_Vec2){1,0}, size);
//    verts[2].uv = HMM_MulV2((HMM_Vec2){0,1}, size);
//    verts[3].uv = HMM_MulV2((HMM_Vec2){1,1}, size);

    for (int i = 0; i < 4; i++)
      verts[i].uv = HMM_AddV2(verts[i].uv, wrapoffset);
  }

  bind_sprite.fs.images[0] = tex->id;
//  YughWarn("Draw sprite %s at %g, %g", tex_get_path(tex), sposes[0].X, sposes[0].Y);

  sg_append_buffer(bind_sprite.vertex_buffers[0], SG_RANGE_REF(verts));
  sg_apply_bindings(&bind_sprite);

  sg_draw(sprite_count * 4, 4, 1);
  sprite_count++;
}

void sprite_draw(struct sprite *sprite) {
  struct gameobject *go = id2go(sprite->go);

  if (sprite->tex) {
    transform2d t = go2t(id2go(sprite->go));
    HMM_Mat3 m = transform2d2mat(t);

    HMM_Mat3 ss = HMM_M3D(1);
    ss.Columns[0].X = sprite->size.X * sprite->tex->width * st_s_w(sprite->frame);
    ss.Columns[1].Y = sprite->size.Y * sprite->tex->height * st_s_h(sprite->frame);
    HMM_Mat3 ts = HMM_M3D(1);
    ts.Columns[2] = (HMM_Vec3){sprite->pos.X, sprite->pos.Y, 1};

    HMM_Mat3 sm = HMM_MulM3(ss, ts);
    m = HMM_MulM3(m, sm);
    tex_draw(sprite->tex, m, sprite->frame, sprite->color, 0, (HMM_Vec2){0,0}, 0, sprite->emissive);
  }
}

void sprite_setanim(struct sprite *sprite, struct TexAnim *anim, int frame) {
  if (!sprite) return;
  sprite->tex = anim->tex;
  sprite->frame = anim->st_frames[frame];
}

void gui_draw_img(const char *img, HMM_Vec2 pos, HMM_Vec2 scale, float angle, int wrap, HMM_Vec2 wrapoffset, float wrapscale, struct rgba color) {
  sg_apply_pipeline(pip_sprite);
  sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, SG_RANGE_REF(hudproj));
  struct Texture *tex = texture_loadfromfile(img);
  transform2d t;
  t.pos = pos;
  t.angle = angle;
  t.scale = scale;
  tex_draw(tex, transform2d2mat(t), tex_get_rect(tex), color, wrap, wrapoffset, wrapscale, (struct rgba){0,0,0,0});
}

void slice9_draw(const char *img, HMM_Vec2 pos, HMM_Vec2 dimensions, struct rgba color)
{
  sg_apply_pipeline(slice9_pipe);
  sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, SG_RANGE_REF(hudproj));
  struct Texture *tex = texture_loadfromfile(img);

  struct glrect r = tex_get_rect(tex);

  struct slice9_vert verts[4];
  
  HMM_Vec2 sposes[4] = {
    {0.0,0.0},
    {1.0,0.0},
    {0.0,1.0},
    {1.0,1.0},
  };

  for (int i = 0; i < 4; i++) {
    verts[i].pos = HMM_MulV2(sposes[i], dimensions);
    //verts[i].uv =z sposes[i];
    verts[i].color = color;
  }

  verts[0].uv.u = r.s0 * USHRT_MAX;
  verts[0].uv.v = r.t1 * USHRT_MAX;
  verts[1].uv.u = r.s1 * USHRT_MAX;
  verts[1].uv.v = r.t1 * USHRT_MAX;
  verts[2].uv.u = r.s0 * USHRT_MAX;
  verts[2].uv.v = r.t0 * USHRT_MAX;
  verts[3].uv.u = r.s1 * USHRT_MAX;
  verts[3].uv.v = r.t0 * USHRT_MAX;

  bind_sprite.fs.images[0] = tex->id;
  sg_append_buffer(bind_sprite.vertex_buffers[0], SG_RANGE_REF(verts));
  sg_apply_bindings(&bind_sprite);

  sg_draw(sprite_count * 4, 4, 1);
  sprite_count++;
  
}

void sprite_setframe(struct sprite *sprite, struct glrect *frame) {
  sprite->frame = *frame;
}
void video_draw(struct datastream *ds, HMM_Vec2 pos, HMM_Vec2 size, float rotate, struct rgba color)
{
//  shader_use(vid_shader);
/*
  static mfloat_t model[16];
  memcpy(model, UNITMAT4, sizeof(UNITMAT4));
  mat4_translate_vec2(model, position);
  mat4_scale_vec2(model, size);
*/
//  shader_setmat4(vid_shader, "model", model);
//  shader_setvec3(vid_shader, "spriteColor", color);
  /*
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, stream->texture_y);
      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, stream->texture_cb);
      glActiveTexture(GL_TEXTURE2);
      glBindTexture(GL_TEXTURE_2D, stream->texture_cr);

      // TODO: video bind VAO
      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  */
}
