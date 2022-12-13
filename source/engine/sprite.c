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

struct TextureOptions TEX_SPRITE = { 1, 0, 0 };

struct sprite *sprites;

static uint32_t quadVAO;

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

void sprite_loadanim(struct sprite *sprite, const char *path, struct Anim2D anim)
{
    sprite->tex = texture_loadfromfile(path);
    sprite->anim = anim;
    sprite->anim.timer = timer_make(sprite->anim.ms, &incrementAnimFrame, sprite);
/*
    sprite->tex = texture_loadanimfromfile(sprite->tex, path, sprite->anim.frames, sprite->anim.dimensions);
*/
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


// TODO: This should be done once for all sprites
void sprite_initialize()
{
    uint32_t VBO;
    float vertices[] = {
	// pos
	0.f, 0.f,
	1.0f, 0.0f,
	0.f, 1.f,
	1.f, 1.f
    };

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(quadVAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);


    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
}

void sprite_draw(struct sprite *sprite)
{
    if (sprite->tex != NULL) {

	//shader_use(spriteShader);


	mfloat_t model[16] = { 0.f };
	mfloat_t r_model[16] = { 0.f };
	mfloat_t s_model[16] = { 0.f };
	memcpy(model, UNITMAT4, sizeof(UNITMAT4));
	memcpy(r_model, UNITMAT4, sizeof(UNITMAT4));
	memcpy(s_model, UNITMAT4, sizeof(UNITMAT4));



	mfloat_t t_move[2] = { 0.f };
	mfloat_t t_scale[2] = { 0.f };

	t_scale[0] = sprite->size[0] * sprite->tex->width * sprite->go->scale;
	t_scale[1] = sprite->size[1] * sprite->tex->height * sprite->go->scale;

	t_move[0] = sprite->pos[0] * t_scale[0];
	t_move[1] = sprite->pos[1] * t_scale[1];

	mat4_translate_vec2(model, t_move);
	mat4_scale_vec2(model, t_scale);
	mat4_rotation_z(r_model, cpBodyGetAngle(sprite->go->body));

	mat4_multiply(model, r_model, model);

	cpVect pos = cpBodyGetPosition(sprite->go->body);
	t_move[0] = pos.x;
	t_move[1] = pos.y;
	mat4_translate_vec2(model, t_move);


	shader_setmat4(spriteShader, "model", model);
	shader_setvec3(spriteShader, "spriteColor", sprite->color);

	//tex_bind(sprite->tex);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, sprite->tex->id);

	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
}

void spriteanim_draw(struct sprite *sprite)
{
    shader_use(animSpriteShader);

    mfloat_t model[16] = { 0.f };
    memcpy(model, UNITMAT4, sizeof(UNITMAT4));

    mat4_translate_vec2(model, sprite->pos);
    mfloat_t msize[2] =
	{ sprite->size[0] * sprite->anim.dimensions[0],
	  sprite->size[1] * sprite->anim.dimensions[1] };
    mat4_scale_vec2(model, msize);

    shader_setmat4(animSpriteShader, "model", model);
    shader_setvec3(animSpriteShader, "spriteColor", sprite->color);
    shader_setfloat(animSpriteShader, "frame", sprite->anim.frame);


    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, sprite->tex->id);

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
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
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

