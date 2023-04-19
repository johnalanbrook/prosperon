#ifndef SCRIPT_H
#define SCRIPT_H

#include "quickjs/quickjs.h"
#include <chipmunk/chipmunk.h>
#include <time.h>

extern JSContext *js;

struct callee {
  JSValue fn;
  JSValue obj;
};

void script_init();
void script_run(const char *script);
int script_dofile(const char *file);
void script_update(double dt);
void script_draw();

void duk_run_err();

void script_editor();
void script_call(const char *f);
void script_call_sym(JSValue sym);
void call_callee(struct callee *c);
void script_callee(struct callee c, int argc, JSValue *argv);
int script_has_sym(void *sym);
void script_eval_w_env(const char *s, JSValue env);

time_t file_mod_secs(const char *file);

void register_update(struct callee c);
void call_updates(double dt);
void call_debugs();

void unregister_gui(struct callee c);
void register_gui(struct callee c);
void register_debug(struct callee c);
void register_nk_gui(struct callee c);
void call_gui();
void call_nk_gui();
void unregister_obj(JSValue obj);

void register_physics(struct callee c);
void call_physics(double dt);

#endif
