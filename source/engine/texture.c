#include "texture.h"

#include "log.h"
#include "render.h"
#include "sokol/sokol_gfx.h"
#include <math.h>
#include <stb_image.h>

#include "resources.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"

#include <stdio.h>

#define QOI_IMPLEMENTATION
#include "qoi.h"

#ifndef NSVG
#include "nanosvgrast.h"
#endif

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
  } else if (!strcmp(ext, ".gif")) {
    data = stbi_load_gif_from_memory(raw, rawlen, &tex->delays, &tex->width, &tex->height, &tex->frames, &n, 4);
    int *dd = tex->delays;
    tex->delays = NULL;
    arrsetlen(tex->delays, tex->frames);
    for (int i = 0; i < tex->frames; i++) tex->delays[i] = dd[i];
    free(dd);
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
    YughWarn("Prosperon was built without SVG capabilities.");
    return;
  #endif
  } else {
    data = stbi_load_from_memory(raw, rawlen, &tex->width, &tex->height, &n, 4);
  }
  free(raw);

  if (data == NULL)
    return NULL;
  
  tex->data = data;
  
  unsigned int nw = next_pow2(tex->width);
  unsigned int nh = next_pow2(tex->height);

  int filter = SG_FILTER_NEAREST;
  
  sg_image_data sg_img_data;
  sg_img_data.subimage[0][0] = (sg_range){.ptr = data, .size=tex->width*tex->height*4};
  /*
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
    mip_wh(tex->width, tex->height, &mipw, &miph, i-1); // mipw miph are previous iteration 
    mip_wh(tex->width, tex->height, &w, &h, i);
    mipdata[i] = malloc(w * h * 4);
    stbir_resize_uint8_linear(mipdata[i-1], mipw, miph, 0, mipdata[i], w, h, 0, 4);
    sg_img_data.subimage[0][i] = (sg_range){ .ptr = mipdata[i], .size = w*h*4 };
    
    mipw = w;
    miph = h;
  }
*/
  tex->id = sg_make_image(&(sg_image_desc){
    .type = SG_IMAGETYPE_2D,
    .width = tex->width,
    .height = tex->height,
    .usage = SG_USAGE_IMMUTABLE,
    //.num_mipmaps = mips,
    .data = sg_img_data
  });

  /*for (int i = 1; i < mips; i++)
    free(mipdata[i]);*/
    
  return tex;
}

void texture_free(texture *tex)
{
  if (!tex) return;
  free(tex->data);
  if (tex->delays) arrfree(tex->delays);
  sg_destroy_image(tex->id);
  free(tex);
}

struct texture *texture_fromdata(void *raw, long size)
{
  struct texture *tex = calloc(1, sizeof(*tex));

  int n;
  void *data = stbi_load_from_memory(raw, size, &tex->width, &tex->height, &n, 4);

  if (data == NULL)
    NULL;

  unsigned int nw = next_pow2(tex->width);
  unsigned int nh = next_pow2(tex->height);
  
  tex->data = data;

  int filter = SG_FILTER_NEAREST;
  
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
    stbir_resize_uint8_linear(mipdata[i-1], mipw, miph, 0, mipdata[i], w, h, 0, 4);
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

static int p[512] = {151,160,137,91,90,15,
   131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
   190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
   88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
   77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
   102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
   135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
   5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
   223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
   129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
   251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
   49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
   138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180,
151,160,137,91,90,15,
   131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
   190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
   88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
   77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
   102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
   135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
   5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
   223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
   129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
   251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
   49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
   138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180   
   };

double perlin(double x, double y, double z)
{
  int X = (int)floor(x)&255;
  int Y = (int)floor(y)&255;
  int Z = (int)floor(z)&255;
      x -= floor(x);                                
      y -= floor(y);                                
      z -= floor(z);
      double u = fade(x),                                
             v = fade(y),                                
             w = fade(z);
      int A = p[X  ]+Y, AA = p[A]+Z, AB = p[A+1]+Z,      
          B = p[X+1]+Y, BA = p[B]+Z, BB = p[B+1]+Z;   
 
      return lerp(w, lerp(v, lerp(u, grad(p[AA  ], x  , y  , z   ), 
                                     grad(p[BA  ], x-1, y  , z   )),
                             lerp(u, grad(p[AB  ], x  , y-1, z   ), 
                                     grad(p[BB  ], x-1, y-1, z   ))),
                     lerp(v, lerp(u, grad(p[AA+1], x  , y  , z-1 ), 
                                     grad(p[BA+1], x-1, y  , z-1 )), 
                             lerp(u, grad(p[AB+1], x  , y-1, z-1 ),
                                     grad(p[BB+1], x-1, y-1, z-1 ))));  
}
