#include "input.h"

int32_t mouseWheelX = 0;
int32_t mouseWheelY = 0;
int ychange = 0;
int xchange = 0;
float deltaT = 0;
int quit = 0;

static double c_xpos;
static double c_ypos;

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

void input_poll()
{
    ychange = 0;
    xchange = 0;
    mouseWheelX = 0;
    mouseWheelY = 0;

    glfwPollEvents();


    //editor_input(&e);

}

void cursor_hide()
{
    glfwSetInputMode(mainwin->window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
}

void cursor_show()
{
    glfwSetInputMode(mainwin->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

int action_down(int scancode)
{
    return glfwGetKey(mainwin->window, scancode);
}
