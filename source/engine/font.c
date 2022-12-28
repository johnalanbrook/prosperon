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

#include <stb_truetype.h>
#include "stb_rect_pack.h"

static uint32_t VBO = 0;
static uint32_t VAO = 0;

unsigned char ttf_buffer[1<<25];
unsigned char temp_bitmap[512 * 512];

struct sFont *font;
static struct shader *shader;

void font_init(struct shader *textshader) {
    shader = textshader;

    shader_use(shader);

    // configure VAO/VBO for texture quads
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 4, NULL, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);


    // Default font
    font = MakeFont("teenytinypixels.ttf", 16);
}

void font_frame(struct window *w) {
    shader_use(shader);
}

struct sFont *MakeFont(const char *fontfile, int height)
{
    shader_use(shader);

    struct sFont *newfont = calloc(1, sizeof(struct sFont));
    newfont->height = height;

    char fontpath[256];
    snprintf(fontpath, 256, "fonts/%s", fontfile);
     fread(ttf_buffer, 1, 1<<25, fopen(fontpath, "rb"));

     unsigned char *bitmap = malloc(1024*1024);

     stbtt_packedchar glyphs[95];

     stbtt_pack_context pc;

     stbtt_PackBegin(&pc, bitmap, 1024, 1024, 0, 1, NULL);
     stbtt_PackSetOversampling(&pc, 1, 1);
     stbtt_PackFontRange(&pc, ttf_buffer, 0, height, 32, 95, glyphs);
     stbtt_PackEnd(&pc);



    stbtt_fontinfo fontinfo;
    if (!stbtt_InitFont(&fontinfo, ttf_buffer, stbtt_GetFontOffsetForIndex(ttf_buffer,0))) {
        YughError("Failed to make font %s", fontfile);
    }

    float scale = stbtt_ScaleForPixelHeight(&fontinfo, height);
     //glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &newfont->texID);
    glBindTexture(GL_TEXTURE_2D, newfont->texID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 1024, 1024, 0, GL_RED, GL_UNSIGNED_BYTE, bitmap);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);



    for (unsigned char c = 32; c < 127; c++) {
	//unsigned char *bitmap;
	int advance, lsb, w, h, x0, y0;
	stbtt_GetCodepointHMetrics(&fontinfo, c, &advance, &lsb);
	stbtt_packedchar glyph = glyphs[c-32];

	YughInfo("Packed char is at %d, %d, %d, %d", glyphs[c-32].x0, glyphs[c-32].y0, glyphs[c-32].x1, glyphs[c-32].y1);

         struct glrect r;
         r.s0 = glyph.x0 / (float)1024;
         r.s1 = glyph.x1 / (float) 1024;
         r.t0 = glyph.y0 / (float) 1024;
         r.t1 = glyph.y1 / (float) 1024;
	YughInfo("That is %f %f %f %f", r.s0, r.t0, r.s1, r.t1);
	newfont->Characters[c].Advance = advance * scale;
	newfont->Characters[c].Size[0] = glyphs[c-32].x1 - glyphs[c-32].x0;
	newfont->Characters[c].Size[1] = glyphs[c-32].y1 - glyphs[c-32].y0;
	newfont->Characters[c].Bearing[0] = x0;
	newfont->Characters[c].Bearing[1] = y0*-1;
	newfont->Characters[c].rect = r;
    }

    return newfont;
}

static int curchar = 0;

void sdrawCharacter(struct Character c, mfloat_t cursor[2], float scale, struct shader *shader, float color[3])
{
    float w = c.Size[0] * scale;
    float h = c.Size[1] * scale;

    float xpos = cursor[0] + c.Bearing[0] * scale;
    float ypos = cursor[1] + (c.Bearing[1] * scale) - h;

    float verts[4 * 4] = {
	xpos, ypos, c.rect.s0, c.rect.t1,
	xpos+w, ypos, c.rect.s1, c.rect.t1,
	xpos, ypos + h, c.rect.s0, c.rect.t0,
	xpos + w, ypos + h, c.rect.s1, c.rect.t0
    };

    glBufferSubData(GL_ARRAY_BUFFER, curchar*sizeof(verts), sizeof(verts), verts);

    curchar++;
}

void text_settype(struct sFont *mfont)
{
    font = mfont;
}

void renderText(const char *text, mfloat_t pos[2], float scale, mfloat_t color[3], float lw)
{
    shader_use(shader);
    shader_setvec3(shader, "textColor", color);

    int len = strlen(text);

    mfloat_t cursor[2] = { 0.f };
    cursor[0] = pos[0];
    cursor[1] = pos[1];

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, font->texID);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, len*16*sizeof(float), NULL, GL_STREAM_DRAW);

    const unsigned char *line, *wordstart;
    line = (unsigned char*)text;

    curchar = 0;



    while (*line != '\0') {

        switch (*line) {
            case '\n':
                cursor[1] -= scale * font->height;
                line++;
                break;

            case ' ':
                sdrawCharacter(font->Characters[*line], cursor, scale, shader, color);
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
                    sdrawCharacter(font->Characters[*wordstart], cursor, scale, shader, color);
                    cursor[0] += font->Characters[*wordstart].Advance * scale;
                    wordstart++;
                }
        }
    }

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4*curchar);
}
