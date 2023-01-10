#include "script.h"

#include "stdio.h"
#include "log.h"

#include "ffi.h"
#include "font.h"

#include "ftw.h"

#include "stb_ds.h"

duk_context *duk = NULL;

duk_idx_t vect2duk(cpVect v) {
    duk_idx_t arr = duk_push_array(duk);
    duk_push_number(duk, v.x);
    duk_put_prop_index(duk, arr, 0);
    duk_push_number(duk, v.y);
    duk_put_prop_index(duk, arr, 1);

    return arr;
}


static int load_prefab(const char *fpath, const struct stat *sb, int typeflag) {
    if (typeflag != FTW_F)
        return 0;

    if (!strcmp(".prefab", strrchr(fpath, '.')))
        script_dofile(fpath);

    return 0;
}

void script_init() {
    duk = duk_create_heap_default();
    ffi_load();

    /* Load all prefabs into memory */
    script_dofile("scripts/engine.js");
    script_dofile("config.js");
    //ftw(".", load_prefab, 10);
}

void script_run(const char *script) {
    duk_eval_string(duk, script);
}

int script_dofile(const char *file) {
    const char *script = slurp_text(file);
    if (!script) {
      YughError("Can't find file %s.", file);
      return 1;
   }
   duk_push_string(duk, script);
   free(script);
   if (duk_peval(duk) != 0) {
       printf("ERROR: %s\n", duk_safe_to_string(duk, -1));
       return 1;
   }
   duk_pop(duk);


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

/* Call the given function name */
void script_call(const char *f) {
    //s7_call(s7, s7_name_to_value(s7, f), s7_nil(s7));
}

void script_eval_w_env(const char *s, void *env) {
    duk_push_heapptr(duk, env);
    duk_push_string(duk, s);

    YughInfo("pressed %s", s);

    if (!duk_has_prop(duk, -2)) {
      duk_pop(duk);
      return;
    }
    duk_push_string(duk, s);
    duk_call_prop(duk, -2, 0);
    duk_pop(duk);
    duk_pop(duk);
}

void script_call_sym(void *sym)
{
    duk_push_heapptr(duk, sym);
    duk_call(duk, 0);
    duk_pop(duk);
}

void script_call_sym_args(void *sym, void *args)
{
    //s7_call(s7, sym, s7_cons(s7, args, s7_nil(s7)));
}


void **updates;
void **guis;
void **physics;

struct obupdate {
    void *obj;
    void *sym;
};

struct obupdate *obupdates = NULL;

void register_update(void *sym) {
    arrput(updates, sym);
}

void register_obupdate(void *obj, void *sym) {
    struct obupdate ob = {obj, sym};
    arrput(obupdates, ob);
}

void call_updates(double dt) {
    for (int i = 0; i < arrlen(updates); i++) {
        duk_push_heapptr(duk, updates[i]);
        duk_push_number(duk, dt);
        duk_call(duk, 1);
    }

    for (int i = 0; i < arrlen(obupdates); i++) {
        duk_push_heapptr(duk, obupdates[i].sym);
        duk_push_heapptr(duk, obupdates[i].obj);
        duk_push_number(duk, dt);
        duk_call_method(duk, 1);
    }
}

void register_gui(void *sym) {
    arrput(guis, sym);
}

void call_gui() {
    for (int i = 0; i < arrlen(guis); i++)
        script_call_sym(guis[i]);
}

void register_physics(void *sym) {
    arrput(physics, sym);
}

void call_physics(double dt) {
    for (int i = 0; i < arrlen(physics); i++) {
        duk_push_pointer(duk, physics[i]);
        duk_push_number(duk, dt);
        duk_call(duk, 1);
    }
}