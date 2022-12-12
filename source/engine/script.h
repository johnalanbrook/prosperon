#ifndef SCRIPT_H
#define SCRIPT_H

#include "s7.h"
extern s7_scheme *s7;

void script_init();
void script_run(const char *script);
int script_dofile(const char *file);
void script_update(double dt);
void script_draw();
void script_editor();
void script_call(const char *f);
void script_call_sym(s7_pointer sym);
int script_has_sym(s7_pointer sym);

#endif
