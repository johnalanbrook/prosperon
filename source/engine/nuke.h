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
void nk_radio_button_label(struct nk_context *ctx, const char *label, int *val, int cmp);
void nuke_nel(int cols);
void nuke_label(const char *s);

#endif