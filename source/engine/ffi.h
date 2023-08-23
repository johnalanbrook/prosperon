#ifndef FFI_H
#define FFI_H

#include "quickjs/quickjs.h"
#include <chipmunk/chipmunk.h>
#include "2dphysics.h"

void ffi_load();

JSValue vec2js(cpVect v);
cpVect js2vec2(JSValue v);

JSValue bitmask2js(cpBitmask mask);
cpBitmask js2bitmask(JSValue v);

struct rgba js2color(JSValue v);
double js2number(JSValue v);
JSValue num2js(double g);
JSValue int2js(int i);
JSValue str2js(const char *c);

#endif
