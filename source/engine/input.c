#include "input.h"

#include "ffi.h"
#include "font.h"
#include "log.h"
#include "script.h"
#include "stb_ds.h"
#include "time.h"
#include <stdio.h>
#include <ctype.h>
#include <wchar.h>
#include "resources.h"

float deltaT = 0;

static int mouse_states[3] = {INPUT_UP};
static int key_states[512] = {INPUT_UP};

cpVect mousewheel = {0,0};
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
      return "lm";
      break;
    case 1:
      return "rm";
      break;
    case 2:
      return "mm";
      break;
  }
  return "NULLMOUSE";
}

JSValue input2js(int state)
{
  switch(state) {
    case INPUT_UP: return jstr("released");
    case INPUT_REPEAT: return jstr("rep");
    case INPUT_DOWN: return jstr("pressed");
    case 3: return jstr("pressrep");
    case 4: return jstr("down");
  }
  return JS_NULL;
}

void input_mouse(int btn, int state, uint32_t mod)
{
  char out[16] = {0};
  snprintf(out, 16, "%s%s%s%s",
    mod & SAPP_MODIFIER_CTRL ? "C-" : "",
    mod & SAPP_MODIFIER_ALT ? "M-" : "",
    mod & SAPP_MODIFIER_SUPER ? "S-" : "",
    mb2str(btn)
  );

  JSValue argv[3];
  argv[0] = jstr("emacs");
  argv[1] = jstr(out);
  argv[2] = input2js(state);
  script_callee(pawn_callee, 3, argv);
}

void input_mouse_move(float x, float y, float dx, float dy, uint32_t mod)
{
  mouse_pos.x = x;
  mouse_pos.y = mainwin.height - y;
  mouse_delta.x = dx;
  mouse_delta.y = -dy;
  
  JSValue argv[4];
  argv[0] = jstr("mouse");
  argv[1] = jstr("move");
  argv[2] = vec2js(mouse_pos);
  argv[3] = vec2js(mouse_delta);
  script_callee(pawn_callee, 4, argv);
  JS_FreeValue(js, argv[2]);
  JS_FreeValue(js, argv[3]);
}

void input_mouse_scroll(float x, float y, uint32_t mod)
{
  mousewheel.x = x;
  mousewheel.y = y;

  JSValue argv[4];
  argv[0] = jstr("mouse");
  char out[16] = {0};
  snprintf(out, 16, "%s%s%sscroll",
    mod & SAPP_MODIFIER_CTRL ? "C-" : "",
    mod & SAPP_MODIFIER_ALT ? "M-" : "",
    mod & SAPP_MODIFIER_SUPER ? "S-" : ""
  );
  argv[1] = jstr(out);
  argv[2] = vec2js(mousewheel);
  script_callee(pawn_callee, 3, argv);
  JS_FreeValue(js, argv[2]);
}

void input_btn(int btn, int state, uint32_t mod)
{
  char keystr[16] = {0};
  strncat(keystr,keyname_extd(btn),16);

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
  argv[1] = jstr(out);
  argv[2] = input2js(state);
  
  argv[0] = jstr("emacs");
  script_callee(pawn_callee, 3, argv);

  argv[0] = jstr("action");
  script_callee(pawn_callee, 3, argv);
  
  if (state == INPUT_DOWN) {
    key_states[btn] = INPUT_DOWN;
    add_downkey(btn);
  }
  else if (state == INPUT_UP) {
    key_states[btn] = INPUT_UP;
    rm_downkey(btn);
  }
}

static const uint32_t UNCHAR_FLAGS = SAPP_MODIFIER_CTRL | SAPP_MODIFIER_ALT | SAPP_MODIFIER_SUPER;

void input_key(uint32_t key, uint32_t mod)
{
  if (mod & UNCHAR_FLAGS) return;
  if (key <= 31 || key >= 127) return;

  JSValue argv[2];
  char s[2] = {key, '\0'};
  argv[0] = jstr("char");
  argv[1] = jstr(s);
  script_callee(pawn_callee, 2, argv);
}

void register_pawn(struct callee c) {
  pawn_callee = c;
}

void register_gamepad(struct callee c) {
  gamepad_callee = c;
}

static void pawn_call_keydown(int key) {
  JSValue argv[4];
  argv[0] = jstr("input");
  argv[1] = jstr("num");
  argv[2] = input2js(INPUT_DOWN);
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
    case SAPP_KEYCODE_ESCAPE:
      return "escape";
    case SAPP_KEYCODE_DELETE:
      return "delete";
    case SAPP_KEYCODE_INSERT:
      return "insert";
    case SAPP_KEYCODE_TAB:
      return "tab";
    case SAPP_KEYCODE_RIGHT:
      return "right";
    case SAPP_KEYCODE_LEFT:
      return "left";
    case SAPP_KEYCODE_UP:
      return "up";
    case SAPP_KEYCODE_DOWN:
      return "down";
    case SAPP_KEYCODE_LEFT_SHIFT:
      return "lshift";
    case SAPP_KEYCODE_RIGHT_SHIFT:
      return "rshift";
    case SAPP_KEYCODE_LEFT_CONTROL:
      return "lctrl";
    case SAPP_KEYCODE_LEFT_ALT:
      return "lalt";
    case SAPP_KEYCODE_RIGHT_CONTROL:
      return "rctrl";
    case SAPP_KEYCODE_RIGHT_ALT:
      return "ralt";
    case SAPP_KEYCODE_SPACE:
      return "space";
    case SAPP_KEYCODE_KP_ADD:
      return "plus";
    case '=':
      return "plus";
    case '-':
      return "minus";
    case SAPP_KEYCODE_KP_SUBTRACT:
      return "minus";
    case SAPP_KEYCODE_GRAVE_ACCENT:
      return "`";
    case SAPP_KEYCODE_LEFT_BRACKET:
      return "lbracket";
    case SAPP_KEYCODE_RIGHT_BRACKET:
      return "rbracket";
    case SAPP_KEYCODE_BACKSPACE:
      return "backspace";
    case SAPP_KEYCODE_PAGE_UP:
      return "pgup";
    case SAPP_KEYCODE_PAGE_DOWN:
      return "pgdown";
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
  argv[0] = jstr("emacs");
  argv[1] = jstr(keyname_extd(*key));
  argv[2] = input2js(4);
  script_callee(pawn_callee, 3, argv);
}

/* This is called once every frame - or more if we want it more! */
void input_poll(double wait) {
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

void quit() {
  sapp_quit();
}
