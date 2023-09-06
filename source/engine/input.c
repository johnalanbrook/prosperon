#include "input.h"

#include "ffi.h"
#include "font.h"
#include "nuke.h"
#include "log.h"
#include "script.h"
#include "stb_ds.h"
#include "time.h"
#include <stdio.h>
#include <ctype.h>
#include "resources.h"

#include "stb_ds.h"

int32_t mouseWheelX = 0;
int32_t mouseWheelY = 0;
float deltaT = 0;

static int mouse_states[3] = {INPUT_UP};
static int key_states[512] = {INPUT_UP};

JSValue jsinput;
JSValue jsnum;
JSValue jsgamepadstr[15];
JSValue jsaxesstr[4];
JSValue jsinputstate[5];
JSValue jsaxis;
JSValue jsany;
JSValue jsmouse;
JSValue jspos;

cpVect mouse_pos = {0, 0};
cpVect mouse_delta = {0, 0};

struct joystick {
  int id;
  //GLFWgamepadstate state;
};

static int *downkeys = NULL;
static struct joystick *joysticks = NULL;

static int mquit = 0;

static struct callee pawn_callee;
static struct callee gamepad_callee;

static struct {
  char *key;
  JSValue value;
} *jshash = NULL;

JSValue input2js(const char *input) {
  int idx = shgeti(jshash, input);
  if (idx != -1)
    return jshash[idx].value;

  if (shlen(jshash) == 0)
    sh_new_arena(jshash);

  JSValue n = str2js(input);
  shput(jshash, input, str2js(input));

  return n;
}

void add_downkey(int key) {
  for (int i = 0; i < arrlen(downkeys); i++)
    if (downkeys[i] == key) return;

  arrput(downkeys, key);
}

void rm_downkey(int key) {
  for (int i = 0; i < arrlen(downkeys); i++)
    if (downkeys[i] == key) {
      arrdelswap(downkeys, i);
      return;
    }
}

char *mb2str(int btn)
{
  switch(btn) {
    case 0:
      return "lmouse";
      break;
    case 1:
      return "rmouse";
      break;
    case 2:
      return "mmouse";
      break;
  }
  return "NULLMOUSE";
}

void input_mouse(int btn, int state)
{
  JSValue argv[2];
  argv[0] = input2js(mb2str(btn));
  argv[1] = jsinputstate[btn];
  script_callee(pawn_callee, 2, argv);  
}

void input_mouse_move(float x, float y, float dx, float dy)
{
  mouse_pos.x = x;
  mouse_pos.y = y;
  mouse_delta.x = dx;
  mouse_delta.y = dy;
  
  JSValue argv[3];
  argv[0] = jsmouse;
  argv[1] = jspos;
  argv[2] = vec2js(mouse_pos);
  script_callee(pawn_callee, 3, argv);
  JS_FreeValue(js, argv[2]);
}

void input_mouse_scroll(float x, float y)
{
  mouseWheelY = y;
  mouseWheelX = x;
}

void input_btn(int btn, int state, uint32_t mod)
{
  char *keystr = keyname_extd(btn);

  if (strlen(keystr) == 1 && mod & SAPP_MODIFIER_SHIFT)
    keystr[0] = toupper(keystr[0]);

  char out[16] = {0};
  snprintf(out, 16, "%s%s%s%s",
    mod & SAPP_MODIFIER_CTRL ? "C-" : "",
    mod & SAPP_MODIFIER_ALT ? "M-" : "",
    mod & SAPP_MODIFIER_SUPER ? "S-" : "",
    keystr
  );

  JSValue argv[3];
  argv[1] = JS_NewString(js, out);  
  argv[2] = jsinputstate[state];
  
  argv[0] = JS_NewString(js, "emacs");
  script_callee(pawn_callee, 3, argv);
  JS_FreeValue(js, argv[0]);

  argv[0] = JS_NewString(js, "action");
  script_callee(pawn_callee, 3, argv);
  JS_FreeValue(js, argv[0]);  
  JS_FreeValue(js, argv[1]);

  if (state == INPUT_DOWN) {
    key_states[btn] = INPUT_DOWN;
    add_downkey(btn);
  }
  else if (state == INPUT_UP) {
    key_states[btn] = INPUT_UP;
    rm_downkey(btn);
  }
}

void input_key(int key, uint32_t mod)
{
  JSValue argv[2];
  char out[2] = {0};
  out[0] = (char)key;
  argv[0] = JS_NewString(js, "char");
  argv[1] = JS_NewString(js, out);
  script_callee(pawn_callee, 2, argv);
  
  JS_FreeValue(js, argv[0]);
  JS_FreeValue(js, argv[1]);
}

void register_pawn(struct callee c) {
  pawn_callee = c;
}

void register_gamepad(struct callee c) {
  gamepad_callee = c;
}

static void pawn_call_keydown(int key) {
  JSValue argv[4];
  argv[0] = jsinput;
  argv[1] = jsnum;
  argv[2] = jsinputstate[INPUT_DOWN];
  /* TODO: Could cache */
  argv[3] = JS_NewInt32(js, key);
  script_callee(pawn_callee, 4, argv);
  JS_FreeValue(js, argv[3]);
}

/*
  0 free
  1 lock
*/
void set_mouse_mode(int mousemode) { sapp_lock_mouse(mousemode); }

void input_init() {
  jsaxesstr[0] = str2js("ljoy");
  jsaxesstr[1] = str2js("rjoy");
  jsaxesstr[2] = str2js("ltrigger");
  jsaxesstr[3] = str2js("rtrigger");
  jsaxis = str2js("axis");
  jsinputstate[INPUT_UP] = str2js("released");
  jsinputstate[INPUT_REPEAT] = str2js("rep");
  jsinputstate[INPUT_DOWN] = str2js("pressed");
  jsinputstate[3] = str2js("pressrep");
  jsinputstate[4] = str2js("down");
  jsinput = str2js("input");
  jsnum = str2js("num");
  jsany = str2js("any");
  jsmouse = str2js("mouse");
  jspos = str2js("pos");

  for (int i = 0; i < 512; i++)
    key_states[i] = INPUT_UP;

  for (int i = 0; i < 3; i++)
    mouse_states[i] = INPUT_UP;
}

char keybuf[50];

const char *keyname_extd(int key) {

  if (key > 289 && key < 302) {
    int num = key - 289;
    sprintf(keybuf, "f%d", num);
    return keybuf;
  }
  
  if (key >= 320 && key <= 329) {
    int num = key - 320;
    sprintf(keybuf, "kp%d", num);
    return keybuf;
  }

  switch (key) {
    case SAPP_KEYCODE_ENTER:
      return "enter";
      break;

    case SAPP_KEYCODE_ESCAPE:
      return "escape";
      break;

    case SAPP_KEYCODE_DELETE:
      return "delete";
      break;

    case SAPP_KEYCODE_INSERT:
      return "insert";
      break;

    case SAPP_KEYCODE_TAB:
      return "tab";
      break;

    case SAPP_KEYCODE_RIGHT:
      return "right";
      break;

    case SAPP_KEYCODE_LEFT:
      return "left";
      break;

    case SAPP_KEYCODE_UP:
      return "up";
      break;

    case SAPP_KEYCODE_DOWN:
      return "down";
      break;

    case SAPP_KEYCODE_LEFT_SHIFT:
      return "lshift";
      break;

    case SAPP_KEYCODE_RIGHT_SHIFT:
      return "rshift";
      break;

    case SAPP_KEYCODE_LEFT_CONTROL:
      return "lctrl";
      break;

    case SAPP_KEYCODE_LEFT_ALT:
      return "lalt";
      break;

    case SAPP_KEYCODE_RIGHT_CONTROL:
      return "rctrl";
      break;

    case SAPP_KEYCODE_RIGHT_ALT:
      return "ralt";
      break;

    case SAPP_KEYCODE_SPACE:
      return "space";
      break;

    case SAPP_KEYCODE_KP_ADD:
      return "plus";
      break;

    case SAPP_KEYCODE_KP_SUBTRACT:
      return "minus";
      break;
    case SAPP_KEYCODE_GRAVE_ACCENT:
      return "backtick";
      break;

    case SAPP_KEYCODE_LEFT_BRACKET:
      return "lbracket";
      break;

    case SAPP_KEYCODE_RIGHT_BRACKET:
      return "rbracket";
      break;

    case SAPP_KEYCODE_BACKSPACE:
      return "backspace";
      break;
  }

  if (key >= 32 && key <=90) {
    keybuf[0] = tolower(key);
    keybuf[1] = '\0';
    
    return keybuf;
  }


  return "NULL";
}

void call_input_down(int *key) {
  JSValue argv[3];
  argv[0] = JS_NewString(js, "emacs");
  argv[1] = input2js(keyname_extd(*key));
  argv[2] = jsinputstate[4];
  script_callee(pawn_callee, 3, argv);
  JS_FreeValue(js, argv[0]);
}

/* This is called once every frame - or more if we want it more! */
void input_poll(double wait) {
  mouse_delta = cpvzero;
  mouseWheelX = 0;
  mouseWheelY = 0;

  for (int i = 0; i < arrlen(downkeys); i++)
    call_input_down(&downkeys[i]);
}

int key_is_num(int key) {
  return key <= 57 && key >= 48;
}

void cursor_hide() { sapp_show_mouse(0); }
void cursor_show() { sapp_show_mouse(1); }

int action_down(int key) { return key_states[key] == INPUT_DOWN; }
int action_up(int key) { return key_states[key] == INPUT_UP; }

int want_quit() {
  return mquit;
}

void quit() {
  exit(0);
  mquit = 1;
}
