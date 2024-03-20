#ifndef FFI_H
#define FFI_H

#include "quickjs/quickjs.h"
#include "HandmadeMath.h"
#include "dsp.h"

void ffi_load();

JSValue vec22js(HMM_Vec2 v);
HMM_Vec2 js2vec2(JSValue v);

JSValue bitmask2js(cpBitmask mask);
cpBitmask js2bitmask(JSValue v);
int js_print_exception(JSValue v);

struct rgba js2color(JSValue v);
double js2number(JSValue v);
JSValue number2js(double g);
JSValue str2js(const char *c);

void nota_int(char *blob);

#endif
