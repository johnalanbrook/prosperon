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
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

static uint32_t VBO = 0;

unsigned char ttf_buffer[1<<25];

struct sFont *font;
static struct shader *shader;

void font_init(struct shader *textshader) {
    shader = textshader;

    shader_use(shader);

    glGenBuffers(1, &VBO);

    // Default font
    //font = MakeFont("teenytinypixels.ttf", 30);
    font = MakeFont("LessPerfectDOSVGA.ttf", 16);
}

void font_frame(struct window *w) {
    shader_use(shader);
}

struct sFont *MakeFont(const char *fontfile, int height)
{
    shader_use(shader);

    int packsize = 128;

    struct sFont *newfont = calloc(1, sizeof(struct sFont));
    newfont->height = height;

    char fontpath[256];
    snprintf(fontpath, 256, "fonts/%s", fontfile);
     fread(ttf_buffer, 1, 1<<25, fopen(fontpath, "rb"));

     unsigned char *bitmap = malloc(packsize*packsize);

     stbtt_packedchar glyphs[95];

     stbtt_pack_context pc;

     stbtt_PackBegin(&pc, bitmap, packsize, packsize, 0, 1, NULL);
     stbtt_PackFontRange(&pc, ttf_buffer, 0, height, 32, 95, glyphs);
     stbtt_PackEnd(&pc);

     stbi_write_png("packedfont.png", packsize, packsize, 1, bitmap, sizeof(char)*packsize);

    stbtt_fontinfo fontinfo;
    if (!stbtt_InitFont(&fontinfo, ttf_buffer, stbtt_GetFontOffsetForIndex(ttf_buffer,0))) {
        YughError("Failed to make font %s", fontfile);
    }

    float scale = stbtt_ScaleForPixelHeight(&fontinfo, height);
     glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &newfont->texID);
    glBindTexture(GL_TEXTURE_2D, newfont->texID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, packsize, packsize, 0, GL_RED, GL_UNSIGNED_BYTE, bitmap);
    //glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);



    for (unsigned char c = 32; c < 127; c++) {
	stbtt_packedchar glyph = glyphs[c-32];

	YughInfo("Packed char %c is at %d, %d, %d, %d", c, glyphs[c-32].x0, glyphs[c-32].y0, glyphs[c-32].x1, glyphs[c-32].y1);

	YughInfo("Offsets are %f %f %f %f", glyph.xoff, glyph.yoff, glyph.xoff2, glyph.yoff2);

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

void sdrawCharacter(struct Character c, mfloat_t cursor[2], float scale, struct shader *shader, float color[3])
{
    float w = c.Size[0] * scale;
    float h = c.Size[1] * scale;

    float xpos = cursor[0] + c.Bearing[0] * scale;
    float ypos = cursor[1] - c.Bearing[1] * scale;

    float verts[4 * 4] = {
	xpos, ypos, c.rect.s0, c.rect.t1,
	xpos+w, ypos, c.rect.s1, c.rect.t1,
	xpos, ypos + h, c.rect.s0, c.rect.t0,
	xpos + w, ypos + h, c.rect.s1, c.rect.t0
    };

    if (verts[5] < 0 || verts[10] < 0 || verts[0] > window_i(0)->width || verts[1] > window_i(0)->height)
        return;

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
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, len*16*sizeof(float), NULL, GL_STREAM_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);

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
