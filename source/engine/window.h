#ifndef WINDOW_H
#define WINDOW_H

#include "render.h"
#include <stdbool.h>
#include <stdint.h>

struct mSDLWindow {
    GLFWwindow *window;
    int id;
    int width;
    int height;
    /* TODO: Bitfield these */
    bool render;
    bool mouseFocus;
    bool keyboardFocus;
    bool fullscreen;
    bool minimized;
    bool iconified;
    bool shown;
    float projection[16];
};

struct Texture;

extern struct mSDLWindow *mainwin;


struct mSDLWindow *MakeSDLWindow(const char *name, int width, int height, uint32_t flags);
void window_set_icon(const char *png);
void window_destroy(struct mSDLWindow *w);
void window_handle_event(struct mSDLWindow *w);
void window_all_handle_events();
void window_makecurrent(struct mSDLWindow *w);
void window_makefullscreen(struct mSDLWindow *w);
void window_togglefullscreen(struct mSDLWindow *w);
void window_swap(struct mSDLWindow *w);
void window_seticon(struct mSDLWindow *w, struct Texture *icon);
int window_hasfocus(struct mSDLWindow *w);

double frame_time();
int elapsed_time();

#endif
