#include "script.h"
#include "log.h"
#include "jsffi.h"
#include "stb_ds.h"
#include "resources.h"

#include <stdarg.h>

JSContext *js = NULL;
JSRuntime *rt = NULL;

#ifndef NDEBUG
#define JS_EVAL_FLAGS JS_EVAL_FLAG_STRICT
#else
#define JS_EVAL_FLAGS JS_EVAL_FLAG_STRICT | JS_EVAL_FLAG_STRIP 
#endif

void script_startup() {
  rt = JS_NewRuntime();
  js = JS_NewContext(rt);

  ffi_load();
    
  size_t len;
  char *eng = slurp_text("scripts/engine.js", &len);
  eval_script_env("scripts/engine.js", JS_GetGlobalObject(js), eng);
  free(eng);
}

void script_stop()
{
  script_evalf("prosperon.quit();");

#if LEAK
  JS_FreeContext(js);
  JS_FreeRuntime(rt);
#endif
}

void script_gc() { JS_RunGC(rt); }

uint8_t *script_compile(const char *file, size_t *len) {
  size_t file_len;
  char *script = slurp_text(file, &file_len);
  JSValue obj = JS_Eval(js, script, file_len, file, JS_EVAL_FLAG_COMPILE_ONLY | JS_EVAL_TYPE_GLOBAL | JS_EVAL_FLAGS);
  free(script);
  uint8_t *out = JS_WriteObject(js, len, obj, JS_WRITE_OBJ_BYTECODE);
  JS_FreeValue(js,obj);
  return out;
}

JSValue script_run_bytecode(uint8_t *code, size_t len)
{
  JSValue b = JS_ReadObject(js, code, len, JS_READ_OBJ_BYTECODE);
  JSValue ret = JS_EvalFunction(js, b);
  js_print_exception(ret);
  JS_FreeValue(js,b);
  return ret;
}

void js_stacktrace() {
#ifndef NDEBUG
  script_evalf("console.stack();");
#endif
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

JSValue eval_script_env(const char *file, JSValue env, const char *script)
{
  JSValue v = JS_EvalThis(js, env, script, strlen(script), file, JS_EVAL_FLAGS);
  js_print_exception(v);
  return v;
}

void script_call_sym(JSValue sym, int argc, JSValue *argv) {
  if (!JS_IsFunction(js, sym)) return;
  JSValue ret = JS_Call(js, sym, JS_GetGlobalObject(js), argc, argv);
  js_print_exception(ret);
  JS_FreeValue(js, ret);
}

void out_memusage(const char *file)
{
  FILE *f = fopen(file, "w");
  if (!f) return;
  JSMemoryUsage jsmem;
  JS_ComputeMemoryUsage(rt, &jsmem);
  JS_DumpMemoryUsage(f, &jsmem, rt);
  fclose(f);
}
