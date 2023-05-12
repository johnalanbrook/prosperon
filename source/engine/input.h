#ifndef INPUT_H
#define INPUT_H

#include "script.h"
#include "window.h"
#include <chipmunk/chipmunk.h>
#include <stdint.h>

extern int32_t mouseWheelX;
extern int32_t mouseWheelY;

extern cpVect mouse_pos;
extern cpVect mouse_delta;

extern float deltaT;

void input_init();
void input_poll(double wait);

void cursor_hide();
void cursor_show();
void set_mouse_mode(int mousemode);

void call_input_signal(char *signal);
const char *keyname_extd(int key, int scancode);
int action_down(int scancode);

void register_pawn(struct callee c);
void register_gamepad(struct callee c);

int want_quit();
void quit();

void win_key_callback(GLFWwindow *w, int key, int scancode, int action, int mods);

struct inputaction {
  int scancode;
};

void input_to_nuke();
void input_to_game();

#endif
