#include "script.h"

#include "stdio.h"
#include "log.h"

#include "mrbffi.h"

s7_scheme *s7 = NULL;

void script_init() {
    s7 = s7_init();
    ffi_load();
}

void script_run(const char *script) {
    s7_eval_c_string(s7, script);
}

int script_dofile(const char *file) {
    s7_load(s7, file);
    return 0;
}

/* Call the "update" function in the master game script */
void script_update(double dt) {
}

/* Call the "draw" function in master game script */
void script_draw() {
}

/* Call "editor" function in master game script */
void script_editor() {
}

void script_call(const char *f) {
    s7_call(s7, s7_name_to_value(s7, f), s7_nil(s7));
}

void script_call_sym(s7_pointer sym)
{
    s7_call(s7, sym, s7_nil(s7));
}

int script_has_sym(s7_pointer sym) {
    return 1;
}
