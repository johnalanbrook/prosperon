#ifndef INPUT_H
#define INPUT_H

#include "sokol/sokol_app.h"

#include "script.h"
#include "window.h"
#include <chipmunk/chipmunk.h>
#include <stdint.h>

extern int32_t mouseWheelX;
extern int32_t mouseWheelY;

extern cpVect mouse_pos;
extern cpVect mouse_delta;

#define INPUT_DOWN 0
#define INPUT_UP 1
#define INPUT_REPEAT 2

extern float deltaT;

void input_init();
void input_poll(double wait);

void cursor_hide();
void cursor_show();
void set_mouse_mode(int mousemode);

void input_mouse(int btn, int state);
void input_mouse_move(float x, float y, float dx, float dy);
void input_mouse_scroll(float x, float y);
void input_btn(int btn, int state, int mod);
void input_key(int key);

const char *keyname_extd(int key);
int action_down(int key);

void register_pawn(struct callee c);
void register_gamepad(struct callee c);

int want_quit();
void quit();

#endif
