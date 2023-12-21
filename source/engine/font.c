#include "font.h"

#include "log.h"
#include "render.h"
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <window.h>
#include <chipmunk/chipmunk.h>
#include "2dphysics.h"
#include "resources.h"
#include "debugdraw.h"
#include "text.sglsl.h"

#include "stb_image_write.h"
#include "stb_rect_pack.h"
#include "stb_truetype.h"

#include "HandmadeMath.h"

struct sFont *font;

#define max_chars 10000

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
      .label = "text",
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
//  font = MakeFont("fonts/c64.ttf", 8);
//  font = MakeFont("fonts/teenytinypixels.ttf", 16);  
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
  newfont->linegap = (newfont->ascent - newfont->descent) * newfont->emscale;

  newfont->texID = sg_make_image(&(sg_image_desc){
      .type = SG_IMAGETYPE_2D,
      .width = packsize,
      .height = packsize,
      .pixel_format = SG_PIXELFORMAT_R8,
      .usage = SG_USAGE_IMMUTABLE,
      .data.subimage[0][0] = {
          .ptr = bitmap,
          .size = packsize * packsize}});


  for (unsigned char c = 32; c < 127; c++) {
    stbtt_packedchar glyph = glyphs[c - 32];

    struct glrect r;
    r.s0 = (glyph.x0) / (float)packsize;
    r.s1 = (glyph.x1) / (float)packsize;
    r.t0 = (glyph.y0) / (float)packsize;
    r.t1 = (glyph.y1) / (float)packsize;

    stbtt_GetCodepointHMetrics(&fontinfo, c, &newfont->Characters[c].Advance, &newfont->Characters[c].leftbearing);
    newfont->Characters[c].leftbearing *= newfont->emscale;

    newfont->Characters[c].Advance = glyph.xadvance; /* x distance from this char to the next */
    newfont->Characters[c].Size[0] = glyph.x1 - glyph.x0; 
    newfont->Characters[c].Size[1] = glyph.y1 - glyph.y0;
    newfont->Characters[c].Bearing[0] = glyph.xoff;
    newfont->Characters[c].Bearing[1] = glyph.yoff2;
    newfont->Characters[c].rect = r;
  }

  free(ttf_buffer);
  free(bitmap);

  return newfont;
}

static int curchar = 0;

void draw_underline_cursor(HMM_Vec2 pos, float scale, struct rgba color)
{
  pos.Y -= 2;
  sdrawCharacter(font->Characters['_'], pos, scale, color);
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

  draw_box(b, wh, color);
}

void text_flush(HMM_Mat4 *proj) {
  if (curchar == 0) return;
  sg_range verts;
  verts.ptr = text_buffer;
  verts.size = sizeof(struct text_vert) * curchar;
  int offset = sg_append_buffer(bind_text.vertex_buffers[0], &verts);
  
  bind_text.vertex_buffer_offsets[0] = offset;
  sg_apply_pipeline(pipe_text);
  sg_apply_bindings(&bind_text);
  sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, SG_RANGE_REF(*proj));
  sg_draw(0, 4, curchar);
  curchar = 0;
}

void sdrawCharacter(struct Character c, HMM_Vec2 cursor, float scale, struct rgba color) {
  if (curchar+1 >=  max_chars)
    return;

  struct rgba colorbox = {0,0,0,255};
  
  struct text_vert vert;

  float lsize = 1.0 / 1024.0;

  float oline = 1.0;
  
  vert.pos.x = cursor.X + c.Bearing[0] * scale + oline;
  vert.pos.y = cursor.Y - c.Bearing[1] * scale - oline;
  vert.wh.x = c.Size[0] * scale + (oline*2);
  vert.wh.y = c.Size[1] * scale + (oline*2);

//  if (vert.pos.x > frame.l || vert.pos.y > frame.t || (vert.pos.y + vert.wh.y) < frame.b || (vert.pos.x + vert.wh.x) < frame.l) return;

  vert.uv.u = (c.rect.s0 - oline*lsize)*USHRT_MAX;
  vert.uv.v = (c.rect.t0 - oline*lsize)*USHRT_MAX;
  vert.st.u = (c.rect.s1-c.rect.s0+oline*lsize*2.0)*USHRT_MAX;
  vert.st.v = (c.rect.t1-c.rect.t0+oline*lsize*2.0)*USHRT_MAX;
  vert.color = color;

//  sg_append_buffer(bind_text.vertex_buffers[0], &vert, sizeof(struct text_vert));
  memcpy(text_buffer + curchar, &vert, sizeof(struct text_vert));
  curchar++;
}

void text_settype(struct sFont *mfont) {
  font = mfont;
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
  struct rgba dummy;
  HMM_Vec2 cursor = {0,0};
  const char *c = text;
  const char *line, *wordstart, *drawstart;
  line = drawstart = text;

  while (*line != '\0') {
    if (isblank(*line)) {
      cursor.X += font->Characters[*line].Advance * tracking * scale;
      line++;
    } else if (isspace(*line)) {
      cursor.Y -= scale * font->linegap;
      cursor.X = 0;
      line++;
    } else {
      if (*line == '\e')
        line = esc_color(line, NULL, dummy);
    
      wordstart = line;
      int wordWidth = 0;

      while (!isspace(*line) && *line != '\0') {
        wordWidth += font->Characters[*line].Advance * tracking * scale; 
        line++;
      }

      if (lw > 0 && (cursor.X + wordWidth) >= lw) {
        cursor.X = 0;
        cursor.Y -= scale * font->linegap;
      }

      while (wordstart < line) {
        if (*wordstart == '\e')
          line = esc_color(wordstart, NULL, dummy);
      
        cursor.X += font->Characters[*wordstart].Advance * tracking * scale;
        wordstart++;
      }
    }
  }

  return cwh2bb((HMM_Vec2){0,0}, (HMM_Vec2){cursor.X,font->linegap-cursor.Y});
}

void check_caret(int caret, int l, HMM_Vec2 pos, float scale, struct rgba color)
{
  if (caret == l)
    draw_underline_cursor(pos,scale,color);
}


/* pos given in screen coordinates */
int renderText(const char *text, HMM_Vec2 pos, float scale, struct rgba color, float lw, int caret, float tracking) {
  int len = strlen(text);

  HMM_Vec2 cursor = pos;

  const char *line, *wordstart, *drawstart;
  line = drawstart = text;

  struct rgba usecolor = color;
  check_caret(caret, line-drawstart, cursor, scale, usecolor);

  while (*line != '\0') {
    if (isblank(*line)) {
      sdrawCharacter(font->Characters[*line], cursor, scale, usecolor);
      cursor.X += font->Characters[*line].Advance * tracking * scale;
      line++;
      check_caret(caret, line-drawstart, cursor, scale, usecolor);      
    } else if (isspace(*line)) {
      sdrawCharacter(font->Characters[*line], cursor, scale, usecolor);
      cursor.Y -= scale * font->linegap;
      cursor.X = pos.X;
      line++;
      check_caret(caret, line-drawstart, cursor, scale, usecolor);
    } else {
      if (*line == '\e')
        line = esc_color(line, &usecolor, color);

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
	if (*wordstart == '\e')
	  wordstart = esc_color(wordstart, &usecolor, color);
      
        sdrawCharacter(font->Characters[*wordstart], cursor, scale, usecolor);
        cursor.X += font->Characters[*wordstart].Advance * tracking * scale;
        wordstart++;
	check_caret(caret, wordstart-drawstart, cursor, scale, usecolor);	
      }
    }
  }

  return cursor.Y - pos.Y;
}
