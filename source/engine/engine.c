#include "engine.h"

#define STB_DS_IMPLEMENTATION
#include <stb_ds.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define PL_MPEG_IMPLEMENTATION
#include <pl_mpeg.h>


#ifdef EDITOR
#include "editor.h"
#endif

#include "render.h"

#include "openglrender.h"
#include "window.h"
#include "camera.h"
#include "input.h"
#include "sprite.h"
#include "2dphysics.h"
#include "gameobject.h"
#include "log.h"
#include "resources.h"
#include "timer.h"
#include "script.h"

#include "sound.h"

#include "engine.h"

void error_callback(int error, const char *description)
{
    fprintf(stderr, "Error: %s\n", description);
    YughError("GLFW Error: %s", description);
}

void engine_init()
{
    glfwSetErrorCallback(error_callback);
    /* Initialize GLFW */
    if (!glfwInit()) {
        YughError("Could not init GLFW. Exiting.");
        exit(1);
    }

    resources_init();
    
    YughInfo("Starting physics ...");
    phys2d_init();
    
    YughInfo("Starting sound ...");
    sound_init();

    YughInfo("Starting scripts ...");
    script_init();
}
