#include "font.h"

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "render.h"

#include "stb_image_write.h"
#include "stb_rect_pack.h"
#include "stb_truetype.h"
#include "stb_ds.h"

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

void font_free(JSRuntime *rt, font *f)
{
  sg_destroy_image(f->texID);
  free(f);
}

struct sFont *MakeSDFFont(const char *fontfile, int height)
{
  int packsize = 1024;
  struct sFont *newfont = calloc(1, sizeof(struct sFont));
  newfont->height = height;

  char fontpath[256];
  snprintf(fontpath, 256, "fonts/%s", fontfile);

//  unsigned char *ttf_buffer = slurp_file(fontpath, NULL);
  unsigned char *bitmap = malloc(packsize * packsize);

  stbtt_fontinfo fontinfo;
//  if (!stbtt_InitFont(&fontinfo, ttf_buffer, stbtt_GetFontOffsetForIndex(ttf_buffer, 0))) {
//    YughError("Failed to make font %s", fontfile);
//  }

  for (int i = 32; i < 95; i++) {
    int w, h, xoff, yoff;
//    unsigned char *stbtt_GetGlyphSDF(&fontinfo, height, i, 1, 0, 1, &w, &h, &xoff, &yoff);
  }

  return newfont;
}

struct sFont *MakeFont(void *ttf_buffer, size_t len, int height) {
  if (!ttf_buffer)
    return NULL;
    
  int packsize = 2048;

  struct sFont *newfont = calloc(1, sizeof(struct sFont));
  newfont->height = height;

  unsigned char *bitmap = malloc(packsize * packsize);

  stbtt_packedchar glyphs[95];

  stbtt_pack_context pc;

  int pad = 2;

  stbtt_PackBegin(&pc, bitmap, packsize, packsize, 0, pad, NULL);
  stbtt_PackFontRange(&pc, ttf_buffer, 0, height, 32, 95, glyphs);
  stbtt_PackEnd(&pc);

  stbtt_fontinfo fontinfo;
  if (!stbtt_InitFont(&fontinfo, ttf_buffer, stbtt_GetFontOffsetForIndex(ttf_buffer, 0))) {
//    YughError("Failed to make font %s", fontfile);
  }
  
  int ascent, descent, linegap;

  stbtt_GetFontVMetrics(&fontinfo, &ascent, &descent, &linegap);
  float emscale = stbtt_ScaleForMappingEmToPixels(&fontinfo, height);
  newfont->ascent = ascent*emscale;
  newfont->descent = descent*emscale;
  newfont->linegap = linegap*emscale;
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
    r.y = (glyph.y1) / (float)packsize;
    r.h = (glyph.y0-glyph.y1) / (float)packsize;

    newfont->Characters[c].size = (HMM_Vec2){
      .x = glyph.x1-glyph.x0,
      .y = glyph.y1-glyph.y0
    };

    newfont->Characters[c].Advance = glyph.xadvance; /* x distance from this char to the next */
    newfont->Characters[c].leftbearing = glyph.xoff;
    newfont->Characters[c].topbearing = -glyph.yoff2;//newfont->ascent - glyph.yoff;
    newfont->Characters[c].rect = r;
  }

  free(bitmap);

  return newfont;
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
  struct text_vert vert;

  vert.pos.x = cursor.X + c.leftbearing;
  vert.pos.y = cursor.Y + c.topbearing;
  vert.wh = c.size;

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

// text is a string, font f, size is height in pixels, wrap is how long a line is before wrapping. -1to not wrap
HMM_Vec2 measure_text(const char *text, font *f, float size, float letterSpacing, float wrap)
{
  HMM_Vec2 dim = {0};
  float maxWidth = 0; // max width of any line
  float lineWidth = 0; // current line width
  float scale = size/f->height;
  scale = 1;
  float lineHeight = f->ascent - f->descent;
  lineHeight *= scale;
  letterSpacing *= scale;

  float height = lineHeight; // total height

  for (char *c = text; *c != 0; c++) {
    if (*c == '\n') {
      maxWidth = fmaxf(maxWidth, lineWidth);
      lineWidth = 0;
      height += lineHeight + f->linegap;
      continue;
    }
    lineWidth += f->Characters[*c].Advance + letterSpacing;
  }

  maxWidth = fmaxf(maxWidth, lineWidth);
  dim.x = maxWidth;
  dim.y = height;
  return dim;
}

/* pos given in screen coordinates */
void renderText(const char *text, HMM_Vec2 pos, font *f, float scale, struct rgba color, float wrap) {
  int len = strlen(text);

  HMM_Vec2 cursor = pos;
  float lineHeight = f->ascent - f->descent;
  float lineWidth = 0;

  for (char *c = text; *c != 0; c++) {
    if (*c == '\n') {
      cursor.x = pos.x;
      cursor.y += lineHeight + f->linegap;
      continue;
    }

    sdrawCharacter(f->Characters[*c], cursor, scale, color);
    cursor.x += f->Characters[*c].Advance;
  }
  return;

  const char *line, *wordstart, *drawstart;
  line = drawstart = text;

  struct rgba usecolor = color;

  while (*line != '\0') {
    if (isblank(*line)) {
      sdrawCharacter(f->Characters[*line], cursor, scale, usecolor);
      cursor.X += f->Characters[*line].Advance * scale;
      line++;
    } else if (isspace(*line)) {
      sdrawCharacter(f->Characters[*line], cursor, scale, usecolor);
      cursor.Y -= scale * f->linegap;
      cursor.X = pos.X;
      line++;
    } else {
      if (*line == '\e')
        line = esc_color(line, &usecolor, color);

      wordstart = line;
      int wordWidth = 0;

      while (!isspace(*line) && *line != '\0') {

        wordWidth += f->Characters[*line].Advance * scale; 
        line++;
      }

      if (wrap > 0 && (cursor.X + wordWidth - pos.X) >= wrap) {
        cursor.X = pos.X;
        cursor.Y -= scale * f->linegap;
      }

      while (wordstart < line) {
        if (*wordstart == '\e')
          wordstart = esc_color(wordstart, &usecolor, color);

        sdrawCharacter(f->Characters[*wordstart], cursor, scale, usecolor);

        cursor.X += f->Characters[*wordstart].Advance * scale;
        wordstart++;
      }
    }
  }
}
