#ifndef WINDOW_H
#define WINDOW_H

#include <SDL.h>
#include <stdbool.h>

struct mSDLWindow {
    SDL_Window *window;
    SDL_GLContext glContext;
    int id;
    int width;
    int height;
    /* TODO: Bitfield these */
    bool render;
    bool mouseFocus;
    bool keyboardFocus;
    bool fullscreen;
    bool minimized;
    bool shown;
    float projection[16];
};

extern struct mSDLWindow *window;

struct mSDLWindow *MakeSDLWindow(const char *name, int width, int height,
				 uint32_t flags);
void window_handle_event(struct mSDLWindow *w, SDL_Event * e);
void window_focus(struct mSDLWindow *w);
void window_makecurrent(struct mSDLWindow *w);
void window_makefullscreen(struct mSDLWindow *w);
void window_togglefullscreen(struct mSDLWindow *w);
void window_swap(struct mSDLWindow *w);

#endif
