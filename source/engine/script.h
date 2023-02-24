#ifndef SCRIPT_H
#define SCRIPT_H

#include "duktape.h"
#include <chipmunk/chipmunk.h>

extern duk_context *duk;


struct callee {
  void *fn;
  void *obj;
};

void script_init();
void script_run(const char *script);
int script_dofile(const char *file);
void script_update(double dt);
void script_draw();

void duk_run_err();

void script_editor();
void script_call(const char *f);
void script_call_sym(void *sym);
void script_call_sym_args(void *sym, void *args);
void call_callee(struct callee *c);
int script_has_sym(void *sym);
void script_eval_w_env(const char *s, void *env);

int script_eval_setup(const char *s, void *env);
void script_eval_exec(int argc);

time_t file_mod_secs(const char *file);

void register_update(struct callee c);
void call_updates(double dt);

void register_gui(struct callee c);
void register_nk_gui(struct callee c);
void call_gui();
void call_nk_gui();
void unregister_obj(void *obj);

void register_physics(struct callee c);
void call_physics(double dt);

#endif
