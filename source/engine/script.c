#include "script.h"

#include "stdio.h"
#include "log.h"


#include "mruby/compile.h"
#include "mrbffi.h"

mrb_value obj;

mrb_state *mrb;
mrbc_context *c;

void script_init() {
    mrb = mrb_open();
    c = mrbc_context_new(mrb);
    ffi_load();
}

void script_run(const char *script) {
    mrb_load_string(mrb, script);
}

int script_dofile(const char *file) {
    FILE *mrbf = fopen(file, "r");
    if (mrbf == NULL) {
        YughError("Could not find script %s.", file);
        return 1;
    }
    mrbc_filename(mrb, c, file);
    obj = mrb_load_file_cxt(mrb, mrbf, c);
    mrb_print_error(mrb);
    fclose(mrbf);

    return 0;
}

void script_update(double dt) {
    mrb_funcall(mrb, obj, "update", 1, mrb_float_value(mrb, dt));
    mrb_print_error(mrb);
}

void script_draw() {
    mrb_funcall(mrb, obj, "draw", 0);
    mrb_print_error(mrb);
}

void script_editor() {
    mrb_funcall(mrb, obj, "editor", 0);
    mrb_print_error(mrb);
}

void script_call(const char *f) {
    script_call_sym(mrb_intern_cstr(mrb, f));
}

void script_call_sym(mrb_sym sym)
{
    if (mrb_respond_to(mrb, obj, sym)) mrb_funcall_argv(mrb, obj, sym, 0, NULL);

    mrb_print_error(mrb);
}

int script_has_sym(mrb_sym sym) {
    return mrb_respond_to(mrb, obj, sym);
}
