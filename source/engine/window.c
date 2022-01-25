#include "window.h"

#include "openglrender.h"

#include <string.h>
#include "texture.h"
#include "log.h"

struct mSDLWindow *window = NULL;

static struct mSDLWindow *windows[5];
static int numWindows = 0;

struct mSDLWindow *MakeSDLWindow(const char *name, int width, int height,
				 uint32_t flags)
{
    struct mSDLWindow *w = calloc(1, sizeof(struct mSDLWindow));
    w->width = width;
    w->height = height;
    w->window = SDL_CreateWindow(name, 0, 0, width, height, flags);

    if (w->window == NULL) {
	YughLog(0, SDL_LOG_PRIORITY_ERROR,
		"Window could not be created! SDL Error: %s",
		SDL_GetError());
    } else {
	w->glContext = SDL_GL_CreateContext(w->window);

	if (w->glContext == NULL) {
	    YughLog(0, SDL_LOG_PRIORITY_ERROR,
		    "OpenGL context could not be created! SDL Error: %s",
		    SDL_GetError());
	}
	w->id = SDL_GetWindowID(w->window);

	glewExperimental = GL_TRUE;
	glewInit();

	SDL_GL_SetSwapInterval(1);

	if (numWindows < 5)
	    windows[numWindows++] = w;
    }


    return w;
}

void window_handle_event(struct mSDLWindow *w, SDL_Event * e)
{
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
	    /*w.projection =
	       glm::ortho(0.f, (float) width, 0.f, (float) height, -1.f,
	       1.f); */
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
}

void window_all_handle_events(SDL_Event *e)
{
    for (int i = 0; i < numWindows; i++) {
        window_handle_event(windows[i], e);
    }
}

void window_makefullscreen(struct mSDLWindow *w)
{
    SDL_SetWindowFullscreen(w->window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    w->fullscreen = true;
}

void window_togglefullscreen(struct mSDLWindow *w)
{
    w->fullscreen = !w->fullscreen;

    if (w->fullscreen) {
	window_makefullscreen(w);
    } else {
	SDL_SetWindowFullscreen(w->window, 0);
    }

}

void window_makecurrent(struct mSDLWindow *w)
{
    if (w->window != SDL_GL_GetCurrentWindow())
	SDL_GL_MakeCurrent(w->window, w->glContext);
    glViewport(0, 0, w->width, w->height);
}

void window_swap(struct mSDLWindow *w)
{
    SDL_GL_SwapWindow(w->window);
}

void window_seticon(struct mSDLWindow *w, struct Texture *icon)
{
    SDL_Surface *winIcon = SDL_CreateRGBSurfaceWithFormatFrom(icon->data, icon->width, icon->height, 32, 4*icon->width, SDL_PIXELFORMAT_RGBA32);
    SDL_SetWindowIcon(w->window, winIcon);
    SDL_FreeSurface(winIcon);
}

int window_hasfocus(struct mSDLWindow *w)
{
    return SDL_GetWindowFlags(w->window) & SDL_WINDOW_INPUT_FOCUS;
}