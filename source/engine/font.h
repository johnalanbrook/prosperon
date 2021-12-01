#ifndef FONT_H
#define FONT_H

#include "mathc.h"

struct mShader;

/// Holds all state information relevant to a character as loaded using FreeType
struct Character {
    uint32_t TextureID;		// ID handle of the glyph texture
    mfloat_t Size[2];		// Size of glyph
    mfloat_t Bearing[2];	// Offset from baseline to left/top of glyph
    unsigned int Advance;	// Horizontal offset to advance to next glyph
};

struct sFont {
    uint32_t fontTexture;
    uint32_t height;
};

struct sFont MakeFont(const char *fontfile, int height);
void sdrawCharacter(struct Character c, mfloat_t cursor[2], float scale,
		    struct mShader *shader, float color[3]);
void renderText(struct sFont font, struct mShader *shader,
		const char *text, mfloat_t pos[2], float scale,
		mfloat_t color[3], float lw);


#endif
