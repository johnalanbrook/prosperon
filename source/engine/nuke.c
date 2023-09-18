#include "nuke.h"

#ifndef NO_EDITOR

#define STBTT_STATIC

#include "config.h"

#include "sokol/sokol_gfx.h"

#define NK_IMPLEMENTATION
#define SOKOL_NUKLEAR_IMPL
#include "nuklear.h"
#include "sokol/sokol_app.h"
#include "sokol/sokol_nuklear.h"

#include <stdarg.h>

#include "log.h"
#include "texture.h"
#include "window.h"

#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024

struct nk_context *ctx;

void nuke_init(struct window *win) {
  snk_setup(&(snk_desc_t){
    .no_default_font = false
  });

  ctx = snk_new_frame();
}

struct rect nk2rect(struct nk_rect rect)
{
  struct rect r;
  r.x = rect.x;
  r.y = rect.y;
  r.w = rect.w;
  r.h = rect.h;
  return r;
}

struct nk_rect rect2nk(struct rect rect)
{
  struct nk_rect r;
  r.x = rect.x;
  r.y = rect.y;
  r.w = rect.w;
  r.h = rect.h;
  return r;
}

void nuke_start() {
  ctx = snk_new_frame();
}

void nuke_end() {
  snk_render(mainwin.width,mainwin.height);
}

int nuke_begin(const char *lbl, struct rect rect) {
  return nk_begin(ctx, lbl, rect2nk(rect), NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_TITLE);
}

int nuke_begin_win(const char *lbl) {
  return nk_begin(ctx, lbl, nk_rect(10, 10, 400, 600), NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_TITLE);
}

void nuke_stop() {
  nk_end(ctx);
}

struct rect nuke_win_get_bounds() {
  return nk2rect(nk_window_get_bounds(ctx));
}

void nuke_row(int height) {
  nk_layout_row_dynamic(ctx, height, 1);
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

void nuke_img(char *path) {
  /*
    struct Texture *t = texture_pullfromfile(path);
    nk_layout_row_static(ctx, t->height, t->width, 1);
    nk_image(ctx, nk_image_id(t->id));
  */
}

void nuke_property_int(const char *lbl, int min, int *val, int max, int step) {
  nk_property_int(ctx, lbl, min, val, max, step, step);
}

void nuke_radio_btn(const char *lbl, int *val, int cmp) {
  if (nk_option_label(ctx, lbl, *val == cmp)) *val = cmp;
}

void nuke_checkbox(const char *lbl, int *val) {
  nk_checkbox_label(ctx, lbl, val);
}

void nuke_scrolltext(char *str) {
  nk_edit_string_zero_terminated(ctx, NK_EDIT_MULTILINE | NK_EDIT_GOTO_END_ON_ACTIVATE, str, 1024 * 1024 * 5, NULL);
}

void nuke_nel(int cols) {
  nk_layout_row_dynamic(ctx, 25, cols);
}

void nuke_nel_h(int cols, int h) {
  nk_layout_row_dynamic(ctx, h, cols);
}

void nuke_label(const char *s) {
  nk_label(ctx, s, NK_TEXT_LEFT);
}

void nuke_edit_str(char *str) {
  nk_edit_string_zero_terminated(ctx, NK_EDIT_BOX | NK_EDIT_NO_HORIZONTAL_SCROLL, str, 130, nk_filter_ascii);
}

int nuke_push_tree_id(const char *name, int id) {
  return nk_tree_push_id(ctx, NK_TREE_NODE, name, NK_MINIMIZED, id);
}

void nuke_tree_pop() {
  nk_tree_pop(ctx);
}

void nuke_labelf(const char *fmt, ...) {
  char buf[512];

  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, 512, fmt, args);
  nuke_label(buf);
  va_end(args);
}

#endif
