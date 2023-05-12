#include "window.h"
#include "input.h"
#include "log.h"
#include "nuke.h"
#include "script.h"
#include "texture.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "openglrender.h"

#include "stb_ds.h"

struct window *mainwin;

static struct window *windows = NULL;

struct Texture *icon = NULL;

int is_win(struct window *s, GLFWwindow *w) {
  return s->window == w;
}

void window_size_callback(GLFWwindow *w, int width, int height) {
}

struct window *winfind(GLFWwindow *w) {
  for (int i = 0; i < arrlen(windows); i++) {
    if (windows[i].window == w)
      return &windows[i];
  }

  return NULL;
}

void window_iconify_callback(GLFWwindow *w, int iconified) {
  struct window *win = winfind(w);
  win->iconified = iconified;
}

void window_focus_callback(GLFWwindow *w, int focused) {
  struct window *win = winfind(w);
  win->mouseFocus = focused;
}

void window_maximize_callback(GLFWwindow *w, int maximized) {
  struct window *win = winfind(w);
  win->minimized = !maximized;
}

void window_framebuffer_size_cb(GLFWwindow *w, int width, int height) {
  struct window *win = winfind(w);
  win->width = width;
  win->height = height;
  window_makecurrent(win);
  win->render = 1;
}

void window_close_callback(GLFWwindow *w) {
  quit();
}

struct window *MakeSDLWindow(const char *name, int width, int height, uint32_t flags) {
  if (arrcap(windows) == 0)
    arrsetcap(windows, 5);

  GLFWwindow *sharewin = mainwin == NULL ? NULL : mainwin->window;

  if (sharewin) return sharewin;

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_SAMPLES,1);

  struct window w = {
      .width = width,
      .height = height,
      .id = arrlen(windows),
      .window = glfwCreateWindow(width, height, name, NULL, sharewin)};

  if (!w.window) {
    YughError("Couldn't make GLFW window\n", 1);
    return NULL;
  }

  glfwMakeContextCurrent(w.window);
  //    int version = gladLoadGL(glfwGetProcAddress);
  //    if (!version) {
  //        YughError("Failed to initialize OpenGL context.");
  //        exit(1);
  //    }
  YughInfo("Loaded OpenGL %d.%d", 3, 3);
  glfwSwapInterval(1);

  // Set callbacks
  glfwSetWindowCloseCallback(w.window, window_close_callback);
  glfwSetWindowSizeCallback(w.window, window_size_callback);
  glfwSetFramebufferSizeCallback(w.window, window_framebuffer_size_cb);
  glfwSetWindowFocusCallback(w.window, window_focus_callback);
  glfwSetKeyCallback(w.window, win_key_callback);

  arrput(windows, w);

  if (arrlen(windows) == 1)
    mainwin = &windows[0];

  return &arrlast(windows);
}

void window_set_icon(const char *png) {
  icon = texture_pullfromfile(png);
  window_seticon(mainwin, icon);
}

void window_destroy(struct window *w) {
  glfwDestroyWindow(w->window);
  arrdelswap(windows, w->id);
}

struct window *window_i(int index) {
  return &windows[index];
}

void window_handle_event(struct window *w) {
  /*
      if (e->type == SDL_WINDOWEVENT && e->window.windowID == w->id) {	// TODO: Check ptr direct?
          switch (e->window.event) {
          case SDL_WINDOWEVENT_SHOWN:
              YughLog(0, SDL_LOG_PRIORITY_INFO, "Showed window %d.", w->id);
              w->shown = true;
              break;

          case SDL_WINDOWEVENT_HIDDEN:
              w->shown = false;
              YughLog(0, SDL_LOG_PRIORITY_INFO, "Hid window %d.", w->id);
              break;

          case SDL_WINDOWEVENT_SIZE_CHANGED:

              w->width = e->window.data1;
              w->height = e->window.data2;
              YughLog(0, SDL_LOG_PRIORITY_INFO,
                      "Changed size of window %d: width %d, height %d.",
                      w->id, w->width, w->height);
              window_makecurrent(w);
              w->render = true;
              break;

          case SDL_WINDOWEVENT_EXPOSED:
              YughLog(0, SDL_LOG_PRIORITY_INFO, "Exposed window %d.", w->id);
              w->render = true;
              break;

          case SDL_WINDOWEVENT_ENTER:
              YughLog(0, SDL_LOG_PRIORITY_INFO, "Entered window %d.", w->id);
              w->mouseFocus = true;
              SDL_RaiseWindow(w->window);
              break;

          case SDL_WINDOWEVENT_LEAVE:
              YughLog(0, SDL_LOG_PRIORITY_INFO, "Left window %d.", w->id);
              w->mouseFocus = false;
              break;

          case SDL_WINDOWEVENT_FOCUS_LOST:
              YughLog(0, SDL_LOG_PRIORITY_INFO,
                      "Lost focus on window %d.", w->id);
              w->keyboardFocus = false;
              break;

          case SDL_WINDOWEVENT_FOCUS_GAINED:
              YughLog(0, SDL_LOG_PRIORITY_INFO,
                      "Gained focus on window %d.", w->id);
              w->keyboardFocus = true;
              break;

          case SDL_WINDOWEVENT_MINIMIZED:
              YughLog(0, SDL_LOG_PRIORITY_INFO,
                      "Minimized window %d.", w->id);
              w->minimized = true;
              break;

          case SDL_WINDOWEVENT_MAXIMIZED:
              YughLog(0, SDL_LOG_PRIORITY_INFO,
                      "Maximized window %d.", w->id);
              w->minimized = false;
              break;

          case SDL_WINDOWEVENT_RESTORED:
              YughLog(0, SDL_LOG_PRIORITY_INFO,
                      "Restored window %d.", w->id);
              w->minimized = false;
              break;

          case SDL_WINDOWEVENT_CLOSE:
              YughLog(0, SDL_LOG_PRIORITY_INFO, "Closed window %d.", w->id);
              break;
          }
      }
  */
}

void window_all_handle_events() {
  for (int i = 0; i < arrlen(windows); i++)
    window_handle_event(&windows[i]);
}

void window_makefullscreen(struct window *w) {
  if (!w->fullscreen)
    window_togglefullscreen(w);
}

void window_unfullscreen(struct window *w) {
  if (w->fullscreen)
    window_togglefullscreen(w);
}

void window_togglefullscreen(struct window *w) {
  w->fullscreen = !w->fullscreen;

  if (w->fullscreen) {
    glfwMaximizeWindow(w->window);
  } else {
    glfwRestoreWindow(w->window);
  }
}

void window_makecurrent(struct window *w) {

  if (w->window != glfwGetCurrentContext())
    glfwMakeContextCurrent(w->window);
  glViewport(0, 0, w->width, w->height);
}

void window_swap(struct window *w) {
  glfwSwapBuffers(w->window);
}

void window_seticon(struct window *w, struct Texture *icon) {

  static GLFWimage images[1];
  images[0].width = icon->width;
  images[0].height = icon->height;
  images[0].pixels = icon->data;
  glfwSetWindowIcon(w->window, 1, images);
}

int window_hasfocus(struct window *w) {
  return glfwGetWindowAttrib(w->window, GLFW_FOCUSED);
}

void window_render(struct window *w) {
  window_makecurrent(w);
  openglRender(w);
  window_swap(w);
}

void window_renderall() {
  // arrwalk(windows, window_render);
  for (int i = 0; i < arrlen(windows); i++)
    window_render(&windows[i]);
}
