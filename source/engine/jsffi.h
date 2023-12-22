#ifndef FFI_H
#define FFI_H

#include "quickjs/quickjs.h"
#include "HandmadeMath.h"

void ffi_load();

JSValue vec2js(HMM_Vec2 v);
HMM_Vec2 js2vec2(JSValue v);

JSValue bitmask2js(cpBitmask mask);
cpBitmask js2bitmask(JSValue v);
int js_print_exception(JSValue v);

struct rgba js2color(JSValue v);
double js2number(JSValue v);
JSValue num2js(double g);
JSValue int2js(int i);
JSValue str2js(const char *c);

#endif
