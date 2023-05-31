#include "sprite.h"

#include "datastream.h"
#include "font.h"
#include "gameobject.h"
#include "log.h"
#include "openglrender.h"
#include "render.h"
#include "shader.h"
#include "stb_ds.h"
#include "texture.h"
#include "timer.h"
#include <string.h>

struct TextureOptions TEX_SPRITE = {1, 0, 0};

static struct sprite *sprites;
static int first = -1;

static sg_pipeline pip_sprite;
static sg_bindings bind_sprite;

int make_sprite(int go) {
  struct sprite sprite = {
      .color = color_white,
      .size = {1.f, 1.f},
      .tex = texture_loadfromfile(NULL),
      .go = go,
      .next = -1,
      .layer = 0,
      .enabled = 1};

  if (first < 0) {
    arrput(sprites, sprite);
    arrlast(sprites).id = arrlen(sprites) - 1;
    return arrlen(sprites) - 1;
  } else {
    int slot = first;
    first = id2sprite(first)->next;
    *id2sprite(slot) = sprite;

    return slot;
  }
}

void sprite_delete(int id) {
  struct sprite *sp = id2sprite(id);
  sp->go = -1;
  sp->next = first;
  first = id;
}

void sprite_enabled(int id, int e) {
  sprites[id].enabled = e;
}

struct sprite *id2sprite(int id) {
  if (id < 0) return NULL;
  return &sprites[id];
}

static sprite_count = 0;

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

void sprite_draw_all() {
  sg_apply_pipeline(pip_sprite);

  static struct sprite **layers[5];

  for (int i = 0; i < 5; i++)
    arrfree(layers[i]);

  for (int i = 0; i < arrlen(sprites); i++)
    if (sprites[i].go >= 0 && sprites[i].enabled) arrpush(layers[sprites[i].layer], &sprites[i]);

  for (int i = 4; i >= 0; i--)
    for (int j = 0; j < arrlen(layers[i]); j++)
      sprite_draw(layers[i][j]);
}

void sprite_loadtex(struct sprite *sprite, const char *path, struct glrect frame) {
  sprite->tex = texture_loadfromfile(path);
  sprite_setframe(sprite, &frame);
}

void sprite_settex(struct sprite *sprite, struct Texture *tex) {
  sprite->tex = tex;
  sprite_setframe(sprite, &ST_UNIT);
}

sg_shader shader_sprite;

void sprite_initialize() {
  shader_sprite = sg_compile_shader("shaders/spritevert.glsl", "shaders/spritefrag.glsl", &(sg_shader_desc){
      .vs.uniform_blocks[0] = {
          .size = 64,
          .layout = SG_UNIFORMLAYOUT_STD140,
          .uniforms = {[0] = {.name = "mpv", .type = SG_UNIFORMTYPE_MAT4}}},

      .fs.images[0] = {
          .name = "image",
          .image_type = SG_IMAGETYPE_2D,
          .sampler_type = SG_SAMPLERTYPE_FLOAT,
      },

      .fs.uniform_blocks[0] = {.size = 12, .uniforms = {[0] = {.name = "spriteColor", .type = SG_UNIFORMTYPE_FLOAT3}}}});

  pip_sprite = sg_make_pipeline(&(sg_pipeline_desc){
      .shader = shader_sprite,
      .layout = {
          .attrs = {
              [0].format = SG_VERTEXFORMAT_FLOAT4}},
      .primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP,
      .label = "sprite pipeline",
/*      .depth = {
        .write_enabled = true,
	.compare = SG_COMPAREFUNC_LESS_EQUAL
      }
  */
  });

  bind_sprite.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
      .size = sizeof(float) * 16 * 500,
      .type = SG_BUFFERTYPE_VERTEXBUFFER,
      .usage = SG_USAGE_STREAM,
      .label = "sprite vertex buffer",
  });
}

void tex_draw(struct Texture *tex, HMM_Vec2 pos, float angle, HMM_Vec2 size, HMM_Vec2 offset, struct glrect r, struct rgba color) {
  HMM_Mat4 model = HMM_M4D(1.0);
  HMM_Mat4 r_model = HMM_M4D(1.0);

  HMM_Vec3 t_scale = {
    tex->width * st_s_w(r) * size.X,
    tex->height * st_s_h(r) * size.Y,
    t_scale.Z = 1.0};
  
  HMM_Vec3 t_offset = {
    offset.X * t_scale.X,
    offset.Y * t_scale.Y,
    0.0};

  HMM_Translate_p(&model, t_offset);
  HMM_Scale_p(&model, t_scale);
  r_model = HMM_Rotate_RH(angle, vZ);
  model = HMM_MulM4(r_model, model);
  HMM_Translate_p(&model, (HMM_Vec3){pos.X, pos.Y, 0.0});

  model = HMM_MulM4(projection, model);

  float vertices[] = {
      0.f, 0.f, r.s0, r.t1,
      1, 0.f, r.s1, r.t1,
      0.f, 1, r.s0, r.t0,
      1.f, 1.f, r.s1, r.t0};

  bind_sprite.fs_images[0] = tex->id;
  sg_append_buffer(bind_sprite.vertex_buffers[0], SG_RANGE_REF(vertices));
  sg_apply_bindings(&bind_sprite);
  sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, SG_RANGE_REF(model.Elements));

  float cl[3] = {
    color.r / 255.0,
    color.g / 255.0,
    color.b / 255.0
  };

  sg_apply_uniforms(SG_SHADERSTAGE_FS, 0, SG_RANGE_REF(cl));

  sg_draw(sprite_count * 4, 4, 1);
  sprite_count++;
}

void sprite_draw(struct sprite *sprite) {
  struct gameobject *go = id2go(sprite->go);

  if (sprite->tex) {
    cpVect cpos = cpBodyGetPosition(go->body);
    HMM_Vec2 pos = {cpos.x, cpos.y};
    HMM_Vec2 size = {sprite->size.X * go->scale * go->flipx, sprite->size.Y * go->scale * go->flipy};
    tex_draw(sprite->tex, pos, cpBodyGetAngle(go->body), size, sprite->pos, sprite->frame, sprite->color);
  }
}

void sprite_setanim(struct sprite *sprite, struct TexAnim *anim, int frame) {
  if (!sprite) return;
  sprite->tex = anim->tex;
  sprite->frame = anim->st_frames[frame];
}

void gui_draw_img(const char *img, HMM_Vec2 pos, float scale, float angle) {
  sg_apply_pipeline(pip_sprite);
  struct Texture *tex = texture_loadfromfile(img);
  HMM_Vec2 size = {scale, scale};
  HMM_Vec2 offset = {0.f, 0.f};
  tex_draw(tex, pos, 0.f, size, offset, tex_get_rect(tex), color_white);
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
