#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>
#include "window.h"

extern int32_t mouseWheelX;
extern int32_t mouseWheelY;
extern int ychange;
extern int xchange;
extern float deltaT;
extern int quit;

void input_init();
void input_poll();

void cursor_hide();
void cursor_show();

int action_down(int scancode);

struct inputaction
{
    int scancode;

};

#endif
