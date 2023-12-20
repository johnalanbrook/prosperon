#include "yugine.h"
#include "font.h"
#include "transform.h"
#include "gameobject.h"
#include "input.h"
#include "render.h"
#include "window.h"
#include "sound.h"
#include "resources.h"
#include "spline.h"
#include <stdio.h>

#include "datastream.h"

#include "timer.h"

#include "quickjs/quickjs.h"

#include "jsffi.h"
#include "script.h"

#include "log.h"
#include <stdio.h>
#include <stdlib.h>

#include "2dphysics.h"

#include <signal.h>
#include <time.h>

#ifdef STEAM
#include "steamffi.h"
#endif

#ifdef DISCORD
#include "discord.h"
#endif

#include "string.h"

#include "render.h"

#include "sokol/sokol_app.h"
#include "sokol/sokol_audio.h"
#include "sokol/sokol_time.h"
#include "sokol/sokol_args.h"
#include <stb_ds.h>
#include <stb_truetype.h>
#include "stb_image.h"
#include "stb_image_write.h"
#include <pl_mpeg.h>

#include "debug.h"

static struct d_prof prof_draw;
static struct d_prof prof_update;
static struct d_prof prof_input;
static struct d_prof prof_physics;

double physlag = 0;
double physMS = 1 / 60.f;
uint64_t physlast = 0;

double updateMS = 1/60.f;
uint64_t updatelast = 0;

static int phys_step = 0;

uint64_t start_t;
uint64_t frame_t;

static float timescale = 1.f;

#define SIM_PLAY 0
#define SIM_PAUSE 1
#define SIM_STEP 2

static int sim_play = SIM_PLAY;

int editor_mode = 0;

const char *engine_info()
{
  static char str[100];
  snprintf(str, 100, "Yugine version %s, %s build.\nCopyright 2022-2023 odplot productions LLC.\n", VER, INFO);
  return str;
}

static int argc;
static char **args;

struct datastream *bjork;

void c_init() {

  input_init();
  script_evalf("world_start();");
  
  render_init();
  window_set_icon("icons/moon.gif");  
  window_resize(sapp_width(), sapp_height());
  script_evalf("Game.init();");
}

int frame_fps() { return 1.0/sapp_frame_duration(); }

static void process_frame()
{
  double elapsed = stm_sec(stm_laptime(&frame_t));
  script_evalf("Register.appupdate.broadcast(%g);", elapsed);
      call_stack();  
    input_poll(0);
    /* Timers all update every frame - once per monitor refresh */
    timer_update(elapsed, timescale);

  /* Update at a high level::
   * Update scene graph
   *
  */

  if (sim_play == SIM_PLAY || sim_play == SIM_STEP) {
    if (stm_sec(stm_diff(frame_t, updatelast)) > updateMS) {
      double dt = stm_sec(stm_diff(frame_t, updatelast));
      updatelast = frame_t;
      prof_start(&prof_update);

      call_updates(dt * timescale);
      prof(&prof_update);

      if (sim_play == SIM_STEP)
        sim_pause();
    }

    physlag += elapsed;
    while (physlag > physMS) {
      physlag -= physMS;
      prof_start(&prof_physics);
      phys_step = 1;
      phys2d_update(physMS * timescale);
      call_physics(physMS * timescale);
      phys_step = 0;
      prof(&prof_physics);
    }
  }

    prof_start(&prof_draw);
    window_render(&mainwin);
    prof(&prof_draw);
    
    gameobjects_cleanup();
}

void c_frame()
{
  if (editor_mode) return;
  process_frame();
}

void c_clean() {
  gif_rec_end("out.gif");
  out_memusage("jsmem.txt");
  script_stop();
  saudio_shutdown();
  sg_shutdown();
};

void c_event(const sapp_event *e)
{
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

    case SAPP_EVENTTYPE_FILES_DROPPED:
      input_mouse_move(e->mouse_x, e->mouse_y, e->mouse_dx, e->mouse_dy, e->modifiers);
      input_dropped_files(sapp_get_num_dropped_files());
      break;
  }

  if (editor_mode)
    process_frame();
}

int sim_playing() { return sim_play == SIM_PLAY; }
int sim_paused() { return sim_play == SIM_PAUSE; }
void sim_start() { sim_play = SIM_PLAY; }
void sim_pause() { sim_play = SIM_PAUSE; }
int phys_stepping() { return sim_play == SIM_STEP; }
void sim_step() { sim_play = SIM_STEP; }
void set_timescale(float val) { timescale = val; }
double get_timescale() { return timescale; }

static sapp_desc start_desc = {
    .width = 720,
    .height = 1080,
    .high_dpi = 0,
    .sample_count = 1,
    .fullscreen = 1,
    .window_title = "Primum Machinam",
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

void app_name(char *name) { start_desc.window_title = strdup(name); }

int main(int argc, char **argv) {
#ifndef NDEBUG
  log_init();
  int logout = 0;
  if (logout) {
    time_t now = time(NULL);
    char fname[100];
    snprintf(fname, 100, "yugine-%d.log", now);
    log_setfile(fname);
  }
#endif

#ifdef STEAM
steaminit();
#endif

#ifdef DISCORD
struct IDiscordCore *core;
DiscordCreate(DISCORD_VERSION, &(struct DiscordCreateParams){
  .client_id = 1176355046590533714,
  .flags = DiscordCreateFlags_Default
}, &core);
struct IDiscordUserManager *dum;
struct IDiscordActivityManager *dam;
dam = core->get_activity_manager(core);

struct DiscordActivity da;
sprintf(da.state, "Playing Solo Pinball");
sprintf(da.details, "COMPetitive");
dam->update_activity(dam, &da, NULL, NULL);
#endif

  stm_setup(); /* time */
  start_t = frame_t = stm_now();
  physlast = updatelast = start_t;
  sound_init();  
  resources_init();
  phys2d_init();  
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

  script_evalf("cmd_args('%s');", cmdstr);

  start_desc.width = mainwin.width;
  start_desc.height = mainwin.height;
  start_desc.fullscreen = 0;
  sapp_run(&start_desc);

  return 0;
}

double apptime() { return stm_sec(stm_diff(stm_now(), start_t)); }
