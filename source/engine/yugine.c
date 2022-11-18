#include "render.h"
#include "camera.h"
#include "window.h"
#include "engine.h"
#include "input.h"
#include "openglrender.h"
#include "script.h"
#include "editor.h"
#include "log.h"

#include "string.h"

int physOn = 0;

double renderlag = 0;
double physlag = 0;
double updatelag = 0;

double renderMS = 1/60.f;
double physMS = 1/120.f;
double updateMS = 1/60.f;

static int ed = 1;


int main(int argc, char **args) {
    for (int i = 1; i < argc; i++) {
        if (args[i][0] == '-') {
            if (strncmp(&args[i][1], "play", 4) == 0) {
                ed = 0;
            }
        }
    }


    engine_init();

    const GLFWvidmode *vidmode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    YughInfo("Refresh rate is %d", vidmode->refreshRate);

    renderMS = 1.0/vidmode->refreshRate;

    double framet = fmin(fmin(renderMS,physMS),updateMS);

    script_dofile("engine.rb");
    script_dofile("config.rb");

    window_set_icon("icon.png");

    if (ed) {
        editor_init(MakeSDLWindow("Editor", 600, 600, 0));
    } else {
        script_dofile("game.rb");
    }

    openglInit();

    renderMS = 0.033;

    double lastTick;

    while (!quit) {
         double elapsed;
         elapsed = glfwGetTime() - lastTick;
         lastTick = glfwGetTime();

         //timer_update(lastTick);

         //renderlag += elapsed;
         //physlag += elapsed;


        if (renderlag >= renderMS) {
            renderlag -= renderMS;
        }

        window_renderall();
        input_poll(renderMS- elapsed < 0 ? 0 : renderMS - elapsed);
        window_all_handle_events();


    }

    return 0;
}
