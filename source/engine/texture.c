#include "texture.h"

#include "render.h"
#include <stdio.h>
#include <stb_image.h>
#include <stb_ds.h>
#include "log.h"
#include <math.h>
#include "util.h"
#include "parson.h"

struct glrect ST_UNIT = { 0.f, 1.f, 0.f, 1.f };

static struct {
    char *key;
    struct Texture *value;
} *texhash = NULL;

struct Texture *tex_default;

struct Texture *texture_pullfromfile(const char *path)
{
    int index = shgeti(texhash, path);
    if (index != -1)
	return texhash[index].value;

    struct Texture *tex = calloc(1, sizeof(*tex));

    /* Find texture's asset; otherwise load default asset */
    JSON_Value *rv = json_parse_file("texture.asset");
    JSON_Object *ro = json_value_get_object(rv);
    tex->opts.sprite = json_object_get_boolean(ro, "sprite");
    tex->opts.mips = json_object_get_boolean(ro, "mips");
    json_value_free(rv);

    tex->opts.gamma = 0;


    tex->anim.ms = 2;
    tex->anim.tex = tex;
    texanim_fromframes(&tex->anim, 2);
    tex->anim.loop = 1;

    int n;

    unsigned char *data = stbi_load(path, &tex->width, &tex->height, &n, 4);

    while (data == NULL) {
        YughError("STBI failed to load file %s with message: %s", path, stbi_failure_reason());
        return NULL;
    }

         glGenTextures(1, &tex->id);

    	glBindTexture(GL_TEXTURE_2D, tex->id);

    	GLenum fmt;

    	switch (n) {
    	    case 1:
    	        fmt = GL_RED;
    	        break;

    	    case 2:
    	        fmt = GL_RG;
    	        break;

    	    case 3:
    	        fmt = GL_RGB;
    	        break;

    	    case 4:
    	        fmt = GL_RGBA;
    	        break;
    	}

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex->width, tex->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

         if (tex->opts.mips)
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




    stbi_image_free(data);

    if (shlen(texhash) == 0)
        sh_new_arena(texhash);

    shput(texhash, path, tex);

    return tex;
}

char *tex_get_path(struct Texture *tex) {
    for (int i = 0; i < shlen(texhash); i++) {
        if (tex == texhash[i].value) {
            YughInfo("Found key %s", texhash[i].key);
            return texhash[i].key;
        }
    }

    return NULL;
}

struct Texture *texture_loadfromfile(const char *path)
{
    struct Texture *new = texture_pullfromfile(path);

    if (new == NULL) {
        YughError("Texture %s not loaded! Loading the default instead ...", path);
        new = texture_pullfromfile("./ph.png");
    }

    if (new->id == 0) {
        glGenTextures(1, &new->id);

        //tex_gpu_load(new);

        YughInfo("Loaded texture path %s", path);
    }

    return new;
}

void tex_gpu_reload(struct Texture *tex)
{
    tex_gpu_free(tex);

    //tex_gpu_load(tex);
}

void anim_calc(struct anim2d *anim)
{
    anim->size[0] = anim->anim->tex->width * st_s_w(anim->anim->st_frames[anim->frame]);
    anim->size[1] = anim->anim->tex->height * st_s_h(anim->anim->st_frames[anim->frame]);
}

void anim_incr(struct anim2d *anim)
{
    anim->frame = (anim->frame + 1) % arrlen(anim->anim->st_frames);

    if (!anim->anim->loop && anim->frame == arrlen(anim->anim->st_frames))
        anim_pause(anim);

    anim_calc(anim);
}

void anim_decr(struct anim2d *anim)
{
    anim->frame = (anim->frame + arrlen(anim->anim->st_frames) - 1) % arrlen(anim->anim->st_frames);
    anim_calc(anim);
}

struct glrect anim_get_rect(struct anim2d *anim)
{
    return anim->anim->st_frames[anim->frame];
}

void anim_setframe(struct anim2d *anim, int frame)
{
    anim->frame = frame;
    anim_calc(anim);
}

void texanim_fromframes(struct TexAnim *anim, int frames)
{
    if (anim->st_frames) free(anim->st_frames);

    arrsetlen(anim->st_frames, frames);

    float width = (float)1/frames;

    for (int i = 0; i < frames; i++) {
        anim->st_frames[i].s0 = width*i;
        anim->st_frames[i].s1 = width*(i+1);
        anim->st_frames[i].t0 = 0.f;
        anim->st_frames[i].t1 = 1.f;
    }
}

void tex_gpu_free(struct Texture *tex)
{
    if (tex->id != 0) {
	glDeleteTextures(1, &tex->id);
	tex->id = 0;
    }
}

int anim_frames(struct TexAnim *a)
{
    return arrlen(a->st_frames);
}

struct glrect tex_get_rect(struct Texture *tex)
{
    return ST_UNIT;
}

void tex_bind(struct Texture *tex)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex->id);
    glBindTexture(GL_TEXTURE_2D_ARRAY, tex->id);
}

/********************** ANIM2D ****************/

void anim_load(struct anim2d *anim, const char *path)
{
    anim->anim = &texture_pullfromfile(path)->anim;
    anim->anim->tex->opts.animation = 1;
    anim_stop(anim);
    anim_play(anim);
}

void anim_play(struct anim2d *anim)
{
    if (anim->playing)
	return;

    if (anim->frame == anim_frames(anim->anim))
	anim->frame = 0;

    anim->playing = 1;

    if (anim->timer == NULL)
        anim->timer = timer_make(1.f / anim->anim->ms, anim_incr, anim);
    else
        timerr_settime(anim->timer, 1.f/anim->anim->ms);

    timer_start(anim->timer);
}

void anim_stop(struct anim2d *anim)
{
    if (!anim->playing)
	return;

    anim->playing = 0;
    anim->frame = 0;
    anim->pausetime = 0;
    timer_stop(anim->timer);
}

void anim_pause(struct anim2d *anim)
{
    if (!anim->playing)
	return;

    anim->playing = 0;
    timer_pause(anim->timer);
}

void anim_fwd(struct anim2d *anim)
{
    anim_incr(anim);
}

void anim_bkwd(struct anim2d *anim)
{
    anim_decr(anim);
}

float st_s_w(struct glrect st)
{
    return (st.s1 - st.s0);
}

float st_s_h(struct glrect st)
{
    return (st.t1 - st.t0);
}