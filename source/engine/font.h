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
  int Advance; // Horizontal offset to advance to next glyph
  float leftbearing; // X offset from cursor to render at
  float topbearing; // Y offset from cursor to render at 
  struct rect rect; // the rect on the font image to render from
};

struct sFont {
  uint32_t height; /* in pixels */
  float ascent; // pixels
  float descent; // pixels
  float linegap; //pixels
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
