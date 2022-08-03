#include "mrbffi.h"

#include "mruby.h"
#include "mruby/compile.h"

#include "script.h"

extern mrb_state *mrb;

int fib(int n) {
    if (n < 2) return n;

    return fib(n-1) + fib(n-2);
}

/* FFI */

mrb_value mrb_fib(mrb_state *mrb, mrb_value self) {
    int n;
    mrb_get_args(mrb, "i", &n);
    return mrb_fixnum_value(fib(n));
}

mrb_value mrb_load(mrb_state *mrb, mrb_value self) {
    char *path;
    mrb_get_args(mrb, "z!", &path);
    script_dofile(path);
    return self;
}

void ffi_load() {
    mrb_define_method(mrb, mrb->object_class, "fib", mrb_fib, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, mrb->object_class, "load", mrb_load, MRB_ARGS_REQ(1));
}

