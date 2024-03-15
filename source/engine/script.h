#ifndef SCRIPT_H
#define SCRIPT_H

#include "quickjs/quickjs.h"
#include <time.h>

extern JSContext *js;

struct phys_cbs {
  JSValue begin;
  JSValue bhit;
  JSValue separate;
  JSValue shit;
};

void script_startup();
void script_stop();

void out_memusage(const char *f);
void js_stacktrace();

void script_evalf(const char *format, ...);
JSValue eval_script_env(const char *file, JSValue env, const char *script);

void script_call_sym(JSValue sym, int argc, JSValue *argv);

void script_gc();

JSValue script_run_bytecode(uint8_t *code, size_t len);
uint8_t *script_compile(const char *file, size_t *len);

#endif
