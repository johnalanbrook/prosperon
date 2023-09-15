#include "texture.h"

#include "log.h"
#include "render.h"
#include "sokol/sokol_gfx.h"
#include <math.h>
#include <stb_ds.h>
#include <stb_image.h>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

#include <stdio.h>

#define QOI_IMPLEMENTATION
#include "qoi.h"

struct glrect ST_UNIT = {0.f, 1.f, 0.f, 1.f};

static struct {
  char *key;
  struct Texture *value;
} *texhash = NULL;

struct Texture *tex_default;

struct Texture *texture_notex() {
  return texture_pullfromfile("./icons/no_tex.png");
}

unsigned int next_pow2(unsigned int v)
{
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v++;
  return v;
}

int mip_levels(int width, int height)
{
  int levels = 0;
	
  while (width > 1 || height > 1)
  {
    width >>= 1;
    height >>= 1;
    levels++;
  }
  return levels;
}

int mip_wh(int w, int h, int *mw, int *mh, int lvl)
{
  w >>= lvl;
  h >>= lvl;

  if (w == 0 && h == 0)
    return 1;

  *mw = w ? w : 1;
  *mh = h ? h : 1;

  return 0;
}

/* If an empty string or null is put for path, loads default texture */
struct Texture *texture_pullfromfile(const char *path) {
  if (!path) return texture_notex();

  int index = shgeti(texhash, path);
  if (index != -1)
    return texhash[index].value;

  YughInfo("Loading texture %s.", path);

  struct Texture *tex = calloc(1, sizeof(*tex));
  tex->opts.sprite = 1;
  tex->opts.mips = 0;
  tex->opts.gamma = 0;
  tex->opts.wrapx = 1;
  tex->opts.wrapy = 1;

  int n;

  unsigned char *data;

  char *ext = strrchr(path, '.');

  if (ext && !strcmp(ext, ".qoi")) {
    qoi_desc qoi;
    data = qoi_read(path, &qoi, 4);
    tex->width = qoi.width;
    tex->height = qoi.height;
    n = qoi.channels;
  } else {
    data = stbi_load(path, &tex->width, &tex->height, &n, 4);
  }

  if (data == NULL) {
    YughError("STBI failed to load file %s with message: %s\nOpening default instead.", path, stbi_failure_reason());
    return texture_notex();
  }

  unsigned int nw = next_pow2(tex->width);
  unsigned int nh = next_pow2(tex->height);
  
  tex->data = data;

  int filter;
  if (tex->opts.sprite) {
      filter = SG_FILTER_NEAREST;
  } else {
      filter = SG_FILTER_LINEAR;
  }
  
  sg_image_data sg_img_data;
  
  int mips = mip_levels(tex->width, tex->height)+1;

  YughInfo("Has %d mip levels, from wxh %dx%d, pow2 is %ux%u.", mips, tex->width, tex->height,nw,nh);
  
  int mipw, miph;
  mipw = tex->width;
  miph = tex->height;
  
  sg_img_data.subimage[0][0] = (sg_range){ .ptr = data, .size = mipw*miph*4 };  
  
  unsigned char *mipdata[mips];
  mipdata[0] = data;
    
  for (int i = 1; i < mips; i++) {
    int w, h, mipw, miph;
    mip_wh(tex->width, tex->height, &mipw, &miph, i-1); /* mipw miph are previous iteration */
    mip_wh(tex->width, tex->height, &w, &h, i);
    mipdata[i] = malloc(w * h * 4);
    stbir_resize_uint8(mipdata[i-1], mipw, miph, 0, mipdata[i], w, h, 0, 4);
    sg_img_data.subimage[0][i] = (sg_range){ .ptr = mipdata[i], .size = w*h*4 };
    
    mipw = w;
    miph = h;
  }

  tex->id = sg_make_image(&(sg_image_desc){
      .type = SG_IMAGETYPE_2D,
      .width = tex->width,
      .height = tex->height,
      .usage = SG_USAGE_IMMUTABLE,
      .num_mipmaps = mips,
      .data = sg_img_data
    });

  if (shlen(texhash) == 0)
    sh_new_arena(texhash);

  shput(texhash, path, tex);

  return tex;
}

void texture_sync(const char *path) {
  YughWarn("Need to implement texture sync.");
}

char *tex_get_path(struct Texture *tex) {
  for (int i = 0; i < shlen(texhash); i++) {
    if (tex == texhash[i].value) {
      YughInfo("Found key %s", texhash[i].key);
      return texhash[i].key;
    }
  }

  return "";
}

struct Texture *texture_loadfromfile(const char *path) {
  struct Texture *new = texture_pullfromfile(path);
  /*
      if (new->id == 0) {
          glGenTextures(1, &new->id);

          //tex_gpu_load(new);

          YughInfo("Loaded texture path %s", path);
      }
  */
  return new;
}

void tex_gpu_reload(struct Texture *tex) {
  tex_gpu_free(tex);

  // tex_gpu_load(tex);
}

void anim_calc(struct anim2d *anim) {
  anim->size[0] = anim->anim->tex->width * st_s_w(anim->anim->st_frames[anim->frame]);
  anim->size[1] = anim->anim->tex->height * st_s_h(anim->anim->st_frames[anim->frame]);
}

void anim_incr(struct anim2d *anim) {
  anim->frame = (anim->frame + 1) % arrlen(anim->anim->st_frames);

  if (!anim->anim->loop && anim->frame == arrlen(anim->anim->st_frames))
    anim_pause(anim);

  anim_calc(anim);
}

void anim_decr(struct anim2d *anim) {
  anim->frame = (anim->frame + arrlen(anim->anim->st_frames) - 1) % arrlen(anim->anim->st_frames);
  anim_calc(anim);
}

struct glrect anim_get_rect(struct anim2d *anim) {
  return anim->anim->st_frames[anim->frame];
}

void anim_setframe(struct anim2d *anim, int frame) {
  anim->frame = frame;
  anim_calc(anim);
}

struct TexAnim *anim2d_from_tex(const char *path, int frames, int fps) {
  struct TexAnim *anim = malloc(sizeof(*anim));
  anim->tex = texture_loadfromfile(path);
  texanim_fromframes(anim, frames);
  anim->ms = (float)1 / fps;

  return anim;
}

void texanim_fromframes(struct TexAnim *anim, int frames) {
  if (anim->st_frames) {
    free(anim->st_frames);
  }

  arrsetlen(anim->st_frames, frames);

  float width = (float)1 / frames;

  for (int i = 0; i < frames; i++) {
    anim->st_frames[i].s0 = width * i;
    anim->st_frames[i].s1 = width * (i + 1);
    anim->st_frames[i].t0 = 0.f;
    anim->st_frames[i].t1 = 1.f;
  }
}

void tex_gpu_free(struct Texture *tex) {
  /*
      if (tex->id != 0) {
          glDeleteTextures(1, &tex->id);
          tex->id = 0;
      }
  */
}

int anim_frames(struct TexAnim *a) {
  return arrlen(a->st_frames);
}

struct glrect tex_get_rect(struct Texture *tex) {
  return ST_UNIT;
}

cpVect tex_get_dimensions(struct Texture *tex) {
  if (!tex) return cpvzero;
  cpVect d;
  d.x = tex->width;
  d.y = tex->height;
  return d;
}

void tex_bind(struct Texture *tex) {
  /*    glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, tex->id);
      glBindTexture(GL_TEXTURE_2D_ARRAY, tex->id);
  */
}

/********************** ANIM2D ****************/

void anim_load(struct anim2d *anim, const char *path) {
  anim->anim = &texture_pullfromfile(path)->anim;
  anim->anim->tex->opts.animation = 1;
  anim_stop(anim);
  anim_play(anim);
}

void anim_play(struct anim2d *anim) {
  if (anim->playing)
    return;

  if (anim->frame == anim_frames(anim->anim))
    anim->frame = 0;

  anim->playing = 1;

  if (anim->timer == NULL)
    anim->timer = id2timer(timer_make(1.f / anim->anim->ms, anim_incr, anim, 0, 0));
  else
    timerr_settime(anim->timer, 1.f / anim->anim->ms);

  timer_start(anim->timer);
}

void anim_stop(struct anim2d *anim) {
  if (!anim->playing)
    return;

  anim->playing = 0;
  anim->frame = 0;
  anim->pausetime = 0;
  timer_stop(anim->timer);
}

void anim_pause(struct anim2d *anim) {
  if (!anim->playing)
    return;

  anim->playing = 0;
  timer_pause(anim->timer);
}

void anim_fwd(struct anim2d *anim) {
  anim_incr(anim);
}

void anim_bkwd(struct anim2d *anim) {
  anim_decr(anim);
}

float st_s_w(struct glrect st) {
  return (st.s1 - st.s0);
}

float st_s_h(struct glrect st) {
  return (st.t1 - st.t0);
}
