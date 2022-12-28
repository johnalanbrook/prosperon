#ifndef SCRIPT_H
#define SCRIPT_H

#include "s7.h"
#include <chipmunk/chipmunk.h>

extern s7_scheme *s7;

void script_init();
void script_run(const char *script);
int script_dofile(const char *file);
void script_update(double dt);
void script_draw();
void script_editor();
void script_call(const char *f);
void script_call_sym(s7_pointer sym);
void script_call_sym_args(s7_pointer sym, s7_pointer args);
int script_has_sym(s7_pointer sym);
void script_eval_w_env(const char *s, s7_pointer env);

void register_update(s7_pointer sym);
void call_updates(double dt);

void register_gui(s7_pointer sym);
void call_gui();

void register_physics(s7_pointer sym);
void call_physics(double dt);

s7_pointer cpvec2s7(cpVect v);

#endif
