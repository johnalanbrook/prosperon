#ifndef FFI_H
#define FFI_H

#include "quickjs/quickjs.h"
#include <chipmunk/chipmunk.h>

void ffi_load();

void duk_dump_stack(JSContext *js);

JSValue vec2js(cpVect v);
cpVect js2vec2(JSValue v);

JSValue bitmask2js(cpBitmask mask);
cpBitmask js2bitmask(JSValue v);

struct color duk2color(JSValue v);

#endif
