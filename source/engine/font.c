#include "font.h"

#include "render.h"
#include <shader.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <window.h>
#include "log.h"

#include "openglrender.h"

#include "stb_truetype.h"
#include "stb_rect_pack.h"
#include "stb_image_write.h"

struct sFont *font;
static struct shader *shader;
static sg_shader fontshader;

unsigned char *slurp_file(const char *filename) {
    FILE *f = fopen(filename, "rb");

    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char *slurp = malloc(fsize+1);
    fread(slurp,fsize,1,f);
     fclose(f);

     return slurp;
}

char *slurp_text(const char *filename) {
 FILE *f = fopen(filename, "r'");
 if (!f) return NULL;

    char *buf;
    long int fsize;
    fseek(f, 0, SEEK_END);
    fsize = ftell(f);
    buf = malloc(fsize+1);
    rewind(f);
    size_t r = fread(buf, sizeof(char), fsize, f);
    buf[r] = '\0';

    fclose(f);

    return buf;
}

int slurp_write(const char *txt, const char *filename)
{
   FILE *f = fopen(filename, "w");
   if (!f) return 1;

   fputs(txt, f);
   fclose(f);
   return 0;
}

static sg_bindings bind_text;
static sg_pipeline pipe_text;

void font_init(struct shader *textshader) {
    shader = textshader;

    fontshader = sg_make_shader(&(sg_shader_desc){
      .vs.source = slurp_text("shaders/textvert.glsl"),
      .fs.source = slurp_text("shaders/textfrag.glsl"),
      .vs.uniform_blocks[0] = {
        .size = sizeof(float)*16,
//	.layout = SG_UNIFORMLAYOUT_STD140,
	.uniforms = {
	  [0] = { .name = "projection", .type = SG_UNIFORMTYPE_MAT4 }
	}
      },

      .fs.images[0] = {
        .name = "text",
	.image_type = SG_IMAGETYPE_2D,
	.sampler_type = SG_SAMPLERTYPE_FLOAT
      }
    });

    pipe_text = sg_make_pipeline(&(sg_pipeline_desc){
      .shader = fontshader,
      .layout = {
        .attrs = {
	  [0].format = SG_VERTEXFORMAT_FLOAT2,
	  [0].buffer_index = 0,
	  [1].format = SG_VERTEXFORMAT_FLOAT2,
	  [1].buffer_index = 0,
	  [2].format = SG_VERTEXFORMAT_FLOAT3,
	  [2].buffer_index = 1},
	.buffers[2].step_func = SG_VERTEXSTEP_PER_INSTANCE,
      },
 //     .primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP,
      .label = "text pipeline"
    });

    bind_text.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
      .size = sizeof(float)*24*3*1024*1024,
      .type = SG_BUFFERTYPE_VERTEXBUFFER,
      .usage = SG_USAGE_STREAM,
      .label = "text buffer"
    });
    
    bind_text.vertex_buffers[1] = sg_make_buffer(&(sg_buffer_desc){
      .size = sizeof(float)*3*40000,
      .type = SG_BUFFERTYPE_VERTEXBUFFER,
      .usage = SG_USAGE_STREAM,
      .label = "text color buffer"
    });
    
    font = MakeFont("LessPerfectDOSVGA.ttf", 16);
    bind_text.fs_images[0] = font->texID;    
}

void font_frame(struct window *w) {
    shader_use(shader);
}

struct sFont *MakeFont(const char *fontfile, int height)
{
    YughInfo("Making font %s.", fontfile);

    int packsize = 128; 

    struct sFont *newfont = calloc(1, sizeof(struct sFont));
    newfont->height = height;

    char fontpath[256];
    snprintf(fontpath, 256, "fonts/%s", fontfile);

     unsigned char *ttf_buffer = slurp_file(fontpath);
     unsigned char *bitmap = malloc(packsize*packsize);

     stbtt_packedchar glyphs[95];

     stbtt_pack_context pc;

     stbtt_PackBegin(&pc, bitmap, packsize, packsize, 0, 1, NULL);
     stbtt_PackFontRange(&pc, ttf_buffer, 0, height, 32, 95, glyphs);
     stbtt_PackEnd(&pc);

     stbi_write_png("packedfont.png", packsize, packsize, 1, bitmap, sizeof(char) * packsize);

    stbtt_fontinfo fontinfo;
    if (!stbtt_InitFont(&fontinfo, ttf_buffer, stbtt_GetFontOffsetForIndex(ttf_buffer,0))) {
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
	.size = packsize*packsize
      }
    });

    free(ttf_buffer);
    free(bitmap);

    for (unsigned char c = 32; c < 127; c++) {
	stbtt_packedchar glyph = glyphs[c-32];

         struct glrect r;
         r.s0 = glyph.x0 / (float) packsize;
         r.s1 = glyph.x1 / (float) packsize;
         r.t0 = glyph.y0 / (float) packsize;
         r.t1 = glyph.y1 / (float) packsize;

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

void draw_char_box(struct Character c, float cursor[2], float scale, float color[3])
{
  int x, y, w, h;

  x = cursor[0];
  y = cursor[1];
  w = 8*scale;
  h = 14;
  x += w/2.f;
  y += h/2.f;

  draw_rect(x,y,w,h,color);
}

void text_flush()
{

    sg_apply_pipeline(pipe_text);
    sg_apply_bindings(&bind_text);
    sg_apply_uniforms(SG_SHADERSTAGE_VS,0,SG_RANGE_REF(projection));
  
  sg_draw(0,6*curchar,1);  
  curchar = 0;
}

void fill_charverts(float *verts, float cursor[2], float scale, struct Character c, float *offset)
{
  float w = c.Size[0] * scale;
  float h = c.Size[1] * scale;

  float xpos = cursor[0] + (c.Bearing[0]+offset[0]) * scale;
  float ypos = cursor[1] - (c.Bearing[1]+offset[1]) * scale;
  
  float v[24] = {
	xpos, ypos, c.rect.s0, c.rect.t1,
	xpos+w, ypos, c.rect.s1, c.rect.t1,
	xpos, ypos + h, c.rect.s0, c.rect.t0,
	xpos, ypos + h, c.rect.s0, c.rect.t0,
	xpos+w, ypos, c.rect.s1, c.rect.t1,	
	xpos + w, ypos + h, c.rect.s1, c.rect.t0
    };
    
  memcpy(verts, v, sizeof(float)*24);
}

static int drawcaret = 0;

void sdrawCharacter(struct Character c, mfloat_t cursor[2], float scale, struct shader *shader, float color[3])
{
    float shadowcolor[3] = {0.f, 0.f, 0.f};	
    float shadowcursor[2];
    
    float verts[24];
    float offset[2] = {-1, 1};
    
    fill_charverts(verts, cursor, scale, c, offset);
    
        /* Check if the vertex is off screen */
    if (verts[5] < 0 || verts[10] < 0 || verts[0] > window_i(0)->width || verts[1] > window_i(0)->height)
        return;
    
    curchar++;    
    /* SET COLOR ? */
    sg_append_buffer(bind_text.vertex_buffers[0], SG_RANGE_REF(verts));
    sg_append_buffer(bind_text.vertex_buffers[1], SG_RANGE_REF(color));
    
    return;

//    fill_charverts(verts, cursor, scale, c, offset);

/*
    if (drawcaret == curchar) {
        draw_char_box(c, cursor, scale, color);
	    shader_use(shader);
    shader_setvec3(shader, "textColor", color);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, font->texID);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
      }
*/
//    sg_apply_uniforms(SG_SHADERSTAGE_FS, 0, SG_RANGE_REF(shadowcolor));
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
    offset[0] = offset[1] = 0;
    fill_charverts(verts, cursor, scale, c, offset);
    /* SET COLOR ? */
    sg_update_buffer(bind_text.vertex_buffers[0], SG_RANGE_REF(verts));
}

void text_settype(struct sFont *mfont)
{
    font = mfont;
}

int renderText(const char *text, mfloat_t pos[2], float scale, mfloat_t color[3], float lw, int caret)
{
    int len = strlen(text);
    drawcaret = caret;

    mfloat_t cursor[2] = { 0.f };
    cursor[0] = pos[0];
    cursor[1] = pos[1];
    
    const unsigned char *line, *wordstart, *drawstart;
    line = drawstart = (unsigned char*)text;

    float *usecolor = color;

    while (*line != '\0') {

        switch (*line) {
            case '\n':
	        sdrawCharacter(font->Characters[*line], cursor, scale, shader, usecolor);
                cursor[1] -= scale * font->height;
		cursor[0] = pos[0];
                line++;
                break;

            case ' ':
                sdrawCharacter(font->Characters[*line], cursor, scale, shader, usecolor);
                cursor[0] += font->Characters[*line].Advance * scale;
                line++;
                break;
		
	    case '\t':
	        sdrawCharacter(font->Characters[*line], cursor, scale, shader, usecolor);
                cursor[0] += font->Characters[*line].Advance * scale;
                line++;
                break;
	      

            default:
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
                    sdrawCharacter(font->Characters[*wordstart], cursor, scale, shader, usecolor);
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
