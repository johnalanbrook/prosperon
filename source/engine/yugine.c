#include "yugine.h"
#include "camera.h"
#include "font.h"
#include "gameobject.h"
#include "input.h"
#include "render.h"
#include "window.h"
#include "sound.h"
#include "resources.h"

#include <stdio.h>

#include "datastream.h"

#include "timer.h"

#include "quickjs/quickjs.h"

#include "ffi.h"
#include "script.h"

#include "log.h"
#include <stdio.h>
#include <stdlib.h>

#include "2dphysics.h"

#ifdef __GLIBC__
#include <execinfo.h>
#endif

#include <signal.h>
#include <time.h>

#include "string.h"

#define SOKOL_TRACE_HOOKS
#define SOKOL_IMPL

#include "sokol/sokol_app.h"
#include "sokol/sokol_audio.h"
#include "sokol/sokol_time.h"
#include "sokol/sokol_args.h"

#define STB_DS_IMPLEMENTATION
#include <stb_ds.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#define STBI_NO_SIMD
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBIR_DEFAULT_FILTER_DOWNSAMPLE STBIR_FILTER_BOX
#include "stb_image_write.h"

#define PL_MPEG_IMPLEMENTATION
#include <pl_mpeg.h>

#include "debug.h"

static struct d_prof prof_draw;
static struct d_prof prof_update;
static struct d_prof prof_input;
static struct d_prof prof_physics;

double physlag = 0;
int render_dirty = 0;

double physMS = 1 / 60.f;

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
#ifdef __GLIBC__
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
#ifdef __GLIBC__
  if (strsignal(sig))
    YughCritical("CRASH! Signal: %s.", strsignal(sig));

  print_stacktrace();

  exit(1);
#endif
}

const char *engine_info()
{
  static char str[100];
  snprintf(str, 100, "Yugine version %s, %s build.\nCopyright 2022-2023 odplot productions LLC.\n", VER, INFO);
  return str;
}

static int argc;
static char **args;

void c_init() {
  render_init();

  script_evalf("initialize();");
}

int frame_fps() {
  return 1.0/sapp_frame_duration();
}

static double low_fps = 1/24.0;
static double low_fps_c = 0.0;

void c_frame()
{
    double elapsed = sapp_frame_duration();
    appTime += elapsed;
    low_fps_c += elapsed;

    input_poll(0);
    timer_update(elapsed, timescale);    
      
    if (sim_play == SIM_PLAY || sim_play == SIM_STEP) {
      prof_start(&prof_update);

      call_updates(elapsed * timescale);
      prof(&prof_update);

      physlag += elapsed;
      while (physlag >= physMS) {
        prof_start(&prof_physics);
        phys_step = 1;
        physlag -= physMS;
        phys2d_update(physMS * timescale);
	call_physics(physMS * timescale);
        if (sim_play == SIM_STEP) sim_pause();
        phys_step = 0;
	prof(&prof_physics);
      }

      if (sim_play == SIM_STEP) {
        sim_pause();
	render_dirty = 1;
      }
      low_fps_c = 0.0f;
    }

    if (sim_play == SIM_PLAY || render_dirty || low_fps_c >= low_fps) {
      prof_start(&prof_draw);
      window_render(&mainwin);
      prof(&prof_draw);
      render_dirty = 0;
    }
    
    gameobjects_cleanup();
}

void c_clean() {
  gif_rec_end("crash.gif");
};

void c_event(const sapp_event *e)
{
  render_dirty = 1;

  #ifndef NO_EDITOR
  snk_handle_event(e);
  #endif

  switch (e->type) {
    case SAPP_EVENTTYPE_MOUSE_MOVE:
      input_mouse_move(e->mouse_x, e->mouse_y, e->mouse_dx, e->mouse_dy, e->modifiers);
      break;

    case SAPP_EVENTTYPE_MOUSE_SCROLL:
      input_mouse_scroll(e->scroll_x, e->scroll_y, e->modifiers);
      break;

    case SAPP_EVENTTYPE_KEY_DOWN:
      input_btn(e->key_code, e->key_repeat ? INPUT_REPEAT : INPUT_DOWN, e->modifiers);
      break;

    case SAPP_EVENTTYPE_KEY_UP:
      input_btn(e->key_code, INPUT_UP, e->modifiers);
      break;

    case SAPP_EVENTTYPE_MOUSE_UP:
      input_mouse(e->mouse_button, INPUT_UP, e->modifiers);
      break;

    case SAPP_EVENTTYPE_MOUSE_DOWN:
      input_mouse(e->mouse_button, INPUT_DOWN, e->modifiers);
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
  sim_play = SIM_STEP;
}

void set_timescale(float val) {
  timescale = val;
}

double get_timescale()
{
  return timescale;
}

static sapp_desc start_desc = {
    .width = 720,
    .height = 1080,
    .high_dpi = 0,
    .sample_count = 1,
    .fullscreen = 1,
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
    .logger.func = sg_logging,
};

void app_name(char *name)
{
  start_desc.window_title = strdup(name);
}

sapp_desc sokol_main(int argc, char **argv) {
#ifndef NDEBUG
  log_init();
  #ifdef __linux__
  int logout = 0;
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
#endif

  stm_setup(); /* time */
  resources_init(); 

  phys2d_init();

  script_startup();
  
  int argsize = 0;
  for (int i = 0; i < argc; i++) {
    argsize += strlen(argv[i]);
    if (argc > i+1) argsize++;
  }

  char cmdstr[argsize];
  cmdstr[0] = '\0';

  for (int i = 0; i < argc; i++) {
    strcat(cmdstr, argv[i]);
    if (argc > i+1) strcat(cmdstr, " ");
  }

  script_evalf("cmd_args('%s');", cmdstr);

  sound_init();
  input_init();

  script_dofile("warmup.js");

  start_desc.width = mainwin.width;
  start_desc.height = mainwin.height;
  start_desc.fullscreen = 0;

  return start_desc;
}
