#include "script.h"

#include "stdio.h"
#include "log.h"

#include "mrbffi.h"

#include "stb_ds.h"

s7_scheme *s7 = NULL;

s7_pointer *updates;

static void my_print(s7_scheme *sc, uint8_t c, s7_pointer port) {
    static char buffer[1024];
    static char *p = buffer;
    if (c != '\0' && p != &buffer[1023]) {
        *p = c;
        p++;
    } else {
        YughInfo(buffer);
        p = buffer;
    }
}

void script_init() {
    s7 = s7_init();
    s7_set_current_error_port(s7, s7_open_output_string(s7));
    s7_set_current_output_port(s7, s7_open_output_function(s7, my_print));
    ffi_load();
}

void script_run(const char *script) {
    s7_eval_c_string(s7, script);

   const char *errmsg = s7_get_output_string(s7, s7_current_error_port(s7));

   if (errmsg && (*errmsg))
       mYughLog(1, 2, 0, "script", "Scripting error: %s", errmsg);

   s7_flush_output_port(s7, s7_current_error_port(s7));
}

int script_dofile(const char *file) {
/*    static char fload[512];
    snprintf(fload, 512, "(write (load \"%s\"))", file);
    s7_eval_c_string(s7, fload);
    */
    if (!s7_load(s7, file)) {
      YughError("Can't find file %s.", file);
      return 1;
   }

   const char *errmsg = s7_get_output_string(s7, s7_current_error_port(s7));

   if (errmsg && (*errmsg))
       mYughLog(1, 2, 0, "script", "Scripting error: %s", errmsg);


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

void register_update(s7_pointer sym) {
    arrput(updates, sym);
}

void call_updates() {
    for (int i = 0; i < arrlen(updates); i++)
        script_call_sym(updates[i]);
}