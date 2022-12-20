#include "render.h"
#include "camera.h"
#include "window.h"
#include "engine.h"
#include "input.h"
#include "openglrender.h"
#include "script.h"
#include "editor.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>


#ifdef __linux__
#include <execinfo.h>
#endif

#include <signal.h>
#include <time.h>

#include "string.h"

int physOn = 0;

double renderlag = 0;
double physlag = 0;
double updatelag = 0;

double renderMS = 1/165.f;
double physMS = 1/120.f;
double updateMS = 1/60.f;

static int ed = 1;

void seghandle(int sig) {
#ifdef __linux__
    void *ents[512];
    size_t size;

    size = backtrace(ents, 512);
    if (strsignal(sig)) {
        YughCritical("CRASH! Signal: %s.", strsignal(sig));
    }
    else {
        YughCritical("CRASH! Signal: %d.", sig);
    }

    YughCritical("====================BACKTRACE====================");
    char **stackstr = backtrace_symbols(ents, size);

    for (int i = 0; i < size; i++) {
        YughCritical(stackstr[i]);
    }

    exit(1);
#endif
}

int main(int argc, char **args) {
    int logout = 1;

    for (int i = 1; i < argc; i++) {
        if (args[i][0] == '-') {
            switch(args[i][1]) {
                case 'p':
                    if (strncmp(&args[i][2], "lay", 3))
                        continue;

                    ed = 0;
                    break;

                case 'l':
                    if (i+1 < argc && args[i+1][0] != '-') {
                        log_setfile(args[i+1]);
                        i++;
                        continue;
                    }
                    else {
                        YughError("Expected a file for command line arg '-l'.");
                        exit(1);
                    }

		case 'v':
		    printf("Yugine version %s, %s build.\n", VER, INFO);
		    printf("Copyright 2022 odplot productions LLC.\n");
		    exit(1);
		    break;

		case 'h':
		    printf("-l       Set log file\n");
		    printf("-play    Launch engine in play mode instead of editor mode\n");
		    printf("-v       Display engine info\n");
		    exit(0);
		    break;

		case 'c':
		    logout = 0;
		    break;

            }
        }
    }

    if (DBG && logout) {
        time_t now = time(NULL);
        char fname[100];
        snprintf(fname, 100, "yugine-%d.log", now);
        log_setfile(fname);
    }

    YughInfo("Starting yugine version %s.", VER);

    signal(SIGSEGV, seghandle);

    FILE *sysinfo = NULL;
    sysinfo = popen("uname -a", "r");
    if (!sysinfo) {
        YughWarn("Failed to get sys info.");
    } else {
        log_cat(sysinfo);
        pclose(sysinfo);
    }

    FILE *gameinfo = NULL;
    gameinfo = fopen("game.info", "w");
    fprintf(gameinfo, "Yugine v. %s, sys %s.", VER, INFO);
    fclose(gameinfo);

    engine_init();



    const GLFWvidmode *vidmode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    YughInfo("Refresh rate is %d", vidmode->refreshRate);

    renderMS = 1.0/vidmode->refreshRate;

    script_dofile("scripts/engine.scm");
    script_dofile("config.scm");

    window_set_icon("icon.png");

    if (ed) {
        editor_init(MakeSDLWindow("Editor", 600, 600, 0));
    } else {
        script_dofile("game.scm");
    }

    openglInit();

    double lastTick;

    while (!want_quit()) {
         double elapsed = glfwGetTime() - lastTick;
         deltaT = elapsed;
         lastTick = glfwGetTime();


         timer_update(lastTick);

         call_updates();

         renderlag += elapsed;
         physlag += elapsed;

         if (physlag >= physMS) {
             physlag -= physMS;
             phys2d_update(physMS);
         }


        if (renderlag >= renderMS) {
            renderlag -= renderMS;
            window_renderall();
        }


        double wait = fmax(0, renderMS-elapsed);
        input_poll(wait);
        window_all_handle_events();
    }

    return 0;
}
