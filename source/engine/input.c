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

static struct callee pawn_callee;
static struct callee gamepad_callee;

void input_dropped_files(int n)
{
  script_evalf("prosperon.droppedfile('%s');", sapp_get_dropped_file_path(0));
}

/*
  0 free
  1 lock
*/
void set_mouse_mode(int mousemode) { sapp_lock_mouse(mousemode); }

void cursor_hide() { sapp_show_mouse(0); }
void cursor_show() { sapp_show_mouse(1); }

void cursor_img(const char *path)
{
/*  NSString *str = [NSString stringWithUTF8String:path];
  NSImage *img = [[NSImage alloc] initWithContentsOfFile:str];
  NSCursor *custom = [[NSCursor alloc] initWithImage:img hotSpot:NSMakePoint(0,0)];
  [custom set];
*/
}
