#ifndef SCRIPT_H
#define SCRIPT_H

#include "duktape.h"
#include <chipmunk/chipmunk.h>

extern duk_context *duk;

void script_init();
void script_run(const char *script);
int script_dofile(const char *file);
void script_update(double dt);
void script_draw();
void script_editor();
void script_call(const char *f);
void script_call_sym(void *sym);
void script_call_sym_args(void *sym, void *args);
int script_has_sym(void *sym);
void script_eval_w_env(const char *s, void *env);

void register_update(void *sym);
void register_obupdate(void *obj, void *sym);
void call_updates(double dt);

void register_gui(void *sym);
void call_gui();

void register_physics(void *sym);
void call_physics(double dt);

duk_idx_t vec2duk(cpVect v);

#endif
