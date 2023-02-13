#include "input.h"

#include <stdio.h>
#include "script.h"
#include "stb_ds.h"
#include "log.h"

int32_t mouseWheelX = 0;
int32_t mouseWheelY = 0;
float deltaT = 0;

cpVect mouse_pos = {0,0};
cpVect mouse_delta = {0,0};

static int *downkeys = NULL;

static int mquit = 0;

static void **pawns = NULL;

void set_pawn(void *pawn) {
    arrput(pawns, pawn);
}

void remove_pawn(void *pawn) {
  for (int i = 0; i < arrlen(pawns); i++) {
    if (pawns[i] == pawn) {
      pawns[i] = NULL;
      return;
    }
  }
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

static void cursor_pos_cb(GLFWwindow *w, double xpos, double ypos)
{
    mouse_delta.x = xpos - mouse_pos.x;
    mouse_delta.y = ypos - mouse_pos.y;

    mouse_pos.x = xpos;
    mouse_pos.y = ypos;

    for (int i = 0; i < arrlen(pawns); i++) {
        if (!pawns[i] || script_eval_setup("input_mouse_pos", pawns[i])) continue;
	vect2duk(duk, mouse_pos);
        script_eval_exec(1);
    }

}

static void pawn_call_keydown(int key)
{
  for (int i = 0; i < arrlen(pawns); i++) {
    if (!pawns[i] || script_eval_setup("input_num_pressed", pawns[i])) continue;
    duk_push_int(duk, key);
    script_eval_exec(1);
  }
}

static void scroll_cb(GLFWwindow *w, double xoffset, double yoffset)
{
    mouseWheelY = yoffset;
    mouseWheelX = xoffset;
}

static void mb_cb(GLFWwindow *w, int button, int action, int mods)
{
  const char *act = NULL;
  const char *btn = NULL;
  switch (action) {
    case GLFW_PRESS:
      act = "pressed";
      add_downkey(button);
      break;

    case GLFW_RELEASE:
      act = "released";
      rm_downkey(button);
      call_input_signal("input_any_released");
      break;

    case GLFW_REPEAT:
      act = "repeat";
      break;
  }

  btn = keyname_extd(button, button);

  if (!act || !btn) {
    YughError("Tried to call a mouse action that doesn't exist.");
    return;
  }

  char keystr[50] = {'\0'};
  snprintf(keystr, 50, "input_%s_%s", btn, act);
  call_input_signal(keystr);
}

void set_mouse_mode(int mousemode)
{
  glfwSetInputMode(mainwin->window, GLFW_CURSOR, mousemode);
}

void input_init()
{
    glfwSetCursorPosCallback(mainwin->window, cursor_pos_cb);
    glfwSetScrollCallback(mainwin->window, scroll_cb);
    glfwSetMouseButtonCallback(mainwin->window, mb_cb);
}

void call_input_signal(char *signal) {
    for (int i = arrlen(pawns)-1; i >= 0; i--) {
        if (pawns[i] == NULL) arrdel(pawns, i);
    }

    for (int i = 0; i < arrlen(pawns); i++) {
      if (!pawns[i]) continue;
      script_eval_w_env(signal, pawns[i]);
    }
}

const char *keyname_extd(int key, int scancode) {
    char keybuf[50];
    const char *kkey = NULL;

    if (key > 289 && key < 302) {
            sprintf(keybuf, "f%d", key-289);
            return keybuf;
        } else {
            switch(key) {
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
                    kkey= "ralt";
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
            }

            if (kkey) return kkey;

        }

    kkey = glfwGetKeyName(key, scancode);

    if (kkey) return kkey;

    return "NULL";
}

void call_input_down(int *key) {
    char keystr[50] = {'\0'};
    snprintf(keystr, 50, "input_%s_down", keyname_extd(*key, 0));
    call_input_signal(keystr);
}

/* This is called once every frame - or more if we want it more! */
void input_poll(double wait)
{
    mouse_delta = cpvzero;
    mouseWheelX = 0;
    mouseWheelY = 0;

    glfwWaitEventsTimeout(wait);

    //editor_input(&e);
    for (int i = 0; i < arrlen(downkeys); i++)
        call_input_down(&downkeys[i]);
}

int key_is_num(int key) {
  return key <= 57 && key >= 48;
}

void win_key_callback(GLFWwindow *w, int key, int scancode, int action, int mods)
{
    char keystr[50] = {'\0'};
    const char *kkey = keyname_extd(key, scancode);

    switch (action) {
        case GLFW_PRESS:
            snprintf(keystr, 50, "input_%s_pressed", kkey);
	    add_downkey(key);
	    call_input_signal("input_any_pressed");
	    
	    if (key_is_num(key)) {
	      pawn_call_keydown(key-48);
	    }
	    
            break;

        case GLFW_RELEASE:
            snprintf(keystr, 50, "input_%s_released", kkey);
	    rm_downkey(key);
	    call_input_signal("input_any_released");
            break;

        case GLFW_REPEAT:
            snprintf(keystr, 50, "input_%s_rep", kkey);
            break;
    }

    call_input_signal(keystr);
}

void cursor_hide()
{
    glfwSetInputMode(mainwin->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void cursor_show()
{
    glfwSetInputMode(mainwin->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

int action_down(int scancode)
{
    for (int i = 0; i < arrlen(downkeys); i++) {
        if (downkeys[i] == scancode)
            return 1;
    }

    return 0;
}

int action_up(int scancode)
{
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
