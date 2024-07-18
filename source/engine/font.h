#ifndef FONT_H
#define FONT_H

#include "sokol/sokol_gfx.h"
#include "render.h"
#include "HandmadeMath.h"

struct shader;
struct window;

extern sg_buffer text_ssbo;

/// Holds all state information relevant to a character as loaded using FreeType
struct Character {
  float Size[2];     // Size of glyph
  float Bearing[2];  // Offset from baseline to left/top of glyph
  int Advance; // Horizontal offset to advance to next glyph
  int leftbearing;
  struct rect rect;
};

struct sFont {
  uint32_t fontTexture;
  uint32_t height; /* in pixels */
  float ascent;
  float descent;
  float linegap;
  struct Character Characters[256];
  sg_image texID;
  texture *texture;
};

typedef struct sFont font;

void font_free(font *f);

struct sFont *MakeFont(const char *fontfile, int height);
void font_set(font *f);
void sdrawCharacter(struct Character c, HMM_Vec2 cursor, float scale, struct rgba color);
void text_settype(struct sFont *font);
struct boundingbox text_bb(const char *text, float scale, float lw, float tracking);
int renderText(const char *text, HMM_Vec2 pos, float scale, struct rgba color, float lw, int caret, float tracking);

int text_flush(sg_buffer *buf);

#endif
