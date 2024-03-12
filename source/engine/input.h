#ifndef INPUT_H
#define INPUT_H

#include "script.h"
#include <stdint.h>
#include "HandmadeMath.h"

void cursor_hide();
void cursor_show();
void cursor_img(const char *path);
void set_mouse_mode(int mousemode);

void input_dropped_files(int n);

const char *keyname_extd(int key);

void quit();

#endif
