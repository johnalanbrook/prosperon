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
JSValue script_eval(const char *file, const char *script);

void script_call_sym(JSValue sym, int argc, JSValue *argv);

void script_gc();

#endif
