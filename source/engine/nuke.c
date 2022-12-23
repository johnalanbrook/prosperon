#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_INCLUDE_STANDARD_BOOL
#define NK_IMPLEMENTATION
#define NK_GLFW_GL3_IMPLEMENTATION
#define NK_KEYSTATE_BASED_INPUT


#include "nuke.h"
#include "nuklear_glfw_gl3.h"

#include <stdarg.h>

#include "window.h"

#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024

struct nk_context *ctx;
static struct nk_glfw nkglfw = {0};


void nuke_init(struct window *win) {
    window_makecurrent(win);

    ctx = nk_glfw3_init(&nkglfw, win->window, NK_GLFW3_INSTALL_CALLBACKS);


    struct nk_font_atlas *atlas;
    nk_glfw3_font_stash_begin(&nkglfw, &atlas);
    struct nk_font *noto = nk_font_atlas_add_from_file(atlas, "fonts/notosans.tff", 14, 0);
    nk_glfw3_font_stash_end(&nkglfw);
}

void nuke_start()
{
    nk_glfw3_new_frame(&nkglfw);
}

void nuke_end()
{
    nk_glfw3_render(&nkglfw, NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
}


int nuke_begin(const char *lbl, struct nk_rect rect, int flags) {
    return nk_begin(ctx, lbl, rect, flags);
}
void nuke_stop() {
    nk_end(ctx);
}

struct nk_rect nuke_win_get_bounds() {
    return nk_window_get_bounds(ctx);
}

void nuke_property_float3(const char *label, float min, float *val, float max, float step, float dragstep) {
    nk_layout_row_dynamic(ctx, 25, 1);
    nk_label(ctx, label, NK_TEXT_LEFT);
    nk_layout_row_dynamic(ctx, 25, 3);
    nuke_property_float("#X", min, &val[0], max, step, dragstep);
    nuke_property_float("#Y", min, &val[1], max, step, dragstep);
    nuke_property_float("#Z", min, &val[2], max, step, dragstep);
}

void nuke_property_float2(const char *label, float min, float *val, float max, float step, float dragstep) {
    nk_layout_row_dynamic(ctx, 25, 1);
    nk_label(ctx, label, NK_TEXT_LEFT);
    nk_layout_row_dynamic(ctx, 25, 2);
    nuke_property_float("#X", min, &val[0], max, step, dragstep);
    nuke_property_float("#Y", min, &val[1], max, step, dragstep);
}

void nuke_property_float(const char *lbl, float min, float *val, float max, float step, float dragstep) {
    nk_property_float(ctx, lbl, min, val, max, step, dragstep);
}

int nuke_btn(const char *lbl) {
    return nk_button_label(ctx, lbl);
}

void nuke_property_int(const char *lbl, int min, int *val, int max, int step) {
    nk_property_int(ctx, lbl, min, val, max, step, step);
}

void nuke_radio_btn(const char *lbl, int *val, int cmp) {
    if (nk_option_label(ctx, lbl, *val==cmp)) *val = cmp;
}

void nuke_checkbox(const char *lbl, int *val) {
    nk_checkbox_label(ctx, lbl, val);
}

void nuke_nel(int cols) {
    nk_layout_row_dynamic(ctx, 25, cols);
}

void nuke_label(const char *s) {
    nk_label(ctx, s, NK_TEXT_LEFT);
}

void nuke_edit_str(char *str) {
    nk_edit_string_zero_terminated(ctx, NK_EDIT_BOX|NK_EDIT_NO_HORIZONTAL_SCROLL, str, 130, nk_filter_ascii);
}

int nuke_push_tree_id(const char *name, int id) {
    return nk_tree_push_id(ctx, NK_TREE_NODE, name, NK_MINIMIZED, id);
}

void nuke_tree_pop() {
    nk_tree_pop(ctx);
}

void nuke_labelf(const char *fmt, ...) {
    va_list args;
    nk_labelf(ctx, NK_TEXT_LEFT, fmt, args);
}