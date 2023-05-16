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

#include "openglrender.h"

#include "stb_image_write.h"
#include "stb_rect_pack.h"
#include "stb_truetype.h"

struct sFont *font;

#define max_chars 40000

unsigned char *slurp_file(const char *filename) {
  FILE *f = fopen(filename, "rb");

  if (!f) return NULL;

  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  fseek(f, 0, SEEK_SET);
  unsigned char *slurp = malloc(fsize + 1);
  fread(slurp, fsize, 1, f);
  fclose(f);

  return slurp;
}

char *slurp_text(const char *filename) {
  FILE *f = fopen(filename, "r'");
  
  if (!f) {
    YughWarn("File %s doesn't exist.", filename);
    return NULL;
  }

  char *buf;
  long int fsize;
  fseek(f, 0, SEEK_END);
  fsize = ftell(f);
  buf = malloc(fsize + 1);
  rewind(f);
  size_t r = fread(buf, sizeof(char), fsize, f);
  buf[r] = '\0';

  fclose(f);

  return buf;
}

int slurp_write(const char *txt, const char *filename) {
  FILE *f = fopen(filename, "w");
  if (!f) return 1;

  fputs(txt, f);
  fclose(f);
  return 0;
}

static sg_shader fontshader;
static sg_bindings bind_text;
static sg_pipeline pipe_text;
struct text_vert {
  cpVect pos;
  cpVect wh;
  struct uv_n uv;
  struct uv_n st;
  struct rgba color;
};

static struct text_vert text_buffer[max_chars];

void font_init(struct shader *textshader) {
  fontshader = sg_make_shader(&(sg_shader_desc){
      .vs.source = slurp_text("shaders/textvert.glsl"),
      .fs.source = slurp_text("shaders/textfrag.glsl"),
      .vs.uniform_blocks[0] = {
          .size = sizeof(float) * 16,
          //	.layout = SG_UNIFORMLAYOUT_STD140,
          .uniforms = {
              [0] = {.name = "projection", .type = SG_UNIFORMTYPE_MAT4}}},

      .fs.images[0] = {.name = "text", .image_type = SG_IMAGETYPE_2D, .sampler_type = SG_SAMPLERTYPE_FLOAT}});

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
      .label = "text buffer"});

  font = MakeFont("LessPerfectDOSVGA.ttf", 16);
  bind_text.fs_images[0] = font->texID;
}

struct sFont *MakeFont(const char *fontfile, int height) {
  YughInfo("Making font %s.", fontfile);

  int packsize = 1024;

  struct sFont *newfont = calloc(1, sizeof(struct sFont));
  newfont->height = height;

  char fontpath[256];
  snprintf(fontpath, 256, "fonts/%s", fontfile);

  unsigned char *ttf_buffer = slurp_file(fontpath);
  unsigned char *bitmap = malloc(packsize * packsize);

  stbtt_packedchar glyphs[95];

  stbtt_pack_context pc;

  stbtt_PackBegin(&pc, bitmap, packsize, packsize, 0, 1, NULL);
  stbtt_PackFontRange(&pc, ttf_buffer, 0, height, 32, 95, glyphs);
  stbtt_PackEnd(&pc);

  stbi_write_png("packedfont.png", packsize, packsize, 1, bitmap, sizeof(char) * packsize);

  stbtt_fontinfo fontinfo;
  if (!stbtt_InitFont(&fontinfo, ttf_buffer, stbtt_GetFontOffsetForIndex(ttf_buffer, 0))) {
    YughError("Failed to make font %s", fontfile);
  }

  newfont->texID = sg_make_image(&(sg_image_desc){
      .type = SG_IMAGETYPE_2D,
      .width = packsize,
      .height = packsize,
      .pixel_format = SG_PIXELFORMAT_R8,
      .usage = SG_USAGE_IMMUTABLE,
      .min_filter = SG_FILTER_NEAREST,
      .mag_filter = SG_FILTER_NEAREST,
      .data.subimage[0][0] = {
          .ptr = bitmap,
          .size = packsize * packsize}});

  free(ttf_buffer);
  free(bitmap);

  for (unsigned char c = 32; c < 127; c++) {
    stbtt_packedchar glyph = glyphs[c - 32];

    struct glrect r;
    r.s0 = glyph.x0 / (float)packsize;
    r.s1 = glyph.x1 / (float)packsize;
    r.t0 = glyph.y0 / (float)packsize;
    r.t1 = glyph.y1 / (float)packsize;

    newfont->Characters[c].Advance = glyph.xadvance;
    newfont->Characters[c].Size[0] = glyph.x1 - glyph.x0;
    newfont->Characters[c].Size[1] = glyph.y1 - glyph.y0;
    newfont->Characters[c].Bearing[0] = glyph.xoff;
    newfont->Characters[c].Bearing[1] = glyph.yoff2;
    newfont->Characters[c].rect = r;
  }

  return newfont;
}

static int curchar = 0;

void draw_char_box(struct Character c, float cursor[2], float scale, float color[3]) {
  int x, y, w, h;

  x = cursor[0];
  y = cursor[1];
  w = 8 * scale;
  h = 14;
  x += w / 2.f;
  y += h / 2.f;

  draw_rect(x, y, w, h, color);
}

void text_flush() {
  if (curchar == 0) return;
  sg_apply_pipeline(pipe_text);
  sg_apply_bindings(&bind_text);
  sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, SG_RANGE_REF(projection));

  sg_range verts;
  verts.ptr = text_buffer;
  verts.size = sizeof(struct text_vert) * curchar;
  sg_update_buffer(bind_text.vertex_buffers[0], &verts);

  sg_draw(0, 4, curchar);
  curchar = 0;
}

static int drawcaret = 0;

void sdrawCharacter(struct Character c, mfloat_t cursor[2], float scale, struct rgba color) {
  float offset[2] = {-1, 1};
  
  struct text_vert vert;
  
  vert.wh.x = c.Size[0] * scale;
  vert.wh.y = c.Size[1] * scale;
  vert.pos.x = cursor[0] - (c.Bearing[0] + offset[0]) * scale;
  vert.pos.y = cursor[1] - (c.Bearing[1] + offset[1]) * scale;
  vert.uv.u = c.rect.s0*USHRT_MAX;
  vert.uv.v = c.rect.t0*USHRT_MAX;
  vert.st.u = (c.rect.s1-c.rect.s0)*USHRT_MAX;
  vert.st.v = (c.rect.t1-c.rect.t0)*USHRT_MAX;
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

int renderText(const char *text, mfloat_t pos[2], float scale, struct rgba color, float lw, int caret) {
  int len = strlen(text);
  drawcaret = caret;

  mfloat_t cursor[2] = {0.f};
  cursor[0] = pos[0];
  cursor[1] = pos[1];

  const unsigned char *line, *wordstart, *drawstart;
  line = drawstart = (unsigned char *)text;

  struct rgba usecolor = color;

  while (*line != '\0') {
    if (isblank(*line)) {
      sdrawCharacter(font->Characters[*line], cursor, scale, usecolor);
      cursor[0] += font->Characters[*line].Advance * scale;
      line++;
    } else if (isspace(*line)) {
      sdrawCharacter(font->Characters[*line], cursor, scale, usecolor);
      cursor[1] -= scale * font->height;
      cursor[0] = pos[0];
      line++;

    } else {

      wordstart = line;
      int wordWidth = 0;

      while (!isspace(*line) && *line != '\0') {
        wordWidth += font->Characters[*line].Advance * scale;
        line++;
      }

      if (lw > 0 && (cursor[0] + wordWidth - pos[0]) >= lw) {
        cursor[0] = pos[0];
        cursor[1] -= scale * font->height;
      }

      while (wordstart < line) {
        sdrawCharacter(font->Characters[*wordstart], cursor, scale, usecolor);
        cursor[0] += font->Characters[*wordstart].Advance * scale;
        wordstart++;
      }
    }
  }
  /*    if (caret > curchar) {
          draw_char_box(font->Characters[69], cursor, scale, color);
      }
  */

  return cursor[1] - pos[1];
}
