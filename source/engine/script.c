#include "script.h"

#include "log.h"
#include "stdio.h"

#include "jsffi.h"
#include "font.h"

#include "gameobject.h"

#include "ftw.h"

#include "stb_ds.h"

#include "sys/stat.h"
#include "sys/types.h"
#include "time.h"
#include "resources.h"
#include "input.h"

#include <stdarg.h>

JSContext *js = NULL;
JSRuntime *rt = NULL;

#ifndef NDEBUG
#define JS_EVAL_FLAGS JS_EVAL_FLAG_STRICT
#else
#define JS_EVAL_FLAGS JS_EVAL_FLAG_STRICT// | JS_EVAL_FLAG_STRIP 
#endif

static struct {
  char *key;
  JSValue value;
} *jsstrs = NULL;

JSValue jstr(const char *str)
{
  int index = shgeti(jsstrs, str);
  if (index != -1) return jsstrs[index].value;

  JSValue v = str2js(str);
  shput(jsstrs, str, v);
  return v;
}

static int load_prefab(const char *fpath, const struct stat *sb, int typeflag) {
  if (typeflag != FTW_F)
    return 0;

  if (!strcmp(".prefab", strrchr(fpath, '.')))
    script_dofile(fpath);

  return 0;
}

void script_startup() {
  rt = JS_NewRuntime();
  js = JS_NewContext(rt);
  
  sh_new_arena(jsstrs);  

  ffi_load();

  for (int i = 0; i < 100; i++)
    num_cache[i] = int2js(i);
    
  script_dofile("scripts/engine.js");  
//  jso_file("scripts/engine.js");
}

void script_stop()
{
  timers_free();
  script_evalf("Event.notify('quit');");
  send_signal("quit",0,NULL);
  
  for (int i = 0; i < shlen(jsstrs); i++)
    JS_FreeValue(js,jsstrs[i].value);
    
  JS_FreeContext(js);
  JS_FreeRuntime(rt);
}

void script_gc()
{
  JS_RunGC(rt);
}

JSValue num_cache[100] = {0};

/*int js_print_exception(JSValue v) {
#ifndef NDEBUG
  if (JS_IsException(v)) {
    JSValue exception = JS_GetException(js);
    
    if (JS_IsNull(exception)) {
      JS_FreeValue(js,exception);
      return 0;
    }
      
    JSValue val = JS_ToCStringJS_GetPropertyStr(js, exception, "stack");
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
*/
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

uint8_t *compile_script(const char *file, size_t *len) {
  char *script = slurp_text(file, len);
  JSValue obj = JS_Eval(js, script, *len, file, JS_EVAL_FLAG_COMPILE_ONLY | JS_EVAL_TYPE_GLOBAL | JS_EVAL_FLAGS);
  free(script);
  size_t out_len;
  uint8_t *out = JS_WriteObject(js, &out_len, obj, JS_WRITE_OBJ_BYTECODE);
  JS_FreeValue(js,obj);
  return out;
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
  JSValue ret = script_runfile(file);
  JS_FreeValue(js,ret);
  return file_mod_secs(file);
}

JSValue script_runjso(const uint8_t *buf, size_t len)
{
  JSValue obj = JS_ReadObject(js, buf, len, JS_EVAL_FLAGS);
  JSValue ret = JS_EvalFunction(js, obj);
  js_print_exception(ret);
  return ret;
}

time_t jso_file(const char *file)
{
  size_t len;
  uint8_t *byte = slurp_file(file, &len);
  JSValue obj = JS_ReadObject(js, byte, len, JS_READ_OBJ_BYTECODE);
  JSValue ret = JS_EvalFunction(js, obj);
  js_print_exception(ret);
  JS_FreeValue(js,ret);
  free(byte);
  return file_mod_secs(file);
}

JSValue script_runfile(const char *file)
{
  size_t len;
  const char *script = slurp_text(file, &len);
  if (!script) return JS_UNDEFINED;

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

struct callenv {
  JSValue v;
  const char *eval;
};

struct callenv *calls;

void call_stack()
{
  for (int i = 0; i < arrlen(calls); i++) {
    JSValue v = JS_EvalThis(js, calls[i].v, calls[i].eval, strlen(calls[i].eval), calls[i].eval, JS_EVAL_FLAGS);
    js_print_exception(v);
    JS_FreeValue(js, v);
 }
  arrfree(calls);
}

void call_env(JSValue env, const char *eval)
{
  if (!JS_IsObject(env)) { YughWarn("NOT AN ENV"); return; };
  struct callenv c;
  c.v = env;
  c.eval = eval;
  arrpush(calls, c);
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
  if (!JS_IsFunction(js, sym)) return;
  struct callee c;
  c.fn = sym;
  c.obj = JS_GetGlobalObject(js);
  call_callee(&c);
}

void out_memusage(const char *file)
{
  FILE *f = fopen(file, "w");
  JSMemoryUsage jsmem;
  JS_ComputeMemoryUsage(rt, &jsmem);
  JS_DumpMemoryUsage(f, &jsmem, rt);
  fclose(f);
}

JSValue js_callee_exec(struct callee *c, int argc, JSValue *argv)
{
  if (JS_IsUndefined(c->fn)) return JS_UNDEFINED;
  if (JS_IsUndefined(c->obj)) return JS_UNDEFINED;
  
  JSValue ret = JS_Call(js, c->fn, c->obj, argc, argv);
  js_print_exception(ret);
  JS_FreeValue(js, ret);
  return JS_UNDEFINED;
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

void callee_vec2(struct callee c, HMM_Vec2 vec) {
  JSValue v = vec2js(vec);
  js_callee_exec(&c, 1, &v);
  JS_FreeValue(js, v);
}

void script_callee(struct callee c, int argc, JSValue *argv) {
  js_callee_exec(&c, argc, argv);
}

struct callee make_callee(JSValue fn, JSValue obj)
{
  struct callee c;
  c.fn = JS_DupValue(js, fn);
  c.obj = JS_DupValue(js, obj);
  return c;  
}

void free_callee(struct callee c)
{
  JS_FreeValue(js,c.fn);
  JS_FreeValue(js,c.obj);
}

void send_signal(const char *signal, int argc, JSValue *argv)
{
  JSValue globalThis = JS_GetGlobalObject(js);
  JSValue sig = JS_GetPropertyStr(js, globalThis, "Signal");
  JS_FreeValue(js, globalThis);
  JSValue fn = JS_GetPropertyStr(js, sig, "call");
  JSValue args[argc+1];
  args[0] = jstr(signal);
  for (int i = 0; i < argc; i++)
    args[1+i] = argv[i];
    
  JS_FreeValue(js,JS_Call(js, fn, sig, argc+1, args));
  JS_FreeValue(js, sig);
  JS_FreeValue(js, fn);
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
void call_physics(double dt) {
  callee_dbl(physupdate_callee, dt);
}

struct callee debug_callee;
void register_debug(struct callee c) { debug_callee = c; }
void call_debugs() { call_callee(&debug_callee); }

static struct callee draw_callee;
void register_draw(struct callee c) { draw_callee = c; }

void call_draw() { call_callee(&draw_callee); }
