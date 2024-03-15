#include "sprite.h"

#include "gameobject.h"
#include "log.h"
#include "render.h"
#include "stb_ds.h"
#include "texture.h"
#include "HandmadeMath.h"

#include "sprite.sglsl.h"
#include "9slice.sglsl.h"

static sprite **sprites = NULL;

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

sprite *sprite_make()
{
  sprite *sp = calloc(sizeof(*sp), 1);
  sp->pos = (HMM_Vec2){0,0};
  sp->scale = (HMM_Vec2){1,1};
  sp->angle = 0;
  sp->color = color_white;
  sp->emissive = color_clear;
  sp->go = NULL;
  sp->tex = texture_from_file(NULL);
  sp->frame = ST_UNIT;
  sp->drawmode = DRAW_SIMPLE;
  sp->enabled = 1;
  sp->parallax = 1;
  
  arrpush(sprites,sp);
  
  return sp;
}

void sprite_free(sprite *sprite)
{
  YughWarn("Freeing sprite %p.", sprite); 
  
  free(sprite);
  for (int i = arrlen(sprites)-1; i >= 0; i--)
    if (sprites[i] == sprite) {
      arrdelswap(sprites,i);
      return;
    }
}

static int sprite_count = 0;

void sprite_flush() { sprite_count = 0; }

int sprite_sort(sprite **sa, sprite **sb)
{
  sprite *a = *sa;
  sprite *b = *sb;
  struct gameobject *goa = a->go;
  struct gameobject *gob= b->go;
  if (!goa && !gob) return 0;
  if (!goa) return -1;
  if (!gob) return 1;
  if (goa->drawlayer == gob->drawlayer) return 0;
  if (goa->drawlayer > gob->drawlayer) return 1;
  return -1;
}

void sprite_draw_all() {
  if (arrlen(sprites) == 0) return;
  
  sg_apply_pipeline(pip_sprite);
  sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, SG_RANGE_REF(projection));

  qsort(sprites, arrlen(sprites), sizeof(*sprites), sprite_sort);

  for (int i = 0; i < arrlen(sprites); i++)
    sprite_draw(sprites[i]);
}

void sprite_initialize() {

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

void tex_draw(struct texture *tex, HMM_Mat3 m, struct rect r, struct rgba color, int wrap, HMM_Vec2 wrapoffset, HMM_Vec2 wrapscale, struct rgba emissive, float parallax) {
  struct sprite_vert verts[4];
  float w = tex->width*r.w;
  float h = tex->height*r.h;
  
  HMM_Vec2 sposes[4] = {
    {0,0},
    {w,0},
    {0,h},
    {w,h}
  };

  for (int i = 0; i < 4; i++) {
    verts[i].pos = mat_t_pos(m, sposes[i]);
    verts[i].color = color;
    verts[i].emissive = emissive;
  }

  if (wrap) {
    r.w *= wrapscale.x;
    r.h *= wrapscale.y;
  }

  verts[0].uv.X = r.x;
  verts[0].uv.Y = r.y+r.h;
  verts[1].uv.X = r.x+r.w;
  verts[1].uv.Y = r.y+r.h;
  verts[2].uv.X = r.x;
  verts[2].uv.Y = r.y;
  verts[3].uv.X = r.x+r.w;
  verts[3].uv.Y = r.y;

  bind_sprite.fs.images[0] = tex->id;

  sg_append_buffer(bind_sprite.vertex_buffers[0], SG_RANGE_REF(verts));
  sg_apply_bindings(&bind_sprite);

  sg_draw(sprite_count * 4, 4, 1);
  sprite_count++;
}

transform2d sprite2t(sprite *s)
{
  return (transform2d){
    .pos = s->pos,
    .scale = s->scale,
    .angle = s->angle
  };
}

void sprite_draw(struct sprite *sprite) {
  if (!sprite->tex) return;
  transform2d t;
  if (!sprite->go) t = t2d_unit;
  else t = go2t(sprite->go);
  
  t.pos.x += (cam_pos().x - (cam_pos().x/sprite->parallax));
  t.pos.y += (cam_pos().y - (cam_pos().y/sprite->parallax));
  HMM_Mat3 m = transform2d2mat(t);
  HMM_Mat3 sm = transform2d2mat(sprite2t(sprite));
  tex_draw(sprite->tex, HMM_MulM3(m,sm), sprite->frame, sprite->color, sprite->drawmode, (HMM_Vec2){0,0}, sprite->scale, sprite->emissive, sprite->parallax);
}

void gui_draw_img(const char *img, transform2d t, int wrap, HMM_Vec2 wrapoffset, float wrapscale, struct rgba color) {
  sg_apply_pipeline(pip_sprite);
  sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, SG_RANGE_REF(hudproj));
  struct texture *tex = texture_from_file(img);
  tex_draw(tex, transform2d2mat(t), ST_UNIT, color, wrap, wrapoffset, (HMM_Vec2){wrapscale,wrapscale}, (struct rgba){0,0,0,0}, 0);
}
