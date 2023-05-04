#include "yugine.h"

#define SOKOL_IMPL
#define SOKOL_GLCORE33
#include "sokol/sokol_gfx.h"


#include "render.h"

#include "camera.h"
#include "window.h"
#include "engine.h"
#include "input.h"
#include "openglrender.h"
#include "gameobject.h"
#include "font.h"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include "timer.h"

#include "quickjs/quickjs.h"

#include "script.h"
#include "ffi.h"

#include "log.h"
#include <stdio.h>
#include <stdlib.h>

#include "2dphysics.h"

#include <signal.h>
#include <time.h>
#include <execinfo.h>

#include "string.h"

int physOn = 0;

double renderlag = 0;
double physlag = 0;
double updatelag = 0;

double renderMS = 1/165.f;
double physMS = 1/120.f;
double updateMS = 1/60.f;

static int ed = 1;
static int sim_play = 0;
static double lastTick;
static int phys_step = 0;

static float timescale = 1.f;

#define FPSBUF 10
static double framems[FPSBUF];
int framei = 0;
int fps;

#define SIM_STOP 0
#define SIM_PLAY 1
#define SIM_PAUSE 2
#define SIM_STEP 3
/*
int __builtin_clz(unsigned int a)
{
  int to;
  __asm__ __volatile__(
    "bsr %edi, %eax\n\t"
    "xor $31, %eax\n\t"
    : "=&a"(to));

  return to;
}
*/
void print_stacktrace()
{
    void *ents[512];
    size_t size;

    size = backtrace(ents, 512);

    YughCritical("====================BACKTRACE====================");
    char **stackstr = backtrace_symbols(ents, size);

    YughInfo("Stack size is %d.", size);

    for (int i = 0; i < size; i++)
        YughCritical(stackstr[i]);

    js_stacktrace();
}

void seghandle(int sig) {
#ifdef __linux__
    if (strsignal(sig))
        YughCritical("CRASH! Signal: %s.", strsignal(sig));

    print_stacktrace();

    exit(1);
#endif
}

void compile_script(const char *file)
{
  const char *script = slurp_text(file);
  JSValue obj = JS_Eval(js, script, strlen(script), file, JS_EVAL_FLAG_COMPILE_ONLY|JS_EVAL_TYPE_GLOBAL);
  size_t out_len;
  uint8_t *out;
  out = JS_WriteObject(js, &out_len, obj, JS_WRITE_OBJ_BYTECODE);

  FILE *f = fopen("out.jsc", "w");
  fwrite(out, sizeof out[0], out_len, f);
  fclose(f);
}

int main(int argc, char **args) {
    int logout = 1;
    ed = 1;

    script_startup();

    for (int i = 1; i < argc; i++) {
        if (args[i][0] == '-') {
            switch(args[i][1]) {
                case 'p':
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
		    printf("Copyright 2022-2023 odplot productions LLC.\n");
		    exit(1);
		    break;

		case 's':
		    compile_script(args[2]);
		    exit(0);

		case 'm':
		    logLevel = atoi(args[2]);
		    break;

		case 'h':
		    printf("-l       Set log file\n");
		    printf("-p    Launch engine in play mode instead of editor mode\n");
		    printf("-v       Display engine info\n");
		    printf("-c       Redirect logging to console\n");
		    exit(0);
		    break;

		case 'c':
		    logout = 0;
		    break;

            }
        }
    }

#if DBG
    if (logout) {
        time_t now = time(NULL);
        char fname[100];
        snprintf(fname, 100, "yugine-%d.log", now);
        log_setfile(fname);
    }

    YughInfo("Starting yugine version %s.", VER);

    FILE *sysinfo = NULL;
/*    sysinfo = popen("uname -a", "r");
    if (!sysinfo) {
        YughWarn("Failed to get sys info.");
    } else {
        log_cat(sysinfo);
        pclose(sysinfo);
    }*/
    signal(SIGSEGV, seghandle);
    signal(SIGABRT, seghandle);
    signal(SIGFPE, seghandle);
    signal(SIGBUS, seghandle);
    
#endif

    engine_init();

    const GLFWvidmode *vidmode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    YughInfo("Refresh rate is %d", vidmode->refreshRate);

    renderMS = 1.0/vidmode->refreshRate;
    
    input_init();    
    openglInit();

    if (ed)
      script_dofile("scripts/editor.js");
    else
      script_dofile("scripts/play.js");

    while (!want_quit()) {
         double elapsed = glfwGetTime() - lastTick;
         deltaT = elapsed;
         lastTick = glfwGetTime();
        double wait = fmax(0, renderMS-elapsed);
        input_poll(wait);
        window_all_handle_events();

         framems[framei++] = elapsed;

         if (framei == FPSBUF) framei = 0;

         if (sim_play == SIM_PLAY || sim_play == SIM_STEP) {
             timer_update(elapsed * timescale);
             physlag += elapsed;
             call_updates(elapsed * timescale);
             while (physlag >= physMS) {
	         phys_step = 1;
                 physlag -= physMS;
                 phys2d_update(physMS  * timescale);
                 call_physics(physMS * timescale); 
                 if (sim_play == SIM_STEP) sim_pause();
		 phys_step = 0;
             }
       }

        renderlag += elapsed;

        if (renderlag >= renderMS) {
            renderlag -= renderMS;
            window_renderall();
        }

        gameobjects_cleanup();
    }

    return 0;
}

int frame_fps()
{
    double fpsms = 0;
    for (int i = 0; i < FPSBUF; i++) {
      fpsms += framems[i];
    }

     return FPSBUF / fpsms;
}

int sim_playing() { return sim_play == SIM_PLAY; }
int sim_paused() { return sim_play == SIM_PAUSE; }
int sim_stopped() { return sim_play == SIM_STOP; }

void sim_start() {
  sim_play = SIM_PLAY;
}

void sim_pause() {
  sim_play = SIM_PAUSE;
}

void sim_stop() {
    /* Revert starting state of everything from sim_start */
  sim_play =  SIM_STOP;
}

int phys_stepping() { return phys_step; }

void sim_step() {
  if (sim_paused()) {
      YughInfo("Step");
      sim_play = SIM_STEP;
  }
}

void set_timescale(float val) {
    timescale = val;
}
