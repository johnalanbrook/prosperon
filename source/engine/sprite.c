#include "sprite.h"

#include "gameobject.h"
#include "log.h"
#include "render.h"
#include "stb_ds.h"
#include "texture.h"
#include "HandmadeMath.h"

#include "sprite.sglsl.h"
#include "9slice.sglsl.h"

static sg_shader shader_sprite;
static sg_pipeline pip_sprite;
sg_bindings bind_sprite;

static sg_shader slice9_shader;
static sg_pipeline slice9_pipe;
static sg_bindings slice9_bind;

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
  sp->pos = v2zero;
  sp->scale = v2one;
  sp->angle = 0;
  sp->color = color_white;
  sp->emissive = color_clear;
  sp->spritesize = v2one;
  sp->spriteoffset = v2zero;
  return sp;
}

void sprite_free(sprite *sprite) { free(sprite); }

static texture *loadedtex;
static int sprite_count = 0;

void sprite_initialize() {
  shader_sprite = sg_make_shader(sprite_shader_desc(sg_query_backend()));

  pip_sprite = sg_make_pipeline(&(sg_pipeline_desc){
    .shader = shader_sprite,
    .layout = {
      .attrs = {
        [0].format = SG_VERTEXFORMAT_FLOAT2
      },
    },
    .primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP,
    .label = "sprite pipeline",
    .colors[0].blend = blend_trans,
  });

  bind_sprite.vertex_buffers[0] = sprite_quad;
  bind_sprite.fs.samplers[0] = std_sampler;

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

  slice9_bind.vertex_buffers[0] = sprite_quad;
}

void sprite_pipe()
{
  sg_apply_pipeline(pip_sprite);
  sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vp, SG_RANGE_REF(useproj));
  sg_apply_bindings(&bind_sprite);
}

transform2d sprite2t(sprite *s)
{
  return (transform2d){
    .pos = s->pos,
    .scale = HMM_MulV2(s->scale, (HMM_Vec2){loadedtex->width, loadedtex->height}),
    .angle = HMM_TurnToRad*s->angle
  };
}

void sprite_tex(texture *t)
{
  loadedtex = t;
  bind_sprite.fs.images[0] = t->id;
}

void sprite_draw(struct sprite *sprite, gameobject *go) {
  HMM_Mat4 m = transform2d2mat4(go2t(go));
  HMM_Mat4 sm = transform2d2mat4(sprite2t(sprite));
  struct spriteuni spv;
  rgba2floats(&spv.color.e, sprite->color);
  rgba2floats(spv.emissive.e, sprite->emissive);
  spv.size = sprite->spritesize;
  spv.offset = sprite->spriteoffset;
  spv.model = HMM_MulM4(m,sm);
  sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_sprite, SG_RANGE_REF(spv));  
  sg_draw(0,4,1);
}

void gui_draw_img(texture *tex, transform2d t, int wrap, HMM_Vec2 wrapoffset, float wrapscale, struct rgba color) {
  sg_apply_pipeline(pip_sprite);
  sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vp, SG_RANGE_REF(useproj));
  sprite_tex(tex);
  //tex_draw(transform2d2mat(t), ST_UNIT, color, wrap, wrapoffset, (HMM_Vec2){wrapscale,wrapscale}, (struct rgba){0,0,0,0});
}

void slice9_draw(texture *tex, transform2d *t, HMM_Vec4 border, struct rgba color)
{
  
}