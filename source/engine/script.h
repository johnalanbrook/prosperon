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
void js_stacktrace();
void script_startup();
void script_stop();
void script_run(const char *script, const char *file);
void script_evalf(const char *format, ...);
int script_dofile(const char *file);
time_t jso_file(const char *file);
JSValue script_runfile(const char *file);
JSValue eval_file_env(const char *script, const char *file, JSValue env);
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
JSValue file_eval_env(const char *file, JSValue env);

time_t file_mod_secs(const char *file);

void send_signal(const char *signal, int argc, JSValue *argv);
void script_gc();

JSValue script_run_bytecode(uint8_t *code, size_t len);
uint8_t *script_compile(const char *file, size_t *len);

#endif
