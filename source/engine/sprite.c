#include "sprite.h"


#include "timer.h"
#include "render.h"
#include "openglrender.h"
#include "texture.h"
#include "shader.h"
#include "datastream.h"
#include "gameobject.h"
#include <string.h>
#include "vec.h"


static struct mGameObject *gui_go = NULL;


/*
static struct mShader *spriteShader = NULL;

static struct mShader *animSpriteShader = NULL;
*/

struct TextureOptions TEX_SPRITE = { 1, 0, 0 };

struct vec sprites;

static uint32_t quadVAO;

struct mSprite *MakeSprite(struct mGameObject *go)
{
    // TODO: Init this once and never check again
    if (sprites.data == NULL) sprites = vec_init(sizeof(struct mSprite), 10);

    struct mSprite *sprite = vec_add(&sprites, NULL);
    sprite->color[0] = 1.f;
    sprite->color[1] = 1.f;
    sprite->color[2] = 1.f;
    sprite->pos[0] = 0.f;
    sprite->pos[1] = 0.f;
    sprite->size[0] = 1.f;
    sprite->size[1] = 1.f;
    sprite->tex = texture_loadfromfile("ph.png");
    sprite_init(sprite, go);
    sprite->index = sprites.last;
    return sprite;
}

void sprite_init(struct mSprite *sprite, struct mGameObject *go)
{
    sprite->go = go;
}

void sprite_draw_all()
{
    shader_use(spriteShader);
    vec_walk(&sprites, sprite_draw);
}

void sprite_loadtex(struct mSprite *sprite, const char *path)
{
    sprite->tex = texture_loadfromfile(path);
}

void sprite_loadanim(struct mSprite *sprite, const char *path, struct Anim2D anim)
{
    sprite->tex = texture_loadfromfile(path);
    sprite->anim = anim;
    sprite->anim.timer = timer_make(sprite->anim.ms, &incrementAnimFrame, sprite);
/*
    sprite->tex = texture_loadanimfromfile(sprite->tex, path, sprite->anim.frames, sprite->anim.dimensions);
*/
}

void sprite_settex(struct mSprite *sprite, struct Texture *tex)
{
    sprite->tex = tex;
}

unsigned int incrementAnimFrame(unsigned int interval, struct mSprite *sprite)
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


    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void sprite_draw(struct mSprite *sprite)
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

void spriteanim_draw(struct mSprite *sprite)
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

void gui_init()
{
    gui_go = MakeGameobject();
}

struct mSprite *gui_makesprite()
{
    struct mSprite *new = MakeSprite(gui_go);
    return new;
}

