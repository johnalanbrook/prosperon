#include "font.h"

#include "render.h"
#include <shader.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <window.h>

#include <stb_truetype.h>

static uint32_t VBO = 0;
static uint32_t VAO = 0;

unsigned char ttf_buffer[1<<25];
unsigned char temp_bitmap[512 * 512];

struct sFont *font;
static struct mShader *shader;

void font_init(struct mShader *textshader) {
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
}

void font_frame(struct mSDLWindow *w) {
    shader_use(shader);
}

// Height in pixels
struct sFont *MakeFont(const char *fontfile, int height)
{
    shader_use(shader);

    struct sFont *newfont = calloc(1, sizeof(struct sFont));
    newfont->height = height;

    char fontpath[256];
    snprintf(fontpath, 256, "fonts/%s", fontfile);
     fread(ttf_buffer, 1, 1<<25, fopen(fontpath, "rb"));


    stbtt_fontinfo fontinfo;
    if (!stbtt_InitFont(&fontinfo, ttf_buffer, stbtt_GetFontOffsetForIndex(ttf_buffer,0))) {
        printf("failed\n");
    }

    float scale = stbtt_ScaleForPixelHeight(&fontinfo, height);

    int ascent, descent, linegap;

    stbtt_GetFontVMetrics(&fontinfo, &ascent, &descent, &linegap);

    ascent = roundf(ascent*scale);
    descent = roundf(descent*scale);


    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);


    for (unsigned char c = 32; c < 128; c++) {
	unsigned char *bitmap;
	int advance, lsb, w, h, x0, y0;
	stbtt_GetCodepointHMetrics(&fontinfo, c, &advance, &lsb);

	bitmap = stbtt_GetCodepointBitmap(&fontinfo, scale, scale, c, &w, &h, &x0, &y0);

	GLuint ftexture;
	glGenTextures(1, &ftexture);
	glBindTexture(GL_TEXTURE_2D, ftexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, bitmap);

	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	newfont->Characters[c].TextureID = ftexture;
	newfont->Characters[c].Advance = advance * scale;
	newfont->Characters[c].Size[0] = w;
	newfont->Characters[c].Size[1] = h;
	newfont->Characters[c].Bearing[0] = x0;
	newfont->Characters[c].Bearing[1] = y0*-1;
    }

    return newfont;
}

void sdrawCharacter(struct Character c, mfloat_t cursor[2], float scale, struct mShader *shader, float color[3])
{
    float w = c.Size[0] * scale;
    float h = c.Size[1] * scale;

    float xpos = cursor[0] + c.Bearing[0] * scale;
    float ypos = cursor[1] + (c.Bearing[1] * scale) - h;

    float verts[4 * 4] = {
	xpos, ypos, 0.f, 0.f,
	xpos+w, ypos, 1.f, 0.f,
	xpos, ypos + h, 0.f, 1.f,
	xpos + w, ypos + h, 1.f, 1.f
    };


////// Outline calculation
    // float outlineWidth = 1.1;

    // float ow = c.Size[0] * scale * outlineWidth;
    // float oh = c.Size[1] * scale * outlineWidth;

    // float oxpos = cursor[0] + c.Bearing[0] * scale * outlineWidth - ((ow-w)/2);
    // float oypos = cursor[1] - (c.Size[1] - c.Bearing[1]) * scale * outlineWidth - ((oh-h)/2);

    // float overts[4*4] = {
    //     oxpos, oypos + oh, 0.f, 0.f,
    //     oxpos, oypos, 0.f, 1.f,
    //     oxpos + ow, oypos + oh, 1.f, 0.f,
    //     oxpos + ow, oypos, 1.f, 1.f
    // };


/////////// Shadow calculation


    float shadowOffset = 6.f;
    float sxpos = cursor[0] + c.Bearing[0] * scale + (scale * shadowOffset);
    float sypos = cursor[1] - (c.Size[1] - c.Bearing[1]) * scale - (scale * shadowOffset);

    float sverts[4 * 4] = {
	sxpos, sypos, 0.f, 0.f,
	sxpos+w, sypos, 1.f, 0.f,
	sxpos, sypos + h, 0.f, 1.f,
	sxpos + w, sypos+h, 1.f, 1.f
    };


    glBindTexture(GL_TEXTURE_2D, c.TextureID);

    //// Shadow pass

    float black[3] = { 0, 0, 0 };
    shader_setvec3(shader, "textColor", black);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(sverts), sverts);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);


    //// Character pass
    shader_setvec3(shader, "textColor", color);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

}

void text_settype(struct sFont *mfont)
{
    font = mfont;
}

void renderText(const char *text, mfloat_t pos[2], float scale, mfloat_t color[3], float lw)
{
    //shader_use(shader);
    shader_setvec3(shader, "textColor", color);

    mfloat_t cursor[2] = { 0.f };
    cursor[0] = pos[0];
    cursor[1] = pos[1];

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    const unsigned char *line, *wordstart;
    line = (unsigned char*)text;


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


/*
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    */
}
