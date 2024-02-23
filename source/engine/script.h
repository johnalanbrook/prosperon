#ifndef SCRIPT_H
#define SCRIPT_H

#include "quickjs/quickjs.h"
#include <time.h>

extern JSContext *js;

struct callee {
  JSValue fn;
  JSValue obj;
};

struct phys_cbs {
  JSValue begin;
  JSValue bhit;
  JSValue separate;
  JSValue shit;
};
void script_call_fn_arg(JSValue fn, JSValue arg);

extern struct callee stacktrace_callee;
extern JSValue num_cache[100];

JSValue jstr(const char *str);
void call_stack();
void js_stacktrace();
void script_startup();
void script_stop();
void script_run(const char *script, const char *file);
void script_evalf(const char *format, ...);
int script_dofile(const char *file);
time_t jso_file(const char *file);
JSValue script_runfile(const char *file);
void script_update(double dt);
void script_draw();
void free_callee(struct callee c);

void duk_run_err();
void js_dump_stack();

void out_memusage(const char *f);

void script_editor();
void script_call(const char *f);
void script_call_sym(JSValue sym);
void call_callee(struct callee *c);
void script_callee(struct callee c, int argc, JSValue *argv);
int script_has_sym(void *sym);
void script_eval_w_env(const char *s, JSValue env, const char *file);
void call_env(JSValue env, const char *eval);
void file_eval_env(const char *file, JSValue env);

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

void send_signal(const char *signal, int argc, JSValue *argv);
void script_gc();

void register_physics(struct callee c);
void call_physics(double dt);

void register_draw(struct callee c);
void call_draw();

JSValue script_run_bytecode(uint8_t *code, size_t len);
uint8_t *script_compile(const char *file, size_t *len);

#endif
