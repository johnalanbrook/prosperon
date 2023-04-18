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
JSRuntime *rt = NULL;

static int load_prefab(const char *fpath, const struct stat *sb, int typeflag) {
    if (typeflag != FTW_F)
        return 0;

    if (!strcmp(".prefab", strrchr(fpath, '.')))
        script_dofile(fpath);

    return 0;
}

void script_init() {
    rt = JS_NewRuntime();
    js = JS_NewContext(rt);
    ffi_load();

    /* Load all prefabs into memory */
    script_dofile("scripts/engine.js");
    script_dofile("config.js");
    //ftw(".", load_prefab, 10);
}

void script_run(const char *script) {
  JS_Eval(js, script, strlen(script), "script", 0);
}

time_t file_mod_secs(const char *file) {
    struct stat attr;
    stat(file, &attr);
    return attr.st_mtime;
}

int script_dofile(const char *file) {
   const char *script = slurp_text(file);
    if (!script) {
      YughError("Can't find file %s.", file);
      return 1;
   }
   JS_Eval(js, script, strlen(script), file, 0);

   return file_mod_secs(file);
}

/* env is an object in the scripting environment;
   s is the function to call on that object
*/
void script_eval_w_env(const char *s, JSValue env) {
  JS_EvalThis(js, env, s, strlen(s), "internal", 0);
}

void script_call_sym(JSValue sym)
{
  JS_Call(js, sym, JS_GetGlobalObject(js), 0, NULL);
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

JSValue js_callee_exec(struct callee *c, int argc, JSValue *argv)
{
  JS_Call(js, c->obj, c->fn, argc, argv);
}

void call_callee(struct callee *c) {
  JS_Call(js, c->obj, c->fn, 0, NULL);
}

void callee_dbl(struct callee c, double d)
{
  JSValue v = num2js(d);
  js_callee_exec(&c, 1, &v);
}

void callee_int(struct callee c, int i)
{
  JSValue v = int2js(i);
  js_callee_exec(&c, 1, &v);
}

void callee_vec2(struct callee c, cpVect vec)
{
  JSValue v = vec2js(vec);
  js_callee_exec(&c, 1, &v);
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
