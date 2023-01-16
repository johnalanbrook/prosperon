#include "input.h"

#include <stdio.h>
#include "script.h"
#include "stb_ds.h"
#include "log.h"

int32_t mouseWheelX = 0;
int32_t mouseWheelY = 0;
int ychange = 0;
int xchange = 0;
float deltaT = 0;


static double c_xpos;
static double c_ypos;

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

static void cursor_pos_cb(GLFWwindow *w, double xpos, double ypos)
{
    xchange = (int)xpos - c_xpos;
    ychange = (int)ypos - c_ypos;

    c_xpos = xpos;
    c_ypos = ypos;
}

static void scroll_cb(GLFWwindow *w, double xoffset, double yoffset)
{
    mouseWheelY = yoffset;
    mouseWheelX = xoffset;
}

void input_init()
{
    glfwSetCursorPosCallback(mainwin->window, cursor_pos_cb);
    glfwSetScrollCallback(mainwin->window, scroll_cb);
}

void call_input_signal(char *signal) {
    for (int i = arrlen(pawns)-1; i >= 0; i--) {
        if (pawns[i] == NULL) arrdel(pawns, i);
    }

    for (int i = 0; i < arrlen(pawns); i++) {
        script_eval_w_env(signal, pawns[i]);
    }
}

const char *keyname_extd(int key, int scancode) {
    char keybuf[50];
    const char *kkey = glfwGetKeyName(key, scancode);

    if (kkey) return kkey;

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
            }

            if (kkey) return kkey;

        }

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
    ychange = 0;
    xchange = 0;
    mouseWheelX = 0;
    mouseWheelY = 0;

    glfwWaitEventsTimeout(wait);


    //editor_input(&e);
    for (int i = 0; i < arrlen(downkeys); i++)
        call_input_down(&downkeys[i]);
}

void win_key_callback(GLFWwindow *w, int key, int scancode, int action, int mods)
{
    char keystr[50] = {'\0'};
    const char *kkey = keyname_extd(key, scancode);

    switch (action) {
        case GLFW_PRESS:
            snprintf(keystr, 50, "input_%s_pressed", kkey);

            int found = 0;

            for (int i = 0; i < arrlen(downkeys); i++) {
                if (downkeys[i] == key)
                    goto SCRIPTCALL;
            }

            arrput(downkeys, key);

            break;

        case GLFW_RELEASE:
            snprintf(keystr, 50, "input_%s_released", kkey);

            for (int i = 0; i < arrlen(downkeys); i++) {
                if (downkeys[i] == key) {
                    arrdelswap(downkeys, i);
                    goto SCRIPTCALL;
                }
            }

            break;

        case GLFW_REPEAT:
            snprintf(keystr, 50, "input_%s_rep", kkey);
            break;
    }

    SCRIPTCALL:
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
