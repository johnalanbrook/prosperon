#include "script.h"
#include "jsffi.h"
#include "stb_ds.h"
#include <sokol/sokol_time.h>
#include <inttypes.h>
#include <limits.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdarg.h>
#include "jsffi.h"
#include <stdlib.h>
#include <assert.h>

#include <tracy/TracyC.h>

static JSContext *js = NULL;
static JSRuntime *rt = NULL;

#ifndef NDEBUG
#define JS_EVAL_FLAGS JS_EVAL_FLAG_STRICT
#else
#define JS_EVAL_FLAGS JS_EVAL_FLAG_STRICT | JS_EVAL_FLAG_STRIP 
#endif

static JSValue report_gc;

char* read_file(const char* filename) {
    // Open the file in read mode
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open file");
        return NULL;
    }

    // Seek to the end of the file to get its size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate memory for the file content, including the null terminator
    char *content = (char*)malloc(file_size + 1);
    if (!content) {
        perror("Failed to allocate memory");
        fclose(file);
        return NULL;
    }

    // Read the entire file into the content buffer
    fread(content, 1, file_size, file);

    // Null-terminate the string
    content[file_size] = '\0';

    fclose(file);
    return content;
}

void script_startup() {
  rt = JS_NewRuntime();
  js = JS_NewContextRaw(rt);
  JS_AddIntrinsicBaseObjects(js);
  JS_AddIntrinsicEval(js);
  JS_AddIntrinsicRegExp(js);
  JS_AddIntrinsicJSON(js);
  JS_AddIntrinsicMapSet(js);
  JS_AddIntrinsicTypedArrays(js);
  JS_AddIntrinsicPromise(js);
  JS_AddIntrinsicProxy(js);
  JS_AddIntrinsicBigInt(js);
  JS_AddIntrinsicBigFloat(js);
  JS_AddIntrinsicBigDecimal(js);
  JS_AddIntrinsicOperators(js);

  ffi_load(js);
  
  char *eng = read_file("core/scripts/engine.js");
  JSValue v = script_eval("core/scripts/engine.js", eng);
  JS_FreeValue(js, v);
  free(eng);
}

void script_stop()
{
  return;
  JS_FreeContext(js);
  js = NULL;
  JS_FreeRuntime(rt);
  rt = NULL;
}

void script_mem_limit(size_t limit) { JS_SetMemoryLimit(rt, limit); }
void script_gc_threshold(size_t threshold) { JS_SetGCThreshold(rt, threshold); }
void script_max_stacksize(size_t size) { JS_SetMaxStackSize(rt, size);  }

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
  js_print_exception(js,obj);
  JS_FreeValue(js,obj);
}

JSValue script_eval(const char *file, const char *script)
{
  JSValue v = JS_Eval(js, script, strlen(script), file, JS_EVAL_FLAGS);
  js_print_exception(js,v);
  return v;
}

void script_call_sym(JSValue sym, int argc, JSValue *argv) {
  if (!JS_IsFunction(js, sym)) return;
  JSValue ret = JS_Call(js, sym, JS_UNDEFINED, argc, argv);
  js_print_exception(js,ret);
  JS_FreeValue(js, ret);
}
