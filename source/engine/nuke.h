#ifndef NUKE_H
#define NUKE_H

#include "nuklear.h"


extern struct nk_context *ctx;

struct mSDLWindow;

void nuke_init(struct mSDLWindow *win);
void nuke_start();
void nuke_end();

void nk_property_float3(struct nk_context *ctx, const char *label, float min, float *val, float max, float step, float dragstep);
void nk_property_float2(struct nk_context *ctx, const char *label, float min, float *val, float max, float step, float dragstep);

void nuke_property_int(const char *lbl, int min, int *val, int max, int step);
void nk_radio_button_label(struct nk_context *ctx, const char *label, int *val, int cmp);
void nuke_radio_btn(const char *lbl, int *val, int cmp);
void nuke_checkbox(const char *lbl, int *val);
void nuke_nel(int cols);
void nuke_label(const char *s);
void nuke_prop_float(const char *label, float min, float *val, float max, float step, float dragstep);

int nuke_btn(const char *lbl);

#define nuke_labelf(STR, ...) nk_labelf(ctx, NK_TEXT_LEFT, STR, __VA_ARGS__)
#define nuke_prop_float(LABEL, MIN, VAL, MAX, STEP, DRAG) nk_property_float(ctx, LABEL, MIN, VAL, MAX, STEP, DRAG)

#endif