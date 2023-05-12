#include "script.h"

#include "log.h"
#include "stdio.h"

#include "ffi.h"
#include "font.h"

#include "ftw.h"

#include "stb_ds.h"

#include "sys/stat.h"
#include "sys/types.h"
#include "time.h"

JSContext *js = NULL;
JSRuntime *rt = NULL;

static int load_prefab(const char *fpath, const struct stat *sb, int typeflag) {
  if (typeflag != FTW_F)
    return 0;

  if (!strcmp(".prefab", strrchr(fpath, '.')))
    script_dofile(fpath);

  return 0;
}

void script_startup() {
  rt = JS_NewRuntime();
  JS_SetMaxStackSize(rt, 0);
  js = JS_NewContext(rt);
  ffi_load();
}

JSValue num_cache[100] = {0};

void script_init() {
  /* Load all prefabs into memory */
  script_dofile("scripts/engine.js");

  for (int i = 0; i < 100; i++)
    num_cache[i] = int2js(i);
}

void script_run(const char *script) {
  JS_FreeValue(js, JS_Eval(js, script, strlen(script), "script", 0));
}

struct callee stacktrace_callee;

time_t file_mod_secs(const char *file) {
  struct stat attr;
  stat(file, &attr);
  return attr.st_mtime;
}

void js_stacktrace() {
  call_callee(&stacktrace_callee);
  return;
}

void js_dump_stack() {
  js_stacktrace();
  return;

  JSValue exception = JS_GetException(js);
  if (JS_IsNull(exception)) return;
  JSValue val = JS_GetPropertyStr(js, exception, "stack");
  if (!JS_IsUndefined(val)) {
    const char *name = JS_ToCString(js, JS_GetPropertyStr(js, exception, "name"));
    const char *msg = JS_ToCString(js, JS_GetPropertyStr(js, exception, "message"));
    const char *stack = JS_ToCString(js, val);
    YughError("%s :: %s\n%s", name, msg, stack);

    JS_FreeCString(js, name);
    JS_FreeCString(js, msg);
    JS_FreeCString(js, stack);
  }
}

int js_print_exception(JSValue v) {
  if (JS_IsException(v)) {
    JSValue exception = JS_GetException(js);
    JSValue val = JS_GetPropertyStr(js, exception, "stack");
    if (!JS_IsUndefined(val)) {
      const char *name = JS_ToCString(js, JS_GetPropertyStr(js, exception, "name"));
      const char *msg = JS_ToCString(js, JS_GetPropertyStr(js, exception, "message"));
      const char *stack = JS_ToCString(js, val);
      YughWarn("%s :: %s\n%s", name, msg, stack);

      JS_FreeCString(js, name);
      JS_FreeCString(js, msg);
      JS_FreeCString(js, stack);
    }

    return 1;
  }

  return 0;
}

int script_dofile(const char *file) {
  YughInfo("Doing script %s", file);
  const char *script = slurp_text(file);
  if (!script) {
    YughError("Can't find file %s.", file);
    return 0;
  }
  JSValue obj = JS_Eval(js, script, strlen(script), file, 0);
  js_print_exception(obj);
  JS_FreeValue(js, obj);

  return file_mod_secs(file);
}

/* env is an object in the scripting environment;
   s is the function to call on that object
*/
void script_eval_w_env(const char *s, JSValue env) {
  JSValue v = JS_EvalThis(js, env, s, strlen(s), "internal", 0);
  js_print_exception(v);
  JS_FreeValue(js, v);
}

void script_call_sym(JSValue sym) {
  struct callee c;
  c.fn = sym;
  c.obj = JS_GetGlobalObject(js);
  call_callee(&c);
}

JSValue js_callee_exec(struct callee *c, int argc, JSValue *argv) {
  JSValue ret = JS_Call(js, c->fn, c->obj, argc, argv);
  js_print_exception(ret);
  JS_FreeValue(js, ret);
  return JS_NULL;
}

void call_callee(struct callee *c) {
  js_callee_exec(c, 0, NULL);
}

void callee_dbl(struct callee c, double d) {
  JSValue v = num2js(d);
  js_callee_exec(&c, 1, &v);
  JS_FreeValue(js, v);
}

void callee_int(struct callee c, int i) {
  JSValue v = int2js(i);
  js_callee_exec(&c, 1, &v);
  JS_FreeValue(js, v);
}

void callee_vec2(struct callee c, cpVect vec) {
  JSValue v = vec2js(vec);
  js_callee_exec(&c, 1, &v);
  JS_FreeValue(js, v);
}

void script_callee(struct callee c, int argc, JSValue *argv) {
  js_callee_exec(&c, argc, argv);
}

static struct callee update_callee;
void register_update(struct callee c) {
  update_callee = c;
}
void call_updates(double dt) {
  callee_dbl(update_callee, dt);
}

static struct callee gui_callee;
void register_gui(struct callee c) { gui_callee = c; }
void call_gui() { js_callee_exec(&gui_callee, 0, NULL); }

static struct callee nk_gui_callee;
void register_nk_gui(struct callee c) { nk_gui_callee = c; }
void call_nk_gui() { js_callee_exec(&nk_gui_callee, 0, NULL); }

static struct callee physupdate_callee;
void register_physics(struct callee c) { physupdate_callee = c; }
void call_physics(double dt) { callee_dbl(physupdate_callee, dt); }

struct callee debug_callee;
void register_debug(struct callee c) { debug_callee = c; }
void call_debugs() { JS_Call(js, debug_callee.fn, debug_callee.obj, 0, NULL); }
