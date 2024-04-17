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
  JSValue v = script_eval("scripts/engine.js", eng);
  JS_FreeValue(js, v);
  free(eng);
}
static int stopped = 0;
void script_stop()
{
  script_evalf("prosperon.quit();");
#ifndef LEAK
return;
#endif

  script_gc();
  JS_FreeContext(js);
  js = NULL;
  JS_FreeRuntime(rt);
  rt = NULL;
}

void script_gc() { JS_RunGC(rt); }

void js_stacktrace() {
  if (!js) return;
#ifndef NDEBUG
  script_evalf("console.stack();");
#endif
}

void script_evalf(const char *format, ...)
{
  JSValue obj;
  va_list args;
  va_start(args, format);
  int len = vsnprintf(NULL, 0, format, args);
  va_end(args);
  
  char *eval = malloc(len+1);
  va_start(args, format);
  vsnprintf(eval, len+1, format, args);
  va_end(args);

  obj = JS_Eval(js, eval, len, "C eval", JS_EVAL_FLAGS);
  free(eval);
  js_print_exception(obj);
  JS_FreeValue(js,obj);
}

JSValue script_eval(const char *file, const char *script)
{
  JSValue v = JS_Eval(js, script, strlen(script), file, JS_EVAL_FLAGS);
  js_print_exception(v);
  return v;
}

void script_call_sym(JSValue sym, int argc, JSValue *argv) {
  if (!JS_IsFunction(js, sym)) return;
  JSValue ret = JS_Call(js, sym, JS_UNDEFINED, argc, argv);
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
