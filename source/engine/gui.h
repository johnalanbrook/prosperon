#ifndef GUI_H
#define GUI_H

#include "jsffi.h"
#include "sokol/sokol_app.h"

#ifdef __cplusplus
extern "C" {
#endif

JSValue gui_init(JSContext *js);
void gui_newframe(int x, int y, float dt);
void gfx_gui();
void gui_input(sapp_event *e);
void gui_endframe();
void gui_exit();

int gui_wantmouse();
int gui_wantkeys();

#ifdef __cplusplus
}
#endif

#endif
