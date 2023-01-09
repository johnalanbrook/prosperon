#include "script.h"

#include "stdio.h"
#include "log.h"

#include "mrbffi.h"

#include "ftw.h"

#include "stb_ds.h"

s7_scheme *s7 = NULL;

s7_pointer cpvec2s7(cpVect v) {
    s7_pointer ret = s7_make_vector(s7, 2);
    s7_vector_set(s7, ret, 0, s7_make_real(s7, v.x));
    s7_vector_set(s7, ret, 1, s7_make_real(s7, v.y));

    return ret;
}

static void null_port(s7_scheme *sc, uint8_t c, s7_pointer port) {

}

static void my_err(s7_scheme *sc, uint8_t c, s7_pointer port) {
    static char buffer[1024];
    static char *p = buffer;
    if (c != '\n' && p != &buffer[1023]) {
        *p = c;
        p++;
    } else {
        *p = '\0';
        if (buffer[0] != '\n')
            YughError(buffer);
        p = buffer;

        //YughInfo("File %s, line %d", s7_port_filename(sc, port), s7_port_line_number(sc, port));

        int lost = s7_flush_output_port(sc, port);
    }
}

static void my_print(s7_scheme *sc, uint8_t c, s7_pointer port) {
    static char buffer[1024];
    static char *p = buffer;
    if (c != '\n' && p != &buffer[1023]) {
        *p = c;
        p++;
    } else {
         *p = '\0';
        YughInfo(buffer);
        p = buffer;
    }
}

static int load_prefab(const char *fpath, const struct stat *sb, int typeflag) {
    if (typeflag != FTW_F)
        return 0;

    if (!strcmp(".prefab", strrchr(fpath, '.')))
        s7_load(s7, fpath);

    return 0;
}

void script_init() {
    s7 = s7_init();
    s7_set_current_error_port(s7, s7_open_output_function(s7, my_err));
    s7_set_current_output_port(s7, s7_open_output_function(s7, my_print));
    ffi_load();

    /* Load all prefabs into memory */
    script_dofile("scripts/engine.scm");
    script_dofile("config.scm");
    ftw(".", load_prefab, 10);
}

void script_run(const char *script) {
    s7_eval_c_string(s7, script);
}

int script_dofile(const char *file) {
    if (!s7_load(s7, file)) {
      YughError("Can't find file %s.", file);
      return 1;
   }
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

void script_eval_w_env(const char *s, s7_pointer env) {
    char buffer[512];
    snprintf(buffer, 512-1, "(%s)", s);

    s7_set_current_error_port(s7, s7_open_output_function(s7, null_port));
    s7_eval_c_string_with_environment(s7, buffer, env);
    s7_set_current_error_port(s7, s7_open_output_function(s7, my_err));
}

void script_call_sym(s7_pointer sym)
{
    s7_call(s7, sym, s7_nil(s7));
}

void script_call_sym_args(s7_pointer sym, s7_pointer args)
{
    s7_call(s7, sym, s7_cons(s7, args, s7_nil(s7)));
}

int script_has_sym(s7_pointer sym) {
    return 1;
}


s7_pointer *updates;
s7_pointer *guis;
s7_pointer *physics;

struct obupdate {
    s7_pointer obj;
    s7_pointer sym;
};

struct obupdate *obupdates = NULL;

void register_update(s7_pointer sym) {
    arrput(updates, sym);
}

void register_obupdate(s7_pointer obj, s7_pointer sym) {
    struct obupdate ob = {obj, sym};
    arrput(obupdates, ob);
}

void call_updates(double dt) {
    for (int i = 0; i < arrlen(updates); i++)
        s7_call(s7, updates[i], s7_cons(s7, s7_make_real(s7, dt), s7_nil(s7)));

    for (int i = 0; i < arrlen(obupdates); i++) {
        s7_set_curlet(s7, obupdates[i].obj);
        s7_call(s7, obupdates[i].sym, s7_cons(s7, s7_make_real(s7, dt), s7_nil(s7)));
    }

}

void register_gui(s7_pointer sym) {
    arrput(guis, sym);
}

void call_gui() {
    for (int i = 0; i < arrlen(guis); i++)
        script_call_sym(guis[i]);
}

void register_physics(s7_pointer sym) {
    arrput(physics, sym);
}

void call_physics(double dt) {
    for (int i = 0; i < arrlen(physics); i++)
        s7_call(s7, physics[i], s7_cons(s7, s7_make_real(s7, dt), s7_nil(s7)));
}