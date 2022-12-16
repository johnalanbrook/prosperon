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

void call_input_down(int *key) {
    const char *keyname = glfwGetKeyName(*key, 0);
    char keystr[50] = {'\0'};
    snprintf(keystr, 50, "input_%s_down", keyname);
    script_call(keystr);
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
            }

            strcat(keystr, kkey);
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
    script_call(keystr);
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