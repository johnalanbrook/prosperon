#ifndef WINDOW_H
#define WINDOW_H

#include "render.h"
#include <stdbool.h>
#include <stdint.h>

struct window {
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
    void (*nuke_gui)();
};

struct Texture;

extern struct window *mainwin;


struct window *MakeSDLWindow(const char *name, int width, int height, uint32_t flags);
void window_set_icon(const char *png);
void window_destroy(struct window *w);
void window_handle_event(struct window *w);
void window_all_handle_events();
void window_makecurrent(struct window *w);
void window_makefullscreen(struct window *w);
void window_togglefullscreen(struct window *w);
void window_unfullscreen(struct window *w);
void window_swap(struct window *w);
void window_seticon(struct window *w, struct Texture *icon);
int window_hasfocus(struct window *w);
struct window *window_i(int index);

void window_render(struct window *w);
void window_renderall();

#endif
