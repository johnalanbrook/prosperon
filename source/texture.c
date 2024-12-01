#include "texture.h"

#include "render.h"
#include <math.h>
#include <stb_image.h>
#include <stb_image_write.h>
#include <string.h>

#include "stb_image_resize2.h"

#include <stdio.h>

#include <tracy/TracyC.h>

#include "qoi.h"


struct rect ST_UNIT = {0.f, 0.f, 1.f, 1.f};

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

void texture_offload(texture *tex)
{
  if (tex->data) {
    TracyCFreeN(tex->data, "texture");
    free(tex->data);
    tex->data = NULL;
  }
}

void texture_free(JSRuntime *rt, texture *tex)
{
  if (!tex) return;
  texture_offload(tex);
  free(tex);
}

struct texture *texture_empty(int w, int h)
{
  int n = 4;
  texture *tex = calloc(1,sizeof(*tex));
  tex->data = calloc(w*h*n, sizeof(unsigned char));
  tex->width = w;
  tex->height = h;
  TracyCAllocN(tex->data, tex->height*tex->width*4, "texture");    
  return tex;
}

struct texture *texture_fromdata(void *raw, long size)
{
  struct texture *tex = calloc(1, sizeof(*tex));

  int n;
  void *data = stbi_load_from_memory(raw, size, &tex->width, &tex->height, &n, 4);

  if (data == NULL) {
    free(tex);
    return NULL;
  }

  tex->data = data;
  TracyCAllocN(tex->data, tex->height*tex->width*4, "texture");  

  return tex;
}

void texture_save(texture *tex, const char *file)
{
  if (!tex->data) return;
  
  char *ext = strrchr(file, '.');
  if (!strcmp(ext, ".png"))
    stbi_write_png(file, tex->width, tex->height, 4, tex->data, 4*tex->width);
  else if (!strcmp(ext, ".bmp"))
    stbi_write_bmp(file, tex->width, tex->height, 4, tex->data);
  else if (!strcmp(ext, ".tga"))
    stbi_write_tga(file, tex->width, tex->height, 4, tex->data);
  else if (!strcmp(ext, ".jpg") || !strcmp(ext, ".jpeg"))
    stbi_write_jpg(file, tex->width, tex->height, 4, tex->data, 5);
  else if (!strcmp(ext, ".qoi"))
    qoi_write(file, tex->data, &(qoi_desc) {
      .width = tex->width,
      .height = tex->height,
      .channels = 4,
      .colorspace = QOI_SRGB
    });
}

texture *texture_scale(texture *tex, int width, int height)
{
  texture *new = calloc(1, sizeof(*new));
  new->width = width;
  new->height = height;
  new->data = malloc(4*width*height);
  
  stbir_resize_uint8_linear(tex->data, tex->width, tex->height, 0, new->data, width, height, 0, 4);
  return new;
}

/*sapp_icon_desc texture2icon(texture *tex)
{
  sapp_icon_desc desc = {0};
  if (!tex->data) return desc;
  struct isize {
    int size;
    unsigned char *data;
  };
  
  struct isize sizes[4];
  sizes[0].size = 16;
  sizes[1].size = 32;
  sizes[2].size = 64;
  sizes[3].size = 128;

  for (int i = 0; i < 4; i++) {
    sizes[i].data = malloc(4*sizes[i].size*sizes[i].size);
    stbir_resize_uint8_linear(tex->data, tex->width, tex->height, 0, sizes[i].data, sizes[i].size, sizes[i].size, 0, 4);
  }

  desc = (sapp_icon_desc){
    .images = {
      { .width = sizes[0].size, .height = sizes[0].size, .pixels = { .ptr=sizes[0].data, .size=4*sizes[0].size*sizes[0].size } },
      { .width = sizes[1].size, .height = sizes[1].size, .pixels = { .ptr=sizes[1].data, .size=4*sizes[1].size*sizes[1].size } },
      { .width = sizes[2].size, .height = sizes[2].size, .pixels = { .ptr=sizes[2].data, .size=4*sizes[2].size*sizes[2].size } },
      { .width = sizes[3].size, .height = sizes[3].size, .pixels = { .ptr=sizes[3].data, .size=4*sizes[3].size*sizes[3].size } },            
    }
  };
  
  return desc;
}
*/
