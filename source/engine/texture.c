#include "texture.h"

#include "render.h"
#include <stdio.h>
#include <stb_image.h>
#include <stb_ds.h>
#include "log.h"
#include <math.h>
#include "util.h"

static struct {
    char *key;
    struct Texture *value;
} *texhash = NULL;

struct Texture *texture_pullfromfile(const char *path)
{
    int index = shgeti(texhash, path);
    if (index != -1)
	return texhash[index].value;

    struct Texture *tex = calloc(1, sizeof(*tex));
    tex->flipy = 0;
    tex->opts.sprite = 1;
    tex->opts.gamma = 0;
    tex->anim.frames = 1;
    tex->anim.ms = 1;

    int n;
    stbi_set_flip_vertically_on_load(0);
    unsigned char *data = stbi_load(path, &tex->width, &tex->height, &n, 4);

    if (stbi_failure_reason())
	YughLog(0, 3, "STBI failed to load file %s with message: %s", path, stbi_failure_reason());

    tex->data = data;

    shput(texhash, path, tex);

    return tex;
}

char *tex_get_path(struct Texture *tex) {
    for (int i = 0; i < shlen(texhash); i++) {
        if (tex == texhash[i].value)
            return texhash[i].key;
    }

    return NULL;
}

struct Texture *texture_loadfromfile(const char *path)
{
    struct Texture *new = texture_pullfromfile(path);

    glGenTextures(1, &new->id);

    tex_gpu_load(new);

    return new;
}



void tex_pull(struct Texture *tex)
{
    if (tex->data != NULL)
        tex_flush(tex);

    int n;
    char *path = tex_get_path(tex);
    stbi_set_flip_vertically_on_load(0);
    tex->data = stbi_load(path, &tex->width, &tex->height, &n, 4);

    if (stbi_failure_reason())
	YughLog(0, 3, "STBI failed to load file %s with message: %s", path, stbi_failure_reason());
}

void tex_flush(struct Texture *tex)
{
    free(tex->data);
}

void tex_gpu_reload(struct Texture *tex)
{
    tex_gpu_free(tex);

    tex_gpu_load(tex);
}

void tex_free(struct Texture *tex)
{
    free(tex->data);
    //free(tex->path);
    free(tex);
}

void tex_gpu_load(struct Texture *tex)
{
	glBindTexture(GL_TEXTURE_2D, tex->id);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex->width, tex->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex->data);

	glGenerateMipmap(GL_TEXTURE_2D);

	if (tex->opts.sprite) {
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	} else {
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void tex_incr_anim(struct TexAnimation *tex_anim)
{
    anim_incr(tex_anim);

    if (!tex_anim->tex->anim.loop && tex_anim->frame == tex_anim->tex->anim.frames)
	anim_pause(tex_anim);
}

void anim_incr(struct TexAnimation *anim)
{
    anim->frame = (anim->frame + 1) % anim->tex->anim.frames;
    tex_anim_calc_uv(anim);
}

void anim_decr(struct TexAnimation *anim)
{
    anim->frame = (anim->frame + anim->tex->anim.frames - 1) % anim->tex->anim.frames;
    tex_anim_calc_uv(anim);
}

void anim_setframe(struct TexAnimation *anim, int frame)
{
    anim->frame = frame;
    tex_anim_calc_uv(anim);
}

void tex_anim_set(struct TexAnimation *anim)
{
    if (anim->playing) {
	timer_remove(anim->timer);
	anim->timer = timer_make(1.f / anim->tex->anim.ms, tex_incr_anim, anim);

    }

    tex_anim_calc_uv(anim);
}



void tex_gpu_free(struct Texture *tex)
{
    if (tex->id != 0) {
	glDeleteTextures(1, &tex->id);
	tex->id = 0;
    }
}

void tex_anim_calc_uv(struct TexAnimation *anim)
{
    struct Rect uv;
    uv.w = 1.f / anim->tex->anim.frames;
    uv.h = 1.f;
    uv.y = 0.f;
    uv.x = uv.w * (anim->frame);

    anim->uv = uv;
}



void tex_bind(struct Texture *tex)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex->id);
    glBindTexture(GL_TEXTURE_2D_ARRAY, tex->id);
}

void anim_play(struct TexAnimation *anim)
{
    if (anim->playing)
	return;

    if (anim->frame == anim->tex->anim.frames)
	anim->frame = 0;

    anim->playing = 1;

    if (anim->timer == NULL)
        anim->timer = timer_make(1.f / anim->tex->anim.ms, tex_incr_anim, anim);
    else
        timer_settime(anim->timer, 1.f/anim->tex->anim.ms);

    timer_start(anim->timer);
}

void anim_stop(struct TexAnimation *anim)
{
    if (!anim->playing)
	return;

    anim->playing = 0;
    anim->frame = 0;
    anim->pausetime = 0;
    timer_stop(anim->timer);
    tex_anim_calc_uv(anim);
}

void anim_pause(struct TexAnimation *anim)
{
    if (!anim->playing)
	return;

    anim->playing = 0;
    timer_pause(anim->timer);
}

void anim_fwd(struct TexAnimation *anim)
{
    anim_incr(anim);
}

void anim_bkwd(struct TexAnimation *anim)
{
    anim_decr(anim);
}
