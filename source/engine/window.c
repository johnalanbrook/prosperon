#include "window.h"
#include "input.h"
#include "log.h"
#include "nuke.h"
#include "script.h"
#include "texture.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ffi.h"

#include "openglrender.h"

#include "stb_ds.h"

struct window mainwin;

static struct window *windows = NULL;

struct Texture *icon = NULL;

void window_resize(int width, int height)
{
  mainwin.width = width;
  mainwin.height = height;
  render_winsize();

  JSValue vals[2] = { int2js(width), int2js(height) };
  send_signal("window_resize", 2, vals);
}

void window_quit()
{
  quit();
}

void window_focused(int focus)
{
  mainwin.focus = focus;
}

void window_iconified(int s)
{
  mainwin.iconified = s;
}


void window_suspended(int s)
{
  
}

void window_set_icon(const char *png) {
  icon = texture_pullfromfile(png);
  window_seticon(&mainwin, icon);
}

void window_makefullscreen(struct window *w) {
  if (!w->fullscreen)
    window_togglefullscreen(w);
}

void window_unfullscreen(struct window *w) {
  if (w->fullscreen)
    window_togglefullscreen(w);
}

void window_togglefullscreen(struct window *w) {
  sapp_toggle_fullscreen();
  mainwin.fullscreen = sapp_is_fullscreen();
}

void window_seticon(struct window *w, struct Texture *icon) {
  
}

void window_render(struct window *w) {
  openglRender(w);
}
