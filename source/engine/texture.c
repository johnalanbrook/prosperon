#include "texture.h"

#include "log.h"
#include "render.h"
#include "sokol/sokol_gfx.h"
#include <math.h>
#include <stb_image.h>
#include <stb_image_write.h>

#include "resources.h"

#include "stb_image_resize2.h"

#include <stdio.h>

#include "qoi.h"

#ifndef NSVG
#include "nanosvgrast.h"
#endif

struct rect ST_UNIT = {0.f, 0.f, 1.f, 1.f};

static inline void write_pixel(unsigned char *data, int idx, rgba color)
{
  memcpy(data+idx, &color, sizeof(color));
}

static inline rgba get_pixel(unsigned char *data, int idx)
{
  rgba color;
  memcpy(&color, data+idx, sizeof(color));
  return color;
}

static inline unsigned char c_clamp(float value) { return (unsigned char) fmaxf(0.0f, fminf(255.0f, roundf(value))); }

static inline rgba blend_colors(rgba a, rgba b)
{
  float a_a = a.a / 255.0f;
  float b_a = b.a / 255.0f;

  float out_a = a_a + b_a * (1.0f - a_a);
  rgba result;

  if (out_a == 0.0f) {
    result.r = result.g = result.b = result.a = 0;
    return result;
  }

  // Use the c_clamp function to safely clamp the values within the range [0, 255]
  result.r = c_clamp(((a.r * a_a) + (b.r * b_a * (1.0f - a_a))) / out_a);
  result.g = c_clamp(((a.g * a_a) + (b.g * b_a * (1.0f - a_a))) / out_a);
  result.b = c_clamp(((a.b * a_a) + (b.b * b_a * (1.0f - a_a))) / out_a);
  result.a = c_clamp(out_a * 255.0f);

  return result;
}

static inline rgba additive_blend(rgba a, rgba b) {
  rgba result;

  result.r = c_clamp(a.r + b.r);
  result.g = c_clamp(a.g + b.g);
  result.b = c_clamp(a.b + b.b);
  result.a = c_clamp((a.a + b.a) * 0.5f);  // Blend alpha channels evenly

  return result;
}

static inline rgba subtractive_blend(rgba a, rgba b) {
  rgba result;

  result.r = c_clamp(a.r - b.r);
  result.g = c_clamp(a.g - b.g);
  result.b = c_clamp(a.b - b.b);
  result.a = c_clamp((a.a + b.a) * 0.5f);  // Blend alpha channels evenly

  return result;
}

static inline rgba multiplicative_blend(rgba a, rgba b) {
  rgba result;

  result.r = c_clamp((a.r * b.r) / 255.0f);
  result.g = c_clamp((a.g * b.g) / 255.0f);
  result.b = c_clamp((a.b * b.b) / 255.0f);
  result.a = c_clamp((a.a + b.a) * 0.5f);  // Blend alpha channels evenly

  return result;
}

static inline rgba dodge_blend(rgba a, rgba b) {
  rgba result;

  result.r = c_clamp(a.r == 255 ? 255 : (b.r * 255) / (255 - a.r));
  result.g = c_clamp(a.g == 255 ? 255 : (b.g * 255) / (255 - a.g));
  result.b = c_clamp(a.b == 255 ? 255 : (b.b * 255) / (255 - a.b));
  result.a = c_clamp((a.a + b.a) * 0.5f);  // Blend alpha channels evenly

  return result;
}

static inline rgba burn_blend(rgba a, rgba b) {
  rgba result;

  result.r = c_clamp(a.r == 0 ? 0 : 255 - ((255 - b.r) * 255) / a.r);
  result.g = c_clamp(a.g == 0 ? 0 : 255 - ((255 - b.g) * 255) / a.g);
  result.b = c_clamp(a.b == 0 ? 0 : 255 - ((255 - b.b) * 255) / a.b);
  result.a = c_clamp((a.a + b.a) * 0.5f);  // Blend alpha channels evenly

  return result;
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

void texture_offload(texture *tex)
{
  if (tex->data) {
    free(tex->data);
    tex->data = NULL;
  }
}

/* If an empty string or null is put for path, loads default texture */
struct texture *texture_from_file(const char *path) {
  if (!path) return NULL;

  size_t rawlen;
  unsigned char *raw = slurp_file(path, &rawlen);
  
  if (!raw) return NULL;

  unsigned char *data;

  struct texture *tex = calloc(1, sizeof(*tex));

  int n;

  char *ext = strrchr(path, '.');

  if (!strcmp(ext, ".qoi")) {
    qoi_desc qoi;
    data = qoi_decode(raw, rawlen, &qoi, 4);
    tex->width = qoi.width;
    tex->height = qoi.height;
    n = qoi.channels;
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
    YughWarn("Prosperon was built without SVG capabilities.");
    return;
  #endif
  } else {
    data = stbi_load_from_memory(raw, rawlen, &tex->width, &tex->height, &n, 4);
  }
  free(raw);

  if (data == NULL) {
    free(tex);
    return NULL;
  }

  tex->data = data;
    
  return tex;
}

void texture_free(texture *tex)
{
  if (!tex) return;
  if (tex->data)
    free(tex->data);

  free(tex);
}

struct texture *texture_empty(int w, int h)
{
  int n = 4;
  texture *tex = calloc(1,sizeof(*tex));
  tex->data = calloc(w*h*n, sizeof(unsigned char));
  tex->width = w;
  tex->height = h;
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

  texture_load_gpu(tex);

  return tex;
}

static double fade (double t) { return t*t*t*(t*(t*6-15)+10); }
double grad (int hash, double x, double y, double z)
{
  int h = hash&15;
  double u = h<8 ? x : y;
  double v = h<4 ? y : h==12||h==14 ? x : z;
  return ((h&1) == 0 ? u : -u) + ((h&2) == 0 ? v : -v);
/* alt */ 
/*    switch(hash & 0xF)
    {
        case 0x0: return  x + y;
        case 0x1: return -x + y;
        case 0x2: return  x - y;
        case 0x3: return -x - y;
        case 0x4: return  x + z;
        case 0x5: return -x + z;
        case 0x6: return  x - z;
        case 0x7: return -x - z;
        case 0x8: return  y + z;
        case 0x9: return -y + z;
        case 0xA: return  y - z;
        case 0xB: return -y - z;
        case 0xC: return  y + x;
        case 0xD: return -y + z;
        case 0xE: return  y - x;
        case 0xF: return -y - z;
        default: return 0; // never happens
    }*/
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

// copy texture src to dest
// sx and sy are the destination coordinates to copy to
// sw the width of the destination to take in pixels
// sh the height of the destination to take in pixels
int texture_blit(texture *dst, texture *src, rect dstrect, rect srcrect, int tile) {
  if (!src || !dst || !src->data || !dst->data) return 0;

  float scaleX = srcrect.w / dstrect.w;
  float scaleY = srcrect.h / dstrect.h;

  if (srcrect.x < 0 || srcrect.y < 0 || srcrect.x + srcrect.w > src->width ||
    dstrect.x < 0 || dstrect.y < 0 || dstrect.x + dstrect.w > dst->width ||
    srcrect.y + srcrect.h > src->height || dstrect.y + dstrect.h > dst->height) {
    return false;  // Rectangles exceed texture bounds
  }

  for (int dstY = 0; dstY < dstrect.h; ++dstY) {
    for (int dstX = 0; dstX < dstrect.w; ++dstX) {
      int srcX;
      int srcY;
      
      if (tile) {
        srcX = srcrect.x + (dstX % (int)srcrect.w);
        srcY = srcrect.y + (dstY % (int)srcrect.h);      
      } else {
        srcX = srcrect.x + (int)(dstX * scaleX);
        srcY = srcrect.y + (int)(dstY * scaleY);
      }
      
      int srcIndex = (srcY * src->width + srcX) * 4;
      int dstIndex = ((dstrect.y + dstY) * dst->width + (dstrect.x + dstX)) * 4;
      
      rgba srccolor = get_pixel(src->data, srcIndex);
      rgba dstcolor = get_pixel(dst->data, dstIndex);
      rgba color = blend_colors(srccolor, dstcolor);
      write_pixel(dst->data, dstIndex, color);
    }
  }

  return 1;
}

int texture_fill_rect(texture *tex, struct rect rect, struct rgba color)
{
  if (!tex || !tex->data) return 0;

  int x_end = rect.x+rect.w;
  int y_end = rect.y+rect.h;

  if (rect.x < 0 || rect.y < 0 || x_end > tex->width || y_end > tex->height) return 0;

  for (int j = rect.y; j < y_end; ++j)
    for (int i = rect.x; i < x_end; ++i) {
      int index = (j*tex->width+i)*4;
      write_pixel(tex->data, index, color);
    }

  return 1;
}

void swap_pixels(unsigned char *p1, unsigned char *p2) {
  for (int i = 0; i < 4; ++i) {
    unsigned char tmp = p1[i];
    p1[i] = p2[i];
    p2[i] = tmp;
  }
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

int texture_flip(texture *tex, int y)
{
  if (!tex || !tex->data) return -1;

  int width = tex->width;
  int height = tex->height;

  if (y) {
    for (int row = 0; row < height / 2; ++row) {
      for (int col = 0; col < width; ++col) {
        unsigned char *top = tex->data+((row*width+col)*4);
        unsigned char *bottom = tex->data+(((height-row-1)*width+col)*4);
        swap_pixels(top,bottom);
      }
    }
  } else {
    for (int row = 0; row < height; ++row) {
      for (int col = 0; col < width / 2; ++col) {
        unsigned char *left = tex->data+((row*width+col)*4);
        unsigned char *right = tex->data+((row*width+(width-col-1))*4);
        swap_pixels(left,right);
      }
    }
  }
  
  return 0;
}


int texture_write_pixel(texture *tex, int x, int y, rgba color)
{
  if (x < 0 || x >= tex->width || y < 0 || y >= tex->height) return 0;
  int i = (y * tex->width + x) * 4;
  write_pixel(tex->data, i, color);
  
  return 1;
}

texture *texture_dup(texture *tex)
{
  texture *new = calloc(1, sizeof(*new));
  *new = *tex;
  new->data = malloc(new->width*new->height*4);
  memcpy(new->data, tex->data, new->width*new->height*4*sizeof(new->data));
  
  return new;
}

sg_image_data tex_img_data(texture *tex, int mipmaps)
{
  if (!mipmaps) {
    sg_image_data sg_img_data = {0};
    sg_img_data.subimage[0][0] = (sg_range) {.ptr = tex->data, .size = tex->width*tex->height*4};
    return sg_img_data;
  }

  sg_image_data sg_img_data = {0};
  
  int mips = mip_levels(tex->width, tex->height)+1;
  
  int mipw, miph;
  mipw = tex->width;
  miph = tex->height;
  
  sg_img_data.subimage[0][0] = (sg_range){ .ptr = tex->data, .size = mipw*miph*4 };

  unsigned char *mipdata[mips];
  mipdata[0] = tex->data;
    
  for (int i = 1; i < mips; i++) {
    int w, h, mipw, miph;
    mip_wh(tex->width, tex->height, &mipw, &miph, i-1); // mipw miph are previous iteration 
    mip_wh(tex->width, tex->height, &w, &h, i);
    mipdata[i] = malloc(w * h * 4);
    stbir_resize_uint8_linear(mipdata[i-1], mipw, miph, 0, mipdata[i], w, h, 0, 4);
    sg_img_data.subimage[0][i] = (sg_range){ .ptr = mipdata[i], .size = w*h*4 };
    tex->vram += w*h*4;
      
    mipw = w;
    miph = h;
  }
}

int texture_fill(texture *tex, struct rgba color)
{
  if (!tex || !tex->data) return 0;  // Ensure valid texture and pixel data

  // Loop through every pixel in the texture
  for (int y = 0; y < tex->height; ++y) {
	  for (int x = 0; x < tex->width; ++x) {
      int index = (y * tex->width + x) * 4;
      write_pixel(tex->data, index, color);
    }
  }
  
  return 1;
}

void texture_load_gpu(texture *tex)
{
  if (!tex->data) return;
  if (tex->id.id == 0) {
    // Doesn't exist, so make a new one
    sg_image_data img_data = tex_img_data(tex, 0);
    tex->id = sg_make_image(&(sg_image_desc){
      .type = SG_IMAGETYPE_2D,
      .width = tex->width,
      .height = tex->height,
      .usage = SG_USAGE_IMMUTABLE,
      .num_mipmaps = 1,
      .data = img_data
    });
  } //else {
    // Simple update
//    sg_image_data img_data = tex_img_data(tex,0);
//    sg_update_image(tex->id, &img_data);
//  }
}
