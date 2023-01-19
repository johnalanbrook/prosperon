#ifndef NUKE_H
#define NUKE_H

#include "nuklear.h"

struct window;

void nuke_init(struct window *win);
void nuke_start();
void nuke_end();

int nuke_begin(const char *lbl, struct nk_rect rect, int flags);
int nuke_begin_win(const char *lbl);
void nuke_stop();
struct nk_rect nuke_win_get_bounds();

void nuke_property_float(const char *lbl, float min, float *val, float max, float step, float dragstep);
#define nuke_prop_float nuke_property_float
void nuke_property_float2(const char *label, float min, float *val, float max, float step, float dragstep);
void nuke_property_float3(const char *label, float min, float *val, float max, float step, float dragstep);

void nuke_property_int(const char *lbl, int min, int *val, int max, int step);
void nuke_radio_btn(const char *lbl, int *val, int cmp);
void nuke_checkbox(const char *lbl, int *val);
void nuke_nel(int cols);
void nuke_label(const char *s);
void nuke_prop_float(const char *label, float min, float *val, float max, float step, float dragstep);
void nuke_edit_str(char *str);

int nuke_push_tree_id(const char *name, int id);
void nuke_tree_pop();

int nuke_btn(const char *lbl);

void nuke_labelf(const char *fmt, ...);

#endif
