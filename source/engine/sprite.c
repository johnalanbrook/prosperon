#include "sprite.h"

#include "timer.h"
#include "render.h"
#include "openglrender.h"
#include "texture.h"
#include "shader.h"
#include "datastream.h"
#include "gameobject.h"
#include <string.h>
#include "stb_ds.h"
#include "log.h"
#include "font.h"

struct TextureOptions TEX_SPRITE = { 1, 0, 0 };

static struct sprite *sprites;
static int first = -1;

static uint32_t VBO;

int make_sprite(int go)
{
    struct sprite sprite = {
        .color = {1.f, 1.f, 1.f},
        .size = {1.f, 1.f},
        .tex = texture_loadfromfile(NULL),
        .go = go,
        .next = -1,
	.layer = 0,
        .enabled = 1  };

    if (first<0) {
        arrput(sprites, sprite);
        arrlast(sprites).id = arrlen(sprites)-1;
        return arrlen(sprites)-1;
    } else {
        int slot = first;
        first = id2sprite(first)->next;
        *id2sprite(slot) = sprite;

        return slot;
    }
}

void sprite_delete(int id)
{
    struct sprite *sp = id2sprite(id);
    sp->go = -1;
    sp->next = first;
    first = id;
}

void sprite_enabled(int id, int e)
{
    sprites[id].enabled = e;
}

struct sprite *id2sprite(int id) {
    if (id < 0) return NULL;
    return &sprites[id];
}

void sprite_io(struct sprite *sprite, FILE *f, int read)
{
    char path[100];
    if (read) {
        //fscanf(f, "%s", &path);
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

void sprite_draw_all()
{
    static struct sprite **layers[5];
    
    for (int i = 0; i < 5; i++)
      arrfree(layers[i]);
      
    for (int i = 0; i < arrlen(sprites); i++) {
        if (sprites[i].go >= 0 && sprites[i].enabled) arrpush(layers[sprites[i].layer], &sprites[i]);
    }

    for (int i = 4; i >= 0; i--)
      for (int j = 0; j < arrlen(layers[i]); j++)
        sprite_draw(layers[i][j]);
}

void sprite_loadtex(struct sprite *sprite, const char *path, struct glrect frame)
{
    sprite->tex = texture_loadfromfile(path);
    sprite_setframe(sprite, &frame);
}

void sprite_settex(struct sprite *sprite, struct Texture *tex)
{
    sprite->tex = tex;
    sprite_setframe(sprite, &ST_UNIT);
}

static uint32_t VAO = 0;

void sprite_initialize()
{
    glGenBuffers(1, &VBO);
    glGenVertexArrays(1, &VAO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(0);
}

void tex_draw(struct Texture *tex, float pos[2], float angle, float size[2], float offset[2], struct glrect r) {
	mfloat_t model[16] = { 0.f };
	mfloat_t r_model[16] = { 0.f };
	mfloat_t s_model[16] = { 0.f };
	memcpy(model, UNITMAT4, sizeof(UNITMAT4));
	memcpy(r_model, UNITMAT4, sizeof(UNITMAT4));
	memcpy(s_model, UNITMAT4, sizeof(UNITMAT4));

         mfloat_t t_scale[2] = { tex->width * st_s_w(r), tex->height * st_s_h(r) };
         mfloat_t t_offset[2] = { offset[0] * t_scale[0] * size[0], offset[1] * t_scale[1] * size[1]};

	mat4_translate_vec2(model, t_offset);

        mat4_scale_vec2(model, t_scale);
	mat4_rotation_z(r_model, angle);

	mat4_multiply(model, r_model, model);

	mat4_translate_vec2(model, pos);

         float white[3] = { 1.f, 1.f, 1.f };
	shader_setmat4(spriteShader, "model", model);
	shader_setvec3(spriteShader, "spriteColor", white);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex->id);
         glBindBuffer(GL_ARRAY_BUFFER, VBO);

         float vertices[] = {
             0.f, 0.f, r.s0, r.t1,
             size[0], 0.f, r.s1, r.t1,
             0.f, size[1], r.s0, r.t0,
             size[0], size[1], r.s1, r.t0
         };

         glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);

         glBindVertexArray(VAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindVertexArray(0);
}

void sprite_draw(struct sprite *sprite)
{
    struct gameobject *go = id2go(sprite->go);

    if (sprite->tex) {
        cpVect cpos = cpBodyGetPosition(go->body);
        float pos[2] = {cpos.x, cpos.y};
        float size[2] = { sprite->size[0] * go->scale * go->flipx, sprite->size[1] * go->scale * go->flipy };
        tex_draw(sprite->tex, pos, cpBodyGetAngle(go->body), size, sprite->pos, sprite->frame);
    }
}

void sprite_setanim(struct sprite *sprite, struct TexAnim *anim, int frame)
{
    if (!sprite) return;
    sprite->tex = anim->tex;
    sprite->frame = anim->st_frames[frame];
}

void gui_draw_img(const char *img, float x, float y) {
    shader_use(spriteShader);
    struct Texture *tex = texture_loadfromfile(img);
    float pos[2] = {x, y};
    float size[2] = {1.f, 1.f};
    float offset[2] = { 0.f, 0.f };
    tex_draw(tex, pos, 0.f, size, offset, tex_get_rect(tex));
}

void sprite_setframe(struct sprite *sprite, struct glrect *frame)
{
    sprite->frame = *frame;
}

void video_draw(struct datastream *stream, mfloat_t position[2], mfloat_t size[2], float rotate, mfloat_t color[3])
{
    shader_use(vid_shader);

    static mfloat_t model[16];
    memcpy(model, UNITMAT4, sizeof(UNITMAT4));
    mat4_translate_vec2(model, position);
    mat4_scale_vec2(model, size);

    shader_setmat4(vid_shader, "model", model);
    shader_setvec3(vid_shader, "spriteColor", color);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, stream->texture_y);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, stream->texture_cb);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, stream->texture_cr);

    // TODO: video bind VAO
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
