#include "engine.h"

#ifdef EDITOR
#include "editor.h"
#endif

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include "openglrender.h"
#include "window.h"
#include "camera.h"
#include "input.h"
#include "sprite.h"
#include "2dphysics.h"
#include "gameobject.h"
#include "registry.h"
#include "log.h"
#include "resources.h"



unsigned int frameCount = 0;
uint32_t lastTick = 0;
uint32_t frameTick = 0;
uint32_t elapsed = 0;

uint32_t physMS = FPS144;
uint32_t physlag = 0;
uint32_t renderMS = FPS144;
uint32_t renderlag = 0;

// TODO: Init on the heap
struct mCamera camera = {0};

#include "engine.h"

void engine_init()
{


     //Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
	YughLog(0, SDL_LOG_PRIORITY_ERROR,
		"SDL could not initialize! SDL Error: %s", SDL_GetError());
    }

    //Use OpenGL 3.3
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
			SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 2);	/* How many x MSAA */

    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 1);
resources_init();
        script_init();
    registry_init();
    init_gameobjects();

    prefabs = vec_make(MAXNAME, 25);
    camera.speed = 500;
    stbi_set_flip_vertically_on_load(1);
    phys2d_init();
    gui_init();
    sound_init();

    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
}

void engine_stop()
{
    SDL_StopTextInput();
    SDL_Quit();
}