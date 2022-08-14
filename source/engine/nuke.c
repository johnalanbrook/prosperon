#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_GLFW_GL3_IMPLEMENTATION
#define NK_KEYSTATE_BASED_INPUT

#include "nuke.h"
#include "nuklear_glfw_gl3.h"

#include "window.h"

#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024

struct nk_context *ctx;
static struct nk_glfw nkglfw = {0};


void nuke_init(struct mSDLWindow *win) {
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

void nk_property_float3(struct nk_context *ctx, const char *label, float min, float *val, float max, float step, float dragstep) {
    nk_layout_row_dynamic(ctx, 25, 1);
    nk_label(ctx, label, NK_TEXT_LEFT);
    nk_layout_row_dynamic(ctx, 25, 3);
    nk_property_float(ctx, "X", min, &val[0], max, step, dragstep);
    nk_property_float(ctx, "Y", min, &val[1], max, step, dragstep);
    nk_property_float(ctx, "Z", min, &val[2], max, step, dragstep);
}

void nk_property_float2(struct nk_context *ctx, const char *label, float min, float *val, float max, float step, float dragstep) {
    nk_layout_row_dynamic(ctx, 25, 1);
    nk_label(ctx, label, NK_TEXT_LEFT);
    nk_layout_row_dynamic(ctx, 25, 2);
    nk_property_float(ctx, "X", min, &val[0], max, step, dragstep);
    nk_property_float(ctx, "Y", min, &val[1], max, step, dragstep);
}

void nk_radio_button_label(struct nk_context *ctx, const char *label, int *val, int cmp) {
    if (nk_option_label(ctx, label, (bool)*val == cmp)) *val = cmp;
}