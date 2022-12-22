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

static s7_pointer *pawns = NULL;

void set_pawn(s7_pointer menv) {
    arrput(pawns, menv);
    YughInfo("Now controling %d pawns.", arrlen(pawns));
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
    for (int i = 0; i < arrlen(pawns); i++)
        script_eval_w_env(signal, pawns[i]);
}

void call_input_down(int *key) {
    const char *keyname = glfwGetKeyName(*key, 0);
    char keystr[50] = {'\0'};
    snprintf(keystr, 50, "input_%s_down", keyname);
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
    strcat(keystr, "input_");
    const char *kkey = glfwGetKeyName(key, scancode);

    if (!kkey) {
        char keybuf[10];
        if (key > 289 && key < 302) {
            sprintf(keybuf, "f%d", key-289);
            strcat(keystr, keybuf);
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
            }

            if (kkey)
                strcat(keystr, kkey);
            else
                YughWarn("Could not get key string for key %d, scancode %d.", key, scancode);
        }


    } else {
        strcat(keystr, kkey);
    }

    switch (action) {
        case GLFW_PRESS:
            strcat(keystr, "_pressed");

            int found = 0;

            for (int i = 0; i < arrlen(downkeys); i++) {
                if (downkeys[i] == key)
                    goto SCRIPTCALL;
            }

            arrput(downkeys, key);

            break;

        case GLFW_RELEASE:
            strcat(keystr, "_released");

            for (int i = 0; i < arrlen(downkeys); i++) {
                if (downkeys[i] == key) {
                    arrdelswap(downkeys, i);
                    goto SCRIPTCALL;
                }
            }

            break;

        case GLFW_REPEAT:
            strcat(keystr, "_rep");
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
