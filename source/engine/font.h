#ifndef FONT_H
#define FONT_H

#include "mathc.h"

struct shader;
struct window;

struct glrect {
    float s0;
    float s1;
    float t0;
    float t1;
};

/// Holds all state information relevant to a character as loaded using FreeType
struct Character {
    mfloat_t Size[2];		// Size of glyph
    mfloat_t Bearing[2];	// Offset from baseline to left/top of glyph
    unsigned int Advance;	// Horizontal offset to advance to next glyph
    struct glrect rect;
};

struct sFont {
    uint32_t fontTexture;
    uint32_t height;
    struct Character Characters[127];
    uint32_t texID;
};

struct glrect runit = { 0.f, 1.f, 0.f, 1.f };

void font_init(struct shader *s);
void font_frame(struct window *w);
struct sFont *MakeFont(const char *fontfile, int height);
void sdrawCharacter(struct Character c, mfloat_t cursor[2], float scale, struct shader *shader, float color[3]);
void text_settype(struct sFont *font);
void renderText(const char *text, mfloat_t pos[2], float scale, mfloat_t color[3], float lw);


#endif
