#ifndef FFI_H
#define FFI_H

#include "duktape.h"
#include <chipmunk/chipmunk.h>

void ffi_load();


void duk_dump_stack(duk_context *duk);

duk_idx_t vect2duk(cpVect v);
cpVect duk2vec2(duk_context *duk, int p);

void bitmask2duk(duk_context *duk, cpBitmask mask);
cpBitmask duk2bitmask(duk_context *duk, int p);

struct color duk2color(duk_context *duk, int p);

#endif
