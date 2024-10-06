#include "input.h"

#include "sokol/sokol_app.h"
#include "font.h"
#include "log.h"
#include "script.h"
#include "stb_ds.h"
#include "time.h"
#include <ctype.h>
#include "resources.h"
#include "jsffi.h"

void input_dropped_files(int n)
{
  script_evalf("prosperon.droppedfile(`%s`);", sapp_get_dropped_file_path(0));
}

void input_clipboard_paste(char *str)
{
  script_evalf("prosperon.clipboardpaste(`%s`);", sapp_get_clipboard_string());
}

static char *touch_jstrn(char *dest, int len, sapp_touchpoint *touch, int n)
{
  dest[0] = 0;
  char touchdest[512] = {0};
  strncat(dest,"[", 512);
  for (int i = 0; i < n; i++) {
    snprintf(touchdest, 512, "{id:%zd, x:%g, y:%g},", touch[i].identifier, touch[i].pos_x, touch[i].pos_y);
    strncat(dest,touchdest,512);
  }
  strncat(dest,"]", 512);
  return dest;
}

void touch_start(sapp_touchpoint *touch, int n)
{
  char dest[512] = {0};
  script_evalf("prosperon.touchpress(%s);", touch_jstrn(dest, 512, touch, n));
}

void touch_move(sapp_touchpoint *touch, int n)
{
  char dest[512] = {0};
  script_evalf("prosperon.touchmove(%s);", touch_jstrn(dest,512,touch,n));
}

void touch_end(sapp_touchpoint *touch, int n)
{
  char dest[512] = {0};
  script_evalf("prosperon.touchend(%s);", touch_jstrn(dest,512,touch,n));
}

void touch_cancelled(sapp_touchpoint *touch, int n)
{
  char dest[512] = {0};
  script_evalf("prosperon.touchend(%s);", touch_jstrn(dest,512,touch,n));
}
