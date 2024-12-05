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

void font_free(JSRuntime *rt, font *f)
{
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
    
  int packsize = 1024;

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
  newfont->surface = SDL_CreateSurface(packsize,packsize, SDL_PIXELFORMAT_RGBA32);
  if (!newfont->surface) printf("SDL ERROR: %s\n", SDL_GetError());  
  for (int i = 0; i < packsize; i++)
    for (int j = 0; j < packsize; j++)
      if (!SDL_WriteSurfacePixel(newfont->surface, j, i, 255,255,255,bitmap[i*packsize+j]))
        printf("SDLERROR: %s\n", SDL_GetError());
  
  for (unsigned char c = 32; c < 127; c++) {
    stbtt_packedchar glyph = glyphs[c - 32];

    rect uv;
    uv.x = (glyph.x0) / (float)packsize;
    uv.w = (glyph.x1-glyph.x0) / (float)packsize;
    uv.y = (glyph.y1) / (float)packsize;
    uv.h = (glyph.y0-glyph.y1) / (float)packsize;
    newfont->Characters[c].uv = uv;

    rect quad;
    quad.x = glyph.xoff;
    quad.w = glyph.xoff2-glyph.xoff;
    quad.y = -glyph.yoff2;
    quad.h = glyph.yoff2-glyph.yoff;
    newfont->Characters[c].quad = quad;
    newfont->Characters[c].advance = glyph.xadvance;
//    printf("glyph for %c is x0,y0,x1,y1: %d,%d,%d,%d\n xoff %g, yoff %g, xadvance %g, xoff2 %g, yoff2 %g\n", c, glyph.x0, glyph.y1, glyph.x1, glyph.y1, glyph.xoff, glyph.yoff, glyph.xadvance, glyph.xoff2, glyph.yoff2);
  }

  free(bitmap);

  return newfont;
}

int text_flush() {
/*  if (arrlen(text_buffer) ==  0) return 0;

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
*/
}

void sdrawCharacter(struct text_vert **buffer, stbtt_packedchar c, HMM_Vec2 cursor, float scale, struct rgba color) {
  struct text_vert vert;

//  vert.pos.x = cursor.X + c.leftbearing;
//  vert.pos.y = cursor.Y + c.topbearing;
//  vert.wh = c.size;

//  if (vert.pos.x > frame.l || vert.pos.y > frame.t || (vert.pos.y + vert.wh.y) < frame.b || (vert.pos.x + vert.wh.x) < frame.l) return;

//  vert.uv.x = c.rect.x;
//  vert.uv.y = c.rect.y;
//  vert.st.x = c.rect.w;
//  vert.st.y = c.rect.h;
  rgba2floats(vert.color.e, color);

  arrput(*buffer, vert);
}

void draw_char_verts(struct text_vert **buffer, struct character c, HMM_Vec2 cursor, float scale, colorf color)
{
  // packedchar has 
  // Adds four verts: bottom left, bottom right, top left, top right
  text_vert bl;
  bl.pos.x = cursor.X + c.quad.x;
  bl.pos.y = cursor.Y + c.quad.y;
  bl.uv.x = c.uv.x;
  bl.uv.y = c.uv.y;
  bl.color = color;
  arrput(*buffer, bl);

  
  text_vert br = bl;
  br.pos.x += c.quad.w;
  br.uv.x += c.uv.w;
  arrput(*buffer, br);
  
  text_vert ul = bl;
  ul.pos.y += c.quad.h;
  ul.uv.y += c.uv.h;
  arrput(*buffer, ul);

  text_vert ur = ul;
  ur.pos.x = br.pos.x;
  ur.uv.x = br.uv.x;
  arrput(*buffer, ur);
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
  int breakAtWord = 0;
    HMM_Vec2 dim = {0};
    float maxWidth = 0; // Maximum width of any line
    float lineWidth = 0; // Current line width
    float scale = size / f->height;
    float lineHeight = (f->ascent - f->descent) * scale;
    letterSpacing *= scale;

    float height = lineHeight; // Total height
    const char *wordStart = text; // Start of the current word for word wrapping
    float wordWidth = 0; // Width of the current word
    float spaceWidth = f->Characters[' '].advance + letterSpacing; // Space character width

    for (const char *c = text; *c != '\0'; c++) {
        if (*c == '\n') {
            // Handle explicit line breaks
            maxWidth = fmaxf(maxWidth, lineWidth);
            lineWidth = 0;
            height += lineHeight + f->linegap;
            wordStart = c + 1;
            wordWidth = 0;
            continue;
        }

        float charWidth = f->Characters[*c].advance + letterSpacing;

        // Handle wrapping
        if (wrap > 0 && lineWidth + charWidth > wrap) {
            if (breakAtWord && *c != ' ') {
                // Roll back to the last word if breaking at word boundaries
                if (wordWidth > 0) {
                    lineWidth -= wordWidth + spaceWidth;
                    c = wordStart - 1; // Reset to start of the word
                }
            }

            // Finish the current line and reset
            maxWidth = fmaxf(maxWidth, lineWidth);
            lineWidth = 0;
            height += lineHeight + f->linegap;
            wordStart = c + 1; // Start a new word
            wordWidth = 0;

            // Skip to next character if wrapping on letters
            if (!breakAtWord) {
                lineWidth += charWidth;
                continue;
            }
        }

        lineWidth += charWidth;

        // Update word width if breaking at word boundaries
        if (breakAtWord) {
            if (*c == ' ') {
                wordWidth = 0;
                wordStart = c + 1;
            } else {
                wordWidth += charWidth;
            }
        }
    }

    // Finalize dimensions
    maxWidth = fmaxf(maxWidth, lineWidth);
    dim.x = maxWidth;
    dim.y = height;

    return dim;
}
/* pos given in screen coordinates */
struct text_vert *renderText(const char *text, HMM_Vec2 pos, font *f, float scale, colorf color, float wrap) {
  int wrapAtWord = 1;
  text_vert *buffer = NULL;
  int len = strlen(text);

  HMM_Vec2 cursor = pos;
  float lineHeight = f->ascent - f->descent;
  float lineWidth = 0;

  for (char *c = text; *c != 0; c++) {
    if (*c == '\n') {
      cursor.x = pos.x;
      cursor.y -= lineHeight + f->linegap;
      lineWidth = 0;
      continue;
    }

    struct character chara = f->Characters[*c];
   
    if (wrap > 0 && lineWidth + chara.advance > wrap) {
      cursor.x = pos.x;
      cursor.y -= lineHeight + f->linegap;
      lineWidth = 0;
    }
  
    draw_char_verts(&buffer, chara, cursor, scale, color);
    lineWidth += chara.advance;
    cursor.x += chara.advance;
  }
  
  return buffer;
}
