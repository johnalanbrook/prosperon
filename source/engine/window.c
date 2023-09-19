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

#include "render.h"

#include "stb_ds.h"

#include "sokol/sokol_app.h"
#include "stb_image_resize.h"

struct window mainwin;

static struct window *windows = NULL;

struct Texture *icon = NULL;

void window_resize(int width, int height)
{
  mainwin.dpi = sapp_dpi_scale();
  mainwin.width = sapp_width();
  mainwin.height = sapp_height();
  mainwin.rwidth = mainwin.width/mainwin.dpi;
  mainwin.rheight = mainwin.height/mainwin.dpi;
  
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

void window_seticon(struct window *w, struct Texture *tex)
{
  struct isize {
    int size;
    unsigned char *data;
  };
  struct isize sizes[4];
  sizes[0].size = 16;
  sizes[1].size = 32;
  sizes[2].size = 64;
  sizes[3].size = 128;

  for (int i = 0; i < 4; i++) {
    sizes[i].data = malloc(4*sizes[i].size*sizes[i].size);
    stbir_resize_uint8(tex->data, tex->width, tex->height, 0, sizes[i].data, sizes[i].size, sizes[i].size, 0, 4);
  }

  sapp_icon_desc idsc = {
    .images = {
      { .width = sizes[0].size, .height = sizes[0].size, .pixels = { .ptr=sizes[0].data, .size=4*sizes[0].size*sizes[0].size } },
      { .width = sizes[1].size, .height = sizes[1].size, .pixels = { .ptr=sizes[1].data, .size=4*sizes[1].size*sizes[1].size } },
      { .width = sizes[2].size, .height = sizes[2].size, .pixels = { .ptr=sizes[2].data, .size=4*sizes[2].size*sizes[2].size } },
      { .width = sizes[3].size, .height = sizes[3].size, .pixels = { .ptr=sizes[3].data, .size=4*sizes[3].size*sizes[3].size } },            
    }
  };

  sapp_set_icon(&idsc);
  for (int i = 0; i < 4; i++)
    free(sizes[i].data);
}

void window_render(struct window *w) {
  openglRender(w);
}
