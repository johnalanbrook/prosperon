#include <SDL2/SDL.h>
#include "camera.h"
#include "window.h"
#include "engine.h"
#include "editor.h"
#include "input.h"
#include "2dphysics.h"
#include "openglrender.h"
#include "gameobject.h"

int physOn = 0;
unsigned int frameCount = 0;
Uint32 lastTick = 0;
Uint32 frameTick = 0;
Uint32 elapsed = 0;

Uint32 physMS = FPS144;
Uint32 physlag = 0;
Uint32 renderMS = FPS144;
Uint32 renderlag = 0;

struct mCamera camera = {0};

int main(int argc, char **args)
{
    camera.speed = 500;

    engine_init();

    struct mSDLWindow *window = MakeSDLWindow("Untitled Game", 1920, 1080,
			   SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN |
			   SDL_WINDOW_RESIZABLE);


    openglInit();


    editor_init(window);


    int quit = 0;
    SDL_Event e;

    //While application is running
    while (!quit) {
	frameTick = SDL_GetTicks();
	elapsed = frameTick - lastTick;
	lastTick = frameTick;
	deltaT = elapsed / 1000.f;

	physlag += elapsed;
	renderlag += elapsed;

	input_poll();

	if (physlag >= physMS) {
	    phys2d_update(physMS / 1000.f);

	    physlag -= physMS;
	}


	if (renderlag >= renderMS) {
	    if (physOn) {
		update_gameobjects();
	    }


	    camera_2d_update(&camera, renderMS / 1000.f);

	    openglRender(window, &camera);



	    editor_render();


	    window_swap(window);

	    renderlag -= renderMS;
	}
    }


    engine_stop();


    return 0;
}
