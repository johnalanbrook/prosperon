#include "input.h"

#include "ffi.h"
#include "font.h"
#include "log.h"
#include "script.h"
#include "stb_ds.h"
#include "time.h"
#include <stdio.h>

#include "stb_ds.h"

int32_t mouseWheelX = 0;
int32_t mouseWheelY = 0;
float deltaT = 0;

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
  GLFWgamepadstate state;
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

const char *gamepad2str(int btn) {
  switch (btn) {
  case GLFW_GAMEPAD_BUTTON_CROSS:
    return "cross";
  case GLFW_GAMEPAD_BUTTON_CIRCLE:
    return "circle";
  case GLFW_GAMEPAD_BUTTON_SQUARE:
    return "square";
  case GLFW_GAMEPAD_BUTTON_TRIANGLE:
    return "triangle";
  case GLFW_GAMEPAD_BUTTON_START:
    return "start";
  case GLFW_GAMEPAD_BUTTON_LEFT_BUMPER:
    return "lbump";
  case GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER:
    return "rbump";
  case GLFW_GAMEPAD_BUTTON_GUIDE:
    return "guide";
  case GLFW_GAMEPAD_BUTTON_BACK:
    return "back";
  case GLFW_GAMEPAD_BUTTON_DPAD_UP:
    return "dup";
  case GLFW_GAMEPAD_BUTTON_DPAD_DOWN:
    return "ddown";
  case GLFW_GAMEPAD_BUTTON_DPAD_LEFT:
    return "dleft";
  case GLFW_GAMEPAD_BUTTON_DPAD_RIGHT:
    return "dright";
  case GLFW_GAMEPAD_BUTTON_LEFT_THUMB:
    return "lthumb";
  case GLFW_GAMEPAD_BUTTON_RIGHT_THUMB:
    return "rthumb";
  }

  return "NOBTN";
}

void register_pawn(struct callee c) {
  pawn_callee = c;
}

void register_gamepad(struct callee c) {
  gamepad_callee = c;
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

static void cursor_pos_cb(GLFWwindow *w, double xpos, double ypos) {
  mouse_delta.x = xpos - mouse_pos.x;
  mouse_delta.y = ypos - mouse_pos.y;

  mouse_pos.x = xpos;
  mouse_pos.y = ypos;

  JSValue argv[4];
  argv[0] = jsinput;
  argv[1] = jsmouse;
  argv[2] = jspos;
  argv[3] = vec2js(mouse_pos);
  script_callee(pawn_callee, 4, argv);
  JS_FreeValue(js, argv[3]);
}

static void pawn_call_keydown(int key) {
  JSValue argv[4];
  argv[0] = jsinput;
  argv[1] = jsnum;
  argv[2] = jsinputstate[2];
  /* TODO: Could cache */
  argv[3] = JS_NewInt32(js, key);
  script_callee(pawn_callee, 4, argv);
  JS_FreeValue(js, argv[3]);
}

static void scroll_cb(GLFWwindow *w, double xoffset, double yoffset) {
  mouseWheelY = yoffset;
  mouseWheelX = xoffset;
}

static void mb_cb(GLFWwindow *w, int button, int action, int mods) {
  JSValue argv[3];
  argv[0] = jsinput;
  switch (action) {
  case GLFW_PRESS:
    argv[2] = jsinputstate[2];
    add_downkey(button);
    break;

  case GLFW_RELEASE:
    rm_downkey(button);
    argv[2] = jsinputstate[0];
    argv[1] = jsany;
    script_callee(pawn_callee, 3, argv);
    break;

  case GLFW_REPEAT:
    argv[2] = jsinputstate[1];
    break;
  }

  argv[1] = input2js(keyname_extd(button, button));
  script_callee(pawn_callee, 3, argv);
}

void set_mouse_mode(int mousemode) {
  glfwSetInputMode(mainwin->window, GLFW_CURSOR, mousemode);
}

void char_cb(GLFWwindow *w, unsigned int codepoint) {
  static char out[2] = {0};
  static JSValue argv[2];

  out[0] = (char)codepoint;
  argv[0] = JS_NewString(js, "input_text");
  argv[1] = JS_NewString(js, out);
  script_callee(pawn_callee, 2, argv);

  JS_FreeValue(js, argv[0]);
  JS_FreeValue(js, argv[1]);
}

static GLFWcharfun nukechar;

void joystick_add(int id) {
  struct joystick joy = {0};
  joy.id = id;
  arrpush(joysticks, joy);
}

void joystick_cb(int jid, int event) {
  YughWarn("IN joystick cb");
  if (event == GLFW_CONNECTED) {
    for (int i = 0; i < arrlen(joysticks); i++)
      if (joysticks[i].id == jid) return;

    joystick_add(jid);
  } else if (event == GLFW_DISCONNECTED) {
    for (int i = 0; i < arrlen(joysticks); i++) {
      if (joysticks[i].id == jid) {
        arrdelswap(joysticks, i);
        return;
      }
    }
  }
}

void input_init() {
  glfwSetCursorPosCallback(mainwin->window, cursor_pos_cb);
  glfwSetScrollCallback(mainwin->window, scroll_cb);
  glfwSetMouseButtonCallback(mainwin->window, mb_cb);
  glfwSetJoystickCallback(joystick_cb);
  nukechar = glfwSetCharCallback(mainwin->window, char_cb);

  char *paddb = slurp_text("data/gamecontrollerdb.txt");
  glfwUpdateGamepadMappings(paddb);
  free(paddb);

  for (int b = 0; b < 15; b++)
    jsgamepadstr[b] = str2js(gamepad2str(b));

  jsaxesstr[0] = str2js("ljoy");
  jsaxesstr[1] = str2js("rjoy");
  jsaxesstr[2] = str2js("ltrigger");
  jsaxesstr[3] = str2js("rtrigger");
  jsaxis = str2js("axis");

  /* Grab all joysticks initially present */
  for (int i = 0; i < 16; i++)
    if (glfwJoystickPresent(i)) joystick_add(i);

  jsinputstate[0] = str2js("released");
  jsinputstate[1] = str2js("rep");
  jsinputstate[2] = str2js("pressed");
  jsinputstate[3] = str2js("pressrep");
  jsinputstate[4] = str2js("down");
  jsinput = str2js("input");
  jsnum = str2js("num");
  jsany = str2js("any");
  jsmouse = str2js("mouse");
  jspos = str2js("pos");
}

void input_to_nuke() {
  glfwSetCharCallback(mainwin->window, nukechar);
}

void input_to_game() {
  glfwSetCharCallback(mainwin->window, char_cb);
}

void call_input_signal(char *signal) {
  JSValue s = JS_NewString(js, signal);
  JS_Call(js, pawn_callee.fn, pawn_callee.obj, 1, &s);
  JS_FreeValue(js, s);
}

char keybuf[50];

const char *keyname_extd(int key, int scancode) {
  const char *kkey = NULL;

  if (key > 289 && key < 302) {
    int num = key - 289;
    sprintf(keybuf, "f%d", num);
    return keybuf;
  } else if (key >= 320 && key <= 329) {
    int num = key - 320;
    sprintf(keybuf, "kp%d", num);
    return keybuf;
  } else {
    switch (key) {
    case GLFW_KEY_ENTER:
      kkey = "enter";
      break;

    case GLFW_KEY_ESCAPE:
      kkey = "escape";
      break;

    case GLFW_KEY_DELETE:
      kkey = "delete";
      break;

    case GLFW_KEY_INSERT:
      kkey = "insert";
      break;

    case GLFW_KEY_TAB:
      kkey = "tab";
      break;

    case GLFW_KEY_RIGHT:
      kkey = "right";
      break;

    case GLFW_KEY_LEFT:
      kkey = "left";
      break;

    case GLFW_KEY_UP:
      kkey = "up";
      break;

    case GLFW_KEY_DOWN:
      kkey = "down";
      break;

    case GLFW_KEY_LEFT_SHIFT:
      kkey = "lshift";
      break;

    case GLFW_KEY_RIGHT_SHIFT:
      kkey = "rshift";
      break;

    case GLFW_KEY_LEFT_CONTROL:
      kkey = "lctrl";
      break;

    case GLFW_KEY_LEFT_ALT:
      kkey = "lalt";
      break;

    case GLFW_KEY_RIGHT_CONTROL:
      kkey = "rctrl";
      break;

    case GLFW_KEY_RIGHT_ALT:
      kkey = "ralt";
      break;

    case GLFW_KEY_SPACE:
      kkey = "space";
      break;

    case GLFW_MOUSE_BUTTON_RIGHT:
      kkey = "rmouse";
      break;

    case GLFW_MOUSE_BUTTON_LEFT:
      kkey = "lmouse";
      break;

    case GLFW_MOUSE_BUTTON_MIDDLE:
      kkey = "mmouse";
      break;

    case GLFW_KEY_KP_ADD:
      kkey = "plus";
      break;

    case GLFW_KEY_KP_SUBTRACT:
      kkey = "minus";
      break;
    case GLFW_KEY_GRAVE_ACCENT:
      kkey = "backtick";
      break;

    case GLFW_KEY_LEFT_BRACKET:
      kkey = "lbracket";
      break;

    case GLFW_KEY_RIGHT_BRACKET:
      kkey = "rbracket";
      break;

    case GLFW_KEY_BACKSPACE:
      kkey = "backspace";
      break;
    }

    if (kkey) return kkey;
  }

  kkey = glfwGetKeyName(key, scancode);

  if (kkey) return kkey;

  return "NULL";
}

void call_input_down(int *key) {
  JSValue argv[3];
  argv[0] = jsinput;
  argv[1] = input2js(keyname_extd(*key, *key));
  argv[2] = jsinputstate[4];
  script_callee(pawn_callee, 3, argv);
}

const char *axis2str(int axis) {
  switch (axis) {
  case GLFW_GAMEPAD_AXIS_LEFT_X:
    return "lx";
  case GLFW_GAMEPAD_AXIS_LEFT_Y:
    return "ly";
  case GLFW_GAMEPAD_AXIS_RIGHT_X:
    return "rx";
  case GLFW_GAMEPAD_AXIS_RIGHT_Y:
    return "ry";
  case GLFW_GAMEPAD_AXIS_LEFT_TRIGGER:
    return "ltrigger";
  case GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER:
    return "rtrigger";
  }

  return "NOAXIS";
}

/* This is called once every frame - or more if we want it more! */
void input_poll(double wait) {
  mouse_delta = cpvzero;
  mouseWheelX = 0;
  mouseWheelY = 0;

  glfwPollEvents();

  //    glfwWaitEventsTimeout(wait);

  for (int i = 0; i < arrlen(downkeys); i++)
    call_input_down(&downkeys[i]);

  for (int i = 0; i < arrlen(joysticks); i++) {
    GLFWgamepadstate state;
    if (!glfwGetGamepadState(joysticks[i].id, &state)) continue;

    JSValue argv[4];
    argv[0] = num_cache[joysticks[i].id];
    for (int b = 0; b < 15; b++) {
      argv[1] = jsgamepadstr[b];

      if (state.buttons[b]) {
        argv[2] = num_cache[0];
        script_callee(gamepad_callee, 3, argv);

        if (!joysticks[i].state.buttons[b]) {
          argv[2] = num_cache[1];
          script_callee(gamepad_callee, 3, argv);
        }
      } else if (!state.buttons[b] && joysticks[i].state.buttons[b]) {
        argv[2] = num_cache[2];
        script_callee(gamepad_callee, 3, argv);
      }
    }

    argv[2] = jsaxis;

    float deadzone = 0.05;

    for (int i = 0; i < 4; i++)
      state.axes[i] = fabs(state.axes[i]) > deadzone ? state.axes[i] : 0;

    argv[1] = jsaxesstr[0];
    cpVect v;
    v.x = state.axes[0];
    v.y = -state.axes[1];
    argv[3] = vec2js(v);
    script_callee(gamepad_callee, 4, argv);
    JS_FreeValue(js, argv[3]);

    argv[1] = jsaxesstr[1];
    v.x = state.axes[2];
    v.y = -state.axes[3];
    argv[3] = vec2js(v);
    script_callee(gamepad_callee, 4, argv);
    JS_FreeValue(js, argv[3]);

    argv[1] = jsaxesstr[2];
    argv[3] = num2js((state.axes[4] + 1) / 2);
    script_callee(gamepad_callee, 4, argv);
    JS_FreeValue(js, argv[3]);

    argv[1] = jsaxesstr[3];
    argv[3] = num2js((state.axes[5] + 1) / 2);
    script_callee(gamepad_callee, 4, argv);
    JS_FreeValue(js, argv[3]);

    joysticks[i].state = state;
  }
}

int key_is_num(int key) {
  return key <= 57 && key >= 48;
}

void win_key_callback(GLFWwindow *w, int key, int scancode, int action, int mods) {
  JSValue argv[3];
  argv[0] = jsinput;
  argv[1] = input2js(keyname_extd(key, scancode));

  switch (action) {
  case GLFW_PRESS:
    argv[2] = jsinputstate[2];
    script_callee(pawn_callee, 3, argv);
    argv[2] = jsinputstate[3];
    script_callee(pawn_callee, 3, argv);
    add_downkey(key);
    argv[1] = jsany;
    argv[2] = jsinputstate[2];
    script_callee(pawn_callee, 3, argv);

    if (key_is_num(key))
      pawn_call_keydown(key - 48);

    break;

  case GLFW_RELEASE:
    argv[2] = jsinputstate[0];
    script_callee(pawn_callee, 3, argv);
    rm_downkey(key);
    argv[1] = jsany;
    script_callee(pawn_callee, 3, argv);
    break;

  case GLFW_REPEAT:
    argv[2] = jsinputstate[1];
    script_callee(pawn_callee, 3, argv);
    argv[2] = jsinputstate[3];
    script_callee(pawn_callee, 3, argv);
    break;
  }
}

void cursor_hide() {
  glfwSetInputMode(mainwin->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void cursor_show() {
  glfwSetInputMode(mainwin->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

int action_down(int scancode) {
  for (int i = 0; i < arrlen(downkeys); i++) {
    if (downkeys[i] == scancode)
      return 1;
  }

  return 0;
}

int action_up(int scancode) {
  int found = 0;
  for (int i = 0; i < arrlen(downkeys); i++) {
    if (downkeys[i] == scancode) {
      found = 1;
      break;
    }
  }

  return !found;
}

int want_quit() {
  return mquit;
}

void quit() {
  YughInfo("Exiting game.");
  mquit = 1;
}
