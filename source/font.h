#ifndef FONT_H
#define FONT_H

#include "render.h"
#include "HandmadeMath.h"
#include <quickjs.h>
#include "texture.h"
#include <SDL3/SDL.h>
#include "render.h"

typedef enum {
  LEFT,
  RIGHT,
  CENTER,
  JUSTIFY
} ALIGN;

struct text_vert {
  HMM_Vec2 pos;
  HMM_Vec2 uv;
  HMM_Vec4 color;
};

typedef struct text_vert text_vert;

struct shader;
struct window;

struct character {
  float advance;
  rect quad;
  rect uv;
  float xoff;
  float yoff;
  float width;
  float height;
};

// text data
struct sFont {
  uint32_t height; /* in pixels */
  float ascent; // pixels
  float descent; // pixels
  float linegap; //pixels
  struct character Characters[256];
  SDL_Surface *surface;
};

typedef struct sFont font;
typedef struct Character glyph;

void font_free(JSRuntime *rt,font *f);

struct sFont *MakeFont(void *data, size_t len, int height);
struct text_vert *renderText(const char *text, HMM_Vec2 pos, font *f, float scale, struct rgba color, float wrap);
HMM_Vec2 measure_text(const char *text, font *f, float scale, float letterSpacing, float wrap);

// Flushes all letters from renderText calls into the provided buffer
int text_flush();

#endif
