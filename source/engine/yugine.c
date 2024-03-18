#include "yugine.h"
#include "transform.h"
#include "input.h"
#include "render.h"
#include "window.h"
#include "sound.h"
#include "resources.h"
#include "spline.h"
#include <stdio.h>
#include "particle.h"
#include "simplex.h"

#include "datastream.h"

#include "quickjs/quickjs.h"

#include "jsffi.h"
#include "script.h"

#include "log.h"
#include <stdio.h>
#include <stdlib.h>

#include "2dphysics.h"

#include <signal.h>
#include <time.h>
#include "string.h"

#include "render.h"

#include "sokol/sokol_app.h"
#include "sokol/sokol_audio.h"
#include "sokol/sokol_time.h"
#include "sokol/sokol_args.h"
#include "sokol/sokol_fetch.h"
#include <stb_ds.h>
#include <stb_truetype.h>
#include "stb_image.h"
#include "stb_image_write.h"
#include <pl_mpeg.h>

uint64_t start_t;
uint64_t frame_t;

#define SIM_PLAY 0
#define SIM_PAUSE 1
#define SIM_STEP 2

static int sim_play = SIM_PLAY;

static int argc;
static char **args;

static JSValue c_process_fn;

void c_init() {
  mainwin.start = 1;
  window_resize(sapp_width(), sapp_height());
//  script_evalf("world_start();");
  render_init();
  set_icon("icons/moon.gif");  
  particle_init();
}

void c_frame() { script_call_sym(c_process_fn,0,NULL); }

void cleanup()
{
  out_memusage(".prosperon/jsmem.txt");
  script_stop();
}

void seghandle()
{
  js_stacktrace();
  cleanup();
  exit(1);
}

void c_clean() {
  cleanup();
  gif_rec_end("out.gif");
  saudio_shutdown();
  sg_shutdown();
};

void c_event(const sapp_event *e)
{
  switch (e->type) {
    case SAPP_EVENTTYPE_MOUSE_MOVE:
      script_evalf("prosperon.mousemove([%g, %g], [%g, %g]);", e->mouse_x, mainwin.size.y -e->mouse_y, e->mouse_dx, -e->mouse_dy);
      break;

    case SAPP_EVENTTYPE_MOUSE_SCROLL:
      script_evalf("prosperon.mousescroll([%g, %g]);", e->scroll_x, e->scroll_y);
      break;

    case SAPP_EVENTTYPE_KEY_DOWN:
      script_evalf("prosperon.keydown(%d, %d);", e->key_code, e->key_repeat);
      break;

    case SAPP_EVENTTYPE_KEY_UP:
      script_evalf("prosperon.keyup(%d);", e->key_code);
      break;

    case SAPP_EVENTTYPE_MOUSE_UP:
      script_evalf("prosperon.mouseup(%d);", e->mouse_button);
      break;

    case SAPP_EVENTTYPE_MOUSE_DOWN:
      script_evalf("prosperon.mousedown(%d);", e->mouse_button);
      break;

    case SAPP_EVENTTYPE_CHAR:
      script_evalf("prosperon.textinput(`%lc`);", e->char_code);
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
      quit();
      break;

    case SAPP_EVENTTYPE_FILES_DROPPED:
      input_dropped_files(sapp_get_num_dropped_files());
      break;
      
    case SAPP_EVENTTYPE_MOUSE_ENTER:
      script_evalf("prosperon.mouseenter();");
      break;
    case SAPP_EVENTTYPE_MOUSE_LEAVE:
      script_evalf("prosperon.mouseleave();");
      break;
    case SAPP_EVENTTYPE_TOUCHES_BEGAN:
      touch_start(e->touches, e->num_touches);
      break;
    case SAPP_EVENTTYPE_TOUCHES_MOVED:
      touch_move(e->touches, e->num_touches);
      break;
    case SAPP_EVENTTYPE_TOUCHES_ENDED:
      touch_end(e->touches, e->num_touches);
      break;
    case SAPP_EVENTTYPE_TOUCHES_CANCELLED:
      touch_cancelled(e->touches, e->num_touches);
      break;
    case SAPP_EVENTTYPE_CLIPBOARD_PASTED:
      input_clipboard_paste(sapp_get_clipboard_string());
      break;
    default:
      break;
  }
}

static sapp_desc start_desc = {
    .width = 720,
    .height = 1080,
    .high_dpi = 0,
    .sample_count = 1,
    .fullscreen = 1,
    .window_title = NULL,
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

int main(int argc, char **argv) {
#ifndef NDEBUG
  log_init();
  signal(SIGSEGV, seghandle);
  signal(SIGABRT, seghandle);
  signal(SIGFPE, seghandle);
#endif

  resources_init();
  stm_setup(); /* time */
  script_startup();
  
  int argsize = 0;
  for (int i = 0; i < argc; i++) {
    argsize += strlen(argv[i]);
    if (argc > i+1) argsize++;
  }

  char cmdstr[argsize+1];
  cmdstr[0] = '\0';

  for (int i = 0; i < argc; i++) {
    strcat(cmdstr, argv[i]);
    if (argc > i+1) strcat(cmdstr, " ");
  }

  while (!LOADED_GAME)
    sfetch_dowork();

  script_evalf("cmd_args('%s');", cmdstr);
 
  out_memusage(".prosperon/jsmem.txt");
  script_stop();

  return 0;
}

void engine_start(JSValue procfn)
{
  c_process_fn = procfn;

  sound_init();
  phys2d_init();  

  start_desc.width = mainwin.size.x;
  start_desc.height = mainwin.size.y;
  start_desc.window_title = mainwin.title;
  start_desc.fullscreen = mainwin.fullscreen;
  sapp_run(&start_desc);
}

double apptime() { return stm_sec(stm_now()); }

void quit() {
  if (mainwin.start)
    sapp_quit();
  else {
    cleanup();
    exit(0);
  }
}
