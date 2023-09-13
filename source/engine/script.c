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
#include "resources.h"

#include <stdarg.h>

JSContext *js = NULL;
JSRuntime *rt = NULL;

#ifndef NDEBUG
#define JS_EVAL_FLAGS JS_EVAL_FLAG_STRICT
#else
#define JS_EVAL_FLAGS JS_EVAL_FLAG_STRICT | JS_EVAL_FLAG_STRIP 
#endif

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

  for (int i = 0; i < 100; i++)
    num_cache[i] = int2js(i);
}

JSValue num_cache[100] = {0};

int js_print_exception(JSValue v) {
#ifndef NDEBUG
  if (JS_IsException(v)) {
    JSValue exception = JS_GetException(js);
    
    /* TODO: Does it need freed if null? */
    if (JS_IsNull(exception))
      return 0;
      
    JSValue val = JS_GetPropertyStr(js, exception, "stack");
    const char *name = JS_ToCString(js, JS_GetPropertyStr(js, exception, "name"));
    const char *msg = JS_ToCString(js, JS_GetPropertyStr(js, exception, "message"));
    const char *stack = JS_ToCString(js, val);
    YughLog(LOG_SCRIPT, LOG_ERROR, "%s :: %s\n%s", name, msg,stack);

    JS_FreeCString(js, name);
    JS_FreeCString(js, msg);
    JS_FreeCString(js, stack);
    JS_FreeValue(js,val);
    JS_FreeValue(js,exception);

    return 1;
  }
#endif
  return 0;
}

void script_run(const char *script, const char *file) {
  JSValue obj = JS_Eval(js, script, strlen(script), file, JS_EVAL_FLAGS);
  js_print_exception(obj);
  JS_FreeValue(js,obj);
}

void script_evalf(const char *format, ...)
{
  char fmtbuf[4096];
  va_list args;
  va_start(args, format);
  vsnprintf(fmtbuf, 4096, format, args);
  va_end(args);

  JSValue obj = JS_Eval(js, fmtbuf, strlen(fmtbuf), "C eval", JS_EVAL_FLAGS);
  js_print_exception(obj);
  JS_FreeValue(js,obj);
}

uint8_t *compile_script(const char *file) {
  size_t len;
  const char *script = slurp_text(file, &len);
  JSValue obj = JS_Eval(js, script, len, file, JS_EVAL_FLAG_COMPILE_ONLY | JS_EVAL_TYPE_GLOBAL | JS_EVAL_FLAGS);
  size_t out_len;
  uint8_t *out;
  return JS_WriteObject(js, &out_len, obj, JS_WRITE_OBJ_BYTECODE);
}

struct callee stacktrace_callee;

time_t file_mod_secs(const char *file) {
  struct stat attr;
  stat(file, &attr);
  return attr.st_mtime;
}

void js_stacktrace() {
#ifndef NDEBUG
  call_callee(&stacktrace_callee);
#endif
}

void js_dump_stack() {
  js_stacktrace();
}

int script_dofile(const char *file) {
  script_runfile(file);
  return file_mod_secs(file);
}

JSValue script_runfile(const char *file)
{
  size_t len;
  const char *script = slurp_text(file, &len);
  if (!script) return JS_NULL;

  JSValue obj = JS_Eval(js, script, len, file, JS_EVAL_FLAGS);
  js_print_exception(obj);
  
  free(script);
  return obj;
}

/* env is an object in the scripting environment;
   s is the function to call on that object
*/
void script_eval_w_env(const char *s, JSValue env, const char *file) {
  JSValue v = JS_EvalThis(js, env, s, strlen(s), file, JS_EVAL_FLAGS);
  js_print_exception(v);
  JS_FreeValue(js, v);
}

void file_eval_env(const char *file, JSValue env)
{
  size_t len;
  char *script = slurp_text(file, &len);
  JSValue v = JS_EvalThis(js, env, script, len, file, JS_EVAL_FLAGS);
  free(script);
  js_print_exception(v);
  JS_FreeValue(js,v);
}

void script_call_sym(JSValue sym) {
  struct callee c;
  c.fn = sym;
  c.obj = JS_GetGlobalObject(js);
  call_callee(&c);
}

JSValue js_callee_exec(struct callee *c, int argc, JSValue *argv)
{
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

void send_signal(const char *signal, int argc, JSValue *argv)
{
  JSValue globalThis = JS_GetGlobalObject(js);
  JSValue sig = JS_GetPropertyStr(js, globalThis, "Signal");
  JSValue fn = JS_GetPropertyStr(js, sig, "call");
  JSValue args[argc+1];
  args[0] = str2js(signal);
  for (int i = 0; i < argc; i++)
    args[1+i] = argv[i];
    
  JS_Call(js, fn, sig, argc+1, args);
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
void call_debugs() { call_callee(&debug_callee); }

static struct callee draw_callee;
void register_draw(struct callee c) { draw_callee = c; }

void call_draw() { call_callee(&draw_callee); }
