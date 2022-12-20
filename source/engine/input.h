#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>
#include "window.h"

#include "script.h"

extern int32_t mouseWheelX;
extern int32_t mouseWheelY;
extern int ychange;
extern int xchange;
extern float deltaT;

void input_init();
void input_poll(double wait);

void cursor_hide();
void cursor_show();

int action_down(int scancode);

int want_quit();
void quit();

void win_key_callback(GLFWwindow *w, int key, int scancode, int action, int mods);

struct inputaction
{
    int scancode;
};

void set_pawn(s7_pointer env);

#endif
