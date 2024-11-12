#ifndef FFI_H
#define FFI_H
#include <quickjs.h>
void ffi_load(JSContext *js);
int js_print_exception(JSContext *js, JSValue v);

#endif
