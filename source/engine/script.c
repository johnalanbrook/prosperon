#include "script.h"

#include "stdio.h"
#include "log.h"

#include "ffi.h"
#include "font.h"

#include "ftw.h"

#include "stb_ds.h"

#include "time.h"
#include "sys/stat.h"
#include "sys/types.h"

JSContext *js = NULL;

static int load_prefab(const char *fpath, const struct stat *sb, int typeflag) {
    if (typeflag != FTW_F)
        return 0;

    if (!strcmp(".prefab", strrchr(fpath, '.')))
        script_dofile(fpath);

    return 0;
}

void script_init() {
    js = duk_create_heap_default();
    ffi_load();

    /* Load all prefabs into memory */
    script_dofile("scripts/engine.js");
    script_dofile("config.js");
    //ftw(".", load_prefab, 10);
}

void script_run(const char *script) {
    duk_eval_string(js, script);
}

time_t file_mod_secs(const char *file) {
    struct stat attr;
    stat(file, &attr);
    return attr.st_mtime;
}

void duk_run_err() {
       duk_get_prop_string(js, -1, "lineNumber");
       duk_get_prop_string(js, -2, "fileName");
       duk_get_prop_string(js, -3, "stack");
       mYughLog(1, 2, duk_get_int(js, -3), duk_safe_to_string(js, -2), "%s\n%s", duk_safe_to_string(js, -4), duk_safe_to_string(js, -1));

       duk_pop_3(js);
}

int script_dofile(const char *file) {
    const char *script = slurp_text(file);
    if (!script) {
      YughError("Can't find file %s.", file);
      return 1;
   }
   duk_push_string(js, script);
   free(script);
   duk_push_string(js, file);

   if (duk_pcompile(js, 0) != 0) {
       duk_run_err();
       return 1;
   }

   if (duk_pcall(js, 0))
       duk_run_err();

   duk_pop(js);

   return file_mod_secs(file);
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

/* env is an object in the scripting environment;
   s is the function to call on that object
*/
void script_eval_w_env(const char *s, void *env) {
    duk_push_heapptr(js, env);
    duk_push_string(js, s);

    if (!duk_has_prop(js, -2)) {
      duk_pop(js);
      return;
    }
    duk_push_string(js, s);
    if (duk_pcall_prop(js, -2, 0))
        duk_run_err();

    duk_pop_2(js);
}

int script_eval_setup(const char *s, void *env)
{
  duk_push_heapptr(js, env);

  if (!duk_has_prop_string(js, -1, s)) {
    duk_pop(js);
    return 1;
  }

  duk_push_string(js, s);
  return 0;
}

void script_eval_exec(int argc)
{
  if (duk_pcall_prop(js, -2 - argc, argc))
    duk_run_err();

  duk_pop_2(js);
}

void script_call_sym(void *sym)
{
    duk_push_heapptr(js, sym);
    if (duk_pcall(js, 0))
        duk_run_err();

    duk_pop(js);
}

void script_call_sym_args(void *sym, void *args)
{
    //s7_call(s7, sym, s7_cons(s7, args, s7_nil(s7)));
}

struct callee *updates = NULL;
struct callee *physics = NULL;
struct callee *guis = NULL;
struct callee *debugs = NULL;
struct callee *nk_guis = NULL;

void unregister_obj(JSValue obj)
{
/*  for (int i = arrlen(updates)-1; i >= 0; i--)
    if (updates[i].obj == obj) arrdel(updates, i);
    
  for (int i = arrlen(physics)-1; i >= 0; i--)
    if (physics[i].obj == obj) arrdel(physics,i);
    
  for (int i = arrlen(guis)-1; i >= 0; i--)
    if (guis[i].obj == obj) arrdel(guis,i);
    
  for (int i = arrlen(nk_guis)-1; i >= 0; i--)
    if (nk_guis[i].obj == obj) arrdel(nk_guis,i);

  for (int i = arrlen(debugs)-1; i >= 0; i--)
    if (debugs[i].obj == obj) arrdel(debugs,i);
*/
}

void register_debug(struct callee c) {
  arrput(debugs, c);
}

void unregister_gui(struct callee c)
{
  for (int i = arrlen(guis)-1; i >= 0; i--) {
/*  
    if (guis[i].obj == c.obj && guis[i].fn == c.fn) {
      arrdel(guis, i);
      return;
    }
    */
  }
}

void register_update(struct callee c) {
    arrput(updates, c);
}

void register_gui(struct callee c) {
    arrput(guis, c);
}

void register_nk_gui(struct callee c) { arrput(nk_guis, c); }

void register_physics(struct callee c) {
    arrput(physics, c);
}

void setup_callee(struct callee c)
{
  duk_push_heapptr(js, c.fn);
  duk_push_heapptr(js, c.obj);
}

void exec_callee(int argc)
{
    if (duk_pcall_method(js, argc))
        duk_run_err();

    duk_pop(js);
}

void call_callee(struct callee *c) {
  setup_callee(*c);
  exec_callee(0);
}


void callee_dbl(struct callee c, double d)
{
  setup_callee(c);
  duk_push_number(js, d);
  exec_callee(1);
}

void callee_int(struct callee c, int i)
{
  setup_callee(c);
  duk_push_int(js, i);
  exec_callee(1);
}

void callee_vec2(struct callee c, cpVect vec)
{
  setup_callee(c);
  vect2duk(vec);
  exec_callee(1);
}

void call_updates(double dt) {
    for (int i = 0; i < arrlen(updates); i++)
        callee_dbl(updates[i], dt);
}

void call_gui() {
    for (int i = 0; i < arrlen(guis); i++)
        call_callee(&guis[i]);
}

void call_debug() {
  for (int i = 0; i < arrlen(debugs); i++)
    call_callee(&debugs[i]);
}

void call_nk_gui() {
    for (int i = 0; i < arrlen(nk_guis); i++)
        call_callee(&nk_guis[i]);
}

void call_physics(double dt) {
    for (int i = 0; i < arrlen(physics); i++)
        callee_dbl(physics[i], dt);
}

void call_debugs()
{
  for (int i = 0; i < arrlen(debugs); i++)
    call_callee(&debugs[i]);
}
