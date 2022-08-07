#ifndef SCRIPT_H
#define SCRIPT_H

#include "mruby.h"

void script_init();
void script_run(const char *script);
void script_dofile(const char *file);
void script_update(double dt);
void script_draw();
void script_editor();
void script_call(const char *f);
void script_call_sym(mrb_sym sym);
int script_has_sym(mrb_sym sym);

#endif
