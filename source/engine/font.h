#ifndef FONT_H
#define FONT_H

#include "mathc.h"
#include "sokol/sokol_gfx.h"
#include "texture.h"
#include "2dphysics.h"

struct shader;
struct window;

/// Holds all state information relevant to a character as loaded using FreeType
struct Character {
  mfloat_t Size[2];     // Size of glyph
  mfloat_t Bearing[2];  // Offset from baseline to left/top of glyph
  unsigned int Advance; // Horizontal offset to advance to next glyph
  struct glrect rect;
};

struct sFont {
  uint32_t fontTexture;
  uint32_t height;
  struct Character Characters[127];
  sg_image texID;
};

void font_init(struct shader *s);
struct sFont *MakeFont(const char *fontfile, int height);
void sdrawCharacter(struct Character c, mfloat_t cursor[2], float scale, struct rgba color);
void text_settype(struct sFont *font);
int renderText(const char *text, mfloat_t pos[2], float scale, struct rgba color, float lw, int caret);

// void text_frame();
void text_flush();

unsigned char *slurp_file(const char *filename);
char *slurp_text(const char *filename);

int slurp_write(const char *txt, const char *filename);

#endif
