#define PL_MPEG_IMPLEMENTATION
#define CGLTF_IMPLEMENTATION
#define GL_GLEXT_PROTOTYPES

//#define MATHC_USE_INT16
//#define MATHC_FLOATING_POINT_TYPE GLfloat
//#define MATHC_USE_DOUBLE_FLOATING_POINT

#define STB_DS_IMPLEMENTATION
#include <stb_ds.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <pl_mpeg.h>

#ifdef EDITOR
#include "editor.h"
#endif

#include <SDL.h>
#include "openglrender.h"
#include "window.h"
#include "camera.h"
#include "input.h"
#include "sprite.h"
#include "2dphysics.h"
#include "gameobject.h"
#include "registry.h"

#define FPS30 33
#define FPS60 17
#define FPS120 8;
#define FPS144 7
#define FPS300 3

unsigned int frameCount = 0;
Uint32 lastTick = 0;
Uint32 frameTick = 0;
Uint32 elapsed = 0;

Uint32 physMS = FPS144;
Uint32 physlag = 0;
Uint32 renderMS = FPS144;
Uint32 renderlag = 0;



int main(int argc, char **args)
{

    script_init();

    registry_init();
    gameobjects = vec_make(sizeof(struct mGameObject), 100);
    prefabs = vec_make(MAXNAME, 25);

    // TODO: Init these on the heap instead
    struct mCamera camera = { 0 };
    camera.speed = 500;

    stbi_set_flip_vertically_on_load(1);

    resources_init();
    openglInit();
    sprite_initialize();

#ifdef EDITOR
    editor_init(window);
#endif

    phys2d_init();

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

	    openglRender(&camera);


#ifdef EDITOR
	    editor_render();
#endif

	    window_swap(window);

	    renderlag -= renderMS;
	}
    }


    SDL_StopTextInput();
    SDL_Quit();

    return 0;
}
