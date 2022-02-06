#include "engine.h"

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

#include "render.h"

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
#include "timer.h"
#include "script.h"
#include "vec.h"
#include "sound.h"

// TODO: Init on the heap


#include "engine.h"

void error_callback(int error, const char *description)
{
    fprintf(stderr, "Error: %s\n", description);
}

void engine_init()
{
    /* Initialize GLFW */
    if (!glfwInit()) {
        printf("Could not init GLFW\n");
    }
    glfwSetErrorCallback(error_callback);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);


    resources_init();
    script_init();
    registry_init();
    init_gameobjects();
    timer_init();

    prefabs = vec_make(MAXNAME, 25);
    stbi_set_flip_vertically_on_load(1);
    phys2d_init();
    gui_init();
    sound_init();
}

void engine_stop()
{
    glfwTerminate();
}
