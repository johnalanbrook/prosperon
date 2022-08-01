#include "script.h"

#include "stdio.h"

#include "mruby.h"
#include "mruby/compile.h"

static mrb_state *mrb;

int fib(int n) {
    if (n < 2) return n;
    return fib(n-1) + fib(n-2);
}

mrb_value mrb_fib(mrb_state *mrb, mrb_value self) {
    int n;
    mrb_get_args(mrb, "i", &n);
    return mrb_fixnum_value(fib(n));
}

void script_init() {
    mrb = mrb_open();
    mrb_define_method(mrb, mrb->object_class, "fib", mrb_fib, MRB_ARGS_REQ(1));
}

void script_run(const char *script) {
    mrb_load_string(mrb, script);
}

void script_dofile(const char *file) {
    FILE *mrbf = fopen(file, "r");
    mrb_load_file(mrb, mrbf);
    fclose(mrbf);
}