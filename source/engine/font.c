#include "font.h"

#include "log.h"
#include "render.h"
#include <ctype.h>
#include <limits.h>
#include <shader.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <window.h>
#include <chipmunk/chipmunk.h>
#include "2dphysics.h"
#include "resources.h"

#include "text.sglsl.h"

#include "stb_image_write.h"
#include "stb_rect_pack.h"
#include "stb_truetype.h"

#include "HandmadeMath.h"

struct sFont *font;

#define max_chars 4000

static sg_shader fontshader;
static sg_bindings bind_text;
static sg_pipeline pipe_text;
struct text_vert {
  struct draw_p pos;
  struct draw_p wh;
  struct uv_n uv;
  struct uv_n st;
  struct rgba color;
};

static struct text_vert *text_buffer;

void font_init() {
  text_buffer = malloc(sizeof(*text_buffer)*max_chars);
  fontshader = sg_make_shader(text_shader_desc(sg_query_backend()));
  pipe_text = sg_make_pipeline(&(sg_pipeline_desc){
      .shader = fontshader,
      .layout = {
          .attrs = {
              [0].format = SG_VERTEXFORMAT_FLOAT2, /* verts */
	      [0].buffer_index = 1,
	      [1].format = SG_VERTEXFORMAT_FLOAT2, /* pos */
	      [2].format = SG_VERTEXFORMAT_FLOAT2, /* width and height */
              [3].format = SG_VERTEXFORMAT_USHORT2N, /* uv pos */
	      [4].format = SG_VERTEXFORMAT_USHORT2N, /* uv width and height */
              [5].format = SG_VERTEXFORMAT_UBYTE4N, /* color */
          },
	.buffers[0].step_func = SG_VERTEXSTEP_PER_INSTANCE
      },
      .primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP,
      .colors[0].blend = blend_trans,
    });
    
  float text_verts[8] = {
    0,0,
    0,1,
    1,0,
    1,1
  };
    
  bind_text.vertex_buffers[1] = sg_make_buffer(&(sg_buffer_desc){
    .data = SG_RANGE(text_verts),
    .usage = SG_USAGE_IMMUTABLE
  });

  bind_text.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
      .size = sizeof(struct text_vert)*max_chars,
      .type = SG_BUFFERTYPE_VERTEXBUFFER,
      .usage = SG_USAGE_STREAM,
      .label = "text buffer"
    });

  font = MakeFont("fonts/LessPerfectDOSVGA.ttf", 16);
  bind_text.fs.images[0] = font->texID;
  bind_text.fs.samplers[0] = sg_make_sampler(&(sg_sampler_desc){});
}

struct sFont *MakeSDFFont(const char *fontfile, int height)
{
  YughInfo("Making sdf font %s.", fontfile);

  int packsize = 1024;
  struct sFont *newfont = calloc(1, sizeof(struct sFont));
  newfont->height = height;

  char fontpath[256];
  snprintf(fontpath, 256, "fonts/%s", fontfile);

  unsigned char *ttf_buffer = slurp_file(fontpath, NULL);
  unsigned char *bitmap = malloc(packsize * packsize);

  stbtt_fontinfo fontinfo;
  if (!stbtt_InitFont(&fontinfo, ttf_buffer, stbtt_GetFontOffsetForIndex(ttf_buffer, 0))) {
    YughError("Failed to make font %s", fontfile);
  }

  for (int i = 32; i < 95; i++) {
    int w, h, xoff, yoff;
//    unsigned char *stbtt_GetGlyphSDF(&fontinfo, height, i, 1, 0, 1, &w, &h, &xoff, &yoff);
  }

  return newfont;
}

struct sFont *MakeFont(const char *fontfile, int height) {
  int packsize = 1024;

  struct sFont *newfont = calloc(1, sizeof(struct sFont));
  newfont->height = height;

  char fontpath[256];
  snprintf(fontpath, 256, "fonts/%s", fontfile);
  
  unsigned char *ttf_buffer = slurp_file(fontfile, NULL);
  unsigned char *bitmap = malloc(packsize * packsize);

  stbtt_packedchar glyphs[95];

  stbtt_pack_context pc;

  int pad = 2;

  stbtt_PackBegin(&pc, bitmap, packsize, packsize, 0, pad, NULL);
  stbtt_PackFontRange(&pc, ttf_buffer, 0, height, 32, 95, glyphs);
  stbtt_PackEnd(&pc);

  stbtt_fontinfo fontinfo;
  if (!stbtt_InitFont(&fontinfo, ttf_buffer, stbtt_GetFontOffsetForIndex(ttf_buffer, 0))) {
    YughError("Failed to make font %s", fontfile);
  }

  stbtt_GetFontVMetrics(&fontinfo, &newfont->ascent, &newfont->descent, &newfont->linegap);
  newfont->emscale = stbtt_ScaleForMappingEmToPixels(&fontinfo, 16);
  newfont->linegap = (newfont->ascent - newfont->descent)* 2 * newfont->emscale;

  newfont->texID = sg_make_image(&(sg_image_desc){
      .type = SG_IMAGETYPE_2D,
      .width = packsize,
      .height = packsize,
      .pixel_format = SG_PIXELFORMAT_R8,
      .usage = SG_USAGE_IMMUTABLE,
//      .min_filter = SG_FILTER_NEAREST,
//      .mag_filter = SG_FILTER_NEAREST,
      .data.subimage[0][0] = {
          .ptr = bitmap,
          .size = packsize * packsize}});

  free(ttf_buffer);
  free(bitmap);

  for (unsigned char c = 32; c < 127; c++) {
    stbtt_packedchar glyph = glyphs[c - 32];

    struct glrect r;
    r.s0 = (glyph.x0) / (float)packsize;
    r.s1 = (glyph.x1) / (float)packsize;
    r.t0 = (glyph.y0) / (float)packsize;
    r.t1 = (glyph.y1) / (float)packsize;

    stbtt_GetCodepointHMetrics(&fontinfo, c, &newfont->Characters[c].Advance, &newfont->Characters[c].leftbearing);
    newfont->Characters[c].Advance *= newfont->emscale;
    newfont->Characters[c].leftbearing *= newfont->emscale;

//    newfont->Characters[c].Advance = glyph.xadvance; /* x distance from this char to the next */
    newfont->Characters[c].Size[0] = glyph.x1 - glyph.x0; 
    newfont->Characters[c].Size[1] = glyph.y1 - glyph.y0;
    newfont->Characters[c].Bearing[0] = glyph.xoff;
    newfont->Characters[c].Bearing[1] = glyph.yoff2;
    newfont->Characters[c].rect = r;
  }

  return newfont;
}

static int curchar = 0;

void draw_char_box(struct Character c, cpVect cursor, float scale, struct rgba color)
{
  cpVect wh;
 
  wh.x = 8 * scale;
  wh.y = 14;
  cursor.x += wh.x / 2.f;
  cursor.y += wh.y / 2.f;

//  draw_box(cursor, wh, color);
}

void text_flush(HMM_Mat4 *proj) {
  if (curchar == 0) return;
  sg_apply_pipeline(pipe_text);
  sg_apply_bindings(&bind_text);
  sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, SG_RANGE_REF(*proj));

  sg_range verts;
  verts.ptr = text_buffer;
  verts.size = sizeof(struct text_vert) * curchar;
  sg_update_buffer(bind_text.vertex_buffers[0], &verts);

  sg_draw(0, 4, curchar);
  curchar = 0;
}

static int drawcaret = 0;

void sdrawCharacter(struct Character c, HMM_Vec2 cursor, float scale, struct rgba color) {
  if (curchar == max_chars)
    return;
    
  struct text_vert vert;

  float lsize = 1.0 / 1024.0;

  float oline = 0.0;
  
  vert.pos.x = cursor.X + c.Bearing[0] * scale + oline;
  vert.pos.y = cursor.Y - c.Bearing[1] * scale - oline;
  vert.wh.x = c.Size[0] * scale + (oline*2);
  vert.wh.y = c.Size[1] * scale + (oline*2);
  vert.uv.u = (c.rect.s0 - oline*lsize)*USHRT_MAX;
  vert.uv.v = (c.rect.t0 - oline*lsize)*USHRT_MAX;
  vert.st.u = (c.rect.s1-c.rect.s0+oline*lsize*2.0)*USHRT_MAX;
  vert.st.v = (c.rect.t1-c.rect.t0+oline*lsize*2.0)*USHRT_MAX;
  vert.color = color;

  memcpy(text_buffer + curchar, &vert, sizeof(struct text_vert));
  curchar++;
  return;

  /*
      if (drawcaret == curchar) {
          draw_char_box(c, cursor, scale, color);
              shader_use(shader);
        }
  */
  /*
      sg_append_buffer(bind_text.vertex_buffers[0], SG_RANGE_REF(verts));

      offset[0] = 1;
      offset[1] = -1;
      fill_charverts(verts, cursor, scale, c, offset);
      sg_update_buffer(bind_text.vertex_buffers[0], SG_RANGE_REF(verts));

      offset[1] = 1;
      fill_charverts(verts, cursor, scale, c, offset);
      sg_update_buffer(bind_text.vertex_buffers[0], SG_RANGE_REF(verts));

      offset[0] = -1;
      offset[1] = -1;
      fill_charverts(verts, cursor, scale, c, offset);
      sg_update_buffer(bind_text.vertex_buffers[0], SG_RANGE_REF(verts));
  */
}

void text_settype(struct sFont *mfont) {
  font = mfont;
}

struct boundingbox text_bb(const char *text, float scale, float lw, float tracking)
{
  HMM_Vec2 cursor = {0,0};
  const unsigned char *c = text;
  const unsigned char *wordstart;

  while (*c != '\0') {
    if (isblank(*c)) {
      cursor.X += font->Characters[*c].Advance * tracking * scale;
      c++;
    } else if (isspace(*c)) {
      cursor.Y -= scale * font->linegap;
      cursor.X = 0;
      c++;
    } else {
      wordstart = c;
      int wordwidth = 0;

      while (!isspace(*c) && *c != '\0') {
        wordwidth += font->Characters[*c].Advance * tracking * scale;
	c++;
      }

      if (lw > 0 && (cursor.X + wordwidth) >= lw) {
        cursor.X = 0;
	cursor.Y -= scale * font->linegap;
      }

      while (wordstart < c) {
        cursor.X += font->Characters[*wordstart].Advance * tracking * scale;
	wordstart++;
      }
    }
  }

  float height = cursor.Y + (font->height*scale);
  float width = lw > 0 ? lw : cursor.X;

  struct boundingbox bb = {};
  bb.l = 0;
  bb.t = font->ascent * font->emscale * scale;
  bb.b = font->descent * font->emscale * scale;
  bb.r = cursor.X;
  return bb;

  return cwh2bb((HMM_Vec2){0,0}, (HMM_Vec2){width,height});
}

int renderText(const char *text, HMM_Vec2 pos, float scale, struct rgba color, float lw, int caret, float tracking) {
  int len = strlen(text);
  drawcaret = caret;

  HMM_Vec2 cursor = pos;

  const unsigned char *line, *wordstart, *drawstart;
  line = drawstart = (unsigned char *)text;

  struct rgba usecolor = color;

  while (*line != '\0') {
    if (isblank(*line)) {
      sdrawCharacter(font->Characters[*line], cursor, scale, usecolor);
      cursor.X += font->Characters[*line].Advance * tracking * scale;
      line++;
    } else if (isspace(*line)) {
      sdrawCharacter(font->Characters[*line], cursor, scale, usecolor);
      cursor.Y -= scale * font->linegap;
      cursor.X = pos.X;
      line++;
    } else {
      wordstart = line;
      int wordWidth = 0;

      while (!isspace(*line) && *line != '\0') {
        wordWidth += font->Characters[*line].Advance * tracking * scale;
        line++;
      }

      if (lw > 0 && (cursor.X + wordWidth - pos.X) >= lw) {
        cursor.X = pos.X;
        cursor.Y -= scale * font->linegap;
      }

      while (wordstart < line) {
        sdrawCharacter(font->Characters[*wordstart], cursor, scale, usecolor);
        cursor.X += font->Characters[*wordstart].Advance * tracking * scale;
        wordstart++;
      }
    }
  }
  /*    if (caret > curchar) {
          draw_char_box(font->Characters[69], cursor, scale, color);
      }
  */

  return cursor.Y - pos.Y;
}
