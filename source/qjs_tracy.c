#include <quickjs.h>
#include "glad.h"
#include <tracy/TracyC.h>
#include <string.h>
#include <stdlib.h>

static JSValue js_tracy_fiber_enter(JSContext *js, JSValue self, int argc, JSValue *argv)
{
#ifdef TRACY_ON_DEMAND
  if (!TracyCIsConnected) {
    JS_Call(js,argv[0], JS_UNDEFINED,0,NULL);
    return JS_UNDEFINED;
  }
#endif

  JSValue jsname = JS_GetPropertyStr(js, argv[0], "name");
  const char *fnname = JS_ToCString(js,jsname);
  TracyCFiberEnter(fnname);
  JS_Call(js, argv[0], JS_UNDEFINED, 0, NULL);
  TracyCFiberLeave(fnname);
  JS_FreeCString(js, fnname);
  JS_FreeValue(js,jsname);
}

static JSValue js_tracy_fiber_leave(JSContext *js, JSValue self, int argc, JSValue *argv)
{
#ifdef TRACY_ON_DEMAND
  if (!TracyCIsConnected)
    return JS_UNDEFINED;
#endif

  JSAtom atom = JS_ValueToAtom(js,argv[0]);
  const char *str = JS_AtomToCString(js, atom);
//  printf("EXIT STR AT %p\n", str);  
  TracyCFiberLeave(str);
  JS_FreeAtom(js,atom);
}

static JSValue js_tracy_plot(JSContext *js, JSValue self, int argc, JSValue *argv)
{
#ifdef TRACY_ON_DEMAND
  if (!TracyCIsConnected)
    return JS_UNDEFINED;
#endif

  const char *str = JS_ToCString(js, argv[0]);
  double n;
  JS_ToFloat64(js, &n, argv[1]);
  TracyCPlot(str, n);
  JS_FreeCString(js,str);
}

static JSValue js_tracy_plot_config(JSContext *js, JSValue self, int argc, JSValue *argv)
{
#ifdef TRACY_ON_DEMAND
  if (!TracyCIsConnected)
    return JS_UNDEFINED;
#endif

//  TracyCPlotConfig(str, js2number(js,argv[1]), JS_ToBool(js,argv[2]), JS_ToBool(js,argv[3]), js2number(js,argv[4]))
  return JS_UNDEFINED;
}

static JSValue js_tracy_frame_mark(JSContext *js, JSValue self, int argc, JSValue *argv)
{
#ifdef TRACY_ON_DEMAND
  if (!TracyCIsConnected)
    return JS_UNDEFINED;
#endif

  TracyCFrameMark
}

static JSValue js_tracy_message(JSContext *js, JSValue self, int argc, JSValue *argv)
{
#ifdef TRACY_ON_DEMAND
  if (!TracyCIsConnected)
    return JS_UNDEFINED;
#endif

  size_t len;
  const char *str = JS_ToCStringLen(js, &len, argv[0]);
  TracyCMessage(str,len);
  JS_FreeCString(js,str);
}

static JSValue js_tracy_thread_name(JSContext *js, JSValue self, int argc, JSValue *argv)
{
#ifdef TRACY_ON_DEMAND
  if (!TracyCIsConnected)
    return JS_UNDEFINED;
#endif

  const char *str = JS_ToCString(js, argv[0]);
  TracyCSetThreadName(str);
  JS_FreeCString(js,str);
}

static JSValue js_tracy_zone_begin(JSContext *js, JSValue self, int argc, JSValue *argv)
{
#ifdef TRACY_ON_DEMAND
  if (!TracyCIsConnected) {
    JS_Call(js,argv[0], JS_UNDEFINED,0,NULL);  
    return JS_UNDEFINED;
  }
#endif

#ifndef DEEP_TRACE
  const char *fn_src = JS_AtomToCString(js, js_fn_filename(js, argv[0]));
  const char *js_fn_name = get_func_name(js, argv[0]);
  const char *fn_name;
  if (!js_fn_name || js_fn_name[0] == 0)
    fn_name = "<anonymous>";
  else
    fn_name = js_fn_name;
  TracyCZoneCtx TCTX = ___tracy_emit_zone_begin_alloc(___tracy_alloc_srcloc(js_fn_linenum(js,argv[0]), fn_src, strlen(fn_src), fn_name, strlen(fn_name), (int)fn_src),1);
  JS_FreeCString(js,js_fn_name);
  
  JS_Call(js, argv[0], JS_UNDEFINED, 0, NULL);
  TracyCZoneEnd(TCTX);
#endif
}

GLuint *ids;
static GLsizei query_count = 64*1024;
static int qhead = 0;
static int qtail = 0;

static JSValue js_tracy_gpu_zone_begin(JSContext *js, JSValue self, int argc, JSValue *argv)
{
#ifdef TRACY_ON_DEMAND
  if (!TracyCIsConnected) {
    JS_Call(js,argv[0], JS_UNDEFINED, 0, NULL);
    return JS_UNDEFINED;
  }
#endif

//  glQueryCounter

  glQueryCounter(ids[qhead], GL_TIMESTAMP);
  struct ___tracy_gpu_zone_begin_data data;

  const char *fn_src = JS_AtomToCString(js, js_fn_filename(js, argv[0]));
  const char *js_fn_name = get_func_name(js, argv[0]);
  const char *fn_name;
  if (!js_fn_name || js_fn_name[0] == 0)
    fn_name = "<anonymous>";
  else
    fn_name = js_fn_name;
  data.srcloc = ___tracy_alloc_srcloc(js_fn_linenum(js,argv[0]), fn_src, strlen(fn_src), fn_name, strlen(fn_name), (int)fn_src);

  data.queryId = ids[qhead];
  data.context = 0;
  ___tracy_emit_gpu_zone_begin_alloc(data);
  JS_FreeCString(js,js_fn_name);  
  qhead = (qhead+1)%query_count;


  JS_Call(js, argv[0], JS_UNDEFINED, 0, NULL);

  glQueryCounter(ids[qhead], GL_TIMESTAMP);
  struct ___tracy_gpu_zone_end_data enddata = {
    .queryId = ids[qhead],
    .context = 0
  };
  ___tracy_emit_gpu_zone_end(enddata);
  qhead = (qhead+1)%query_count;

  
  return JS_UNDEFINED;
}

static JSValue js_tracy_gpu_zone_end(JSContext *js, JSValue self, int argc, JSValue *argv)
{
#ifdef TRACY_ON_DEMAND
  if (!TracyCIsConnected)
    return JS_UNDEFINED;
#endif

  glQueryCounter(ids[qhead], GL_TIMESTAMP);
  struct ___tracy_gpu_zone_end_data data = {
    .queryId = ids[qhead],
    .context = 0
  };
  ___tracy_emit_gpu_zone_end(data);
  qhead = (qhead+1)%query_count;
  
  return JS_UNDEFINED;
}

static JSValue js_tracy_gpu_init(JSContext *js, JSValue self, int argc, JSValue *argv)
{
  ids = malloc(sizeof(GLuint)*query_count);
  glGenQueries(query_count, ids); // generate new query ids
  int64_t tgpu;
  glGetInteger64v(GL_TIMESTAMP, &tgpu);

  float period = 1.f;
  struct ___tracy_gpu_new_context_data gpuctx = {
    .gpuTime = tgpu,
    .period = period,
    .context = 0,
    .flags = 0,
    .type = 1
  };
  ___tracy_emit_gpu_new_context(gpuctx);
  
  return JS_UNDEFINED;
}

static JSValue js_tracy_gpu_sync(JSContext *js, JSValue self, int argc, JSValue *argv)
{
#ifdef TRACY_ON_DEMAND
  if (!TracyCIsConnected)
    return JS_UNDEFINED;
#endif

  int64_t tgpu;
  glGetInteger64v(GL_TIMESTAMP, &tgpu);
  
  struct ___tracy_gpu_time_sync_data data = {
    .context = 0,
    .gpuTime = tgpu
  };
  ___tracy_emit_gpu_time_sync(data);
  return JS_UNDEFINED;
}

static JSValue js_tracy_gpu_collect(JSContext *js, JSValue self, int argc, JSValue *argv)
{
  GLint available;
  while (qtail != qhead) {
    glGetQueryObjectiv(ids[qtail], GL_QUERY_RESULT_AVAILABLE, &available);
    if (!available) return JS_UNDEFINED;

    uint64_t time;
    glGetQueryObjectui64v(ids[qtail], GL_QUERY_RESULT, &time);
    struct ___tracy_gpu_time_data gtime = {
      .gpuTime = time,
      .queryId = ids[qtail],
      .context = 0
    };
    qtail = (qtail+1)%query_count;
    ___tracy_emit_gpu_time(gtime);
  }
  return JS_UNDEFINED;
}

static const JSCFunctionListEntry js_tracy_funcs[] = {
  JS_CFUNC_DEF("fiber", 1, js_tracy_fiber_enter),
  JS_CFUNC_DEF("fiber_leave", 1, js_tracy_fiber_leave),
  JS_CFUNC_DEF("gpu_zone", 1, js_tracy_gpu_zone_begin),
  JS_CFUNC_DEF("gpu_collect", 0, js_tracy_gpu_collect),
  JS_CFUNC_DEF("gpu_init", 0, js_tracy_gpu_init),
  JS_CFUNC_DEF("end_frame", 0, js_tracy_frame_mark),
  JS_CFUNC_DEF("zone", 1, js_tracy_zone_begin),
  JS_CFUNC_DEF("message", 1, js_tracy_message),
  JS_CFUNC_DEF("plot", 2, js_tracy_plot),
};

JSValue js_tracy_use(JSContext *js)
{
  JSValue expy = JS_NewObject(js);
  JS_SetPropertyFunctionList(js, expy, js_tracy_funcs, sizeof(js_tracy_funcs)/sizeof(JSCFunctionListEntry));
  return expy;
}

static int js_tracy_init(JSContext *js, JSModuleDef *m) {
  js_tracy_use(js);
  JS_SetModuleExportList(js, m, js_tracy_funcs, sizeof(js_tracy_funcs)/sizeof(JSCFunctionListEntry));
  return 0;
}

#ifdef JS_SHARED_LIBRARY
#define JS_INIT_MODULE js_init_module
#else
#define JS_INIT_MODULE js_init_module_tracy
#endif

JSModuleDef *JS_INIT_MODULE(JSContext *ctx, const char *module_name) {
  JSModuleDef *m = JS_NewCModule(ctx, module_name, js_tracy_init);
  if (!m) return NULL;
  JS_AddModuleExportList(ctx, m, js_tracy_funcs, sizeof(js_tracy_funcs)/sizeof(JSCFunctionListEntry));
  return m;
}
