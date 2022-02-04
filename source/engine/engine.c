#include "engine.h"

#define PL_MPEG_IMPLEMENTATION
#define CGLTF_IMPLEMENTATION
#define GL_GLEXT_PROTOTYPES
#define STB_DS_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION

#ifdef EDITOR
#include "editor.h"
#endif

#include <stb_ds.h>
#include <stb_image.h>
#include <pl_mpeg.h>

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

// TODO: Init on the heap


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