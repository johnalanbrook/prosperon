#include "render.h"
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

double physMS = sFPS144;
double physlag = 0;
double renderMS = sFPS144;
double renderlag = 0;

struct mCamera camera = {0};

int main(int argc, char **args)
{
    camera.speed = 500;

    engine_init();

    struct mSDLWindow *window = MakeSDLWindow("Untitled Game", 1920, 1080, 0);

    openglInit();

    editor_init(window);

    int quit = 0;

    //While application is running
    while (!quit) {
	deltaT = elapsed_time();

	physlag += deltaT;
	renderlag += deltaT;

	input_poll();

	if (physlag >= physMS) {
	    phys2d_update(physMS);

	    physlag -= physMS;
	}

	if (renderlag >= renderMS) {
	    if (physOn) {
		update_gameobjects();
	    }

	    camera_2d_update(&camera, renderMS);

	    openglRender(window, &camera);

	    editor_render();

	    window_swap(window);

	    renderlag -= renderMS;
	}
    }

    engine_stop();

    return 0;
}
