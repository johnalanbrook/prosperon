#define PL_MPEG_IMPLEMENTATION
#define CGLTF_IMPLEMENTATION
#define GL_GLEXT_PROTOTYPES
#define STB_DS_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION

#include <SDL2/SDL.h>

#include "engine.h"

int main(int argc, char **args)
{
/*
    engine_init();

    struct mSDLWindow *window = MakeSDLWindow("Untitled Game", 1920, 1080,
			   SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN |
			   SDL_WINDOW_RESIZABLE);


    openglInit(window);


    editor_init(window);


    quit = false;
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
		vec_walk(gameobjects, gameobject_update);
	    }


	    camera_2d_update(&camera, renderMS / 1000.f);

	    openglRender(window, &camera);



	    editor_render();


	    window_swap(window);

	    renderlag -= renderMS;
	}
    }


    engine_stop();
    */

    return 0;
}
