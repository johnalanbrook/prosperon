#ifndef FFI_H
#define FFI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "quickjs/quickjs.h"
#include "HandmadeMath.h"
#include <stdarg.h>
#include <chipmunk/chipmunk.h>
#include "qjs_macros.h"

void script_report_gc_time(double t, double startmem, double mem);

int JS_Is(JSValue v);

extern JSValue cpShape2js(cpShape *s);

void ffi_load();

JSValue vec22js(HMM_Vec2 v);
HMM_Vec2 js2vec2(JSValue v);
HMM_Vec4 js2vec4(JSValue v);

const char *js2str(JSValue v);
char *js2strdup(JSValue v);

JSValue bitmask2js(cpBitmask mask);
cpBitmask js2bitmask(JSValue v);
int js_print_exception(JSValue v);

struct rgba js2color(JSValue v);

double js2number(JSValue v);
JSValue number2js(double g);

uint64_t js2uint64(JSValue v);

JSValue str2js(const char *c, ...);

struct texture;
struct texture *js2texture(JSValue v);

void nota_int(char *blob);

void js_setprop_num(JSValue a, uint32_t n, JSValue v);
JSValue js_getpropidx(JSValue v, uint32_t i);
JSValue js_getpropstr(JSValue v, const char *str);
void jsfreestr(const char *str);
int js_arrlen(JSValue v);
void js_setpropstr(JSValue v, const char *str, JSValue p);
void js2floatarr(JSValue v, int n, float *a);
JSValue floatarr2js(int n, float *a);

float *js2newfloatarr(JSValue v);

int js2boolean(JSValue v);
JSValue boolean2js(int b);

char **js2strarr(JSValue v);

#define PREP_PARENT(TYPE, PARENT) \
TYPE##_proto = JS_NewObject(js); \
JS_SetPropertyFunctionList(js, TYPE##_proto, js_##TYPE##_funcs, countof(js_##TYPE##_funcs)); \
JS_SetPrototype(js, TYPE##_proto, PARENT##_proto); \

#ifdef __cplusplus
}
#endif

#endif
