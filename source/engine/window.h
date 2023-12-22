#ifndef WINDOW_H
#define WINDOW_H

struct window {
  int id;
  int width;
  int height;
  double dpi;
  int rwidth;
  int rheight;
  int render;
  int mouseFocus;
  int keyboardFocus;
  int fullscreen;
  int minimized;
  int iconified;
  int focus;
  int shown;
};
struct Texture;
extern struct window mainwin;

void window_resize(int width, int height);
void window_quit();
void window_focused(int focus);
void window_iconified(int s);
void window_suspended(int s);

void window_makefullscreen(struct window *w);
void window_togglefullscreen(struct window *w);
void window_unfullscreen(struct window *w);

void window_set_icon(const char *png);
void window_seticon(struct window *w, struct Texture *icon);

void window_render(struct window *w);

#endif
