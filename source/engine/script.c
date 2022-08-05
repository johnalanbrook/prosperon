#include "script.h"

#include "stdio.h"
#include "log.h"

#include "mruby.h"
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

void script_dofile(const char *file) {
    FILE *mrbf = fopen(file, "r");
    if (mrbf == NULL) {
        YughError("Could not find game.rb in root folder.",0);
        return;
    }
    mrbc_filename(mrb, c, file);
    obj = mrb_load_file_cxt(mrb, mrbf, c);
    mrb_print_error(mrb);
    fclose(mrbf);
}

void script_update() {
    mrb_funcall(mrb, obj, "update", 0);
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
    mrb_funcall(mrb, obj, f, 0);
    mrb_print_error(mrb);
}