#include "font.h"

#include <shader.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <GL/glew.h>


#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

static struct Character Characters[127] = { '\0' };

static uint32_t VBO = 0;
static uint32_t VAO = 0;

unsigned char ttf_buffer[24 << 20];
unsigned char temp_bitmap[512 * 512];

static struct sFont *font;
static struct mShader *shader;

struct sFont *MakeFont(const char *fontfile, int height)
{
    struct sFont *newfont = calloc(1, sizeof(struct sFont));
    newfont->height = height;

    char fontpath[256];
    snprintf(fontpath, 256, "fonts/%s", fontfile);

    stbtt_fontinfo fontinfo = { 0 };
    int i, j, ascent, baseline, ch = 0;

    stbtt_InitFont(&fontinfo, ttf_buffer, 0);
    stbtt_GetFontVMetrics(&fontinfo, &ascent, 0, 0);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    for (unsigned char c = 0; c < 128; c++) {
	unsigned char *bitmap;
	int advance, lsb, w, h;
	stbtt_GetCodepointHMetrics(&fontinfo, c, &advance, &lsb);
	bitmap =
	    stbtt_GetCodepointBitmap(&fontinfo, 0,
				     stbtt_ScaleForPixelHeight(&fontinfo,
							       newfont->
							       height), c,
				     &w, &h, 0, 0);

	GLuint ftexture;
	glGenTextures(1, &ftexture);
	glBindTexture(GL_TEXTURE_2D, ftexture);
	glTexImage2D(GL_TEXTURE_2D,
		     0, GL_RED, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, bitmap);

	//glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
			GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
			GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
			GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
			GL_LINEAR_MIPMAP_LINEAR);

	Characters[c].TextureID = ftexture;
	Characters[c].Advance = advance;
	Characters[c].Size[0] = w;
	Characters[c].Size[1] = h;
	Characters[c].Bearing[0] = lsb;
	Characters[c].Bearing[1] = 0;
    }

    // configure VAO/VBO for texture quads
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 4, NULL,
		 GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return newfont;
}

void sdrawCharacter(struct Character c, mfloat_t cursor[2], float scale,
		    struct mShader *shader, float color[3])
{
    float xpos = cursor[0] + c.Bearing[0] * scale;
    float ypos = cursor[1] - (c.Size[1] - c.Bearing[1]) * scale;

    float w = c.Size[0] * scale;
    float h = c.Size[1] * scale;

    float verts[4 * 4] = {
	xpos, ypos + h, 0.f, 0.f,
	xpos, ypos, 0.f, 1.f,
	xpos + w, ypos + h, 1.f, 0.f,
	xpos + w, ypos, 1.f, 1.f
    };

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

    float shadowOffset = 6.f;
    float sxpos =
	cursor[0] + c.Bearing[0] * scale + (scale * shadowOffset);
    float sypos =
	cursor[1] - (c.Size[1] - c.Bearing[1]) * scale -
	(scale * shadowOffset);

    float sverts[4 * 4] = {
	sxpos, sypos + h, 0.f, 0.f,
	sxpos, sypos, 0.f, 1.f,
	sxpos + w, sypos + h, 1.f, 0.f,
	sxpos + w, sypos, 1.f, 1.f
    };


    glBindTexture(GL_TEXTURE_2D, c.TextureID);

    //// Shadow pass
    float black[3] = { 0, 0, 0 };
    shader_setvec3(shader, "textColor", black);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(sverts), sverts);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    //// Outline pass



    shader_setvec3(shader, "textColor", color);

    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

}

void text_settype(struct sFont *mfont, struct mShader *mshader)
{
    font = mfont;
    shader = mshader;
}

void renderText(const char *text, mfloat_t pos[2], float scale, mfloat_t color[3], float lw)
{
    shader_use(shader);
    shader_setvec3(shader, "textColor", color);

    mfloat_t cursor[2] = { 0.f };
    cursor[0] = pos[0];
    cursor[1] = pos[1];

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    char modText[sizeof(*text)];
    strcpy(modText, text);

    char *line, *wordstart;
    line = strtok(modText, "\n");

    while (line != NULL) {
	cursor[0] = pos[0];

	int wordWidth = 0;

	// iterate through all characters
	while (*line != '\0') {

	    wordstart = line;
	    if (!isspace(*line)) {
		struct Character ch = Characters[*line];

		if (lw > 0
		    && (cursor[0] + ((ch.Advance >> 6) * scale) - pos[0] >=
			lw)) {
		    cursor[0] = pos[0];
		    cursor[1] -= scale * font->height;

		} else {
		    // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
		    cursor[0] += (ch.Advance >> 6) * scale;	// bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
		}

		sdrawCharacter(ch, cursor, scale, shader, color);
		++line;
	    } else {
		while (!isspace(*line) || *line != '\0') {	// Seek line to the end of the word
		    ++line;
		    wordWidth += (Characters[*line].Advance >> 6) * scale;
		}

		// Now wordStart and stringPos surround the word, go through them. If the word that's about to be drawn goes past the line width, go to next line
		if (lw > 0 && (cursor[0] + wordWidth - pos[0] >= lw)) {
		    cursor[0] = pos[0];
		    cursor[1] -= scale * font->height;
		}

		while (wordstart < line) {	// Go through


		    struct Character ch = Characters[*wordstart];
		    sdrawCharacter(Characters[*wordstart], cursor, scale,
				   shader, color);

		    cursor[0] += (ch.Advance >> 6) * scale;
		    ++wordstart;
		}
	    }



	}
	cursor[1] -= scale * font->height;

	line = strtok(NULL, "\n");
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}
