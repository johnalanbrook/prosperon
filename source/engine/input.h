#ifndef INPUT_H
#define INPUT_H

#include <SDL2/SDL.h>
#include <stdint.h>

extern int32_t mouseWheelX;
extern int32_t mouseWheelY;
extern int ychange;
extern int xchange;
extern float deltaT;
extern int quit;
extern SDL_Event e;
extern uint8_t *currentKeystates;

void input_poll();

#endif
