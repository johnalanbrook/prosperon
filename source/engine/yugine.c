#include "yugine.h"

#include "camera.h"
#include "engine.h"
#include "font.h"
#include "gameobject.h"
#include "input.h"
#include "openglrender.h"
#include "window.h"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include "timer.h"

#include "quickjs/quickjs.h"

#include "ffi.h"
#include "script.h"

#include "log.h"
#include <stdio.h>
#include <stdlib.h>

#include "2dphysics.h"

#ifdef __linux__
#include <execinfo.h>
#endif

#include <signal.h>
#include <time.h>

#include "string.h"

#define SOKOL_TRACE_HOOKS
#define SOKOL_GFX_IMPL
#define SOKOL_GLES3
#include "sokol/sokol_gfx.h"

int physOn = 0;

double renderlag = 0;
double physlag = 0;
double updatelag = 0;

double renderMS = 1 / 165.f;
double physMS = 1 / 165.f;
double updateMS = 1 / 165.f;

static int sim_play = 0;
double lastTick = 0.0;
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

#ifdef __TINYC__
int backtrace(void **buffer, int size) {
    extern uint64_t *__libc_stack_end;
    uint64_t **p, *bp, *frame;
    asm ("mov %%rbp, %0;" : "=r" (bp));
    p = (uint64_t**) bp;
    int i = 0;
    while (i < size) {
        frame = p[0];
        if (frame < bp || frame > __libc_stack_end) {
            return i;
        }
        buffer[i++] = p[1];
        p = (uint64_t**) frame;
    }
    return i;
}
#endif


void print_stacktrace() {
#ifdef __linux__
  void *ents[512];
  size_t size = backtrace(ents, 512);

  YughCritical("====================BACKTRACE====================");
  char **stackstr = backtrace_symbols(ents, size);

  YughCritical("Stack size is %d.", size);

  for (int i = 0; i < size; i++)
    YughCritical(stackstr[i]);

  js_stacktrace();
#endif
}

void seghandle(int sig) {
#ifdef __linux__
  if (strsignal(sig))
    YughCritical("CRASH! Signal: %s.", strsignal(sig));

  print_stacktrace();

  exit(1);
#endif
}

const char *engine_info()
{
  char str[100];
  snprintf(str, 100, "Yugine version %s, %s build.\nCopyright 2022-2023 odplot productions LLC.\n", VER, INFO);
  return str;
}

void sg_logging(const char *tag, uint32_t lvl, uint32_t id, const char *msg, uint32_t line, const char *file, void *data) {
  mYughLog(0, 1, line, file, "tag: %s, msg: %s", tag, msg);
}

int main(int argc, char **args) {
  int logout = 1;

  script_startup();

  logout = 0;

  for (int i = 1; i < argc; i++) {
    if (args[i][0] == '-') {
      switch (args[i][1]) {
      case 'l':
        if (i + 1 < argc && args[i + 1][0] != '-') {
          log_setfile(args[i + 1]);
          i++;
          continue;
        } else {
          YughError("Expected a file for command line arg '-l'.");
          exit(1);
        }

      case 'v':
        printf(engine_info());
        exit(1);
        break;

      case 's':
        compile_script(args[2]);
        exit(0);

      case 'm':
        logLevel = atoi(args[2]);
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

  renderMS = 1.0 / vidmode->refreshRate;

  sg_setup(&(sg_desc){
      .logger = {
          .func = sg_logging,
          .user_data = NULL,
      },
 
      .buffer_pool_size = 1024,
      .context.sample_count = 1,
  });

  input_init();
  openglInit();

  int argsize = 0;
  for (int i = 1; i < argc; i++) {
    argsize += strlen(args[i]);
    if (argc > i+1) argsize++;
  }

  char cmdstr[argsize];
  cmdstr[0] = '\0';

  YughWarn("num is %d", argc);
  
  for (int i = 0; i < argc; i++) {
    strcat(cmdstr, args[i]);
    if (argc > i+1) strcat(cmdstr, " ");
  }
  
  script_evalf("cmd_args('%s');", cmdstr);

  while (!want_quit()) {
    double elapsed = glfwGetTime() - lastTick;
    deltaT = elapsed;
    lastTick = glfwGetTime();
    //double wait = fmax(0, renderMS - elapsed);
    if (sim_playing())
      input_poll(fmax(0, renderMS-elapsed));
    else
      input_poll(1000);
    window_all_handle_events();

    framems[framei++] = elapsed;

    if (framei == FPSBUF) framei = 0;

    if (sim_play == SIM_PLAY || sim_play == SIM_STEP) {
      timer_update(elapsed * timescale);
      physlag += elapsed;
      call_updates(elapsed * timescale);
//      while (physlag >= physMS) {
        phys_step = 1;
        physlag -= physMS;
        phys2d_update(physMS * timescale);
	call_physics(physMS * timescale);
        if (sim_play == SIM_STEP) sim_pause();
        phys_step = 0;
//      }
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

int frame_fps() {
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
  sim_play = SIM_STOP;
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
