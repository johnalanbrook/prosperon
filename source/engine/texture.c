#include "texture.h"

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stb_image.h>
#include <stb_ds.h>
#include <GL/glew.h>
#include "log.h"

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
    tex->path = malloc(strlen(path) + 1);
    strncpy(tex->path, path, strlen(path) + 1);
    tex->flipy = 0;
    tex->opts.sprite = 1;
    tex->opts.gamma = 0;
    tex->anim.frames = 1;
    tex->anim.ms = 1;

    int n;
    stbi_set_flip_vertically_on_load(0);
    unsigned char *data = stbi_load(path, &tex->width, &tex->height, &n, 4);

    if (stbi_failure_reason()) {
	YughLog(0, 3, "STBI failed to load file %s with message: %s",
		tex->path, stbi_failure_reason());

    }

    tex->data = data;

    shput(texhash, tex->path, tex);

    glGenTextures(1, &tex->id);

    return tex;
}

struct Texture *texture_loadfromfile(const char *path)
{
    struct Texture *new = texture_pullfromfile(path);

    tex_gpu_load(new);

    return new;
}



void tex_pull(struct Texture *tex)
{
    uint8_t n;
    stbi_set_flip_vertically_on_load(0);
    tex->data = stbi_load(tex->path, &tex->width, &tex->height, &n, 4);

    if (stbi_failure_reason())
	YughLog(0, 3, "STBI failed to load file %s with message: %s",
		tex->path, stbi_failure_reason());
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
    free(tex->path);
    free(tex);
}

void tex_gpu_load(struct Texture *tex)
{
    if (tex->anim.frames >= 0) {
	glBindTexture(GL_TEXTURE_2D, tex->id);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex->width, tex->height, 0,
		     GL_RGBA, GL_UNSIGNED_BYTE, tex->data);

	glGenerateMipmap(GL_TEXTURE_2D);

	if (tex->opts.sprite) {
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
			    GL_NEAREST);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
			    GL_NEAREST);
	} else {
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
			    GL_LINEAR_MIPMAP_LINEAR);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
			    GL_LINEAR);
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    } else {
	glBindTexture(GL_TEXTURE_2D_ARRAY, tex->id);
	glTexImage3D(GL_TEXTURE_2D_ARRAY,
		     0,
		     GL_RGBA,
		     tex->anim.dimensions[0],
		     tex->anim.dimensions[1],
		     tex->anim.frames, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	SDL_Surface *loadedSurface =
	    SDL_CreateRGBSurfaceFrom(tex->data, tex->width, tex->height,
				     32, (4 * tex->width),
				     0x000000ff, 0x0000ff00, 0x00ff0000,
				     0xff000000);

	SDL_Surface *tempSurface;
	tempSurface =
	    SDL_CreateRGBSurfaceWithFormat(0, tex->anim.dimensions[0],
					   tex->anim.dimensions[1], 32,
					   SDL_PIXELFORMAT_RGBA32);

	SDL_Rect srcRect;
	srcRect.x = 0;
	srcRect.y = 0;
	srcRect.w = tex->anim.dimensions[0];
	srcRect.h = tex->anim.dimensions[1];

	for (int i = 0; i < tex->anim.frames; i++) {
	    srcRect.x = i * srcRect.w;
	    SDL_FillRect(tempSurface, NULL, 0x000000);

	    TestSDLError(SDL_BlitSurface
			 (loadedSurface, &srcRect, tempSurface, NULL));

	    //SDL_UnlockSurface(tempSurface);
	    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i,
			    tex->anim.dimensions[0],
			    tex->anim.dimensions[1], 1, GL_RGBA,
			    GL_UNSIGNED_BYTE, tempSurface->pixels);
	}

	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER,
			GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER,
			GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S,
			GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T,
			GL_CLAMP_TO_EDGE);

	SDL_FreeSurface(loadedSurface);
	SDL_FreeSurface(tempSurface);
    }
}

Uint32 tex_incr_anim(Uint32 interval, struct TexAnimation *tex_anim)
{
    anim_incr(tex_anim);

    if (!tex_anim->tex->anim.loop
	&& tex_anim->frame == tex_anim->tex->anim.frames)
	anim_pause(tex_anim);


    return interval;
}

void anim_incr(struct TexAnimation *anim)
{
    anim->frame = (anim->frame + 1) % anim->tex->anim.frames;
    tex_anim_calc_uv(anim);
}

void anim_decr(struct TexAnimation *anim)
{
    anim->frame =
	(anim->frame + anim->tex->anim.frames -
	 1) % anim->tex->anim.frames;
    tex_anim_calc_uv(anim);
}

void tex_anim_set(struct TexAnimation *anim)
{
    if (anim->playing) {
	SDL_RemoveTimer(anim->timer);
	anim->timer =
	    SDL_AddTimer(floor(1000.f / anim->tex->anim.ms), tex_incr_anim,
			 anim);

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

unsigned int powof2(unsigned int num)
{
    if (num != 0) {
	num--;
	num |= (num >> 1);
	num |= (num >> 2);
	num |= (num >> 4);
	num |= (num >> 8);
	num |= (num >> 16);
	num++;
    }

    return num;
}

int ispow2(int num)
{
    return (num && !(num & (num - 1)));
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
    SDL_RemoveTimer(anim->timer);
    anim->timer =
	SDL_AddTimer(floor(1000.f / anim->tex->anim.ms), tex_incr_anim,
		     anim);
}

void anim_stop(struct TexAnimation *anim)
{
    if (!anim->playing)
	return;

    anim->playing = 0;
    anim->frame = 0;
    anim->pausetime = 0;
    SDL_RemoveTimer(anim->timer);
    tex_anim_calc_uv(anim);
}

void anim_pause(struct TexAnimation *anim)
{
    if (!anim->playing)
	return;

    anim->playing = 0;
    SDL_RemoveTimer(anim->timer);
}

void anim_fwd(struct TexAnimation *anim)
{
    anim_incr(anim);
}

void anim_bkwd(struct TexAnimation *anim)
{
    anim_decr(anim);
}
