#include "script.h"
#include "log.h"
#include "jsffi.h"
#include "stb_ds.h"
#include "resources.h"
#include <sokol/sokol_time.h>
#include <inttypes.h>
#include <limits.h>
#include <sys/stat.h>
#include <errno.h>

#include <stdarg.h>

JSContext *js = NULL;
JSRuntime *rt = NULL;

#ifndef NDEBUG
#define JS_EVAL_FLAGS JS_EVAL_FLAG_STRICT
#else
#define JS_EVAL_FLAGS JS_EVAL_FLAG_STRICT | JS_EVAL_FLAG_STRIP 
#endif

static JSValue report_gc;

static uint8_t *js_load_file(JSContext *ctx, size_t *pbuf_len, const char *filename)
{
    FILE *f;
    uint8_t *buf;
    size_t buf_len;
    long lret;

    f = fopen(filename, "rb");
    if (!f)
        return NULL;
    if (fseek(f, 0, SEEK_END) < 0)
        goto fail;
    lret = ftell(f);
    if (lret < 0)
        goto fail;
    /* XXX: on Linux, ftell() return LONG_MAX for directories */
    if (lret == LONG_MAX) {
        errno = EISDIR;
        goto fail;
    }
    buf_len = lret;
    if (fseek(f, 0, SEEK_SET) < 0)
        goto fail;
    if (ctx)
        buf = js_malloc(ctx, buf_len + 1);
    else
        buf = malloc(buf_len + 1);
    if (!buf)
        goto fail;
    if (fread(buf, 1, buf_len, f) != buf_len) {
        errno = EIO;
        if (ctx)
            js_free(ctx, buf);
        else
            free(buf);
    fail:
        fclose(f);
        return NULL;
    }
    buf[buf_len] = '\0';
    fclose(f);
    *pbuf_len = buf_len;
    return buf;
}

static JSModuleDef *js_module_loader(JSContext *ctx,
                              const char *module_name, void *opaque)
{
    JSModuleDef *m;
        size_t buf_len;
        uint8_t *buf;
        JSValue func_val;

        buf = js_load_file(ctx, &buf_len, module_name);
        if (!buf) {
            JS_ThrowReferenceError(ctx, "could not load module filename '%s'",
                                   module_name);
            return NULL;
        }

        /* compile the module */
        func_val = JS_Eval(ctx, (char *)buf, buf_len, module_name,
                           JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
        js_free(ctx, buf);
        if (JS_IsException(func_val))
            return NULL;
        /* XXX: could propagate the exception */
        js_module_set_import_meta(ctx, func_val, 1, 0);
        /* the module is already referenced, so we must free it */
        m = JS_VALUE_GET_PTR(func_val);
        JS_FreeValue(ctx, func_val);
    return m;
}

void script_startup() {
  rt = JS_NewRuntime();
  js = JS_NewContext(rt);

  JS_SetModuleLoaderFunc(rt, NULL, js_module_loader, NULL);

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
void script_mem_limit(size_t limit) { JS_SetMemoryLimit(rt, limit); }
void script_gc_threshold(size_t threshold) { JS_SetGCThreshold(rt, threshold); }
void script_max_stacksize(size_t size) { JS_SetMaxStackSize(rt, size);  }

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

JSValue script_call_sym_ret(JSValue sym, int argc, JSValue *argv) {
  if (!JS_IsFunction(js, sym)) return JS_UNDEFINED;
  JSValue ret = JS_Call(js, sym, JS_UNDEFINED, argc, argv);
  return ret;
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
