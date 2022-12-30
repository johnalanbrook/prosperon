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

struct sprite *sprites;

static uint32_t VBO;

struct sprite *make_sprite(struct gameobject *go)
{
    if (arrcap(sprites) == 0)
        arrsetcap(sprites, 100);

    struct sprite sprite = {
        .color = {1.f, 1.f, 1.f},
        .size = {1.f, 1.f},
        .tex = texture_loadfromfile("ph.png"),
        .index = arrlen(sprites)    };
    sprite_init(&sprite, go);
    arrput(sprites, sprite);

    return &arrlast(sprites);
}

void sprite_init(struct sprite *sprite, struct gameobject *go)
{
    sprite->go = go;

    YughInfo("Added sprite address %p to sprite array %p.", sprite, &arrlast(sprites));
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
        sprite_loadtex(sprite, path);
    } else {
        fputs(tex_get_path(sprite->tex), f);
        fputc('\0', f);
        fwrite(sprite, sizeof(*sprite), 1, f);
    }
}

void sprite_delete(struct sprite *sprite)
{
    YughInfo("Attempting to delete sprite, address is %p.", sprite);
    YughInfo("Number of sprites is %d.", arrlen(sprites));
    for (int i = 0; i < arrlen(sprites); i++) {
        YughInfo("Address of try sprite is %p.", &sprites[i]);
        if (&sprites[i] == sprite) {
            YughInfo("Deleted a sprite.");
            arrdel(sprites, i);
            return;
        }
    }
}

void sprite_draw_all()
{
    //shader_use(spriteShader);
    for (int i = 0; i < arrlen(sprites); i++)
        sprite_draw(&sprites[i]);
}

void sprite_loadtex(struct sprite *sprite, const char *path)
{
    sprite->tex = texture_loadfromfile(path);
}

void sprite_settex(struct sprite *sprite, struct Texture *tex)
{
    sprite->tex = tex;
}

unsigned int incrementAnimFrame(unsigned int interval, struct sprite *sprite)
{
    sprite->anim.frame = (sprite->anim.frame + 1) % sprite->anim.frames;
    return interval;
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

struct glrect sprite_get_rect(struct sprite *sprite) {
    if (sprite->tex->opts.animation) {

    } else {
        return tex_get_rect(sprite->tex);
    }
}

void tex_draw(struct Texture *tex, float pos[2], float angle, float size[2], float offset[2], struct glrect r) {
	mfloat_t model[16] = { 0.f };
	mfloat_t r_model[16] = { 0.f };
	mfloat_t s_model[16] = { 0.f };
	memcpy(model, UNITMAT4, sizeof(UNITMAT4));
	memcpy(r_model, UNITMAT4, sizeof(UNITMAT4));
	memcpy(s_model, UNITMAT4, sizeof(UNITMAT4));

         mfloat_t t_scale[2] = { size[0] * tex->width, size[1] * tex->height };
         mfloat_t t_offset[2] = { offset[0] * t_scale[0], offset[1] * t_scale[1] };

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
             1.f, 0.f, r.s1, r.t1,
             0.f, 1.f, r.s0, r.t0,
             1.f, 1.f, r.s1, r.t0
         };

         glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);

         glBindVertexArray(VAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindVertexArray(0);
}

void sprite_draw(struct sprite *sprite)
{
    if (sprite->tex) {
        cpVect cpos = cpBodyGetPosition(sprite->go->body);
        float pos[2] = {cpos.x, cpos.y};

        if (sprite->tex->opts.animation) {
            float size[2];
            struct Anim2D *a = &sprite->anim;
            a->frames = sprite->tex->anim.frames;
            size[0] = sprite->tex->anim.dimensions[0];
            size[1] = sprite->tex->anim.dimensions[1];
            tex_draw(sprite->tex, pos, cpBodyGetAngle(sprite->go->body), size, sprite->pos, tex_get_rect(sprite->tex));
        } else {
            float size[2] = { sprite->size[0] * sprite->go->scale, sprite->size[1] * sprite->go->scale };

            tex_draw(sprite->tex, pos, cpBodyGetAngle(sprite->go->body), size, sprite->pos, tex_get_rect(sprite->tex));
        }
    }
}

void gui_draw_img(const char *img, float x, float y) {
    shader_use(spriteShader);
    struct Texture *tex = texture_loadfromfile(img);
    float pos[2] = {x, y};
    float size[2] = {1.f, 1.f};
    float offset[2] = { 0.f, 0.f };
    tex_draw(tex, pos, 0.f, size, offset, tex_get_rect(tex));
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

