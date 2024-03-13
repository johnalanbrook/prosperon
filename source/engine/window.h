#ifndef WINDOW_H
#define WINDOW_H

#include <HandmadeMath.h>

struct window {
  int id;
  HMM_Vec2 size; // Actual width and height of the window
  HMM_Vec2 rendersize; // The desired rendering resolution, what the assets are at
  HMM_Vec2 psize;
  float left;
  float top;
  double dpi;
  int render;
  int mouseFocus;
  int keyboardFocus;
  int fullscreen;
  int minimized;
  int iconified;
  int focus;
  int shown;
  int mode;
  int editor; // true if only should redraw on input
  float aspect;
  float raspect;
  char *title;
  int start;
};
typedef struct window window;
struct texture;
extern struct window mainwin;

void window_resize(int width, int height);
void window_focused(int focus);
void window_iconified(int s);
void window_suspended(int s);


void window_apply(window *w);
void window_free(window *w);

void window_setfullscreen(window *w, int f);
void window_set_icon(const char *png);
void window_seticon(struct window *w, struct texture *icon);

void window_render(struct window *w);

#endif
