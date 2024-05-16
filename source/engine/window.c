#include "window.h"
#include "input.h"
#include "log.h"
#include "script.h"
#include "texture.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "jsffi.h"

#include "render.h"

#include "stb_ds.h"

#include "sokol/sokol_app.h"
#include "stb_image_resize2.h"

struct window mainwin = {
  .sample_count = 1,
  .vsync = 1,
  .enable_clipboard = 0,
  .enable_dragndrop = 0,
};

static struct window *windows = NULL;

struct texture *icon = NULL;

void window_resize(int width, int height)
{
  script_evalf("prosperon.resize([%d,%d]);", width,height);
}

void window_focused(int focus)
{
  mainwin.focus = focus;
  script_evalf("prosperon.focus(%d);", focus);
}

void window_iconified(int s)
{
  mainwin.iconified = s;
  script_evalf("prosperon.iconified(%d);", s);
}

void window_suspended(int s)
{
  script_evalf("prosperon.suspended(%d);", s);
}

void set_icon(const char *png) {
  icon = texture_from_file(png);
  window_seticon(&mainwin, icon);
}

void window_setfullscreen(window *w, int f)
{
  if (w->fullscreen == f) return;
  w->fullscreen = f;
  
  if (!w->start) return;
  
  if (sapp_is_fullscreen() == f) return;
  sapp_toggle_fullscreen();
}

void window_seticon(struct window *w, struct texture *tex)
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
    stbir_resize_uint8_linear(tex->data, tex->width, tex->height, 0, sizes[i].data, sizes[i].size, sizes[i].size, 0, 4);
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

void window_free(window *w)
{

}
