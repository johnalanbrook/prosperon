#include <quickjs.h>
#include <tracy/TracyC.h>
#include <string.h>
#include <stdlib.h>
#include "render.h"
#include <SDL3/SDL.h>

static JSValue js_tracy_fiber_enter(JSContext *js, JSValue self, int argc, JSValue *argv)
{
  return JS_UNDEFINED;
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
  return JS_UNDEFINED;
#ifdef TRACY_ON_DEMAND
  if (!TracyCIsConnected)
    return JS_UNDEFINED;
#endif

  JSAtom atom = JS_ValueToAtom(js,argv[0]);
  const char *str = JS_AtomToCString(js, atom);
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
}

#ifdef SOKOL_GLCORE
#include "glad.h"
GLuint *ids;
static GLsizei query_count = 64*1024;
static int qhead = 0;
static int qtail = 0;

static JSValue js_tracy_gpu_init(JSContext *js, JSValue self, int argc, JSValue *argv)
{
  printf("GLAD LOAD %d\n", gladLoadGL());
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

static JSValue js_tracy_gpu_zone_begin(JSContext *js, JSValue self, int argc, JSValue *argv)
{
#ifdef TRACY_ON_DEMAND
  if (!TracyCIsConnected) {
    JS_Call(js,argv[0], JS_UNDEFINED, 0, NULL);
    return JS_UNDEFINED;
  }
#endif

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
  JS_FreeCString(js, fn_src);
  
  qhead = (qhead+1)%query_count;

  JSValue ret = JS_Call(js, argv[0], JS_UNDEFINED, 0, NULL);

  glQueryCounter(ids[qhead], GL_TIMESTAMP);
  struct ___tracy_gpu_zone_end_data enddata = {
    .queryId = ids[qhead],
    .context = 0
  };
  ___tracy_emit_gpu_zone_end(enddata);
  qhead = (qhead+1)%query_count;

  
  return ret;
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
#ifdef TRACY_ON_DEMAND
  if (!TracyCIsConnected)
    return JS_UNDEFINED;
#endif

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

static JSValue js_tracy_image(JSContext *js, JSValue self, int argc, JSValue *argv)
{
  size_t len;
  double width, height;
  JS_ToFloat64(js,&width,argv[1]);
  JS_ToFloat64(js,&height,argv[2]);
  ___tracy_emit_frame_image(JS_GetArrayBuffer(js, &len, argv[0]), width, height, 0, 0);
  return JS_UNDEFINED;
}

#elifdef SOKOL_D3D11
#include <d3d11.h>
static int query_count = 64*1024;

ID3D11Device *device = NULL;
ID3D11DeviceContext *context = NULL;
ID3D11RenderTargetView *render_view = NULL;
ID3D11Query *disjoint_query = NULL;
ID3D11Query **queries;

int qtail = 0;
int qhead = 1;

static JSValue js_tracy_gpu_init(JSContext *js, JSValue self, int argc, JSValue *argv)
{
  HRESULT hr;
  queries = malloc(sizeof(ID3D11Query*) * query_count);
  device = sapp_d3d11_get_device();
  context = sapp_d3d11_get_device_context();
  render_view = sapp_d3d11_get_render_view();
  
  D3D11_QUERY_DESC disjoint_desc = {0};
  disjoint_desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;

  if (FAILED(hr = device->lpVtbl->CreateQuery(device, &disjoint_desc, &disjoint_query)))
    return JS_ThrowReferenceError(js,"Failed to create disjoint query, HRESULT: 0x%X\n", hr);

  D3D11_QUERY_DESC timestamp_desc = {0};
  timestamp_desc.Query = D3D11_QUERY_TIMESTAMP;
  
  for (int i = 0; i < query_count; ++i) 
    if (FAILED(hr = device->lpVtbl->CreateQuery(device, &timestamp_desc, &queries[i])))
      return JS_ThrowReferenceError(js,"Failed to create query %d, HRESULT: 0x%X\n", i, hr);

  UINT64 tgpu = 0;
  D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjoint_data;

  context->lpVtbl->Begin(context, disjoint_query);
  context->lpVtbl->End(context, queries[0]);
  context->lpVtbl->End(context, disjoint_query);
  while (context->lpVtbl->GetData(context, disjoint_query, &disjoint_data, sizeof(disjoint_data), 0) != S_OK);
  while (context->lpVtbl->GetData(context, queries[0], &tgpu, sizeof(tgpu), 0) != S_OK);

  struct ___tracy_gpu_new_context_data gpuctx = {
    .gpuTime = tgpu * (1000000000 / disjoint_data.Frequency),
    .period = 1.0f,
    .context = 0,
    .flags = 0,
    .type = 2
  };
  ___tracy_emit_gpu_new_context(gpuctx);

  context->lpVtbl->Begin(context, disjoint_query);
  qtail = qhead = 0;
  
  return JS_UNDEFINED;
}

static JSValue js_tracy_gpu_zone_begin(JSContext *js, JSValue self, int argc, JSValue *argv)
{
#ifdef TRACY_ON_DEMAND
  if (!TracyCIsConnected) {
    JS_Call(js,argv[0], JS_UNDEFINED, 0, NULL);
    return JS_UNDEFINED;
  }
#endif

  context->lpVtbl->End(context, queries[qhead]);
  struct ___tracy_gpu_zone_begin_data data;

  const char *fn_src = JS_AtomToCString(js, js_fn_filename(js, argv[0]));
  const char *js_fn_name = get_func_name(js, argv[0]);
  const char *fn_name;
  if (!js_fn_name || js_fn_name[0] == 0)
    fn_name = "<anonymous>";
  else
    fn_name = js_fn_name;
  data.srcloc = ___tracy_alloc_srcloc(js_fn_linenum(js,argv[0]), fn_src, strlen(fn_src), fn_name, strlen(fn_name), (int)fn_src);

  data.queryId = qhead;
  data.context = 0;
  ___tracy_emit_gpu_zone_begin_alloc(data);
  JS_FreeCString(js,js_fn_name);
  JS_FreeCString(js, fn_src);
  qhead = (qhead+1)%query_count;

  JSValue ret = JS_Call(js, argv[0], JS_UNDEFINED, 0, NULL);
  context->lpVtbl->End(context, queries[qhead]);
  struct ___tracy_gpu_zone_end_data enddata = {
    .queryId = qhead,
    .context = 0
  };
  ___tracy_emit_gpu_zone_end(enddata);
  qhead = (qhead+1)%query_count;
  
  return ret;
}

static JSValue js_tracy_gpu_sync(JSContext *js, JSValue self, int argc, JSValue *argv)
{
#ifdef TRACY_ON_DEMAND
  if (!TracyCIsConnected)
    return JS_UNDEFINED;
#endif

  return JS_UNDEFINED;
}

static JSValue js_tracy_gpu_collect(JSContext *js, JSValue self, int argc, JSValue *argv)
{
#ifdef TRACY_ON_DEMAND
  if (!TracyCIsConnected)
    return JS_UNDEFINED;
#endif
  HRESULT hr;
  // Use the current head to get a sync value
  context->lpVtbl->End(context, queries[qhead]);
  context->lpVtbl->End(context, disjoint_query);

  D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjoint_data;
  while(context->lpVtbl->GetData(context, disjoint_query, &disjoint_data, sizeof(disjoint_data), 0) != S_OK);

  if (disjoint_data.Disjoint) {
    qtail = qhead;
    return JS_ThrowReferenceError(js,"disjoint timestamps detected; dropping.");
  }

/*  uint64_t synctime = 0;
  while (context->lpVtbl->GetData(context, queries[qhead], &synctime, sizeof(synctime), 0) != S_OK);
  ___tracy_emit_gpu_time_sync((struct ___tracy_gpu_time_sync_data) {
    .context = 0,
    .gpuTime = synctime * 1000000000 / disjoint_data.Frequency
  });
*/

  while (qtail != qhead) {
    uint64_t time = 0;
    while(context->lpVtbl->GetData(context, queries[qtail], &time, sizeof(time), 0) != S_OK);
    
    struct ___tracy_gpu_time_data gtime = {
      .gpuTime = time * (1000000000 / disjoint_data.Frequency),
      .queryId = qtail,
      .context = 0
    };
    qtail = (qtail+1)%query_count;
    ___tracy_emit_gpu_time(gtime);    
  }

  qtail = qhead;
  context->lpVtbl->Begin(context, disjoint_query);

  return JS_UNDEFINED;
}

#include <stb_image_resize2.h>
#include <stb_image_write.h>

static JSValue js_tracy_image(JSContext *js, JSValue self, int argc, JSValue *argv)
{
  if (!TracyCIsConnected) return JS_UNDEFINED;

  HRESULT hr;
  // create staging texture
  // Get render target texture
  ID3D11Texture2D *render_target_tex = NULL;
  render_view->lpVtbl->GetResource(sapp_d3d11_get_render_view(), &render_target_tex);
//  if (FAILED(hr)) return JS_ThrowReferenceError(js, "Failed to query texture from render target");

  D3D11_TEXTURE2D_DESC render_tex_desc;
  render_target_tex->lpVtbl->GetDesc(render_target_tex, &render_tex_desc);

  render_tex_desc.Usage = D3D11_USAGE_STAGING;
  render_tex_desc.BindFlags = 0;
  render_tex_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
  render_tex_desc.MiscFlags = 0;
  render_tex_desc.SampleDesc = (DXGI_SAMPLE_DESC){ .Count = 1, .Quality = 0, };

  ID3D11Texture2D *staging_tex = NULL;
  hr = device->lpVtbl->CreateTexture2D(device, &render_tex_desc, NULL, &staging_tex);
  if (FAILED(hr)) return JS_ThrowReferenceError(js, "Failed to create staging texture: 0x%08X\n", hr);

  context->lpVtbl->CopyResource(context, staging_tex, render_target_tex);

  D3D11_MAPPED_SUBRESOURCE mapped_res;
  hr = context->lpVtbl->Map(context, staging_tex, 0, D3D11_MAP_READ, 0, &mapped_res);
  if (FAILED(hr)) return JS_ThrowReferenceError(js, "Failed to map subresource");

// resize
  int newwidth = 320;
  int newheight = 180;
  void *newdata = malloc(320*180*4);
  stbir_resize_uint8_linear(mapped_res.pData, render_tex_desc.Width, render_tex_desc.Height, 0, newdata, newwidth, newheight, 0, 4);

  ___tracy_emit_frame_image(newdata, newwidth, newheight, 0, 0);

  context->lpVtbl->Unmap(context, staging_tex, 0);
  staging_tex->lpVtbl->Release(staging_tex);
  render_target_tex->lpVtbl->Release(render_target_tex);
  free(newdata);

  return JS_UNDEFINED;
}

#else

static JSValue js_tracy_gpu_init(JSContext *js, JSValue self, int argc, JSValue *argv)
{
  return JS_UNDEFINED;
}

static JSValue js_tracy_gpu_zone_begin(JSContext *js, JSValue self, int argc, JSValue *argv)
{
  return JS_Call(js,argv[0], JS_UNDEFINED, 0, NULL);
}

static JSValue js_tracy_gpu_sync(JSContext *js, JSValue self, int argc, JSValue *argv)
{
  return JS_UNDEFINED;
}

static JSValue js_tracy_gpu_collect(JSContext *js, JSValue self, int argc, JSValue *argv)
{
  return JS_UNDEFINED;
}

static JSValue js_tracy_image(JSContext *js, JSValue self, int argc, JSValue *argv)
{
/*  SDL_Surface *img = js2SDL_Surface(js,argv[0]);
  SDL_Surface *scaled = SDL_ScaleSurface(img, 320,180,SDL_SCALEMODE_LINEAR);
  ___tracy_emit_frame_image(scaled->pixels, scaled->w,scaled->h, 0,0);
  SDL_DestroySurface(scaled);
  return JS_UNDEFINED;*/
}

#endif

static const JSCFunctionListEntry js_tracy_funcs[] = {
  JS_CFUNC_DEF("fiber", 1, js_tracy_fiber_enter),
  JS_CFUNC_DEF("fiber_leave", 1, js_tracy_fiber_leave),
  JS_CFUNC_DEF("gpu_zone", 1, js_tracy_gpu_zone_begin),
  JS_CFUNC_DEF("gpu_collect", 1, js_tracy_gpu_collect),
  JS_CFUNC_DEF("gpu_init", 0, js_tracy_gpu_init),
  JS_CFUNC_DEF("gpu_sync", 0, js_tracy_gpu_sync),
  JS_CFUNC_DEF("end_frame", 0, js_tracy_frame_mark),
  JS_CFUNC_DEF("zone", 1, js_tracy_zone_begin),
  JS_CFUNC_DEF("message", 1, js_tracy_message),
  JS_CFUNC_DEF("plot", 2, js_tracy_plot),
  JS_CFUNC_DEF("image", 3, js_tracy_image),
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
