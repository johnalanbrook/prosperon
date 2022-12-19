#ifndef SCRIPT_H
#define SCRIPT_H

#include "s7.h"

void script_init();
void script_run(const char *script);
int script_dofile(const char *file);
void script_update(double dt);
void script_draw();
void script_editor();
void script_call(const char *f);
void script_call_sym(s7_pointer sym);
int script_has_sym(s7_pointer sym);

void register_update(s7_pointer sym);
void call_updates();

#endif
