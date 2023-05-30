#ifndef FONT_H
#define FONT_H

#include "sokol/sokol_gfx.h"
#include "texture.h"
#include "2dphysics.h"
#include "HandmadeMath.h"

struct shader;
struct window;

/// Holds all state information relevant to a character as loaded using FreeType
struct Character {
  float Size[2];     // Size of glyph
  float Bearing[2];  // Offset from baseline to left/top of glyph
  int Advance; // Horizontal offset to advance to next glyph
  int leftbearing;
  struct glrect rect;
};

struct sFont {
  uint32_t fontTexture;
  uint32_t height; /* in pixels */
  int ascent;
  int descent;
  int linegap;
  struct Character Characters[127];
  sg_image texID;
};

void font_init(struct shader *s);
struct sFont *MakeFont(const char *fontfile, int height);
void sdrawCharacter(struct Character c, HMM_Vec2 cursor, float scale, struct rgba color);
void text_settype(struct sFont *font);
struct boundingbox text_bb(const char *text, float scale, float lw, float tracking);
int renderText(const char *text, HMM_Vec2 pos, float scale, struct rgba color, float lw, int caret, float tracking);

// void text_frame();
void text_flush();

unsigned char *slurp_file(const char *filename);
char *slurp_text(const char *filename);

int slurp_write(const char *txt, const char *filename);

#endif
