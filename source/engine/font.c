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

static uint32_t VBO = 0;
static uint32_t VAO = 0;

struct sFont *font;
static struct shader *shader;

unsigned char *slurp_file(const char *filename) {
    FILE *f = fopen(filename, "rb");

    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char *slurp = malloc(fsize);
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

void font_init(struct shader *textshader) {
    shader = textshader;

    shader_use(shader);

    glGenBuffers(1, &VBO);
    glGenVertexArrays(1, &VAO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(0);
 
    glBindVertexArray(0);

    // Default font
    //font = MakeFont("teenytinypixels.ttf", 30);
    font = MakeFont("LessPerfectDOSVGA.ttf", 16);
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

    FILE *f = fopen(fontpath, "rb");
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char *ttf_buffer = malloc(fsize+1);
     fread(ttf_buffer, fsize, 1, f);
     fclose(f);

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

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &newfont->texID);
    glBindTexture(GL_TEXTURE_2D, newfont->texID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, packsize, packsize, 0, GL_RED, GL_UNSIGNED_BYTE, bitmap);

    //glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

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
static float *buffdraw;

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

void fill_charverts(float *verts, float cursor[2], float scale, struct Character c, float *offset)
{
  float w = c.Size[0] * scale;
  float h = c.Size[1] * scale;

  float xpos = cursor[0] + (c.Bearing[0]+offset[0]) * scale;
  float ypos = cursor[1] - (c.Bearing[1]+offset[1]) * scale;
  
  float v[16] = {
	xpos, ypos, c.rect.s0, c.rect.t1,
	xpos+w, ypos, c.rect.s1, c.rect.t1,
	xpos, ypos + h, c.rect.s0, c.rect.t0,
	xpos + w, ypos + h, c.rect.s1, c.rect.t0
    };
    
  memcpy(verts, v, sizeof(float)*16);
}

static int drawcaret = 0;

void sdrawCharacter(struct Character c, mfloat_t cursor[2], float scale, struct shader *shader, float color[3])
{
    float shadowcolor[3] = {0.f, 0.f, 0.f};	
    float shadowcursor[2];
    
    float verts[16];
    float offset[2] = {-1, 1};

    fill_charverts(verts, cursor, scale, c, offset);
    curchar++;
    
        /* Check if the vertex is off screen */
    if (verts[5] < 0 || verts[10] < 0 || verts[0] > window_i(0)->width || verts[1] > window_i(0)->height)
        return;

    if (drawcaret == curchar) {
        draw_char_box(c, cursor, scale, color);
	    shader_use(shader);
    shader_setvec3(shader, "textColor", color);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, font->texID);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

      }


    shader_setvec3(shader, "textColor", shadowcolor);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    
    offset[0] = 1;
    offset[1] = -1;
    fill_charverts(verts, cursor, scale, c, offset);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    
    offset[1] = 1;
    fill_charverts(verts, cursor, scale, c, offset);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    offset[0] = -1;
    offset[1] = -1;
        fill_charverts(verts, cursor, scale, c, offset);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    
    
    offset[0] = offset[1] = 0;
    fill_charverts(verts, cursor, scale, c, offset);
    shader_setvec3(shader, "textColor", color);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);


}

void text_settype(struct sFont *mfont)
{
    font = mfont;
}

void renderText(const char *text, mfloat_t pos[2], float scale, mfloat_t color[3], float lw, int caret)
{
    int len = strlen(text);
    drawcaret = caret;

    mfloat_t cursor[2] = { 0.f };
    cursor[0] = pos[0];
    cursor[1] = pos[1];
    shader_use(shader);
    shader_setvec3(shader, "textColor", color);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, font->texID);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, len*16*sizeof(float)*2, NULL, GL_STREAM_DRAW); /* x2 on the size for the outline pass */
    
    const unsigned char *line, *wordstart, *drawstart;
    line = drawstart = (unsigned char*)text;

    curchar = 0;

    float *usecolor = color;
    float caretcolor[3] = {0.4,0.98,0.75};

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

//   glDrawArrays(GL_TRIANGLE_STRIP, 0, 4*2);
}
