#ifndef WINDOW_H
#define WINDOW_H

#include <HandmadeMath.h>

struct window {
  double dpi;
  int render;
  int mouseFocus;
  int keyboardFocus;
  int fullscreen;
  int minimized;
  int iconified;
  int focus;
  int shown;
  char *title;
  int vsync;
  int enable_clipboard;
  int enable_dragndrop;
  int high_dpi;
  int sample_count;
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
void window_seticon(struct window *w, struct texture *icon);

#endif
