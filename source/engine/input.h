#ifndef INPUT_H
#define INPUT_H

#include "script.h"
#include <stdint.h>
#include "HandmadeMath.h"

extern HMM_Vec2 mousewheel;
extern HMM_Vec2 mouse_pos;
extern HMM_Vec2 mouse_delta;

#define INPUT_DOWN 0
#define INPUT_UP 1
#define INPUT_REPEAT 2

void input_init();
void input_poll(double wait);

void cursor_hide();
void cursor_show();
void set_mouse_mode(int mousemode);

void input_mouse(int btn, int state, uint32_t mod);
void input_mouse_move(float x, float y, float dx, float dy, uint32_t mod);
void input_mouse_scroll(float x, float y, uint32_t mod);
void input_btn(int btn, int state, uint32_t mod);
void input_key(uint32_t key, uint32_t mod);

void input_dropped_files(int n);

const char *keyname_extd(int key);
int action_down(int key);

void register_pawn(struct callee c);
void register_gamepad(struct callee c);

void quit();

#endif
