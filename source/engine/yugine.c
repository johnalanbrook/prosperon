#include "render.h"
#include "camera.h"
#include "window.h"
#include "engine.h"
#include "input.h"
#include "openglrender.h"

int physOn = 0;

double renderlag = 0;

static double renderMS = 0.033;

int main(int argc, char **args) {
    engine_init();

    window_set_icon("icon.png");

    script_dofile("game.rb");

    openglInit();

    renderMS = 0.033;

    double lastTick;
    double frameTick;

    while (!quit) {
         double elapsed;
         double lastTick;
         frameTick = glfwGetTime();
         elapsed = frameTick - lastTick;
         lastTick = frameTick;

         renderlag += elapsed;

        input_poll();

       window_all_handle_events();

       script_update(elapsed);

        if (renderlag >= renderMS) {
            renderlag -= renderMS;
            window_renderall();
        }

    }
}
