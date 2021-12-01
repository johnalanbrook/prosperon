#include "input.h"

#include "window.h"
#include <SDL.h>

int32_t mouseWheelX = 0;
int32_t mouseWheelY = 0;
int ychange = 0;
int xchange = 0;
float deltaT = 0;
int quit = 0;
SDL_Event e = { 0 };

uint8_t *currentKeystates = NULL;

void input_poll()
{
    ychange = 0;
    xchange = 0;
    mouseWheelX = 0;
    mouseWheelY = 0;
    currentKeystates = SDL_GetKeyboardState(NULL);

    while (SDL_PollEvent(&e)) {
	window_handle_event(window, &e);

#ifdef EDITOR
	editor_input(&e);
#endif
    }


}
