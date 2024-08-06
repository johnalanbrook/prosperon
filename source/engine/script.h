#ifndef SCRIPT_H
#define SCRIPT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "quickjs/quickjs.h"
#include <time.h>

extern JSContext *js;
extern JSRuntime *rt;

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
void script_mem_limit(size_t limit);
void script_gc_threshold(size_t threshold);
void script_max_stacksize(size_t size);

void _script_gcstart();
void _script_gcend();



#ifdef __cplusplus
}
#endif

#endif
