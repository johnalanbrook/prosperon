#ifndef INPUT_H
#define INPUT_H

#include "script.h"
#include <stdint.h>
#include "HandmadeMath.h"
#include "sokol_app.h"

void cursor_hide();
void cursor_show();
void cursor_img(const char *path);
void set_mouse_mode(int mousemode);
void touch_start(sapp_touchpoint *touch, int n);
void touch_move(sapp_touchpoint *touch, int n);
void touch_end(sapp_touchpoint *touch, int n);
void touch_cancelled(sapp_touchpoint *touch, int n);

void input_dropped_files(int n);
void input_clipboard_paste(char *str);

#endif
