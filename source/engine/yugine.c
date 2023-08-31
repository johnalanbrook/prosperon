#include "yugine.h"

#include "camera.h"
#include "engine.h"
#include "font.h"
#include "gameobject.h"
#include "input.h"
#include "openglrender.h"
#include "window.h"

#include "datastream.h"

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
#define SOKOL_IMPL
#define SOKOL_GLCORE33
#include "sokol/sokol_gfx.h"
#include "sokol/sokol_app.h"
#include "sokol/sokol_audio.h"

int physOn = 0;

double renderlag = 0;
double physlag = 0;
double updatelag = 0;

double renderMS = 1 / 165.f;
double physMS = 1 / 165.f;
double updateMS = 1 / 165.f;


static int phys_step = 0;

double appTime = 0;

static float timescale = 1.f;

#define SIM_PLAY 0
#define SIM_PAUSE 1
#define SIM_STEP 2

static int sim_play = SIM_PLAY;

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

static int argc;
static char **args;

void c_init() {
int logout = 0;
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

  int argsize = 0;
  for (int i = 1; i < argc; i++) {
    argsize += strlen(args[i]);
    if (argc > i+1) argsize++;
  }

  char cmdstr[argsize];
  cmdstr[0] = '\0';

  for (int i = 0; i < argc; i++) {
    strcat(cmdstr, args[i]);
    if (argc > i+1) strcat(cmdstr, " ");
  }
  
  script_evalf("cmd_args('%s');", cmdstr);


  mainwin.width = sapp_width();
  mainwin.height = sapp_height();

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
}

int frame_fps() {
  return 1.0/sapp_frame_duration();
}

void c_frame()
{
    double elapsed = sapp_frame_duration();
    appTime += elapsed;

    nuke_input_begin();
    
//    if (sim_playing())
      input_poll(fmax(0, renderMS-elapsed));
//    else
//      input_poll(1000);
      
    if (sim_play == SIM_PLAY || sim_play == SIM_STEP) {
      timer_update(elapsed * timescale);
      physlag += elapsed;
      call_updates(elapsed * timescale);
      // TODO: Physics is not independent ...
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

//    if (renderlag >= renderMS) {
//      renderlag -= renderMS;
      window_render(&mainwin);
//    }

    gameobjects_cleanup();
}

void c_clean()
{

}

void c_event(const sapp_event *e)
{
  switch (e->type) {
    case SAPP_EVENTTYPE_MOUSE_MOVE:
      input_mouse_move(e->mouse_x, e->mouse_y, e->mouse_dx, e->mouse_dy);
      break;

    case SAPP_EVENTTYPE_MOUSE_SCROLL:
      input_mouse_scroll(e->scroll_x, e->scroll_y);
      break;

    case SAPP_EVENTTYPE_KEY_DOWN:
      input_btn(e->key_code, e->key_repeat ? INPUT_REPEAT : INPUT_DOWN, e->modifiers);
      break;

    case SAPP_EVENTTYPE_KEY_UP:
      input_btn(e->key_code, INPUT_UP, e->modifiers);
      break;

    case SAPP_EVENTTYPE_MOUSE_UP:
      input_mouse(e->mouse_button, INPUT_UP);
      break;

    case SAPP_EVENTTYPE_MOUSE_DOWN:
      input_mouse(e->mouse_button, INPUT_DOWN);
      break;

    case SAPP_EVENTTYPE_CHAR:
      input_key(e->char_code, e->modifiers);
      break;

    case SAPP_EVENTTYPE_RESIZED:
      window_resize(e->window_width, e->window_height);
      break;

    case SAPP_EVENTTYPE_ICONIFIED:
      window_iconified(1);
      break;

    case SAPP_EVENTTYPE_RESTORED:
      window_iconified(0);
      break;

    case SAPP_EVENTTYPE_FOCUSED:
      window_focused(1);
      break;

    case SAPP_EVENTTYPE_UNFOCUSED:
      window_focused(0);
      break;

    case SAPP_EVENTTYPE_SUSPENDED:
      window_suspended(1);
      break;

    case SAPP_EVENTTYPE_QUIT_REQUESTED:
      window_quit();
      break;
  }
}

int sim_playing() { return sim_play == SIM_PLAY; }
int sim_paused() { return sim_play == SIM_PAUSE; }

void sim_start() {
  sim_play = SIM_PLAY;
}

void sim_pause() {
  sim_play = SIM_PAUSE;
}

int phys_stepping() { return sim_play == SIM_STEP; }

void sim_step() {
  if (sim_paused()) {
    YughInfo("Step");
    sim_play = SIM_STEP;
  }
}

void set_timescale(float val) {
  timescale = val;
}

double get_timescale()
{
  return timescale;
}

sapp_desc sokol_main(int sargc, char **sargs) {
  argc = sargc;
  args = sargs;
  
  script_startup();

  return (sapp_desc){
    .width = 720,
    .height = 480,
    .high_dpi = 0,
    .sample_count = 8,
    .fullscreen = 0,
    .window_title = "Yugine",
    .enable_clipboard = false,
    .clipboard_size = 0,
    .enable_dragndrop = true,
    .max_dropped_files = 1,
    .max_dropped_file_path_length = 2048,
    .init_cb = c_init,
    .frame_cb = c_frame,
    .cleanup_cb = c_clean,
    .event_cb = c_event,
  };
}
