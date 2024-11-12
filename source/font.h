#ifndef FONT_H
#define FONT_H

#include "sokol/sokol_gfx.h"
#include "render.h"
#include "HandmadeMath.h"
#include <quickjs.h>
#include "texture.h"

typedef enum {
  LEFT,
  RIGHT,
  CENTER,
  JUSTIFY
} ALIGN;

struct shader;
struct window;

extern sg_buffer text_ssbo;

/// Holds all state information relevant to a character as loaded using FreeType
struct Character {
  float Advance; // Horizontal offset to advance to next glyph
  float leftbearing; // X offset from cursor to render at
  float topbearing; // Y offset from cursor to render at 
  struct rect rect; // the rect on the font image to render from, uv coordinates
  HMM_Vec2 size; // The pixel size of this letter
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
typedef struct Character glyph;

void font_free(JSRuntime *rt,font *f);

struct sFont *MakeFont(void *data, size_t len, int height);
void sdrawCharacter(struct Character c, HMM_Vec2 cursor, float scale, struct rgba color);
void renderText(const char *text, HMM_Vec2 pos, font *f, float scale, struct rgba color, float wrap);
HMM_Vec2 measure_text(const char *text, font *f, float scale, float letterSpacing, float wrap);

// Flushes all letters from renderText calls into the provided buffer
int text_flush(sg_buffer *buf);

#endif
