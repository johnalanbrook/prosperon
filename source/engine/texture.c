#include "texture.h"

#include "log.h"
#include "render.h"
#include "sokol/sokol_gfx.h"
#include <math.h>
#include <stb_ds.h>
#include <stb_image.h>

#include "resources.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

#include <stdio.h>

#define QOI_IMPLEMENTATION
#include "qoi.h"

#ifndef NSVG
#include "nanosvgrast.h"
#endif

struct glrect ST_UNIT = {0.f, 1.f, 0.f, 1.f};

static struct {
  char *key;
  struct Texture *value;
} *texhash = NULL;

struct Texture *tex_default;
struct Texture *texture_notex() { return texture_pullfromfile("icons/no_tex.gif"); }

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

int gif_nframes(const char *path)
{
  struct Texture *t = texture_pullfromfile(path);
  return t->frames;
}

/* If an empty string or null is put for path, loads default texture */
struct Texture *texture_pullfromfile(const char *path) {
  if (!path) return texture_notex();
  if (shlen(texhash) == 0) sh_new_arena(texhash);

  int index = shgeti(texhash, path);
  if (index != -1)
    return texhash[index].value;

  size_t rawlen;
  unsigned char *raw = slurp_file(path, &rawlen);
  
  if (!raw) return texture_notex();

  unsigned char *data;

  struct Texture *tex = calloc(1, sizeof(*tex));
  tex->opts.sprite = 1;
  tex->opts.mips = 0;
  tex->opts.gamma = 0;
  tex->opts.wrapx = 1;
  tex->opts.wrapy = 1;

  int n;

  char *ext = strrchr(path, '.');
  
  if (!strcmp(ext, ".qoi")) {
    qoi_desc qoi;
    data = qoi_decode(raw, rawlen, &qoi, 4);
    tex->width = qoi.width;
    tex->height = qoi.height;
    n = qoi.channels;
  } else if (!strcmp(ext, ".gif")) {
    data = stbi_load_gif_from_memory(raw, rawlen, NULL, &tex->width, &tex->height, &tex->frames, &n, 4);
    tex->height *= tex->frames;
  } else if (!strcmp(ext, ".svg")) {
  #ifndef NSVG
    NSVGimage *svg = nsvgParse(raw, "px", 96);
    struct NSVGrasterizer *rast = nsvgCreateRasterizer();
    n=4;
    tex->width=100;
    tex->height=100;
    float scale = tex->width/svg->width;
    
    data = malloc(tex->width*tex->height*n);
    nsvgRasterize(rast, svg, 0, 0, scale, data, tex->width, tex->height, tex->width*n);
    free(svg);
    free(rast);
  #else
    YughWarn("Primum was built without SVG capabilities.");
    return;
  #endif
  } else {
    data = stbi_load_from_memory(raw, rawlen, &tex->width, &tex->height, &n, 4);
  }
  free(raw);

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

  shput(texhash, path, tex);

  for (int i = 1; i < mips; i++)
    free(mipdata[i]);

  return tex;
}

void texture_sync(const char *path) { YughWarn("Need to implement texture sync."); }

char *tex_get_path(struct Texture *tex) {
  for (int i = 0; i < shlen(texhash); i++) {
    if (tex == texhash[i].value) {
      YughInfo("Found key %s", texhash[i].key);
      return texhash[i].key;
    }
  }

  return "";
}

struct Texture *texture_fromdata(void *raw, long size)
{
  struct Texture *tex = calloc(1, sizeof(*tex));
  tex->opts.sprite = 1;
  tex->opts.mips = 0;
  tex->opts.gamma = 0;
  tex->opts.wrapx = 1;
  tex->opts.wrapy = 1;

  int n;
  void *data = stbi_load_from_memory(raw, size, &tex->width, &tex->height, &n, 4);

  if (data == NULL) {
    YughError("Given raw data not valid. Loading default instead.");
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

  for (int i = 1; i < mips; i++)
    free(mipdata[i]);

  return tex;
}

struct Texture *texture_loadfromfile(const char *path) { return texture_pullfromfile(path); }

struct glrect tex_get_rect(struct Texture *tex) { return ST_UNIT; }

HMM_Vec2 tex_get_dimensions(struct Texture *tex) {
  if (!tex) return (HMM_Vec2){0,0};
  HMM_Vec2 d;
  d.x = tex->width;
  d.y = tex->height;
  return d;
}

float st_s_w(struct glrect st) { return (st.s1 - st.s0); }

float st_s_h(struct glrect st) { return (st.t1 - st.t0); }
