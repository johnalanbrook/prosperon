#include "window.h"



#include <string.h>
#include "texture.h"
#include "log.h"
#include <stdlib.h>
#include <stdio.h>

#include <vec.h>

static struct mSDLWindow *mainwin;

static struct vec windows;
struct Texture *icon = NULL;

struct mSDLWindow *MakeSDLWindow(const char *name, int width, int height,
				 uint32_t flags)
{
     struct mSDLWindow *w;

    if (windows.data == NULL) {
        windows = vec_init(sizeof(struct mSDLWindow), 5);
        w = vec_add(&windows, NULL);
        mainwin = w;
    } else {
        w = vec_add(&windows, NULL);
    }

    GLFWwindow *sharewin = mainwin ? NULL : mainwin->window;

    w->width = width;
    w->height = height;
    printf("NUmber of windows: %d.\n", windows.len);
    w->id = windows.len-1;

    w->window = glfwCreateWindow(width, height, name, NULL, sharewin);

    if (!w->window) {
        YughError("Couldn't make GLFW window\n", 1);
        return w;
    }

    if (icon) window_seticon(w, icon);

    glfwMakeContextCurrent(w->window);
    gladLoadGL(glfwGetProcAddress);
    glfwSwapInterval(1);



    return w;
}

void window_set_icon(const char *png)
{
    icon = texture_pullfromfile(png);
}

void window_destroy(struct mSDLWindow *w)
{
    glfwDestroyWindow(w->window);
    vec_delete(&windows, w->id);
}

void window_handle_event(struct mSDLWindow *w)
{
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
	    w.projection =
	       glm::ortho(0.f, (float) width, 0.f, (float) height, -1.f,
	       1.f);
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

void window_all_handle_events()
{
    vec_walk(&windows, window_handle_event);
}

void window_makefullscreen(struct mSDLWindow *w)
{
    glfwMaximizeWindow(w->window);
    w->fullscreen = true;
}

void window_togglefullscreen(struct mSDLWindow *w)
{
    w->fullscreen = !w->fullscreen;

    if (w->fullscreen) {
	window_makefullscreen(w);
    } else {
	glfwRestoreWindow(w->window);
    }

}

void window_makecurrent(struct mSDLWindow *w)
{

    if (w->window != glfwGetCurrentContext())
	glfwMakeContextCurrent(w->window);
    glViewport(0, 0, w->width, w->height);

}

void window_swap(struct mSDLWindow *w)
{
    glfwSwapBuffers(w->window);
}

void window_seticon(struct mSDLWindow *w, struct Texture *icon)
{

    static GLFWimage images[1];
    images[0].width = icon->width;
    images[0].height = icon->height;
    images[0].pixels = icon->data;
    glfwSetWindowIcon(w->window, 1, images);

}

int window_hasfocus(struct mSDLWindow *w)
{
    return glfwGetWindowAttrib(w->window, GLFW_FOCUSED);
}

double frame_time()
{
    return glfwGetTime();
}

double elapsed_time()
{
    static double last_time;
    static double elapsed;
    elapsed = frame_time() - last_time;
    return elapsed;
}

int elapsed_time_ms()
{
    return elapsed_time() * 1000;
}


