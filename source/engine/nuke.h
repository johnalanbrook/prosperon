#ifndef NUKE_H
#define NUKE_H

#include "nuklear.h"


extern struct nk_context *ctx;

struct mSDLWindow;

void nuke_init(struct mSDLWindow *win);
void nuke_start();
void nuke_end();

#endif