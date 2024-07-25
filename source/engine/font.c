#include "font.h"

#include "log.h"

#include <ctype.h>
#include "log.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <window.h>
#include "resources.h"
#include "render.h"

#include "stb_image_write.h"
#include "stb_rect_pack.h"
#include "stb_truetype.h"

#include "HandmadeMath.h"

struct sFont *use_font;

struct text_vert {
  HMM_Vec2 pos;
  HMM_Vec2 wh;
  HMM_Vec2 uv;
  HMM_Vec2 st;
  HMM_Vec4 color;
};

static struct text_vert *text_buffer;

void font_free(font *f)
{
  sg_destroy_image(f->texID);
  free(f);
}

void font_set(font *f)
{
  use_font = f;
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
  int packsize = 2048;

  struct sFont *newfont = calloc(1, sizeof(struct sFont));
  newfont->height = height;

  unsigned char *ttf_buffer = slurp_file(fontfile, NULL);
  if (!ttf_buffer) {
    YughWarn("Could not find font at %s.");
    return NULL;
  }
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
  
  int ascent, descent, linegap;

  stbtt_GetFontVMetrics(&fontinfo, &ascent, &descent, &linegap);
  float emscale = stbtt_ScaleForPixelHeight(&fontinfo, height);
  newfont->ascent = ascent*emscale;
  newfont->descent = descent*emscale;
  newfont->linegap = linegap*emscale;
  newfont->linegap = ((newfont->ascent - newfont->descent) - newfont->linegap);

  newfont->texture = malloc(sizeof(texture));
  newfont->texture->id = sg_make_image(&(sg_image_desc){
    .type = SG_IMAGETYPE_2D,
    .width = packsize,
    .height = packsize,
    .pixel_format = SG_PIXELFORMAT_R8,
    .usage = SG_USAGE_IMMUTABLE,
    .data.subimage[0][0] = {
      .ptr = bitmap,
      .size = packsize * packsize
    }
  });
  
  newfont->texture->width = packsize;
  newfont->texture->height = packsize;

  for (unsigned char c = 32; c < 127; c++) {
    stbtt_packedchar glyph = glyphs[c - 32];

    struct rect r;
    r.x = (glyph.x0) / (float)packsize;
    r.w = (glyph.x1-glyph.x0) / (float)packsize;
    r.y = (glyph.y0) / (float)packsize;
    r.h = (glyph.y1-glyph.y0) / (float)packsize;

    newfont->Characters[c].Advance = glyph.xadvance; /* x distance from this char to the next */
    newfont->Characters[c].Size[0] = (glyph.x1 - glyph.x0);
    newfont->Characters[c].Size[1] = (glyph.y1 - glyph.y0);
    newfont->Characters[c].Bearing[0] = glyph.xoff;
    newfont->Characters[c].Bearing[1] = glyph.yoff2;
    newfont->Characters[c].rect = r;
  }

  free(ttf_buffer);
  free(bitmap);

  return newfont;
}

void draw_underline_cursor(HMM_Vec2 pos, float scale, struct rgba color)
{
  pos.Y -= 2;
  sdrawCharacter(use_font->Characters['_'], pos, scale, color);
}

void draw_char_box(struct Character c, HMM_Vec2 cursor, float scale, struct rgba color)
{
  HMM_Vec2 wh;
  color.a = 30;
 
  wh.x = c.Size[0] * scale + 2;
  wh.y = c.Size[1] * scale + 2;
  cursor.X += c.Bearing[0] * scale + 1;
  cursor.Y -= (c.Bearing[1] * scale + 1);

  HMM_Vec2 b;
  b.x = cursor.X + wh.x/2;
  b.y = cursor.Y + wh.y/2;
}

int text_flush(sg_buffer *buf) {
  if (arrlen(text_buffer) ==  0) return 0;

  sg_range verts;
  verts.ptr = text_buffer;
  verts.size = sizeof(struct text_vert) * arrlen(text_buffer);
  if (sg_query_buffer_will_overflow(*buf, verts.size)) {
    sg_destroy_buffer(*buf);
    *buf = sg_make_buffer(&(sg_buffer_desc){
      .size = verts.size,
      .type = SG_BUFFERTYPE_STORAGEBUFFER,
      .usage = SG_USAGE_STREAM,
      .label = "text buffer"
    });
  }
    
  sg_append_buffer(*buf, &verts);
  int n = arrlen(text_buffer);
  arrsetlen(text_buffer, 0);
  return n;
}

void sdrawCharacter(struct Character c, HMM_Vec2 cursor, float scale, struct rgba color) {
  struct rgba colorbox = {0,0,0,255};
  
  struct text_vert vert;

  vert.pos.x = cursor.X + c.Bearing[0] * scale;
  vert.pos.y = cursor.Y - c.Bearing[1] * scale;
  vert.wh.x = c.Size[0] * scale;
  vert.wh.y = c.Size[1] * scale;

//  if (vert.pos.x > frame.l || vert.pos.y > frame.t || (vert.pos.y + vert.wh.y) < frame.b || (vert.pos.x + vert.wh.x) < frame.l) return;

  vert.uv.x = c.rect.x;
  vert.uv.y = c.rect.y;
  vert.st.x = c.rect.w;
  vert.st.y = c.rect.h;
  rgba2floats(vert.color.e, color);

  arrput(text_buffer, vert);
}

const char *esc_color(const char *c, struct rgba *color, struct rgba defc)
{
  struct rgba d;
  if (!color) color = &d;
  if (*c != '\e') return c;
  c++;
  if (*c != '[') return c;
  c++;
  if (*c == '0') {
    *color = defc;
    c++;
    return c;
  }
  else if (!strncmp(c, "38;2;", 5)) {
    c += 5;
    *color = (struct rgba){0,0,0,255};
    color->r = atoi(c);
    c = strchr(c, ';')+1;
    color->g = atoi(c);
    c = strchr(c,';')+1;
    color->b = atoi(c);
    c = strchr(c,';')+1;
    return c;
  }
  return c;
}

struct boundingbox text_bb(const char *text, float scale, float lw, float tracking)
{
  if (!use_font) return cwh2bb((HMM_Vec2){0,0}, (HMM_Vec2){0,0});
  struct rgba dummy;
  HMM_Vec2 cursor = {0,0};
  const char *line, *wordstart;
  line = text;

  while (*line != '\0') {
    if (isblank(*line)) {
      cursor.X += use_font->Characters[*line].Advance * tracking * scale;
      line++;
    } else if (isspace(*line)) {
      cursor.Y -= scale * use_font->linegap;
      cursor.X = 0;
      line++;
    } else {
      if (*line == '\e')
        line = esc_color(line, NULL, dummy);
    
      wordstart = line;
      int wordWidth = 0;

      while (!isspace(*line) && *line != '\0') {
        wordWidth += use_font->Characters[*line].Advance * tracking * scale; 
        line++;
      }

      if (lw > 0 && (cursor.X + wordWidth) >= lw) {
        cursor.X = 0;
        cursor.Y -= scale * use_font->linegap;
      }

      while (wordstart < line) {
        if (*wordstart == '\e')
          line = esc_color(wordstart, NULL, dummy);
      
        cursor.X += use_font->Characters[*wordstart].Advance * tracking * scale;
        wordstart++;
      }
    }
  }

  return (struct boundingbox){
    .b = cursor.Y + use_font->descent,
    .t = cursor.Y + use_font->ascent,
    .l = 0,
    .r = cursor.X
  };
}

void check_caret(int caret, int l, HMM_Vec2 pos, float scale, struct rgba color)
{
  if (caret == l)
    draw_underline_cursor(pos,scale,color);
}

/* pos given in screen coordinates */
int renderText(const char *text, HMM_Vec2 pos, float scale, struct rgba color, float lw, int caret, float tracking) {
  if (!use_font) {
    YughError("Cannot render text before a font is set.");
    return pos.y;
  }

  int len = strlen(text);

  HMM_Vec2 cursor = pos;

  const char *line, *wordstart, *drawstart;
  line = drawstart = text;

  struct rgba usecolor = color;
  check_caret(caret, line-drawstart, cursor, scale, usecolor);

  while (*line != '\0') {
    if (isblank(*line)) {
      sdrawCharacter(use_font->Characters[*line], cursor, scale, usecolor);
      cursor.X += use_font->Characters[*line].Advance * tracking * scale;
      line++;
      check_caret(caret, line-drawstart, cursor, scale, usecolor);      
    } else if (isspace(*line)) {
      sdrawCharacter(use_font->Characters[*line], cursor, scale, usecolor);
      cursor.Y -= scale * use_font->linegap;
      cursor.X = pos.X;
      line++;
      check_caret(caret, line-drawstart, cursor, scale, usecolor);
    } else {
      if (*line == '\e')
        line = esc_color(line, &usecolor, color);

      wordstart = line;
      int wordWidth = 0;

      while (!isspace(*line) && *line != '\0') {

        wordWidth += use_font->Characters[*line].Advance * tracking * scale; 
        line++;
      }

      if (lw > 0 && (cursor.X + wordWidth - pos.X) >= lw) {
        cursor.X = pos.X;
        cursor.Y -= scale * use_font->linegap;
      }

      while (wordstart < line) {
        if (*wordstart == '\e')
          wordstart = esc_color(wordstart, &usecolor, color);

	      //sdrawCharacter(use_font->Characters[*wordstart], HMM_AddV2(cursor, HMM_MulV2F((HMM_Vec2){1,-1},scale)), scale, (rgba){0,0,0,255});
        sdrawCharacter(use_font->Characters[*wordstart], cursor, scale, usecolor);

        cursor.X += use_font->Characters[*wordstart].Advance * tracking * scale;
        wordstart++;
	      check_caret(caret, wordstart-drawstart, cursor, scale, usecolor);	
      }
    }
  }

  return cursor.Y - pos.Y;
}
