#include "jsffi.h"

#include "script.h" 
#include "font.h"
#include "datastream.h"
#include "stb_ds.h"
#include "stb_image.h"
#include "stb_rect_pack.h"
#define STB_DXT_IMPLEMENTATION
#include "stb_dxt.h"
#include "string.h"
#include "spline.h"
#include "yugine.h"
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>
#include "render.h"
#include "model.h"
#include "HandmadeMath.h"
#include "par/par_streamlines.h"
#include "par/par_shapes.h"
#include <stdint.h>
#include "timer.h"
#include <signal.h>
#include "cute_aseprite.h"
#include "cgltf.h"
#include "physfs.h"

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_error.h>

#ifdef __APPLE__
#include <Accelerate/Accelerate.h>
//#else
//#include <cblas.h>
#endif

static JSAtom width_atom;
static JSAtom height_atom;
static JSAtom x_atom;
static JSAtom y_atom;
static JSAtom anchor_x_atom;
static JSAtom anchor_y_atom;
static JSAtom pos_atom;
static JSAtom uv_atom;
static JSAtom color_atom;
static JSAtom indices_atom;
static JSAtom vertices_atom;
static JSAtom dst_atom;
static JSAtom src_atom;
static JSAtom count_atom;
static JSAtom transform_atom;

// GPU ATOMS  
static JSAtom cw_atom;
static JSAtom ccw_atom;
static JSAtom zero_atom;
static JSAtom one_atom;
static JSAtom add_atom;
static JSAtom sub_atom;
static JSAtom rev_sub_atom;
static JSAtom min_atom;
static JSAtom max_atom;
static JSAtom none_atom;
static JSAtom norm_atom;
static JSAtom front_atom;
static JSAtom back_atom;
static JSAtom never_atom;
static JSAtom less_atom;
static JSAtom equal_atom;
static JSAtom less_equal_atom;
static JSAtom greater_atom;
static JSAtom not_equal_atom;
static JSAtom greater_equal_atom;
static JSAtom always_atom;
static JSAtom keep_atom;
static JSAtom zero_stencil_atom;
static JSAtom replace_atom;
static JSAtom incr_clamp_atom;
static JSAtom decr_clamp_atom;
static JSAtom invert_atom;
static JSAtom incr_wrap_atom;
static JSAtom decr_wrap_atom;
static JSAtom point_atom;
static JSAtom line_atom;
static JSAtom linestrip_atom;
static JSAtom triangle_atom;
static JSAtom trianglestrip_atom;
static JSAtom src_color_atom;
static JSAtom one_minus_src_color_atom;
static JSAtom dst_color_atom;
static JSAtom one_minus_dst_color_atom;
static JSAtom src_alpha_atom;
static JSAtom one_minus_src_alpha_atom;
static JSAtom dst_alpha_atom;
static JSAtom one_minus_dst_alpha_atom;
static JSAtom constant_color_atom;
static JSAtom one_minus_constant_color_atom;
static JSAtom src_alpha_saturate_atom;
static JSAtom none_cull_atom;
static JSAtom front_cull_atom;
static JSAtom back_cull_atom;

// For sampler filtering:
static JSAtom nearest_atom;
static JSAtom linear_atom;

// For sampler mipmap modes:
static JSAtom mipmap_nearest_atom;
static JSAtom mipmap_linear_atom;

// For sampler address modes:
static JSAtom repeat_atom;
static JSAtom mirror_atom;
static JSAtom clamp_edge_atom;
static JSAtom clamp_border_atom;

static JSAtom vertex_atom;
static JSAtom index_atom;
static JSAtom indirect_atom;

typedef struct texture_vertex {
  float x, y, z;
  float u, v;
  uint8_t r, g, b,a;
} texture_vertex;

typedef struct app_data {
  HMM_Mat4 vp;
  HMM_Mat4 view;
  float time;
} app_data;

typedef struct {
  HMM_Mat4 camera_matrix;
  size_t start_vert;
  size_t start_indx;
} render_batch;
static render_batch *batches = NULL;

static inline size_t typed_array_bytes(JSTypedArrayEnum type) {
    switch(type) {
        case JS_TYPED_ARRAY_UINT8C:
        case JS_TYPED_ARRAY_INT8:
        case JS_TYPED_ARRAY_UINT8:
            return 1;

        case JS_TYPED_ARRAY_INT16:
        case JS_TYPED_ARRAY_UINT16:
            return 2;

        case JS_TYPED_ARRAY_INT32:
        case JS_TYPED_ARRAY_UINT32:
        case JS_TYPED_ARRAY_FLOAT32:
            return 4;

        case JS_TYPED_ARRAY_BIG_INT64:
        case JS_TYPED_ARRAY_BIG_UINT64:
        case JS_TYPED_ARRAY_FLOAT64:
            return 8;

        default:
            return 0; // Return 0 for unknown types
    }
}

#define JS_GETNUM(JS,VAL,I,TO,TYPE) { \
JSValue val = JS_GetPropertyUint32(JS,VAL,I); \
TO = js2##TYPE(JS, val); \
JS_FreeValue(JS, val); } \

JSValue number2js(JSContext *js, double g) { return JS_NewFloat64(js,g); }
double js2number(JSContext *js, JSValue v) {
  double g;
  JS_ToFloat64(js, &g, v);
  if (isnan(g)) g = 0;
  return g;
}

JSValue js_getpropertyuint32(JSContext *js, JSValue v, unsigned int i)
{
  JSValue ret = JS_GetPropertyUint32(js,v,i);
  JS_FreeValue(js,ret);
  return ret;
}

double js_getnum_uint32(JSContext *js, JSValue v, unsigned int i)
{
  JSValue val = JS_GetPropertyUint32(js,v,i);
  double ret = js2number(js, val);
  JS_FreeValue(js,val);
  return ret;
}

static HMM_Mat3 cam_mat;

double js_getnum_str(JSContext *js, JSValue v, const char *str)
{
  JSValue val = JS_GetPropertyStr(js,v,str);
  double ret = js2number(js,val);
  JS_FreeValue(js,val);
  return ret;
}

double js_getnum(JSContext *js, JSValue v, JSAtom prop)
{
  JSValue val = JS_GetProperty(js,v,prop);
  double ret = js2number(js,val);
  JS_FreeValue(js,val);
  return ret;
}

#define JS_GETPROPSTR(JS, VALUE, TARGET, STR, TYPE) {\
JSValue __v = JS_GetPropertyStr(JS,VALUE,#STR); \
TARGET.STR = js2##TYPE(JS, __v); \
JS_FreeValue(JS,__v); }\

#define JS_GETPROP(JS, VALUE, TARGET, PROP, TYPE) {\
JSValue __v = JS_GetProperty(JS,VALUE,PROP); \
TARGET.STR = js2##TYPE(JS, __v); \
JS_FreeValue(JS,__v); }\

JSValue js_getpropertystr(JSContext *js, JSValue v, const char *str)
{
  JSValue ret = JS_GetPropertyStr(js, v, str);
  JS_FreeValue(js,ret);
  return ret;
}

JSValue js_getproperty(JSContext *js, JSValue v, JSAtom atom)
{
  JSValue ret = JS_GetProperty(js, v, atom);
  JS_FreeValue(js,ret);
  return ret;
}

SDL_Window *global_window;

void free_gpu_buffer(JSRuntime *rt, void *opaque, void *ptr)
{
  free(ptr);
}

JSValue make_gpu_buffer(JSContext *js, void *data, size_t size, int type, int elements, int copy)
{
  JSValue tstack[3];
  tstack[1] = JS_UNDEFINED;
  tstack[2] = JS_UNDEFINED;
  if (copy)
    tstack[0] = JS_NewArrayBufferCopy(js,data,size);//, make_gpu_buffer, NULL, 1);
  else
    tstack[0] = JS_NewArrayBuffer(js,data,size,free_gpu_buffer, NULL, 0);
  JSValue ret =  JS_NewTypedArray(js, 3, tstack, type);
  JS_SetPropertyStr(js,ret,"stride", number2js(js,typed_array_bytes(type)*elements));
  JS_FreeValue(js,tstack[0]);
  return ret;
}

void *get_gpu_buffer(JSContext *js, JSValue argv, size_t *stride, size_t *size)
{
  size_t o, len, bytes, msize;
  JSValue buf = JS_GetTypedArrayBuffer(js, argv, &o, &len, &bytes);
  void *data = JS_GetArrayBuffer(js, &msize, buf);
  JS_FreeValue(js,buf); 
  *stride = js_getnum_str(js, argv, "stride");
  if (size) *size = msize;
  return data;
}

#ifndef _WIN32
#include <sys/resource.h>
#endif

#include "stb_perlin.h"

#if (defined(_WIN32) || defined(__WIN32__))
#include <direct.h>
#define mkdir(x,y) _mkdir(x)
#endif

struct lrtb {
  float l;
  float r;
  float t;
  float b;
};

typedef struct lrtb lrtb;

lrtb js2lrtb(JSContext *js, JSValue v)
{
  lrtb ret = {0};
  JS_GETPROPSTR(js,v,ret,l,number)
  JS_GETPROPSTR(js,v,ret,b,number)
  JS_GETPROPSTR(js,v,ret,t,number)
  JS_GETPROPSTR(js,v,ret,r,number)
  return ret;
}

JSValue vec22js(JSContext *js,HMM_Vec2 v)
{
  JSValue array = JS_NewArray(js);
  JS_SetPropertyUint32(js, array,0,number2js(js,v.x));
  JS_SetPropertyUint32(js, array,1,number2js(js,v.y));
  return array;
}

char *js2strdup(JSContext *js, JSValue v) {
  const char *str = JS_ToCString(js, v);
  char *ret = strdup(str);
  JS_FreeCString(js, str);
  return ret;
}

void skin_free(JSRuntime *rt,skin *sk) {
  arrfree(sk->invbind);
  free(sk);
}

#include "qjs_macros.h"

void SDL_Window_free(JSRuntime *rt, SDL_Window *w)
{
  SDL_DestroyWindow(w);
}

void SDL_Renderer_free(JSRuntime *rt, SDL_Renderer *r)
{
  SDL_DestroyRenderer(r);
}

void SDL_Surface_free(JSRuntime *rt, SDL_Surface *s) {
  if (s->flags & SDL_SURFACE_PREALLOCATED)
    free(s->pixels);
  TracyCFreeN(s,"texture memory");
  SDL_DestroySurface(s);
}

void SDL_Camera_free(JSRuntime *rt, SDL_Camera *cam)
{
  SDL_CloseCamera(cam);
}

void SDL_Cursor_free(JSRuntime *rt, SDL_Cursor *c)
{
  SDL_DestroyCursor(c);
}

void SDL_GPUDevice_free(JSRuntime *rt, SDL_GPUDevice *d)
{
  SDL_DestroyGPUDevice(d);
}

void SDL_GPUCommandBuffer_free(JSRuntime *rt, SDL_GPUCommandBuffer *c)
{
}

void SDL_Thread_free(JSRuntime *rt, SDL_Thread *t)
{
}

void SDL_GPUComputePass_free(JSRuntime *rt, SDL_GPUComputePass *c) { SDL_EndGPUComputePass(c); }
void SDL_GPUCopyPass_free(JSRuntime *rt, SDL_GPUCopyPass *c) {  }
void SDL_GPURenderPass_free(JSRuntime *rt, SDL_GPURenderPass *c) {  }

#define GPURELEASECLASS(NAME) \
void SDL_GPU##NAME##_free(JSRuntime *rt, SDL_GPU##NAME *c) { printf("IMPLEMENT %s FREE\n", #NAME); } \
QJSCLASS(SDL_GPU##NAME) \

QJSCLASS(transform)
QJSCLASS(font)
//QJSCLASS(warp_gravity)
//QJSCLASS(warp_damp)
QJSCLASS(datastream)
QJSCLASS(timer)
QJSCLASS(skin)

QJSCLASS(SDL_Window)
QJSCLASS(SDL_Renderer)
QJSCLASS(SDL_Camera)

void SDL_Texture_free(JSRuntime *rt, SDL_Texture *t){
  TracyCFreeN(t, "vram");
  SDL_DestroyTexture(t);
}

QJSCLASS(SDL_Texture,
  TracyCAllocN(n, n->w*n->h*4, "vram");
  JS_SetProperty(js, j, width_atom, number2js(js,n->w));
  JS_SetProperty(js,j,height_atom,number2js(js,n->h));
)

QJSCLASS(SDL_Surface,
  TracyCAllocN(n, n->pitch*n->h, "texture memory"); 
  JS_SetProperty(js, j, width_atom, number2js(js,n->w));
  JS_SetProperty(js,j,height_atom,number2js(js,n->h));
)
QJSCLASS(SDL_GPUDevice)
QJSCLASS(SDL_Thread)

GPURELEASECLASS(Buffer)
GPURELEASECLASS(ComputePipeline)
GPURELEASECLASS(GraphicsPipeline)
GPURELEASECLASS(Sampler)
GPURELEASECLASS(Shader)
GPURELEASECLASS(Texture)

QJSCLASS(SDL_GPUCommandBuffer)
QJSCLASS(SDL_GPUComputePass)
QJSCLASS(SDL_GPUCopyPass)
QJSCLASS(SDL_GPURenderPass)

QJSCLASS(SDL_Cursor)

int js_arrlen(JSContext *js,JSValue v) {
  if (JS_IsUndefined(v)) return 0;
  int len;
  len = js_getnum_str(js,v,"length");
  return len;
}

static inline HMM_Mat3 js2transform_mat3(JSContext *js, JSValue v)
{
  transform *T = js2transform(js,v);
  transform *P = js2transform(js,js_getpropertystr(js,v,"parent"));
  if (P) {
    HMM_Mat3 pm = transform2mat3(P);
    HMM_Mat3 tm = transform2mat3(T);
    return HMM_MulM3(pm,tm);
  }
  return transform2mat3(T);
}

void *get_typed_buffer(JSContext *js, JSValue argv, size_t *len)
{
  size_t o,bytes,size;
  JSValue buf = JS_GetTypedArrayBuffer(js,argv,&o,len,&bytes);
  void *data = JS_GetArrayBuffer(js,&size,buf);
  JS_FreeValue(js,buf);
  return data;
}

float *js2floatarray(JSContext *js, JSValue v) {
  JSClassID class = JS_GetClassID(v);
  if (class == JS_CLASS_FLOAT32_ARRAY) {
    printf("FAST PATH\n");
    size_t len;
    return get_typed_buffer(js,v, &len);
  }
//  switch(p->class_id){}
  int len = js_arrlen(js,v);
  float *array = malloc(sizeof(float)*len);
  for (int i = 0; i < len; i++)
    array[i] = js_getnum_uint32(js,v,i);
  return array;
}

JSValue angle2js(JSContext *js,double g) {
  return number2js(js,g*HMM_RadToTurn);
}

double js2angle(JSContext *js,JSValue v) {
  double n = js2number(js,v);
  return n * HMM_TurnToRad;
}

typedef HMM_Vec4 colorf;

colorf js2color(JSContext *js,JSValue v) {
  JSValue c[4];
  for (int i = 0; i < 4; i++) c[i] = JS_GetPropertyUint32(js,v,i);
  float a = JS_IsUndefined(c[3]) ? 1.0 : js2number(js,c[3]);
  colorf color = {
    .r = js2number(js,c[0]),
    .g = js2number(js,c[1]),
    .b = js2number(js,c[2]),
    .a = a,
  };

  for (int i = 0; i < 4; i++) JS_FreeValue(js,c[i]);

  return color;
}

JSValue color2js(JSContext *js, colorf color)
{
  JSValue arr = JS_NewArray(js);
  JS_SetPropertyUint32(js, arr,0,number2js(js,(double)color.r));
  JS_SetPropertyUint32(js, arr,1,number2js(js,(double)color.g));  
  JS_SetPropertyUint32(js, arr,2,number2js(js,(double)color.b));
  JS_SetPropertyUint32(js, arr,3,number2js(js,(double)color.a));
  return arr;
}

HMM_Vec2 js2vec2(JSContext *js,JSValue v)
{
  HMM_Vec2 v2;
  v2.X = js_getnum_uint32(js,v,0);
  v2.Y = js_getnum_uint32(js,v,1);
  return v2;
}

HMM_Vec3 js2vec3(JSContext *js,JSValue v)
{
  HMM_Vec3 v3;
  v3.x = js_getnum_uint32(js, v,0);
  v3.y = js_getnum_uint32(js, v,1);
  v3.z = js_getnum_uint32(js, v,2);
  return v3;
}

float *js2floats(JSContext *js, JSValue v, size_t *len)
{
  *len = js_arrlen(js,v);
  float *arr = malloc(sizeof(float)* *len);
  for (int i = 0; i < *len; i++)
    arr[i] = js_getnum_uint32(js,v,i);
  return arr;
}

double *js2doubles(JSContext *js, JSValue v, size_t *len)
{
  *len = js_arrlen(js,v);
  double *arr = malloc(sizeof(double)* *len);
  for (int i = 0; i < *len; i++)
    arr[i] = js_getnum_uint32(js,v,i);
  return arr;
}

HMM_Vec3 js2vec3f(JSContext *js, JSValue v)
{
  HMM_Vec3 vec;
  if (JS_IsArray(js, v))
    return js2vec3(js,v);
  else
    vec.x = vec.y = vec.z = js2number(js,v);
  return vec;
}

JSValue vec32js(JSContext *js, HMM_Vec3 v)
{
  JSValue array = JS_NewArray(js);
  JS_SetPropertyUint32(js, array,0,number2js(js,v.x));
  JS_SetPropertyUint32(js, array,1,number2js(js,v.y));
  JS_SetPropertyUint32(js, array,2,number2js(js,v.z));
  return array;
}

JSValue vec3f2js(JSContext *js, HMM_Vec3 v)
{
  return vec32js(js,v);
}

JSValue quat2js(JSContext *js, HMM_Quat q)
{
  JSValue arr = JS_NewArray(js);
  JS_SetPropertyUint32(js, arr, 0, number2js(js,q.x));
  JS_SetPropertyUint32(js, arr,1,number2js(js,q.y));
  JS_SetPropertyUint32(js, arr,2,number2js(js,q.z));
  JS_SetPropertyUint32(js, arr,3,number2js(js,q.w));
  return arr;
}

HMM_Vec4 js2vec4(JSContext *js, JSValue v)
{
  HMM_Vec4 v4;
  for (int i = 0; i < 4; i++)
    v4.e[i] = js_getnum_uint32(js, v,i);
  return v4;
}

double arr_vec_length(JSContext *js,JSValue v)
{
  int len = js_arrlen(js,v);
  switch(len) {
    case 2: return HMM_LenV2(js2vec2(js,v));
    case 3: return HMM_LenV3(js2vec3(js,v));
    case 4: return HMM_LenV4(js2vec4(js,v));
  }

  double sum = 0;
  for (int i = 0; i < len; i++)
    sum += pow(js_getnum_uint32(js, v, i), 2);

  return sqrt(sum);
}

HMM_Quat js2quat(JSContext *js,JSValue v)
{
  return js2vec4(js,v).quat;
}

JSValue vec42js(JSContext *js, HMM_Vec4 v)
{
  JSValue array = JS_NewArray(js);
  for (int i = 0; i < 4; i++)
    JS_SetPropertyUint32(js, array,i,number2js(js,v.e[i]));
  return array;
}

HMM_Vec2 *js2cpvec2arr(JSContext *js,JSValue v) {
  HMM_Vec2 *arr = NULL;
  int n = js_arrlen(js,v);
  arrsetlen(arr,n);
  
  for (int i = 0; i < n; i++)
    arr[i] = js2vec2(js,js_getpropertyuint32(js,  v, i));
    
  return arr;
}

JSValue vecarr2js(JSContext *js,HMM_Vec2 *points, int n) {
  JSValue array = JS_NewArray(js);
  for (int i = 0; i < n; i++)
    JS_SetPropertyUint32(js, array,i,vec22js(js,points[i]));
    
  return array;
}

int js_print_exception(JSContext *js, JSValue v)
{
  if (!js) return 0;
#ifndef NDEBUG
  if (!JS_IsException(v))
    return 0;

  JSValue ex = JS_GetException(js);
    
    const char *name = JS_ToCString(js, js_getpropertystr(js,ex, "name"));
    const char *msg = JS_ToCString(js, js_getpropertystr(js,ex, "message"));
    const char *stack = JS_ToCString(js, js_getpropertystr(js,ex, "stack"));
    printf("%s :: %s\n%s", name, msg, stack);

    JS_FreeCString(js, name);
    JS_FreeCString(js, msg);
    JS_FreeCString(js, stack);
    JS_FreeValue(js,ex);
    
    return 1;
#endif
  return 0;
}

rect js2rect(JSContext *js,JSValue v) {
  rect rect;
  rect.w = js_getnum(js,v,width_atom);
  rect.h = js_getnum(js,v,height_atom);
  JSValue xv = JS_GetProperty(js,v,x_atom);
  rect.x = js2number(js,xv);
  JS_FreeValue(js,xv);
  JSValue yv = JS_GetProperty(js,v,y_atom);
  rect.y = js2number(js,yv);
  JS_FreeValue(js,yv);
  float anchor_x = js_getnum(js,v, anchor_x_atom);
  float anchor_y = js_getnum(js,v, anchor_y_atom);

  rect.y -= anchor_y*rect.h;
  rect.x -= anchor_x*rect.w;

  return rect;
}

rect transform_rect(SDL_Renderer *ren, rect in, HMM_Mat3 *t)
{
  HMM_Vec3 bottom_left = (HMM_Vec3){in.x,in.y,1.0};
  HMM_Vec3 transformed_bl = HMM_MulM3V3(*t, bottom_left);
  in.x = transformed_bl.x;
  in.y = transformed_bl.y;
  in.y = in.y - in.h; // should be done for any platform that draws rectangles from top left
  return in;
}

HMM_Vec2 transform_point(SDL_Renderer *ren, HMM_Vec2 in, HMM_Mat3 *t)
{
  rect logical;
  SDL_GetRenderLogicalPresentationRect(ren, &logical);
  in.y *= -1;
  in.y += logical.h;
  in.x -= t->Columns[2].x;
  in.y -= t->Columns[2].y;
  return in;
}

JSValue rect2js(JSContext *js,rect rect) {
  JSValue obj = JS_NewObject(js);
  JS_SetProperty(js, obj, x_atom, number2js(js,rect.x));
  JS_SetProperty(js, obj, y_atom, number2js(js,rect.y));
  JS_SetProperty(js, obj, width_atom, number2js(js,rect.w));
  JS_SetProperty(js, obj, height_atom, number2js(js,rect.h));
  return obj;
}

JSValue ints2js(JSContext *js,int *ints) {
  JSValue arr = JS_NewArray(js);
  for (int i = 0; i < arrlen(ints); i++)
    JS_SetPropertyUint32(js, arr,i, number2js(js,ints[i]));

  return arr;
}

int vec_between(HMM_Vec2 p, HMM_Vec2 a, HMM_Vec2 b) {
  HMM_Vec2 n;
  n.x = b.x - a.x;
  n.y = b.y - a.y;
  n = HMM_NormV2(n);

  return HMM_DotV2(n, HMM_SubV2(p,a)) > 0 && HMM_DotV2(HMM_MulV2F(n, -1), HMM_SubV2(p,b)) > 0;
}

/* Determines between which two points in 'segs' point 'p' falls.
   0 indicates 'p' comes before the first point.
   arrlen(segs) indicates it comes after the last point.
*/
int point2segindex(HMM_Vec2 p, HMM_Vec2 *segs, double slop) {
  float shortest = slop < 0 ? INFINITY : slop;
  int best = -1;

  for (int i = 0; i < arrlen(segs) - 1; i++) {
    float a = (segs[i + 1].y - segs[i].y) / (segs[i + 1].x - segs[i].x);
    float c = segs[i].y - (a * segs[i].x);
    float b = -1;

    float dist = fabsf(a * p.x + b * p.y + c) / sqrt(pow(a, 2) + 1);

    if (dist > shortest) continue;

    int between = vec_between(p, segs[i], segs[i + 1]);

    if (between) {
      shortest = dist;
      best = i + 1;
    } else {
      if (i == 0 && HMM_DistV2(p,segs[0]) < slop) {
        shortest = dist;
        best = i;
      } else if (i == arrlen(segs) - 2 && HMM_DistV2(p,arrlast(segs)) < slop) {
        shortest = dist;
        best = arrlen(segs);
      }
    }
  }

  if (best == 1) {
    HMM_Vec2 n;
    n.x = segs[1].x - segs[0].x;
    n.y = segs[1].y - segs[0].y;
    n = HMM_NormV2(n);
    if (HMM_DotV2(n, HMM_SubV2(p,segs[0])) < 0 ){
      if (HMM_DistV2(p, segs[0]) >= slop)
        best = -1;
      else
        best = 0;
     }
  }

  if (best == arrlen(segs) - 1) {
    HMM_Vec2 n;
    n.x = segs[best - 1].x - segs[best].x;
    n.y = segs[best - 1].y - segs[best - 1].y;
    n = HMM_NormV2(n);

    if (HMM_DotV2(n, HMM_SubV2(p, segs[best])) < 0) {
      if (HMM_DistV2(p, segs[best]) >= slop)
        best = -1;
      else
        best = arrlen(segs);
     }
  }

  return best;
}

/*JSC_GETSET(warp_gravity, strength, number)
JSC_GETSET(warp_gravity, decay, number)
JSC_GETSET(warp_gravity, spherical, bool)
JSC_GETSET(warp_gravity, mask, bitmask)
JSC_GETSET(warp_gravity, planar_force, vec3)

static const JSCFunctionListEntry js_warp_gravity_funcs [] = {
  CGETSET_ADD(warp_gravity, strength),
  CGETSET_ADD(warp_gravity, decay),
  CGETSET_ADD(warp_gravity, spherical),
  CGETSET_ADD(warp_gravity, mask),
  CGETSET_ADD(warp_gravity, planar_force),  
};

JSC_GETSET(warp_damp, damp, vec3)

static const JSCFunctionListEntry js_warp_damp_funcs [] = {
  CGETSET_ADD(warp_damp, damp)
};
*/

HMM_Mat4 transform2view(transform *t)
{
  HMM_Vec3 look = HMM_AddV3(t->pos, transform_direction(t, vFWD));
  HMM_Mat4 ret = HMM_LookAt_RH(t->pos, look, vUP);
  ret = HMM_MulM4(ret, HMM_Scale(t->scale));
  return ret;
}

JSC_CCALL(render_set_projection_ortho,
  lrtb extents = js2lrtb(js, argv[0]);
  float nearme = js2number(js,argv[1]);
  float farme = js2number(js,argv[2]);
  globalview.p = HMM_Orthographic_RH_ZO(
    extents.l,
    extents.r,
    extents.b,
    extents.t,
    nearme,
    farme
  );
  globalview.vp = HMM_MulM4(globalview.p, globalview.v);
)

JSC_CCALL(render_set_projection_perspective,
  float fov = js2number(js,argv[0]);
  float aspect = js2number(js,argv[1]);
  float nearme = js2number(js,argv[2]);
  float farme = js2number(js,argv[3]);
  globalview.p = HMM_Perspective_RH_NO(fov, aspect, nearme, farme);
  globalview.vp = HMM_MulM4(globalview.p, globalview.v);
)

JSC_CCALL(render_set_view,
  globalview.v = transform2view(js2transform(js,argv[0]));
  globalview.vp = HMM_MulM4(globalview.p, globalview.v);
)

JSC_SCALL(render_text_size,
  font *f = js2font(js,argv[1]);
  float size = js2number(js,argv[2]);
  if (!size) size = f->height;
  float letterSpacing = js2number(js,argv[3]);
  float wrap = js2number(js,argv[4]);
  ret = vec22js(js,measure_text(str, f, size, letterSpacing, wrap));
)

JSC_CCALL(render_draw_color,
  SDL_Renderer *renderer = js2SDL_Renderer(js,self);
  colorf rgba = js2color(js,argv[0]);
  SDL_SetRenderDrawColorFloat(renderer, rgba.r, rgba.g, rgba.b, rgba.a);
)

static const JSCFunctionListEntry js_render_funcs[] = {
  MIST_FUNC_DEF(render, text_size, 5),
  MIST_FUNC_DEF(render, set_projection_ortho, 3),
  MIST_FUNC_DEF(render, set_projection_perspective, 4),  
  MIST_FUNC_DEF(render, set_view, 1),
  MIST_FUNC_DEF(render, draw_color, 1),
};

static JSValue idx_buffer = JS_UNDEFINED;
static int idx_count = 0;

JSValue make_quad_indices_buffer(JSContext *js, int quads)
{
  int count = quads*6;
  if (!JS_IsUndefined(idx_buffer) && idx_count >= count)
    return JS_DupValue(js,idx_buffer);
  
  int verts = quads*4;
  uint16_t *indices = malloc(sizeof(*indices)*count);
  for (int i = 0, v = 0; v < verts; i +=6, v += 4) {
    indices[i] = v;
    indices[i+1] = v+1;
    indices[i+2] = v+2;
    indices[i+3] = v+2;
    indices[i+4] = v+1;
    indices[i+5] = v+3;
  }

  if (!JS_IsUndefined(idx_buffer))
    JS_FreeValue(js,idx_buffer);
    
  idx_buffer = make_gpu_buffer(js,indices, sizeof(*indices)*count, JS_TYPED_ARRAY_UINT16, 1,0);
  return JS_DupValue(js,idx_buffer);
}

JSC_CCALL(os_make_text_buffer,
  const char *s = JS_ToCString(js, argv[0]);
  rect rectpos = js2rect(js,argv[1]);
  float size = js2number(js,argv[2]);
  font *f = js2font(js,argv[5]);
  if (!size) size = f->height;
  colorf c = js2color(js,argv[3]);
  int wrap = js2number(js,argv[4]);
  HMM_Vec2 startpos = {.x = rectpos.x, .y = rectpos.y };
  text_vert *buffer = renderText(s, startpos, f, size, c, wrap);
  size_t verts = arrlen(buffer);  

  HMM_Vec2 *pos = malloc(arrlen(buffer)*sizeof(HMM_Vec2));
  HMM_Vec2 *uv = malloc(arrlen(buffer)*sizeof(HMM_Vec2));
  HMM_Vec4 *color = malloc(arrlen(buffer)*sizeof(HMM_Vec4));

  for (int i = 0; i < arrlen(buffer); i++) {
    pos[i] = buffer[i].pos;
    uv[i] = buffer[i].uv;
    color[i] = buffer[i].color;
  }

  arrfree(buffer);    

  JSValue jspos = make_gpu_buffer(js, pos, sizeof(HMM_Vec2)*arrlen(buffer), JS_TYPED_ARRAY_FLOAT32, 2,0);
  JSValue jsuv = make_gpu_buffer(js, uv, sizeof(HMM_Vec2)*arrlen(buffer), JS_TYPED_ARRAY_FLOAT32, 2,0);
  JSValue jscolor = make_gpu_buffer(js, color, sizeof(HMM_Vec4)*arrlen(buffer), JS_TYPED_ARRAY_FLOAT32, 4,0);

/*
  JSValue jsbuffer = JS_NewArrayBuffer(js,buffer,sizeof(*buffer)*arrlen(buffer), free_gpu_buffer, NULL, 0);
  JSValue tstack[3];
  tstack[0] = jsbuffer;
  tstack[1] = JS_UNDEFINED;//number2js(js,0);
  tstack[2] = JS_UNDEFINED;
  JSValue jspos = JS_NewTypedArray(js, 3, tstack, JS_TYPED_ARRAY_FLOAT32);
  JS_SetPropertyStr(js,jspos, "stride", number2js(js,sizeof(*buffer)));
  tstack[1] = number2js(js,8);
  JSValue jsuv = JS_NewTypedArray(js,3,tstack,JS_TYPED_ARRAY_FLOAT32);
  JS_SetPropertyStr(js,jsuv, "stride", number2js(js,sizeof(*buffer)));  
  tstack[1] = number2js(js,16);
  JSValue jscolor = JS_NewTypedArray(js,3,tstack,JS_TYPED_ARRAY_FLOAT32);
  JS_SetPropertyStr(js,jscolor, "stride", number2js(js,sizeof(*buffer)));
*/

  size_t quads = verts/4;
  size_t count = verts/2*3;  
  JSValue jsidx = make_quad_indices_buffer(js, quads);
  
  JS_FreeCString(js, s);

  ret = JS_NewObject(js);
  JS_SetProperty(js, ret, pos_atom, jspos);
  JS_SetProperty(js, ret, uv_atom, jsuv);
  JS_SetProperty(js, ret, color_atom, jscolor);
  JS_SetProperty(js, ret, indices_atom, jsidx);
  JS_SetProperty(js, ret, vertices_atom, number2js(js, verts));
  JS_SetProperty(js, ret, count_atom, number2js(js, count));

  return ret;
)

JSC_CCALL(spline_catmull,
  HMM_Vec2 *points = js2cpvec2arr(js,argv[0]);
  float param = js2number(js,argv[1]);
  HMM_Vec2 *samples = catmull_rom_ma_v2(points,param);
  arrfree(points);
  if (!samples) return JS_UNDEFINED;
  JSValue arr = vecarr2js(js,samples, arrlen(samples));
  arrfree(samples);
  return arr;
)

JSC_CCALL(spline_bezier,
  HMM_Vec2 *points = js2cpvec2arr(js,argv[0]);
  float param = js2number(js,argv[1]);
  HMM_Vec2 *samples = catmull_rom_ma_v2(points,param);
  arrfree(points);
  if (!samples) return JS_UNDEFINED;
  JSValue arr = vecarr2js(js,samples, arrlen(samples));
  arrfree(samples);
  return arr;
)

static const JSCFunctionListEntry js_spline_funcs[] = {
  MIST_FUNC_DEF(spline, catmull, 2),
  MIST_FUNC_DEF(spline, bezier, 2)
};

JSValue js_vector_dot(JSContext *js, JSValue self, int argc, JSValue *argv) {
  size_t alen, blen;
  float *a = js2floats(js,argv[0], &alen);
  float *b = js2floats(js,argv[1], &blen);
//  JSValue ret = number2js(js, cblas_sdot(alen, a, 1, b,1));
  free(a);
  free(b);
  return JS_UNDEFINED;
};

JSC_CCALL(vector_project, return vec22js(js,HMM_ProjV2(js2vec2(js,argv[0]), js2vec2(js,argv[1]))))

JSC_CCALL(vector_midpoint,
  HMM_Vec2 a = js2vec2(js,argv[0]);
  HMM_Vec2 b = js2vec2(js,argv[1]);
//  HMM_Vec2 c = HMM_AddV2(a,b);
//  c = HMM_Div2VF(c, 2);
  return vec22js(js,(HMM_Vec2){(a.x+b.x)/2, (a.y+b.y)/2});
)

JSC_CCALL(vector_distance,
  HMM_Vec2 a = js2vec2(js,argv[0]);
  HMM_Vec2 b = js2vec2(js,argv[1]);
  return number2js(js,HMM_DistV2(a,b));
)

JSC_CCALL(vector_angle,
  HMM_Vec2 a = js2vec2(js,argv[0]);
  return angle2js(js,atan2(a.y,a.x));
)

/* Given a series of points p, computes a new series with them expanded on either side by d */
HMM_Vec2 *inflatepoints(HMM_Vec2 *p, float d, int n)
{
  if (d == 0) {
    HMM_Vec2 *ret = NULL;
    arraddn(ret,n);
       for (int i = 0; i < n; i++)
         ret[i] = p[i];

       return ret;
     }

  parsl_position par_v[n];
  uint16_t spine_lens[] = {n};
  for (int i = 0; i < n; i++) {
    par_v[i].x = p[i].x;
    par_v[i].y = p[i].y;
  };

  parsl_context *par_ctx = parsl_create_context((parsl_config){
    .thickness = d,
    .flags= PARSL_FLAG_ANNOTATIONS,
    .u_mode = PAR_U_MODE_DISTANCE
  });
  
  parsl_mesh *mesh = parsl_mesh_from_lines(par_ctx, (parsl_spine_list){
    .num_vertices = n,
    .num_spines = 1,
    .vertices = par_v,
    .spine_lengths = spine_lens,
    .closed = 0,
  });

  HMM_Vec2 *ret = NULL;
  arraddn(ret,mesh->num_vertices);
  for (int i = 0; i < mesh->num_vertices; i++) {
    ret[i].x = mesh->positions[i].x;
    ret[i].y = mesh->positions[i].y;
  };
  
  return ret;
}

JSC_CCALL(vector_inflate,
  HMM_Vec2 *p = js2cpvec2arr(js,argv[0]);
  double d = js2number(js,argv[1]);
  HMM_Vec2 *infl = inflatepoints(p,d, js_arrlen(js,argv[0]));
  ret = vecarr2js(js,infl, arrlen(infl));
  arrfree(infl);
  arrfree(p);
)

JSC_CCALL(vector_rotate,
  HMM_Vec2 vec = js2vec2(js,argv[0]);
  double angle = js2angle(js, argv[1]);
  HMM_Vec2 pivot = JS_IsUndefined(argv[2]) ? v2zero : js2vec2(js,argv[2]);  
  // vec = vec - pivot
//  cblas_saxpy(2, -1.0f, pivot.e, 1, vec.e, 1);
  vec = HMM_SubV2(vec,pivot);

  // Length of the vector (r)
//  float r = sqrtf(cblas_sdot(2, vec.e, 1, vec.e, 1));
  float r = HMM_LenV2(vec);
  
  // Update angle
  angle += atan2f(vec.y, vec.x);

  // Update vec to rotated position
  vec.x = r * cosf(angle);
  vec.y = r * sinf(angle);

  // vec = vec + pivot
//  cblas_saxpy(2, 1.0f, pivot.e, 1, vec.e, 1);
  vec = HMM_AddV2(vec,pivot);

  // Convert back to JS and return
  return vec22js(js, vec);
)
/*

JSC_CCALL(vector_rotate,
  HMM_Vec2 vec = js2vec2(js,argv[0]);
  double angle = js2angle(js,argv[1]);
  HMM_Vec2 pivot = JS_IsUndefined(argv[2]) ? v2zero : js2vec2(js,argv[2]);
  vec = HMM_SubV2(vec,pivot);
  float r = HMM_LenV2(vec);  
  angle += atan2(vec.y,vec.x);
  vec.x = r*cos(angle);
  vec.y = r*sin(angle);
  return vec22js(js,HMM_AddV2(vec,pivot));
)
*/
JSC_CCALL(vector_add,
  HMM_Vec4 a = js2vec4(js,argv[0]);
  HMM_Vec4 b = js2vec4(js,argv[1]);
  HMM_Vec4 c = HMM_AddV4(a,b);
  return vec42js(js,c);
)

JSC_CCALL(vector_norm,
  int len = js_arrlen(js,argv[0]);

  switch(len) {
    case 2: return vec22js(js,HMM_NormV2(js2vec2(js,argv[0])));
    case 3: return vec32js(js, HMM_NormV3(js2vec3(js,argv[0])));
    case 4: return vec42js(js,HMM_NormV4(js2vec4(js,argv[0])));
  }

  double length = arr_vec_length(js,argv[0]);
  JSValue newarr = JS_NewArray(js);

  for (int i = 0; i < len; i++)
    JS_SetPropertyUint32(js, newarr, i, number2js(js,js_getnum_uint32(js, argv[0],i)/length));

  return newarr;
)

JSC_CCALL(vector_angle_between,
  int len = js_arrlen(js,argv[0]);
  switch(len) {
    case 2: return angle2js(js,HMM_AngleV2(js2vec2(js,argv[0]), js2vec2(js,argv[1])));
    case 3: return angle2js(js,HMM_AngleV3(js2vec3(js,argv[0]), js2vec3(js,argv[1])));
    case 4: return angle2js(js,HMM_AngleV4(js2vec4(js,argv[0]), js2vec4(js,argv[1])));
  }
  return angle2js(js,0);
)

JSC_CCALL(vector_lerp,
  double s = js2number(js,argv[0]);
  double f = js2number(js,argv[1]);
  double t = js2number(js,argv[2]);
  
  return number2js(js,(f-s)*t+s);
)

int gcd(int a, int b) {
    if (b == 0)
        return a;
    return gcd(b, a % b);
}

JSC_CCALL(vector_gcd,
  return number2js(js,gcd(js2number(js,argv[0]), js2number(js,argv[1])));
)

JSC_CCALL(vector_lcm,
  double a = js2number(js,argv[0]);
  double b = js2number(js,argv[1]);
  return number2js(js,(a*b)/gcd(a,b));
)

JSC_CCALL(vector_clamp,
  double x = js2number(js,argv[0]);
  double l = js2number(js,argv[1]);
  double h = js2number(js,argv[2]);
  return number2js(js,x > h ? h : x < l ? l : x);
)

JSC_SSCALL(vector_trimchr,
  int len = js2number(js,js_getpropertystr(js,argv[0], "length"));
  const char *start = str;
  
  while (*start == *str2)
    start++;
    
  const char *end = str + len-1;
  while(*end == *str2)
    end--;
  
  ret = JS_NewStringLen(js, start, end-start+1);
)

JSC_CCALL(vector_angledist,
  double a1 = js2number(js,argv[0]);
  double a2 = js2number(js,argv[1]);
  a1 = fmod(a1,1);
  a2 = fmod(a2,1);
  double dist = a2-a1;
  if (dist == 0) return number2js(js,dist);
  if (dist > 0) {
    if (dist > 0.5) return number2js(js,dist-1);
    return number2js(js,dist);
  }
  
  if (dist < -0.5) return number2js(js,dist+1);
  
  return number2js(js,dist);
)

JSC_CCALL(vector_length,
  return number2js(js,arr_vec_length(js,argv[0]));
)

double r2()
{
    return (double)rand() / (double)RAND_MAX ;
}

double rand_range(double min, double max) {
  return r2() * (max-min) + min;
}

JSC_CCALL(vector_variate,
  double n = js2number(js,argv[0]);
  double pct = js2number(js,argv[1]);
  
  return number2js(js,n + (rand_range(-pct,pct)*n));
)

JSC_CCALL(vector_random_range, return number2js(js,rand_range(js2number(js,argv[0]), js2number(js,argv[1]))))

JSC_CCALL(vector_mean,
  double len =  js_arrlen(js,argv[0]);
  double sum = 0;
  for (int i = 0; i < len; i++)
    sum += js_getnum_uint32(js, argv[0], i);
    
  return number2js(js,sum/len);
)

JSC_CCALL(vector_sum,
  double sum = 0.0;
  int len = js_arrlen(js,argv[0]);
  for (int i = 0; i < len; i++)
    sum += js_getnum_uint32(js, argv[0], i);
  
  return number2js(js,sum);
)

JSC_CCALL(vector_fastsum,
  float sum = 0.0;
  size_t len;
  float *a = get_typed_buffer(js, argv[0], &len);
//  sum = cblas_sasum(len, a,1);
  ret = number2js(js,sum);
)

JSC_CCALL(vector_sigma,
  int len = js_arrlen(js,argv[0]);
  double sum = 0;
  for (int i = 0; i < len; i++)
    sum += js_getnum_uint32(js, argv[0], i);
  
  double mean = sum/(double)len;
  
  double sq_diff = 0;

  for (int i = 0; i < len; i++) {
    double x = js_getnum_uint32(js, argv[0],i);
    sq_diff += pow(x-mean, 2);
  }
  
  double variance = sq_diff/((double)len);
  
  return number2js(js,sqrt(variance));
)

JSC_CCALL(vector_median,
  int len = js_arrlen(js,argv[0]);
  double arr[len];
  double temp;
  
  for (int i = 0; i < len; i++)
    arr[i] = js_getnum_uint32(js, argv[0], i);
    
  for (int i = 0; i < len-1; i++) {
    for (int j = i+1; j < len; j++) {
      if (arr[i] > arr[j]) {
        temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
      }
    }
  }
  
  if (len % 2 == 0) return number2js(js,(arr[len/2-1] + arr[len/2])/2.0);
  return number2js(js,arr[len/2]);
)

int fibonacci(int n) {
  if (n <= 1) return n;
  return fibonacci(n-1) + fibonacci(n-2);
}
JSC_CCALL(vector_fib,
  int n = js2number(js,argv[0]);
  int fib = fibonacci(n);

  printf("ANSWER IS %d\n", fib);
)

JSC_CCALL(vector_from_to,
  HMM_Vec2 from = js2vec2(js,argv[0]);
  HMM_Vec2 to = js2vec2(js,argv[1]);
  float space = js2number(js,argv[2]);
  float from_offset = js2number(js,argv[3]);
  float to_offset = js2number(js,argv[4]);
  
  HMM_Vec2 dir = HMM_NormV2(HMM_SubV2(to, from));
  from = HMM_AddV2(from, HMM_MulV2F(dir, from_offset));
  to = HMM_SubV2(to,HMM_MulV2F(dir, to_offset));
  float length = HMM_DistV2(from, to);
  int steps = floor(length/space);
  int stepsize = length/(steps+1);

  ret = JS_NewArray(js);
  JS_SetPropertyUint32(js,ret,0,vec22js(js,from));
  for (int i = 1; i < steps+1; i++) {
    HMM_Vec2 val = HMM_AddV2(from, HMM_MulV2F(dir, i*stepsize));
    JS_SetPropertyUint32(js, ret, i, vec22js(js,val));
  }
  JS_SetPropertyUint32(js, ret, steps+1, vec22js(js,to));
)

JSC_CCALL(vector_float32add,
  size_t len;
  float *vec_a = get_typed_buffer(js,self, &len);
  float *vec_b = get_typed_buffer(js,argv[0], &len);
//  cblas_saxpy(len,1.0f,vec_b,1,vec_a,1);
  JSValue tstack[3];
  tstack[0] = JS_NewArrayBufferCopy(js,vec_a,sizeof(float)*4);
  tstack[1] = JS_UNDEFINED;
  tstack[2] = JS_UNDEFINED;
  ret = JS_NewTypedArray(js, 3, tstack, JS_TYPED_ARRAY_FLOAT32);
  JS_FreeValue(js,tstack[0]);
)

static const JSCFunctionListEntry js_vector_funcs[] = {
  MIST_FUNC_DEF(vector, dot,2),
  MIST_FUNC_DEF(vector, project,2),
  MIST_FUNC_DEF(vector, inflate, 2),
  MIST_FUNC_DEF(vector, rotate, 3),
  MIST_FUNC_DEF(vector, add, 2),
  MIST_FUNC_DEF(vector, midpoint, 2),
  MIST_FUNC_DEF(vector, distance, 2),
  MIST_FUNC_DEF(vector, angle, 1),
  MIST_FUNC_DEF(vector, norm, 1),
  MIST_FUNC_DEF(vector, angle_between, 2),
  MIST_FUNC_DEF(vector, lerp, 3),
  MIST_FUNC_DEF(vector, gcd, 2),
  MIST_FUNC_DEF(vector, lcm, 2),
  MIST_FUNC_DEF(vector, clamp, 3),
  MIST_FUNC_DEF(vector, trimchr, 2),
  MIST_FUNC_DEF(vector, angledist, 2),
  MIST_FUNC_DEF(vector, variate, 2),
  MIST_FUNC_DEF(vector, random_range, 2),
  MIST_FUNC_DEF(vector, mean, 1),
  MIST_FUNC_DEF(vector, sum, 1),
  MIST_FUNC_DEF(vector, fastsum, 1),
  MIST_FUNC_DEF(vector, sigma, 1),
  MIST_FUNC_DEF(vector, median, 1),
  MIST_FUNC_DEF(vector, length, 1),
  MIST_FUNC_DEF(vector, fib, 1),
  MIST_FUNC_DEF(vector, from_to, 5),
  MIST_FUNC_DEF(vector, float32add, 2),
};

#define JS_HMM_FN(OP, HMM, SIGN) \
JSC_CCALL(array_##OP, \
  int len = js_arrlen(js,self); \
  if (!JS_IsArray(js, argv[0])) { \
    double n = js2number(js,argv[0]); \
    JSValue arr = JS_NewArray(js); \
    for (int i = 0; i < len; i++) \
      JS_SetPropertyUint32(js, arr, i, number2js(js,js_getnum_uint32(js, self,i) SIGN n)); \
    return arr; \
  } \
  switch(len) { \
    case 2: \
      return vec22js(js,HMM_##HMM##V2(js2vec2(js,self), js2vec2(js,argv[0]))); \
    case 3: \
      return vec32js(js, HMM_##HMM##V3(js2vec3(js,self), js2vec3(js,argv[0]))); \
    case 4: \
      return vec42js(js,HMM_##HMM##V4(js2vec4(js,self), js2vec4(js,argv[0]))); \
  } \
  \
  JSValue arr = JS_NewArray(js); \
  for (int i = 0; i < len; i++) { \
    double a = js_getnum_uint32(js, self,i); \
    double b = js_getnum_uint32(js, argv[0],i); \
    JS_SetPropertyUint32(js, arr, i, number2js(js,a SIGN b)); \
  } \
  return arr; \
) \

/*JSC_CCALL(array_add,
  int len = js_arrlen(js,self); 
  if (!JS_IsArray(js, argv[0])) { 
    double n = js2number(js,argv[0]); 
    JSValue arr = JS_NewArray(js); 
    for (int i = 0; i < len; i++) 
      JS_SetPropertyUint32(js, arr, i, number2js(js,js_getnum_uint32(js, self,i) + n)); 
    return arr; 
  } 
  
  JSValue arr = JS_NewArray(js);
  size_t len_a, len_b;
  float *a = js2floats(js,self, &len_a);
  float *b = js2floats(js,argv[0], &len_b);
  cblas_saxpy(len_a, 1.0, a, 1, b, 1);
  for (int i = 0; i < len; i++)
    JS_SetPropertyUint32(js, arr, i, number2js(js,b[i]));

  ret = arr;
  free(a);
  free(b);
  return arr; 
) */

/*JSC_CCALL(array_add,
  int len = js_arrlen(js,self);
  if (!JS_IsArray(js,argv[0])) {
    double n = js2number(js,argv[0]);
    JSValue arr = JS_NewArray(js);
    for (int i = 0; i < len; i++)
      JS_SetPropertyUint32(js,arr,i,number2js(js,js_getnum_uint32(js,self,i) + n));

    return arr;
  }
  float *vec_a = js2floatarray(js,self);
  float *vec_b = js2floatarray(js,argv[0]);
  cblas_saxpy(len,1.0f,vec_b,1,vec_a,1);
  JSValue tstack[3];
  tstack[0] = JS_NewArrayBuffer(js,vec_a,sizeof(float)*2,free_gpu_buffer, NULL, 0);
  tstack[1] = JS_UNDEFINED;
  tstack[2] = JS_UNDEFINED;
  ret = JS_NewTypedArray(js, 3, tstack, JS_TYPED_ARRAY_FLOAT32);
  JS_FreeValue(js,tstack[0]);
  free(vec_b);
)
*/

JS_HMM_FN(add, Add, +)
JS_HMM_FN(sub, Sub, -)
JS_HMM_FN(div, Div, /)
JS_HMM_FN(scale, Mul, *)

JSC_CCALL(array_lerp,
  double t = js2number(js,argv[1]);
  int len = js_arrlen(js,self);
  JSValue arr =  JS_NewArray(js);
  
  for (int i = 0; i < len; i++) {
    double from = js_getnum_uint32(js, self, i);
    double to = js_getnum_uint32(js, argv[0], i);
    JS_SetPropertyUint32(js, arr, i, number2js(js,(to - from) * t + from));
  }
  return arr;
)

static const JSCFunctionListEntry js_array_funcs[] = {
  PROTO_FUNC_DEF(array, add, 1),
  PROTO_FUNC_DEF(array, sub, 1),
  PROTO_FUNC_DEF(array, div,1),
  PROTO_FUNC_DEF(array, scale, 1),
  PROTO_FUNC_DEF(array, lerp, 2)
};

JSC_SCALL(game_engine_start,
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_CAMERA) < 0)
    return JS_ThrowReferenceError(js, "Couldn't initialize SDL: %s\n", SDL_GetError());

  const char *title = JS_ToCString(js,js_getpropertystr(js,argv[0], "title"));
  SDL_Window *new = SDL_CreateWindow(title, js2number(js, js_getproperty(js,argv[0], width_atom)), js2number(js,js_getproperty(js,argv[0], height_atom)), SDL_WINDOW_RESIZABLE);

  if (!new) return JS_ThrowReferenceError(js, "Couldn't open window: %s\n", SDL_GetError());

  SDL_StartTextInput(new);
  global_window = new;
  return SDL_Window2js(js,new);
)

typedef struct {
  SDL_EventType key;
  JSAtom value;
} SDL_EventTypePair;

struct {SDL_EventType key; JSAtom value; } *event_hash = NULL;

void fill_event_atoms(JSContext *js)
{
  if (event_hash != NULL) return;

  // Application events
  hmput(event_hash, SDL_EVENT_QUIT, JS_NewAtom(js, "quit"));
  hmput(event_hash, SDL_EVENT_TERMINATING, JS_NewAtom(js, "terminating"));
  hmput(event_hash, SDL_EVENT_LOW_MEMORY, JS_NewAtom(js, "low_memory"));
  hmput(event_hash, SDL_EVENT_WILL_ENTER_BACKGROUND, JS_NewAtom(js, "will_enter_background"));
  hmput(event_hash, SDL_EVENT_DID_ENTER_BACKGROUND, JS_NewAtom(js, "did_enter_background"));
  hmput(event_hash, SDL_EVENT_WILL_ENTER_FOREGROUND, JS_NewAtom(js, "will_enter_foreground"));
  hmput(event_hash, SDL_EVENT_DID_ENTER_FOREGROUND, JS_NewAtom(js, "did_enter_foreground"));
  hmput(event_hash, SDL_EVENT_LOCALE_CHANGED, JS_NewAtom(js, "locale_changed"));
  hmput(event_hash, SDL_EVENT_SYSTEM_THEME_CHANGED, JS_NewAtom(js, "system_theme_changed"));

  // Display events
  hmput(event_hash, SDL_EVENT_DISPLAY_ORIENTATION, JS_NewAtom(js, "display_orientation"));
  hmput(event_hash, SDL_EVENT_DISPLAY_ADDED, JS_NewAtom(js, "display_added"));
  hmput(event_hash, SDL_EVENT_DISPLAY_REMOVED, JS_NewAtom(js, "display_removed"));
  hmput(event_hash, SDL_EVENT_DISPLAY_MOVED, JS_NewAtom(js, "display_moved"));
  hmput(event_hash, SDL_EVENT_DISPLAY_DESKTOP_MODE_CHANGED, JS_NewAtom(js, "display_desktop_mode_changed"));
  hmput(event_hash, SDL_EVENT_DISPLAY_CURRENT_MODE_CHANGED, JS_NewAtom(js, "display_current_mode_changed"));
  hmput(event_hash, SDL_EVENT_DISPLAY_CONTENT_SCALE_CHANGED, JS_NewAtom(js, "display_content_scale_changed"));

  // Window events
  hmput(event_hash, SDL_EVENT_WINDOW_SHOWN, JS_NewAtom(js, "window_shown"));
  hmput(event_hash, SDL_EVENT_WINDOW_HIDDEN, JS_NewAtom(js, "window_hidden"));
  hmput(event_hash, SDL_EVENT_WINDOW_EXPOSED, JS_NewAtom(js, "window_exposed"));
  hmput(event_hash, SDL_EVENT_WINDOW_MOVED, JS_NewAtom(js, "window_moved"));
  hmput(event_hash, SDL_EVENT_WINDOW_RESIZED, JS_NewAtom(js, "window_resized"));
  hmput(event_hash, SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED, JS_NewAtom(js, "window_pixel_size_changed"));
  hmput(event_hash, SDL_EVENT_WINDOW_METAL_VIEW_RESIZED, JS_NewAtom(js, "window_metal_view_resized"));
  hmput(event_hash, SDL_EVENT_WINDOW_MINIMIZED, JS_NewAtom(js, "window_minimized"));
  hmput(event_hash, SDL_EVENT_WINDOW_MAXIMIZED, JS_NewAtom(js, "window_maximized"));
  hmput(event_hash, SDL_EVENT_WINDOW_RESTORED, JS_NewAtom(js, "window_restored"));
  hmput(event_hash, SDL_EVENT_WINDOW_MOUSE_ENTER, JS_NewAtom(js, "window_mouse_enter"));
  hmput(event_hash, SDL_EVENT_WINDOW_MOUSE_LEAVE, JS_NewAtom(js, "window_mouse_leave"));
  hmput(event_hash, SDL_EVENT_WINDOW_FOCUS_GAINED, JS_NewAtom(js, "window_focus_gained"));
  hmput(event_hash, SDL_EVENT_WINDOW_FOCUS_LOST, JS_NewAtom(js, "window_focus_lost"));
  hmput(event_hash, SDL_EVENT_WINDOW_CLOSE_REQUESTED, JS_NewAtom(js, "window_close_requested"));
  hmput(event_hash, SDL_EVENT_WINDOW_HIT_TEST, JS_NewAtom(js, "window_hit_test"));
  hmput(event_hash, SDL_EVENT_WINDOW_ICCPROF_CHANGED, JS_NewAtom(js, "window_iccprof_changed"));
  hmput(event_hash, SDL_EVENT_WINDOW_DISPLAY_CHANGED, JS_NewAtom(js, "window_display_changed"));
  hmput(event_hash, SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED, JS_NewAtom(js, "window_display_scale_changed"));
  hmput(event_hash, SDL_EVENT_WINDOW_SAFE_AREA_CHANGED, JS_NewAtom(js, "window_safe_area_changed"));
  hmput(event_hash, SDL_EVENT_WINDOW_OCCLUDED, JS_NewAtom(js, "window_occluded"));
  hmput(event_hash, SDL_EVENT_WINDOW_ENTER_FULLSCREEN, JS_NewAtom(js, "window_enter_fullscreen"));
  hmput(event_hash, SDL_EVENT_WINDOW_LEAVE_FULLSCREEN, JS_NewAtom(js, "window_leave_fullscreen"));
  hmput(event_hash, SDL_EVENT_WINDOW_DESTROYED, JS_NewAtom(js, "window_destroyed"));
  hmput(event_hash, SDL_EVENT_WINDOW_HDR_STATE_CHANGED, JS_NewAtom(js, "window_hdr_state_changed"));

  // Keyboard events
  hmput(event_hash, SDL_EVENT_KEY_DOWN, JS_NewAtom(js, "key_down"));
  hmput(event_hash, SDL_EVENT_KEY_UP, JS_NewAtom(js, "key_up"));
  hmput(event_hash, SDL_EVENT_TEXT_EDITING, JS_NewAtom(js, "text_editing"));
  hmput(event_hash, SDL_EVENT_TEXT_INPUT, JS_NewAtom(js, "text_input"));
  hmput(event_hash, SDL_EVENT_KEYMAP_CHANGED, JS_NewAtom(js, "keymap_changed"));
  hmput(event_hash, SDL_EVENT_KEYBOARD_ADDED, JS_NewAtom(js, "keyboard_added"));
  hmput(event_hash, SDL_EVENT_KEYBOARD_REMOVED, JS_NewAtom(js, "keyboard_removed"));
  hmput(event_hash, SDL_EVENT_TEXT_EDITING_CANDIDATES, JS_NewAtom(js, "text_editing_candidates"));

  // Mouse events
  hmput(event_hash, SDL_EVENT_MOUSE_MOTION, JS_NewAtom(js, "mouse_motion"));
  hmput(event_hash, SDL_EVENT_MOUSE_BUTTON_DOWN, JS_NewAtom(js, "mouse_button_down"));
  hmput(event_hash, SDL_EVENT_MOUSE_BUTTON_UP, JS_NewAtom(js, "mouse_button_up"));
  hmput(event_hash, SDL_EVENT_MOUSE_WHEEL, JS_NewAtom(js, "mouse_wheel"));
  hmput(event_hash, SDL_EVENT_MOUSE_ADDED, JS_NewAtom(js, "mouse_added"));
  hmput(event_hash, SDL_EVENT_MOUSE_REMOVED, JS_NewAtom(js, "mouse_removed"));

  // Joystick events
  hmput(event_hash, SDL_EVENT_JOYSTICK_AXIS_MOTION, JS_NewAtom(js, "joystick_axis_motion"));
  hmput(event_hash, SDL_EVENT_JOYSTICK_BALL_MOTION, JS_NewAtom(js, "joystick_ball_motion"));
  hmput(event_hash, SDL_EVENT_JOYSTICK_HAT_MOTION, JS_NewAtom(js, "joystick_hat_motion"));
  hmput(event_hash, SDL_EVENT_JOYSTICK_BUTTON_DOWN, JS_NewAtom(js, "joystick_button_down"));
  hmput(event_hash, SDL_EVENT_JOYSTICK_BUTTON_UP, JS_NewAtom(js, "joystick_button_up"));
  hmput(event_hash, SDL_EVENT_JOYSTICK_ADDED, JS_NewAtom(js, "joystick_added"));
  hmput(event_hash, SDL_EVENT_JOYSTICK_REMOVED, JS_NewAtom(js, "joystick_removed"));
  hmput(event_hash, SDL_EVENT_JOYSTICK_BATTERY_UPDATED, JS_NewAtom(js, "joystick_battery_updated"));
  hmput(event_hash, SDL_EVENT_JOYSTICK_UPDATE_COMPLETE, JS_NewAtom(js, "joystick_update_complete"));

  // Gamepad events
  hmput(event_hash, SDL_EVENT_GAMEPAD_AXIS_MOTION, JS_NewAtom(js, "gamepad_axis_motion"));
  hmput(event_hash, SDL_EVENT_GAMEPAD_BUTTON_DOWN, JS_NewAtom(js, "gamepad_button_down"));
  hmput(event_hash, SDL_EVENT_GAMEPAD_BUTTON_UP, JS_NewAtom(js, "gamepad_button_up"));
  hmput(event_hash, SDL_EVENT_GAMEPAD_ADDED, JS_NewAtom(js, "gamepad_added"));
  hmput(event_hash, SDL_EVENT_GAMEPAD_REMOVED, JS_NewAtom(js, "gamepad_removed"));
  hmput(event_hash, SDL_EVENT_GAMEPAD_REMAPPED, JS_NewAtom(js, "gamepad_remapped"));
  hmput(event_hash, SDL_EVENT_GAMEPAD_TOUCHPAD_DOWN, JS_NewAtom(js, "gamepad_touchpad_down"));
  hmput(event_hash, SDL_EVENT_GAMEPAD_TOUCHPAD_MOTION, JS_NewAtom(js, "gamepad_touchpad_motion"));
  hmput(event_hash, SDL_EVENT_GAMEPAD_TOUCHPAD_UP, JS_NewAtom(js, "gamepad_touchpad_up"));
  hmput(event_hash, SDL_EVENT_GAMEPAD_SENSOR_UPDATE, JS_NewAtom(js, "gamepad_sensor_update"));
  hmput(event_hash, SDL_EVENT_GAMEPAD_UPDATE_COMPLETE, JS_NewAtom(js, "gamepad_update_complete"));
  hmput(event_hash, SDL_EVENT_GAMEPAD_STEAM_HANDLE_UPDATED, JS_NewAtom(js, "gamepad_steam_handle_updated"));

  // Touch events
  hmput(event_hash, SDL_EVENT_FINGER_DOWN, JS_NewAtom(js, "finger_down"));
  hmput(event_hash, SDL_EVENT_FINGER_UP, JS_NewAtom(js, "finger_up"));
  hmput(event_hash, SDL_EVENT_FINGER_MOTION, JS_NewAtom(js, "finger_motion"));

  // Clipboard events
  hmput(event_hash, SDL_EVENT_CLIPBOARD_UPDATE, JS_NewAtom(js, "clipboard_update"));

  // Drag and drop events
  hmput(event_hash, SDL_EVENT_DROP_FILE, JS_NewAtom(js, "drop_file"));
  hmput(event_hash, SDL_EVENT_DROP_TEXT, JS_NewAtom(js, "drop_text"));
  hmput(event_hash, SDL_EVENT_DROP_BEGIN, JS_NewAtom(js, "drop_begin"));
  hmput(event_hash, SDL_EVENT_DROP_COMPLETE, JS_NewAtom(js, "drop_complete"));
  hmput(event_hash, SDL_EVENT_DROP_POSITION, JS_NewAtom(js, "drop_position"));

  // Audio device events
  hmput(event_hash, SDL_EVENT_AUDIO_DEVICE_ADDED, JS_NewAtom(js, "audio_device_added"));
  hmput(event_hash, SDL_EVENT_AUDIO_DEVICE_REMOVED, JS_NewAtom(js, "audio_device_removed"));
  hmput(event_hash, SDL_EVENT_AUDIO_DEVICE_FORMAT_CHANGED, JS_NewAtom(js, "audio_device_format_changed"));

  // Sensor events
  hmput(event_hash, SDL_EVENT_SENSOR_UPDATE, JS_NewAtom(js, "sensor_update"));

  // Pen events
  hmput(event_hash, SDL_EVENT_PEN_PROXIMITY_IN, JS_NewAtom(js, "pen_proximity_in"));
  hmput(event_hash, SDL_EVENT_PEN_PROXIMITY_OUT, JS_NewAtom(js, "pen_proximity_out"));
  hmput(event_hash, SDL_EVENT_PEN_DOWN, JS_NewAtom(js, "pen_down"));
  hmput(event_hash, SDL_EVENT_PEN_UP, JS_NewAtom(js, "pen_up"));
  hmput(event_hash, SDL_EVENT_PEN_BUTTON_DOWN, JS_NewAtom(js, "pen_button_down"));
  hmput(event_hash, SDL_EVENT_PEN_BUTTON_UP, JS_NewAtom(js, "pen_button_up"));
  hmput(event_hash, SDL_EVENT_PEN_MOTION, JS_NewAtom(js, "pen_motion"));
  hmput(event_hash, SDL_EVENT_PEN_AXIS, JS_NewAtom(js, "pen_axis"));

  // Camera events
  hmput(event_hash, SDL_EVENT_CAMERA_DEVICE_ADDED, JS_NewAtom(js, "camera_device_added"));
  hmput(event_hash, SDL_EVENT_CAMERA_DEVICE_REMOVED, JS_NewAtom(js, "camera_device_removed"));
  hmput(event_hash, SDL_EVENT_CAMERA_DEVICE_APPROVED, JS_NewAtom(js, "camera_device_approved"));
  hmput(event_hash, SDL_EVENT_CAMERA_DEVICE_DENIED, JS_NewAtom(js, "camera_device_denied"));

  // Render events
  hmput(event_hash, SDL_EVENT_RENDER_TARGETS_RESET, JS_NewAtom(js, "render_targets_reset"));
  hmput(event_hash, SDL_EVENT_RENDER_DEVICE_RESET, JS_NewAtom(js, "render_device_reset"));
  hmput(event_hash, SDL_EVENT_RENDER_DEVICE_LOST, JS_NewAtom(js, "render_device_lost"));
}

static JSAtom mouse2atom(JSContext *js, int mouse)
{
  switch(mouse) {
    case SDL_BUTTON_LEFT: return JS_NewAtom(js,"left");
    case SDL_BUTTON_MIDDLE: return JS_NewAtom(js,"middle");
    case SDL_BUTTON_RIGHT: return JS_NewAtom(js,"right");
    case SDL_BUTTON_X1: return JS_NewAtom(js,"x1");
    case SDL_BUTTON_X2: return JS_NewAtom(js,"x2");
  }
  return JS_NewAtom(js,"left");
}

static JSValue js_keymod(JSContext *js)
{
  SDL_Keymod modstate = SDL_GetModState();
  JSValue ret = JS_NewObject(js);
  if (SDL_KMOD_CTRL & modstate)
    JS_SetPropertyStr(js,ret,"ctrl", JS_NewBool(js,1));
  if (SDL_KMOD_SHIFT & modstate)
    JS_SetPropertyStr(js,ret,"shift", JS_NewBool(js,1));
  if (SDL_KMOD_ALT & modstate)
    JS_SetPropertyStr(js,ret,"alt", JS_NewBool(js,1));
  if (SDL_KMOD_GUI & modstate)
    JS_SetPropertyStr(js,ret,"super", JS_NewBool(js,1));
  if (SDL_KMOD_NUM & modstate)
    JS_SetPropertyStr(js,ret,"numlock", JS_NewBool(js,1));
  if (SDL_KMOD_CAPS & modstate)
    JS_SetPropertyStr(js,ret,"caps", JS_NewBool(js,1));
  if (SDL_KMOD_SCROLL & modstate)
    JS_SetPropertyStr(js,ret,"scrolllock", JS_NewBool(js,1));
  if (SDL_KMOD_MODE & modstate)
    JS_SetPropertyStr(js,ret,"mode", JS_NewBool(js,1));

  return ret;
}

static JSValue event2js(JSContext *js, SDL_Event event)
{
  JSValue e = JS_NewObject(js);
  JS_SetPropertyStr(js, e, "type", JS_AtomToString(js,hmget(event_hash, event.type)));
  JS_SetPropertyStr(js,e,"timestamp", number2js(js,event.common.timestamp));

  switch(event.type) {
    case SDL_EVENT_AUDIO_DEVICE_ADDED:
    case SDL_EVENT_AUDIO_DEVICE_REMOVED:
      JS_SetPropertyStr(js,e,"which", number2js(js,event.adevice.which));
      JS_SetPropertyStr(js,e,"recording", JS_NewBool(js,event.adevice.recording));
      break;
    case SDL_EVENT_DISPLAY_ORIENTATION:
    case SDL_EVENT_DISPLAY_ADDED:
    case SDL_EVENT_DISPLAY_REMOVED:
    case SDL_EVENT_DISPLAY_MOVED:
    case SDL_EVENT_DISPLAY_DESKTOP_MODE_CHANGED:
    case SDL_EVENT_DISPLAY_CURRENT_MODE_CHANGED:
    case SDL_EVENT_DISPLAY_CONTENT_SCALE_CHANGED:
      JS_SetPropertyStr(js,e,"which", number2js(js,event.display.displayID));
      JS_SetPropertyStr(js,e,"data1", number2js(js,event.display.data1));
      JS_SetPropertyStr(js,e,"data2", number2js(js,event.display.data2));
      break;
    case SDL_EVENT_MOUSE_MOTION:
      JS_SetPropertyStr(js,e,"window", number2js(js,event.motion.windowID));
      JS_SetPropertyStr(js,e,"which", number2js(js,event.motion.which));
      JS_SetPropertyStr(js, e, "state", number2js(js,event.motion.state));
      JS_SetPropertyStr(js,e, "pos", vec22js(js,(HMM_Vec2){event.motion.x,event.motion.y}));
      JS_SetPropertyStr(js,e,"d_pos", vec22js(js,(HMM_Vec2){event.motion.xrel, event.motion.yrel}));
      break;
    case SDL_EVENT_MOUSE_WHEEL:
      JS_SetPropertyStr(js,e,"window", number2js(js,event.wheel.windowID));
      JS_SetPropertyStr(js,e,"which", number2js(js,event.wheel.which));
      JS_SetPropertyStr(js,e,"scroll", vec22js(js,(HMM_Vec2){event.wheel.x,event.wheel.y}));
      JS_SetPropertyStr(js,e, "mouse", vec22js(js,(HMM_Vec2){event.wheel.mouse_x,event.wheel.mouse_y}));
      break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
      JS_SetPropertyStr(js,e,"window", number2js(js,event.button.windowID));
      JS_SetPropertyStr(js,e,"which", number2js(js,event.button.which));
      JS_SetPropertyStr(js,e,"down", JS_NewBool(js,event.button.down));
      JS_SetPropertyStr(js,e,"button", JS_AtomToString(js,mouse2atom(js,event.button.button)));
      JS_SetPropertyStr(js,e,"clicks", number2js(js,event.button.clicks));
      JS_SetPropertyStr(js,e,"mouse", vec22js(js,(HMM_Vec2){event.button.x,event.button.y}));
      break;
    case SDL_EVENT_SENSOR_UPDATE:
      JS_SetPropertyStr(js,e,"which", number2js(js,event.sensor.which));
      JS_SetPropertyStr(js,e, "sensor_timestamp", number2js(js,event.sensor.sensor_timestamp));
      break;
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
      JS_SetPropertyStr(js,e,"window", number2js(js,event.key.windowID));
      JS_SetPropertyStr(js,e,"which", number2js(js,event.key.which));
      JS_SetPropertyStr(js,e,"down", JS_NewBool(js,event.key.down));
      JS_SetPropertyStr(js,e,"repeat", JS_NewBool(js,event.key.repeat));
      JS_SetPropertyStr(js,e,"key", number2js(js,event.key.key));
      JS_SetPropertyStr(js,e,"scancode", number2js(js,event.key.scancode));
      JS_SetPropertyStr(js,e,"mod", js_keymod(js));
      break;
    case SDL_EVENT_FINGER_MOTION:
    case SDL_EVENT_FINGER_DOWN:
    case SDL_EVENT_FINGER_UP:
      JS_SetPropertyStr(js,e,"touch", number2js(js,event.tfinger.touchID));
      JS_SetPropertyStr(js,e,"finger", number2js(js,event.tfinger.fingerID));
      JS_SetPropertyStr(js,e,"pos", vec22js(js, (HMM_Vec2){event.tfinger.x, event.tfinger.y}));
      JS_SetPropertyStr(js,e,"d_pos", vec22js(js,(HMM_Vec2){event.tfinger.x, event.tfinger.dy}));
      JS_SetPropertyStr(js,e,"pressure", number2js(js,event.tfinger.pressure));
      JS_SetPropertyStr(js,e,"window", number2js(js,event.key.windowID));
      break;
    case SDL_EVENT_DROP_BEGIN:
    case SDL_EVENT_DROP_FILE:
    case SDL_EVENT_DROP_TEXT:
    case SDL_EVENT_DROP_COMPLETE:
    case SDL_EVENT_DROP_POSITION:
      JS_SetPropertyStr(js,e,"window", number2js(js,event.drop.windowID));
      JS_SetPropertyStr(js,e,"pos", vec22js(js, (HMM_Vec2){event.drop.x,event.drop.y}));
      JS_SetPropertyStr(js,e,"data", JS_NewString(js,event.drop.data));
      JS_SetPropertyStr(js,e,"source",JS_NewString(js,event.drop.source));
      break;
    case SDL_EVENT_TEXT_INPUT:
      JS_SetPropertyStr(js,e,"window", number2js(js,event.text.windowID));
      JS_SetPropertyStr(js,e,"text", JS_NewString(js,event.text.text));
      JS_SetPropertyStr(js,e,"mod", js_keymod(js));
      break;
    case SDL_EVENT_CAMERA_DEVICE_APPROVED:
    case SDL_EVENT_CAMERA_DEVICE_REMOVED:
    case SDL_EVENT_CAMERA_DEVICE_ADDED:
    case SDL_EVENT_CAMERA_DEVICE_DENIED:
      JS_SetPropertyStr(js, e, "which", number2js(js,event.cdevice.which));
      break;
    case SDL_EVENT_CLIPBOARD_UPDATE:
      JS_SetPropertyStr(js, e, "owner", JS_NewBool(js,event.clipboard.owner));
      break;
    case SDL_EVENT_WINDOW_SHOWN:
    case SDL_EVENT_WINDOW_HIDDEN:
    case SDL_EVENT_WINDOW_EXPOSED:
    case SDL_EVENT_WINDOW_MOVED:
    case SDL_EVENT_WINDOW_RESIZED:
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
    case SDL_EVENT_WINDOW_METAL_VIEW_RESIZED:
    case SDL_EVENT_WINDOW_MINIMIZED:
    case SDL_EVENT_WINDOW_MAXIMIZED:
    case SDL_EVENT_WINDOW_RESTORED:
    case SDL_EVENT_WINDOW_MOUSE_ENTER:
    case SDL_EVENT_WINDOW_MOUSE_LEAVE:
    case SDL_EVENT_WINDOW_FOCUS_GAINED:
    case SDL_EVENT_WINDOW_FOCUS_LOST:
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
    case SDL_EVENT_WINDOW_HIT_TEST:
    case SDL_EVENT_WINDOW_ICCPROF_CHANGED:
    case SDL_EVENT_WINDOW_DISPLAY_CHANGED:
    case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
    case SDL_EVENT_WINDOW_SAFE_AREA_CHANGED:
    case SDL_EVENT_WINDOW_OCCLUDED:
    case SDL_EVENT_WINDOW_ENTER_FULLSCREEN:
    case SDL_EVENT_WINDOW_LEAVE_FULLSCREEN:
    case SDL_EVENT_WINDOW_DESTROYED:
    case SDL_EVENT_WINDOW_HDR_STATE_CHANGED:      
    /* rest of SDL_EVENT_WINDOW_ here */
      JS_SetPropertyStr(js,e,"which", number2js(js, event.window.windowID));
      JS_SetPropertyStr(js,e,"data1", number2js(js, event.window.data1));
      JS_SetPropertyStr(js,e,"data2", number2js(js, event.window.data2));
      break;

    case SDL_EVENT_JOYSTICK_ADDED:
    case SDL_EVENT_JOYSTICK_REMOVED:
    case SDL_EVENT_JOYSTICK_UPDATE_COMPLETE:
      JS_SetPropertyStr(js,e,"which", number2js(js,event.jdevice.which));
      break;
    case SDL_EVENT_JOYSTICK_AXIS_MOTION:
      JS_SetPropertyStr(js,e,"which", number2js(js,event.jaxis.which));
      JS_SetPropertyStr(js,e,"axis", number2js(js,event.jaxis.axis));
      JS_SetPropertyStr(js,e,"value", number2js(js,event.jaxis.value));
      break;
    case SDL_EVENT_JOYSTICK_BALL_MOTION:
      JS_SetPropertyStr(js,e,"which", number2js(js,event.jball.which));
      JS_SetPropertyStr(js,e,"ball",number2js(js,event.jball.ball));
      JS_SetPropertyStr(js,e, "rel", vec22js(js,(HMM_Vec2){event.jball.xrel,event.jball.yrel}));
      break;
    case SDL_EVENT_JOYSTICK_BUTTON_DOWN:
    case SDL_EVENT_JOYSTICK_BUTTON_UP:
      JS_SetPropertyStr(js,e,"which", number2js(js,event.jbutton.which));
      JS_SetPropertyStr(js,e,"button", number2js(js,event.jbutton.button));
      JS_SetPropertyStr(js,e,"down", JS_NewBool(js,event.jbutton.down));
      break;

    case SDL_EVENT_GAMEPAD_ADDED:
    case SDL_EVENT_GAMEPAD_REMOVED:
    case SDL_EVENT_GAMEPAD_REMAPPED:
    case SDL_EVENT_GAMEPAD_UPDATE_COMPLETE:
    case SDL_EVENT_GAMEPAD_STEAM_HANDLE_UPDATED:
      JS_SetPropertyStr(js,e,"which", number2js(js,event.gdevice.which));
      break;
    case SDL_EVENT_GAMEPAD_AXIS_MOTION:
      JS_SetPropertyStr(js,e,"which", number2js(js,event.gaxis.which));
      JS_SetPropertyStr(js,e,"axis", number2js(js,event.gaxis.axis));
      JS_SetPropertyStr(js,e,"value", number2js(js,event.gaxis.value));
      break;
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
    case SDL_EVENT_GAMEPAD_BUTTON_UP:
      JS_SetPropertyStr(js,e,"which", number2js(js,event.gbutton.which));
      JS_SetPropertyStr(js,e,"button", number2js(js,event.gbutton.button));
      JS_SetPropertyStr(js,e,"down", JS_NewBool(js,event.gbutton.down));
      break;
    case SDL_EVENT_GAMEPAD_TOUCHPAD_DOWN:
    case SDL_EVENT_GAMEPAD_TOUCHPAD_MOTION:
    case SDL_EVENT_GAMEPAD_TOUCHPAD_UP:
      JS_SetPropertyStr(js,e,"which", number2js(js,event.gtouchpad.which));
      JS_SetPropertyStr(js,e,"touchpad", number2js(js,event.gtouchpad.touchpad));
      JS_SetPropertyStr(js,e,"finger", number2js(js,event.gtouchpad.finger));
      JS_SetPropertyStr(js,e,"pos", vec22js(js,(HMM_Vec2){event.gtouchpad.x,event.gtouchpad.y}));
      JS_SetPropertyStr(js,e,"pressure", number2js(js,event.gtouchpad.pressure));
      break;
    case SDL_EVENT_GAMEPAD_SENSOR_UPDATE:
      JS_SetPropertyStr(js,e,"which", number2js(js,event.gsensor.which));
      JS_SetPropertyStr(js,e,"sensor", number2js(js,event.gsensor.sensor));
      JS_SetPropertyStr(js,e,"sensor_timestamp", number2js(js,event.gsensor.sensor_timestamp));
      break;
    case SDL_EVENT_USER:
      JS_SetPropertyStr(js,e,"cb", JS_DupValue(js,*(JSValue*)event.user.data1));
      JS_FreeValue(js,*(JSValue*)event.user.data1);
      break;
  }
  return e;
}

void gui_input(SDL_Event *e);
// Polls and handles all input events
JSC_CCALL(game_engine_input,
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
#ifndef NEDITOR
    gui_input(&event);
#endif
    JSValue e = event2js(js,event);
    JS_Call(js,argv[0], JS_UNDEFINED, 1, &e);
  }
)

JSC_CCALL(game_engine_delay,
  SDL_Delay(js2number(js,argv[0]));
)

JSC_CCALL(game_renderers,
  int num = SDL_GetNumRenderDrivers();
  JSValue arr = JS_NewArray(js);
  for (int i = 0; i < num; i++)
    JS_SetPropertyUint32(js, arr, i, JS_NewString(js, SDL_GetRenderDriver(i)));

  return arr;
)

JSC_CCALL(game_cameras,
  int num;
  SDL_CameraID *ids = SDL_GetCameras(&num);
  if (num == 0) return JS_UNDEFINED;
  JSValue jsids = JS_NewArray(js);
  for (int i = 0; i < num; i++)
    JS_SetPropertyUint32(js,jsids, i, number2js(js,ids[i]));

  return jsids;
)

JSC_CCALL(game_open_camera,
  int id = js2number(js,argv[0]);
  SDL_Camera *cam = SDL_OpenCamera(id, NULL);
  if (!cam) ret = JS_ThrowReferenceError(js, "Could not open camera %d: %s\n", id, SDL_GetError());
  else
    ret = SDL_Camera2js(js,cam);
)

#include "wildmatch.h"
JSC_SSCALL(game_glob,
  if (wildmatch(str, str2, WM_PATHNAME | WM_PERIOD | WM_WILDSTAR) == WM_MATCH)
    ret = JS_NewBool(js,1);
  else
    ret = JS_NewBool(js, 0);
)

JSC_CCALL(game_camera_name,
  const char *name = SDL_GetCameraName(js2number(js,argv[0]));
  if (!name) return JS_ThrowReferenceError(js, "Could not get camera name from id %d.", (int)js2number(js,argv[0]));
  
  return JS_NewString(js, name);
)

JSC_CCALL(game_camera_position,
  SDL_CameraPosition pos = SDL_GetCameraPosition(js2number(js,argv[0]));
  switch(pos) {
    case SDL_CAMERA_POSITION_UNKNOWN: return JS_NewString(js,"unknown");
    case SDL_CAMERA_POSITION_FRONT_FACING: return JS_NewString(js,"front");
    case SDL_CAMERA_POSITION_BACK_FACING: return JS_NewString(js,"back");
  } 
)

static const JSCFunctionListEntry js_game_funcs[] = {
  MIST_FUNC_DEF(game, engine_start, 1),
  MIST_FUNC_DEF(game, engine_input,1),
  MIST_FUNC_DEF(game, engine_delay, 1),
  MIST_FUNC_DEF(game, renderers, 0),
  MIST_FUNC_DEF(game, cameras, 0),
  MIST_FUNC_DEF(game, open_camera, 1),
  MIST_FUNC_DEF(game, camera_name,1),
  MIST_FUNC_DEF(game, camera_position,1),
  MIST_FUNC_DEF(game, glob, 2),
};

JSC_SCALL(SDL_Window_make_renderer,
  SDL_Window *win = js2SDL_Window(js,self);
  SDL_PropertiesID props = SDL_CreateProperties();
  SDL_SetNumberProperty(props, SDL_PROP_RENDERER_CREATE_PRESENT_VSYNC_NUMBER, 0);
  SDL_SetPointerProperty(props, SDL_PROP_RENDERER_CREATE_WINDOW_POINTER, win);
  SDL_SetStringProperty(props, SDL_PROP_RENDERER_CREATE_NAME_STRING, str);
  SDL_Renderer *r = SDL_CreateRendererWithProperties(props);
  SDL_DestroyProperties(props);
  if (!r) return JS_ThrowReferenceError(js, "Error creating renderer: %s",SDL_GetError());
  SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
  return SDL_Renderer2js(js,r);
)

JSC_SCALL(SDL_Window_make_gpu,
  SDL_Window *win = js2SDL_Window(js,self);
  SDL_GPUDevice *gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL, 1, NULL);
  return SDL_GPUDevice2js(js,gpu);
)

JSC_CCALL(SDL_Window_fullscreen,
  SDL_SetWindowFullscreen(js2SDL_Window(js,self), SDL_WINDOW_FULLSCREEN)
)

JSValue js_SDL_Window_keyboard_shown(JSContext *js, JSValue self) {
  SDL_Window *window = js2SDL_Window(js,self);
  return JS_NewBool(js,SDL_ScreenKeyboardShown(window));
}

JSValue js_window_theme(JSContext *js, JSValue self)
{
  return JS_UNDEFINED;
}

JSValue js_window_safe_area(JSContext *js, JSValue self)
{
  SDL_Window *w = js2SDL_Window(js,self);
  rect r;
  SDL_GetWindowSafeArea(w, &r);
  return rect2js(js,r);
}

JSValue js_window_bordered(JSContext *js, JSValue self, int argc, JSValue *argv)
{
  SDL_Window *w = js2SDL_Window(js,self);
  SDL_SetWindowBordered(w, JS_ToBool(js,argv[0]));
  return JS_UNDEFINED;
}

JSValue js_window_get_title(JSContext *js, JSValue self)
{
  SDL_Window *w = js2SDL_Window(js,self);
  const char *title = SDL_GetWindowTitle(w);
  return JS_NewString(js,title);
  
}

JSValue js_window_set_title(JSContext *js, JSValue self, JSValue val)
{
  SDL_Window *w = js2SDL_Window(js,self);
  const char *title = JS_ToCString(js,val);
  SDL_SetWindowTitle(w,title);
  JS_FreeCString(js,title);
  return JS_UNDEFINED;
}

JSValue js_window_get_size(JSContext *js, JSValue self)
{
  SDL_Window *win = js2SDL_Window(js,self);
  int w, h;
  SDL_GetWindowSize(win, &w, &h);
  return vec22js(js, (HMM_Vec2){w,h});
}

JSValue js_window_set_size(JSContext *js, JSValue self, JSValue val)
{
  SDL_Window *w = js2SDL_Window(js,self);
  HMM_Vec2 size = js2vec2(js,val);
  SDL_SetWindowSize(w,size.x,size.y);
}

JSValue js_window_set_icon(JSContext *js, JSValue self, int argc, JSValue *argv)
{
  SDL_Window *w = js2SDL_Window(js,self);
  SDL_Surface *s = js2SDL_Surface(js,argv[0]);
  if (!SDL_SetWindowIcon(w,s))
    return JS_ThrowReferenceError(js, "could not set window icon: %s", SDL_GetError());
}

JSValue js_window_mouse_grab(JSContext *js, JSValue self, int argc, JSValue *argv)
{
  SDL_Window *w = js2SDL_Window(js,self);
  SDL_SetWindowMouseGrab(w, JS_ToBool(js,argv[0]));
  
}

static const JSCFunctionListEntry js_SDL_Window_funcs[] = {
  MIST_FUNC_DEF(SDL_Window, fullscreen, 0),
  MIST_FUNC_DEF(SDL_Window, make_renderer, 1),
  MIST_FUNC_DEF(SDL_Window, make_gpu, 2),
  MIST_FUNC_DEF(SDL_Window, keyboard_shown, 0),
  MIST_FUNC_DEF(window, theme, 0),
  MIST_FUNC_DEF(window, safe_area, 0),
  MIST_FUNC_DEF(window, bordered, 1),
  MIST_FUNC_DEF(window, set_icon, 1),
  CGETSET_ADD(window, title),
  CGETSET_ADD(window, size),
  MIST_FUNC_DEF(window, mouse_grab, 1),
};

JSC_CCALL(SDL_Renderer_clear,
  SDL_Renderer *renderer = js2SDL_Renderer(js,self);
  SDL_RenderClear(renderer);
)

const char *tn = "present thread";
static int present_thread(SDL_Renderer *r)
{
  TracyCZone(present,1)
  SDL_RenderPresent(r);
  TracyCZoneEnd(present)
  return 0;
}

JSC_CCALL(SDL_Renderer_present,
//  SDL_Thread *thread;
  SDL_Renderer *ren = js2SDL_Renderer(js,self);
//  thread  = SDL_CreateThread(present_thread, "present", ren);
  SDL_RenderPresent(ren);
//  return SDL_Thread2js(js,thread);
)

JSC_CCALL(SDL_Renderer_draw_color,
  SDL_Renderer *renderer = js2SDL_Renderer(js,self);
  colorf color = js2color(js,argv[0]);
  SDL_SetRenderDrawColorFloat(renderer, color.r,color.g,color.b,color.a);
)

JSC_CCALL(SDL_Renderer_rect,
  SDL_Renderer *r = js2SDL_Renderer(js,self);
  if (!JS_IsUndefined(argv[1])) {
    colorf color = js2color(js,argv[1]);  
    SDL_SetRenderDrawColorFloat(r, color.r, color.g, color.b, color.a);
  }

  if (JS_IsArray(js,argv[0])) {
    int len = js_arrlen(js,argv[0]);
    rect rects[len];
    for (int i = 0; i < len; i++) {
      JSValue val = JS_GetPropertyUint32(js,argv[0],i);
      rects[i] = transform_rect(r,js2rect(js,val), &cam_mat);
      JS_FreeValue(js,val);
    }
    SDL_RenderRects(r,rects,len);
    return JS_UNDEFINED;
  }
  
  rect rect = js2rect(js,argv[0]);
  
  rect = transform_rect(r,rect, &cam_mat);
  
  SDL_RenderRect(r, &rect);
)

JSC_CCALL(renderer_load_texture,
  SDL_Renderer *r = js2SDL_Renderer(js,self);
  SDL_Surface *surf = js2SDL_Surface(js,argv[0]);
  if (!surf) return JS_ThrowReferenceError(js, "Surface was not a surface.");
  SDL_Texture *tex = SDL_CreateTextureFromSurface(r,surf);
  if (!tex) return JS_ThrowReferenceError(js, "Could not create texture from surface: %s", SDL_GetError());
  ret = SDL_Texture2js(js,tex);
  JS_SetProperty(js,ret,width_atom, number2js(js,tex->w));
  JS_SetProperty(js,ret,height_atom, number2js(js,tex->h));
)

JSC_CCALL(SDL_Renderer_fillrect,
  SDL_Renderer *r = js2SDL_Renderer(js,self);
  if (!JS_IsUndefined(argv[1])) {
    colorf color = js2color(js,argv[1]);  
    SDL_SetRenderDrawColorFloat(r, color.r, color.g, color.b, color.a);
  }

  if (JS_IsArray(js,argv[0])) {
    int len = js_arrlen(js,argv[0]);
    rect rects[len];
    for (int i = 0; i < len; i++) {
      JSValue val = JS_GetPropertyUint32(js,argv[0],i);
      rects[i] = js2rect(js,val);
      JS_FreeValue(js,val);
    }
    if (!SDL_RenderFillRects(r,rects,len))
      return JS_ThrowReferenceError("Could not render rectangle: %s", SDL_GetError());
  }
  rect rect = transform_rect(r,js2rect(js,argv[0]),&cam_mat);
  
  if (!SDL_RenderFillRect(r, &rect))
    return JS_ThrowReferenceError("Could not render rectangle: %s", SDL_GetError());
)

JSC_CCALL(renderer_texture,
  SDL_Renderer *renderer = js2SDL_Renderer(js,self);
  SDL_Texture *tex = js2SDL_Texture(js,argv[0]);
  rect dst = transform_rect(renderer,js2rect(js,argv[1]), &cam_mat);
  
  if (!JS_IsUndefined(argv[3])) {
    colorf color = js2color(js,argv[3]);
    SDL_SetTextureColorModFloat(tex, color.r, color.g, color.b);
    SDL_SetTextureAlphaModFloat(tex,color.a);
  }
  if (JS_IsUndefined(argv[2]))
    SDL_RenderTexture(renderer,tex,NULL,&dst);
  else {

    rect src = js2rect(js,argv[2]);

    SDL_RenderTextureRotated(renderer, tex, &src, &dst, 0, NULL, SDL_FLIP_NONE);
  }
)

JSC_CCALL(renderer_tile,
  SDL_Renderer *renderer = js2SDL_Renderer(js,self);
  if (!renderer) return JS_ThrowTypeError(js,"self was not a renderer");
  SDL_Texture *tex = js2SDL_Texture(js,argv[0]);
  if (!tex) return JS_ThrowTypeError(js,"first argument was not a texture");
  rect dst = js2rect(js,argv[1]);
  if (!dst.w) dst.w = tex->w;
  if (!dst.h) dst.h = tex->h;
  float scale = js2number(js,argv[3]);
  if (!scale) scale = 1;
  if (JS_IsUndefined(argv[2]))
    SDL_RenderTextureTiled(renderer,tex,NULL,scale, &dst);
  else {
    rect src = js2rect(js,argv[2]);
    SDL_RenderTextureTiled(renderer,tex,&src,scale, &dst);
  }
)

JSC_CCALL(renderer_slice9,
  SDL_Renderer *renderer = js2SDL_Renderer(js,self);
  SDL_Texture *tex = js2SDL_Texture(js,argv[0]);
  lrtb bounds = js2lrtb(js,argv[2]);
  rect src, dst;
  src = transform_rect(renderer,js2rect(js,argv[3]),&cam_mat);
  dst = transform_rect(renderer,js2rect(js,argv[1]), &cam_mat);

  SDL_RenderTexture9Grid(renderer, tex,
    JS_IsUndefined(argv[3]) ? NULL : &src,
    bounds.l, bounds.r, bounds.t, bounds.b, 0.0,
    JS_IsUndefined(argv[1]) ? NULL : &dst);
)

JSC_CCALL(renderer_get_image,
  SDL_Renderer *r = js2SDL_Renderer(js,self);
  SDL_Surface *surf = NULL;
  if (!JS_IsUndefined(argv[0])) {
    rect rect = js2rect(js,argv[0]);
    surf = SDL_RenderReadPixels(r,&rect);
  } else
    surf = SDL_RenderReadPixels(r,NULL);
  if (!surf) return JS_ThrowReferenceError(js, "could not make surface from renderer");
  return SDL_Surface2js(js,surf);
)

JSC_SCALL(renderer_fasttext,
  SDL_Renderer *r = js2SDL_Renderer(js,self);
  if (!JS_IsUndefined(argv[2])) {
    colorf color = js2color(js,argv[2]);  
    SDL_SetRenderDrawColorFloat(r, color.r, color.g, color.b, color.a);
  }
  HMM_Vec2 pos = js2vec2(js,argv[1]);
  pos.y += 8;
  HMM_Vec2 tpos = HMM_MulM3V3(cam_mat, (HMM_Vec3){pos.x,pos.y,1}).xy;
  SDL_RenderDebugText(r, tpos.x, tpos.y, str);
)

JSC_CCALL(renderer_line,
  SDL_Renderer *r = js2SDL_Renderer(js,self);
  if (!JS_IsUndefined(argv[1])) {
    colorf color = js2color(js,argv[1]);  
    SDL_SetRenderDrawColorFloat(r, color.r, color.g, color.b, color.a);
  }

  if (JS_IsArray(js,argv[0])) {
    int len = js_arrlen(js,argv[0]);
    HMM_Vec2 points[len];
    assert(sizeof(HMM_Vec2) == sizeof(SDL_FPoint));
    for (int i = 0; i < len; i++) {
      JSValue val = JS_GetPropertyUint32(js,argv[0],i);
      points[i] = js2vec2(js,val);
      JS_FreeValue(js,val);
    }
    SDL_RenderLines(r,points,len);
  }
)

JSC_CCALL(renderer_point,
  SDL_Renderer *r = js2SDL_Renderer(js,self);
  if (!JS_IsUndefined(argv[1])) {
    colorf color = js2color(js,argv[1]);  
    SDL_SetRenderDrawColorFloat(r, color.r, color.g, color.b, color.a);
  }

  if (JS_IsArray(js,argv[0])) {
    int len = js_arrlen(js,argv[0]);
    HMM_Vec2 points[len];
    assert(sizeof(HMM_Vec2) ==sizeof(SDL_FPoint));
    for (int i = 0; i < len; i++) {
      JSValue val = JS_GetPropertyUint32(js,argv[0],i);
      points[i] = js2vec2(js,val);
      JS_FreeValue(js,val);
    }
    SDL_RenderPoints(r, points, len);
    return JS_UNDEFINED;
  }

  HMM_Vec2 point = transform_point(r, js2vec2(js,argv[0]), &cam_mat);
  SDL_RenderPoint(r,point.x,point.y);
)

// Function to translate a list of 2D points
void Translate2DPoints(HMM_Vec2 *points, int count, HMM_Vec3 position, HMM_Quat rotation, HMM_Vec3 scale) {
    // Precompute the 2D rotation matrix from the quaternion
    float xx = rotation.x * rotation.x;
    float yy = rotation.y * rotation.y;
    float zz = rotation.z * rotation.z;
    float xy = rotation.x * rotation.y;
    float zw = rotation.z * rotation.w;

    // Extract 2D affine rotation and scaling
    float m00 = (1.0f - 2.0f * (yy + zz)) * scale.x; // Row 1, Column 1
    float m01 = (2.0f * (xy + zw)) * scale.y;        // Row 1, Column 2
    float m10 = (2.0f * (xy - zw)) * scale.x;        // Row 2, Column 1
    float m11 = (1.0f - 2.0f * (xx + zz)) * scale.y; // Row 2, Column 2

    // Translation components (ignore the z position)
    float tx = position.x;
    float ty = position.y;

    // Transform each point
    for (int i = 0; i < count; ++i) {
        HMM_Vec2 p = points[i];
        points[i].x = m00 * p.x + m01 * p.y + tx;
        points[i].y = m10 * p.x + m11 * p.y + ty;
    }
}

// Should take a single struct with pos, color, uv, and indices arrays
JSC_CCALL(renderer_geometry,
  SDL_Renderer *r = js2SDL_Renderer(js,self);
  JSValue pos = JS_GetPropertyStr(js,argv[1], "pos");
  JSValue color = JS_GetPropertyStr(js,argv[1], "color");
  JSValue uv = JS_GetPropertyStr(js,argv[1], "uv");
  JSValue indices = JS_GetPropertyStr(js,argv[1], "indices");
  int vertices = js_getnum_str(js, argv[1], "vertices");
  int count = js_getnum_str(js, argv[1], "count");

  size_t pos_stride, indices_stride, uv_stride, color_stride;
  void *posdata = get_gpu_buffer(js,pos, &pos_stride, NULL);
  void *idxdata = get_gpu_buffer(js,indices, &indices_stride, NULL);
  void *uvdata = get_gpu_buffer(js,uv, &uv_stride, NULL);
  void *colordata = get_gpu_buffer(js,color,&color_stride, NULL);

  SDL_Texture *tex = js2SDL_Texture(js,argv[0]);

  HMM_Vec2 *trans_pos = malloc(vertices*sizeof(HMM_Vec2));
  memcpy(trans_pos,posdata, sizeof(HMM_Vec2)*vertices);

  for (int i = 0; i < vertices; i++)
    trans_pos[i] = HMM_MulM3V3(cam_mat, (HMM_Vec3){trans_pos[i].x, trans_pos[i].y, 1}).xy;
    
  if (!SDL_RenderGeometryRaw(r, tex, trans_pos, pos_stride,colordata,color_stride,uvdata, uv_stride, vertices, idxdata, count, indices_stride))
    ret = JS_ThrowReferenceError(js, "Error rendering geometry: %s",SDL_GetError());

    free(trans_pos);

  JS_FreeValue(js,pos);
  JS_FreeValue(js,color);
  JS_FreeValue(js,uv);
  JS_FreeValue(js,indices);
)

JSC_CCALL(renderer_logical_size,
  SDL_Renderer *r = js2SDL_Renderer(js,self);
  HMM_Vec2 v = js2vec2(js,argv[0]);
  SDL_SetRenderLogicalPresentation(r,v.x,v.y,SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
)

JSC_CCALL(renderer_viewport,
  SDL_Renderer *r = js2SDL_Renderer(js,self);
  if (JS_IsUndefined(argv[0]))
    SDL_SetRenderViewport(r,NULL);
  else {
    rect view = js2rect(js,argv[0]);
    SDL_SetRenderViewport(r,&view);
  }
)

JSC_CCALL(renderer_get_viewport,
  SDL_Renderer *r = js2SDL_Renderer(js,self);  
  SDL_Rect vp;
  SDL_GetRenderViewport(r, &vp);
  rect re;
  re.x = vp.x;
  re.y = vp.y;
  re.h = vp.h;
  re.w = vp.w;
  return rect2js(js,re);
)

JSC_CCALL(renderer_clip,
  SDL_Renderer *r = js2SDL_Renderer(js,self);
  if (JS_IsUndefined(argv[0]))
    SDL_SetRenderClipRect(r,NULL);
  else {
    rect view = js2rect(js,argv[0]);
    SDL_SetRenderClipRect(r,&view);
  }
)

JSC_CCALL(renderer_scale,
  SDL_Renderer *r = js2SDL_Renderer(js,self);
  HMM_Vec2 v = js2vec2(js,argv[0]);
  SDL_SetRenderScale(r, v.x, v.y);
)

JSC_CCALL(renderer_vsync,
  SDL_Renderer *r = js2SDL_Renderer(js,self);
  SDL_SetRenderVSync(r,js2number(js,argv[0]));
)

// This returns the coordinates inside the 
JSC_CCALL(renderer_coords,
  SDL_Renderer *r = js2SDL_Renderer(js,self);
  HMM_Vec2 pos, coord;
  pos = js2vec2(js,argv[0]);
  SDL_RenderCoordinatesFromWindow(r,pos.x,pos.y, &coord.x, &coord.y);
  return vec22js(js,coord);
)

JSC_CCALL(renderer_camera,
  int centered = JS_ToBool(js,argv[1]);
  SDL_Renderer *ren = js2SDL_Renderer(js,self);
  SDL_Rect vp;
  SDL_GetRenderViewport(ren, &vp);
  HMM_Mat3 proj;
  proj.Columns[0] = (HMM_Vec3){1,0,0};
  proj.Columns[1] = (HMM_Vec3){0,-1,0};
  if (centered)
    proj.Columns[2] = (HMM_Vec3){vp.w/2.0,vp.h/2.0,1};
  else
    proj.Columns[2] = (HMM_Vec3){0,vp.h,1};
    
  transform *tra = js2transform(js,argv[0]);  
  HMM_Mat3 view;
  view.Columns[0] = (HMM_Vec3){1,0,0};
  view.Columns[1] = (HMM_Vec3){0,1,0};
  view.Columns[2] = (HMM_Vec3){-tra->pos.x, -tra->pos.y,1};
  cam_mat = HMM_MulM3(proj,view);
)

JSC_CCALL(renderer_screen2world,
  HMM_Mat3 inv = HMM_InvGeneralM3(cam_mat);
  HMM_Vec3 pos = js2vec3(js,argv[0]);
  return vec22js(js, HMM_MulM3V3(inv, pos).xy);
)

JSC_CCALL(renderer_target,
  SDL_Renderer *r = js2SDL_Renderer(js,self);
  if (JS_IsUndefined(argv[0]))
    SDL_SetRenderTarget(r, NULL);
  else {
    SDL_Texture *tex = js2SDL_Texture(js,argv[0]);
    SDL_SetRenderTarget(r,tex);
  }
)

#include "cgltf.h"
#include <math.h>

#include "cgltf.h"
#include <math.h>

static void generate_normals(float* positions, size_t vertex_count, uint16_t* indices, size_t index_count, int16_t* normal_data) {
    // Initialize normals to zero
    float* temp_normals = malloc(sizeof(float)*3*vertex_count);
    for (size_t i = 0; i < vertex_count*3; i++) {
        temp_normals[i] = 0.0f;
    }

    // Compute face normals and accumulate
    for (size_t i = 0; i < index_count; i += 3) {
        uint16_t i0 = indices[i+0];
        uint16_t i1 = indices[i+1];
        uint16_t i2 = indices[i+2];

        float* p0 = &positions[i0*3];
        float* p1 = &positions[i1*3];
        float* p2 = &positions[i2*3];

        float ux = p1[0] - p0[0];
        float uy = p1[1] - p0[1];
        float uz = p1[2] - p0[2];

        float vx = p2[0] - p0[0];
        float vy = p2[1] - p0[1];
        float vz = p2[2] - p0[2];

        // Cross product (u x v)
        float nx = uy*vz - uz*vy;
        float ny = uz*vx - ux*vz;
        float nz = ux*vy - uy*vx;

        // Accumulate to each vertex normal
        temp_normals[i0*3+0] += nx; temp_normals[i0*3+1] += ny; temp_normals[i0*3+2] += nz;
        temp_normals[i1*3+0] += nx; temp_normals[i1*3+1] += ny; temp_normals[i1*3+2] += nz;
        temp_normals[i2*3+0] += nx; temp_normals[i2*3+1] += ny; temp_normals[i2*3+2] += nz;
    }

    // Normalize and pack
    for (size_t i = 0; i < vertex_count; i++) {
        float x = temp_normals[i*3+0];
        float y = temp_normals[i*3+1];
        float z = temp_normals[i*3+2];
        float length = sqrtf(x*x + y*y + z*z);
        if (length > 1e-6f) {
            x /= length; y /= length; z /= length;
        } else {
            x = 0; y = 0; z = 1; // arbitrary default
        }

        normal_data[i*3+0] = (int16_t)(x * 32767.0f);
        normal_data[i*3+1] = (int16_t)(y * 32767.0f);
        normal_data[i*3+2] = (int16_t)(z * 32767.0f);
    }

    free(temp_normals);
}

JSC_CCALL(gpu_load_gltf_model,
  const char* path = JS_ToCString(js, argv[0]);
  if (!path) {
    // Handle error
    return JS_EXCEPTION;
  }

  cgltf_options options = {0};
  cgltf_data* data = NULL;

  cgltf_result result = cgltf_parse_file(&options, path, &data);
  if (result != cgltf_result_success) {
    JS_FreeCString(js, path);
    return JS_EXCEPTION;
  }

  result = cgltf_load_buffers(&options, data, path);
  JS_FreeCString(js, path);
  if (result != cgltf_result_success) {
    cgltf_free(data);
    return JS_EXCEPTION;
  }

  if (data->meshes_count == 0 || data->meshes[0].primitives_count == 0) {
    cgltf_free(data);
    return JS_EXCEPTION;
  }

  cgltf_primitive* prim = &data->meshes[0].primitives[0];
  cgltf_accessor* indices_accessor = prim->indices;

  // Load indices first, so we have them for normal generation if needed.
  size_t index_count = 0;
  uint16_t* indices_data = NULL;
  if (indices_accessor) {
    index_count = indices_accessor->count;
    // Check if we can fit indices into uint16
    // For simplicity, assume we can (if index accessor max index > 65535, we must use bigger type)
    indices_data = malloc(sizeof(uint16_t)*index_count);
    for (size_t i = 0; i < index_count; i++) {
      size_t idx = cgltf_accessor_read_index(indices_accessor, i);
      indices_data[i] = (uint16_t)idx;
    }
  }

  // We'll fill these out as we find attributes
  float* positions = NULL;   size_t vertex_count = 0;
  int16_t* normals_packed = NULL;
  int have_normals = 0;
  uint16_t* uvs_packed = NULL;

  ret = JS_NewObject(js);

  // Iterate over attributes and handle them immediately
  for (size_t i = 0; i < prim->attributes_count; i++) {
    cgltf_attribute* attr = &prim->attributes[i];
    cgltf_accessor* accessor = attr->data;

    switch (attr->type) {
      case cgltf_attribute_type_position: {
        // Load positions immediately
        vertex_count = accessor->count;
        positions = malloc(sizeof(float)*3*vertex_count);
        for (size_t v = 0; v < vertex_count; v++) {
          cgltf_accessor_read_float(accessor, v, &positions[v*3], 3);
        }

        // Create a JS buffer for positions right now
        // Positions remain float32.
        JS_SetProperty(js, ret, pos_atom,
          make_gpu_buffer(js, positions, sizeof(float)*3*vertex_count, JS_TYPED_ARRAY_FLOAT32, 3, 0));

        break;
      }
      case cgltf_attribute_type_normal: {
        // Load normals and pack into int16 right away
        // If no positions yet, we must assume they come first or handle after we find positions.
        // We'll assume position found first as per instructions.
        have_normals = 1;
        normals_packed = malloc(sizeof(int16_t)*3*accessor->count);

        for (size_t v = 0; v < accessor->count; v++) {
          float n[3];
          cgltf_accessor_read_float(accessor, v, n, 3);
          // Normalize just in case (should be normalized anyway)
          float len = sqrtf(n[0]*n[0] + n[1]*n[1] + n[2]*n[2]);
          if (len > 1e-6f) { n[0]/=len; n[1]/=len; n[2]/=len; } else { n[0]=0; n[1]=0; n[2]=1; }

          normals_packed[v*3+0] = (int16_t)(n[0] * 32767.0f);
          normals_packed[v*3+1] = (int16_t)(n[1] * 32767.0f);
          normals_packed[v*3+2] = (int16_t)(n[2] * 32767.0f);
        }

        // Immediately store normals
        JS_SetProperty(js, ret, norm_atom,
          make_gpu_buffer(js, normals_packed, sizeof(int16_t)*3*accessor->count, JS_TYPED_ARRAY_INT16, 3, 1 /* normalized */));
        break;
      }
      case cgltf_attribute_type_texcoord: {
        // Assume texcoord_0 only
        if (attr->index == 0) {
          // Pack UVs into uint16 normalized
          uvs_packed = malloc(sizeof(uint16_t)*2*accessor->count);
          for (size_t v = 0; v < accessor->count; v++) {
            float uv[2];
            cgltf_accessor_read_float(accessor, v, uv, 2);
            // Clamp and convert to [0,1]
            float u = uv[0]; if (u < 0) u=0; if (u>1) u=1;
            float vv = uv[1]; if (vv < 0) vv=0; if (vv>1) vv=1;
            uvs_packed[v*2+0] = (uint16_t)(u * 65535.0f);
            uvs_packed[v*2+1] = (uint16_t)(vv * 65535.0f);
          }

          // Immediately store UVs
          JS_SetProperty(js, ret, uv_atom,
            make_gpu_buffer(js, uvs_packed, sizeof(uint16_t)*2*accessor->count, JS_TYPED_ARRAY_UINT16, 2, 1 /* normalized */));
        }
        break;
      }
      default:
        // Other attributes not handled here.
        break;
    }
  }

  // If we didn't find normals, generate them now.
  if (!have_normals) {
    if (!positions) {
      // Can't generate normals without positions
      cgltf_free(data);
      if (positions) free(positions);
      if (indices_data) free(indices_data);
      if (uvs_packed) free(uvs_packed);
      return JS_EXCEPTION;
    }

    // If we have no indices, we can generate normals in a non-indexed fashion,
    // but here we assume we have indices. If not, handle that case as needed.
    if (indices_data && index_count > 0) {
      normals_packed = malloc(sizeof(int16_t)*3*vertex_count);
      generate_normals(positions, vertex_count, indices_data, index_count, normals_packed);

      JS_SetProperty(js, ret, norm_atom,
        make_gpu_buffer(js, normals_packed, sizeof(int16_t)*3*vertex_count, JS_TYPED_ARRAY_INT16, 3, 1 /* normalized */));
    } else {
      // Non-indexed normal generation would go here if needed.
      // For simplicity, just create default normals (0,0,1)
      normals_packed = malloc(sizeof(int16_t)*3*vertex_count);
      for (size_t i = 0; i < vertex_count; i++) {
        normals_packed[i*3+0] = 0;
        normals_packed[i*3+1] = 0;
        normals_packed[i*3+2] = 32767; // (0,0,1)
      }
      JS_SetProperty(js, ret, norm_atom,
        make_gpu_buffer(js, normals_packed, sizeof(int16_t)*3*vertex_count, JS_TYPED_ARRAY_INT16, 3, 1));
    }
  }

  // Store indices if we have them
  if (indices_data && index_count > 0) {
    JS_SetProperty(js, ret, indices_atom,
      make_gpu_buffer(js, indices_data, sizeof(uint16_t)*index_count, JS_TYPED_ARRAY_UINT16, 1, 0));
  }

  // Store vertices and count
  // count is usually the number of indices if available, else vertex_count
  JS_SetProperty(js, ret, vertices_atom, number2js(js, vertex_count));
  JS_SetProperty(js, ret, count_atom, number2js(js, index_count > 0 ? index_count : vertex_count));

  // Cleanup
  free(positions);
  free(normals_packed);
  free(uvs_packed);
  free(indices_data);

  cgltf_free(data);

  return ret;
)

// Given an array of sprites, make the necessary geometry
// A sprite is expected to have:
// transform: a transform encoding position and rotation. its scale is in pixels - so a scale of 1 means the image will draw only on a single pixel.
// image: a standard prosperon image of a surface, rect, and texture
// color: the color this sprite should be hued by

// This might change depending on the backend, so best to not investigate. It should be consumed with "renderer_geometry"
// It might be a list of rectangles, it might be a handful of buffers, etc ..
JSC_CCALL(renderer_make_sprite_mesh,
  JSValue sprites = argv[0];
  JSValue old = argv[1];
  size_t quads = js_arrlen(js,argv[0]);
  size_t verts = quads*4;
  size_t count = quads*6;

  HMM_Vec2 *posdata = malloc(sizeof(*posdata)*verts);
  HMM_Vec2 *uvdata = malloc(sizeof(*uvdata)*verts);
  HMM_Vec4 *colordata = malloc(sizeof(*colordata)*quads);

  for (int i = 0; i < quads; i++) {
    JSValue sub = JS_GetPropertyUint32(js,sprites,i);
    JSValue jstransform = JS_GetProperty(js,sub,transform_atom);
    transform *tr = js2transform(js,jstransform);
    JSValue jssrc = JS_GetProperty(js,sub,src_atom);
    JSValue jscolor = JS_GetProperty(js,sub,color_atom);
    HMM_Vec4 color;

    rect src;
    if (JS_IsUndefined(jssrc))
      src = (rect){.x = 0, .y = 0, .w = 1, .h = 1};
    else
      src = js2rect(js,jssrc);
    
    if (JS_IsUndefined(jscolor))
      color = (HMM_Vec4){1,1,1,1};
    else
      color = js2vec4(js,jscolor);

    // Calculate the base index for the current quad
    size_t base = i * 4;

    HMM_Mat3 trmat = js2transform_mat3(js,jstransform);

    HMM_Vec3 base_quad[4] = {
      {0.0,0.0,1.0},
      {1.0,0.0,1.0},
      {0.0,1.0,1.0},
      {1.0,1.0,1.0}
    };
    for (int j = 0; j < 4; j++)
      posdata[base+j] = HMM_MulM3V3(trmat, base_quad[j]).xy;

    // Define the UV coordinates based on the source rectangle
    uvdata[base + 0] = (HMM_Vec2){ src.x,                  src.y + src.h };
    uvdata[base + 1] = (HMM_Vec2){ src.x + src.w,      src.y + src.h };    
    uvdata[base + 2] = (HMM_Vec2){ src.x,                  src.y };
    uvdata[base + 3] = (HMM_Vec2){ src.x + src.w,      src.y };

    colordata[i] = color;

    JS_FreeValue(js,sub);
    JS_FreeValue(js,jscolor);
    JS_FreeValue(js,jssrc);
  }

  ret = JS_NewObject(js);
  JS_SetProperty(js, ret, pos_atom, make_gpu_buffer(js, posdata, sizeof(*posdata) * verts, JS_TYPED_ARRAY_FLOAT32, 2, 0));
  JS_SetProperty(js, ret, uv_atom, make_gpu_buffer(js, uvdata, sizeof(*uvdata) * verts, JS_TYPED_ARRAY_FLOAT32, 2, 0));
  JS_SetProperty(js, ret, color_atom, make_gpu_buffer(js, colordata, sizeof(*colordata) * quads, JS_TYPED_ARRAY_FLOAT32, 0, 0));
  JS_SetProperty(js, ret, indices_atom, make_quad_indices_buffer(js, quads));
  JS_SetProperty(js, ret, vertices_atom, number2js(js, verts));
  JS_SetProperty(js, ret, count_atom, number2js(js, count));
)


static const JSCFunctionListEntry js_SDL_Renderer_funcs[] = {
  MIST_FUNC_DEF(SDL_Renderer, draw_color, 1),
  MIST_FUNC_DEF(SDL_Renderer, present, 0),
  MIST_FUNC_DEF(SDL_Renderer, clear, 0),
  MIST_FUNC_DEF(SDL_Renderer, rect, 2),
  MIST_FUNC_DEF(SDL_Renderer, fillrect, 2),
  MIST_FUNC_DEF(renderer, line, 2),
  MIST_FUNC_DEF(renderer, point, 2),
  MIST_FUNC_DEF(renderer, load_texture, 1),
  MIST_FUNC_DEF(renderer, texture, 4),
  MIST_FUNC_DEF(renderer, slice9, 4),
  MIST_FUNC_DEF(renderer, tile, 4),
  MIST_FUNC_DEF(renderer, get_image, 1),
  MIST_FUNC_DEF(renderer, fasttext, 2),
  MIST_FUNC_DEF(renderer, geometry, 2),
  MIST_FUNC_DEF(renderer, scale, 1),
  MIST_FUNC_DEF(renderer,logical_size,1),
  MIST_FUNC_DEF(renderer,viewport,1),
  MIST_FUNC_DEF(renderer,clip,1),
  MIST_FUNC_DEF(renderer,vsync,1),
  MIST_FUNC_DEF(renderer, coords, 1),
  MIST_FUNC_DEF(renderer, camera, 2),
  MIST_FUNC_DEF(renderer, get_viewport,0),
  MIST_FUNC_DEF(renderer, screen2world, 1),
  MIST_FUNC_DEF(renderer, target, 1),
  MIST_FUNC_DEF(renderer, make_sprite_mesh, 2),
};

// GPU API
JSC_CCALL(gpu_claim_window,
  SDL_GPUDevice *d = js2SDL_GPUDevice(js,self);
  SDL_Window *w = js2SDL_Window(js, argv[0]);
  SDL_ClaimWindowForGPUDevice(d,w);
)

JSC_CCALL(gpu_load_texture,
  SDL_GPUDevice *gpu = js2SDL_GPUDevice(js, self);
  SDL_Surface *surf = js2SDL_Surface(js, argv[0]);
  if (!surf) return JS_ThrowReferenceError(js, "Surface was not a surface.");

  int compression_level = 0;
  if (argc > 1)
    compression_level = JS_ToBool(js,argv[1]);

  compression_level = 2; // use DXT highqual if available

  int dofree = 0;
  
SDL_PixelFormat sfmt = surf->format;
const SDL_PixelFormatDetails *pdetails = SDL_GetPixelFormatDetails(sfmt);

if (!pdetails) {
    // If we can't get pixel format details, fall back to converting to RGBA8888
    surf = SDL_ConvertSurface(surf, SDL_PIXELFORMAT_RGBA8888);
    dofree = 1;
    sfmt = SDL_PIXELFORMAT_RGBA8888;
    pdetails = SDL_GetPixelFormatDetails(sfmt);
    if (!pdetails) {
        // Should never happen with RGBA8888, but just in case
        return JS_ThrowReferenceError(js, "Unable to get pixel format details.");
    }
}

// Check if format has alpha
bool has_alpha = (pdetails->Amask != 0);

// Choose a GPU format that closely matches the surface's format
SDL_GPUTextureFormat chosen_format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;

switch (sfmt) {
    case SDL_PIXELFORMAT_RGBA32:
    case SDL_PIXELFORMAT_ABGR32:
    case SDL_PIXELFORMAT_ARGB32:
    case SDL_PIXELFORMAT_BGRA32:
        // 8-8-8-8 format with alpha present
        chosen_format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        break;

    case SDL_PIXELFORMAT_RGBX8888:
    case SDL_PIXELFORMAT_XRGB8888:
    case SDL_PIXELFORMAT_BGRX8888:
    case SDL_PIXELFORMAT_XBGR8888:
        // 8-8-8-8 format no alpha
        chosen_format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        break;

    case SDL_PIXELFORMAT_RGBA4444:
    case SDL_PIXELFORMAT_ABGR4444:
    case SDL_PIXELFORMAT_ARGB4444:
    case SDL_PIXELFORMAT_BGRA4444:
        // 4-4-4-4 format
        chosen_format = SDL_GPU_TEXTUREFORMAT_B4G4R4A4_UNORM;
        break;

    case SDL_PIXELFORMAT_RGB565:
    case SDL_PIXELFORMAT_BGR565:
        // 5-6-5 format
        chosen_format = SDL_GPU_TEXTUREFORMAT_B5G6R5_UNORM;
        break;

    case SDL_PIXELFORMAT_RGBA5551:
    case SDL_PIXELFORMAT_ARGB1555:
    case SDL_PIXELFORMAT_BGRA5551:
    case SDL_PIXELFORMAT_ABGR1555:
        // 5-5-5-1 format
        chosen_format = SDL_GPU_TEXTUREFORMAT_B5G5R5A1_UNORM;
        break;

    default:
        // If no direct mapping, convert to RGBA8888
        // Note: If we reach here, we probably don't have a direct map
        surf = SDL_ConvertSurface(surf, SDL_PIXELFORMAT_RGBA8888);
        dofree = 1;
        sfmt = SDL_PIXELFORMAT_RGBA8888;
        pdetails = SDL_GetPixelFormatDetails(sfmt);
        chosen_format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        has_alpha = true;
        break;
}

// If compression_level > 0, we override format with BC1/BC3
bool compress = (compression_level > 0);
int stb_mode = STB_DXT_NORMAL;
SDL_GPUTextureFormat compressed_format = chosen_format;

if (compress) {
    if (has_alpha)
        compressed_format = SDL_GPU_TEXTUREFORMAT_BC3_RGBA_UNORM; // DXT5
    else
        compressed_format = SDL_GPU_TEXTUREFORMAT_BC1_RGBA_UNORM; // DXT1

    if (compression_level > 1) stb_mode = STB_DXT_HIGHQUAL;
}

SDL_GPUTexture *tex = SDL_CreateGPUTexture(gpu, &(SDL_GPUTextureCreateInfo) {
    .type = SDL_GPU_TEXTURETYPE_2D,
    .format = compress ? compressed_format : chosen_format,
    .width = surf->w,
    .height = surf->h,
    .layer_count_or_depth = 1,
    .num_levels = 1,
    .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER
});

const void *pixel_data = surf->pixels;
size_t pixel_data_size = surf->pitch * surf->h;
unsigned char *upload_data = NULL;
size_t upload_size = pixel_data_size;

if (compress) {
    // Compress with stb_dxt
    int block_width = (surf->w + 3) / 4;
    int block_height = (surf->h + 3) / 4;
    int block_size = (compressed_format == SDL_GPU_TEXTUREFORMAT_BC1_RGBA_UNORM) ? 8 : 16;
    int blocks_count = block_width * block_height;

    unsigned char *compressed_data = (unsigned char*)malloc(blocks_count * block_size);
    if (!compressed_data) {
        if (dofree) SDL_DestroySurface(surf);
        return JS_ThrowOutOfMemory(js);
    }

    const unsigned char *src_pixels = (const unsigned char *)pixel_data;
    for (int by = 0; by < block_height; ++by) {
        for (int bx = 0; bx < block_width; ++bx) {
            unsigned char block_rgba[4*4*4];
            memset(block_rgba, 0, sizeof(block_rgba));

            for (int y = 0; y < 4; ++y) {
                for (int x = 0; x < 4; ++x) {
                    int sx = bx*4 + x;
                    int sy = by*4 + y;
                    if (sx < surf->w && sy < surf->h) {
                        const unsigned char *pixel = src_pixels + sy * surf->pitch + sx * pdetails->bytes_per_pixel;
                        Uint32 pixelValue = 0;
                        // Copy pixel data into a Uint32 for SDL_GetRGBA-like extraction
                        // We'll use masks/shifts from pdetails
                        memcpy(&pixelValue, pixel, pdetails->bytes_per_pixel);

                        // Extract RGBA
                        unsigned char r = (pixelValue & pdetails->Rmask) >> pdetails->Rshift;
                        unsigned char g = (pixelValue & pdetails->Gmask) >> pdetails->Gshift;
                        unsigned char b = (pixelValue & pdetails->Bmask) >> pdetails->Bshift;
                        unsigned char a = pdetails->Amask ? ((pixelValue & pdetails->Amask) >> pdetails->Ashift) : 255;

                        // If bits are not fully 8-bit, scale them:
                        // pdetails->Rbits (etc.) give how many bits each channel uses
                        int Rmax = (1 << pdetails->Rbits) - 1;
                        int Gmax = (1 << pdetails->Gbits) - 1;
                        int Bmax = (1 << pdetails->Bbits) - 1;
                        int Amax = pdetails->Abits ? ((1 << pdetails->Abits) - 1) : 255;

                        if (pdetails->Rbits < 8) r = (r * 255) / Rmax;
                        if (pdetails->Gbits < 8) g = (g * 255) / Gmax;
                        if (pdetails->Bbits < 8) b = (b * 255) / Bmax;
                        if (pdetails->Amask && pdetails->Abits < 8) a = (a * 255) / Amax;

                        // Set block pixel
                        block_rgba[(y*4+x)*4+0] = r;
                        block_rgba[(y*4+x)*4+1] = g;
                        block_rgba[(y*4+x)*4+2] = b;
                        block_rgba[(y*4+x)*4+3] = a;
                    }
                }
            }

            unsigned char *dest_block = compressed_data + (by * block_width + bx)*block_size;
            int alpha = (compressed_format == SDL_GPU_TEXTUREFORMAT_BC3_RGBA_UNORM) ? 1 : 0;
            stb_compress_dxt_block(dest_block, block_rgba, alpha, stb_mode);
        }
    }

    upload_data = compressed_data;
    upload_size = blocks_count * block_size;
} else {
    // No compression, upload directly
    upload_data = (unsigned char*)pixel_data; 
    upload_size = pixel_data_size;
}

SDL_GPUTransferBuffer *tex_buffer = SDL_CreateGPUTransferBuffer(
    gpu,
    &(SDL_GPUTransferBufferCreateInfo) {
      .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
      .size = upload_size
    });
void *tex_ptr = SDL_MapGPUTransferBuffer(gpu, tex_buffer, false);
memcpy(tex_ptr, upload_data, upload_size);
SDL_UnmapGPUTransferBuffer(gpu, tex_buffer);

SDL_GPUCommandBuffer *uploadcmd = SDL_AcquireGPUCommandBuffer(gpu);
SDL_GPUCopyPass *copypass = SDL_BeginGPUCopyPass(uploadcmd);
SDL_UploadToGPUTexture(
    copypass,
    &(SDL_GPUTextureTransferInfo) {
      .transfer_buffer = tex_buffer,
      .offset = 0
    },
    &(SDL_GPUTextureRegion) {
      .texture = tex,
      .w = surf->w,
      .h = surf->h,
      .d = 1
    },
    false
);

SDL_EndGPUCopyPass(copypass);
SDL_SubmitGPUCommandBuffer(uploadcmd);
SDL_ReleaseGPUTransferBuffer(gpu, tex_buffer);

if (compress) {
    free(upload_data);
}

ret = SDL_GPUTexture2js(js, tex);
JS_SetProperty(js, ret, width_atom, number2js(js, surf->w));
JS_SetProperty(js, ret, height_atom, number2js(js, surf->h));

if (dofree) SDL_DestroySurface(surf);
)

JSC_CCALL(gpu_logical_size,
  
)

static SDL_GPUVertexInputState state_2d;

int atom2front_face(JSAtom atom)
{
	if(atom == cw_atom) return SDL_GPU_FRONTFACE_CLOCKWISE;
	else return SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
}

int atom2cull_mode(JSAtom atom)
{
	if(atom == front_atom) return SDL_GPU_CULLMODE_FRONT;
	else if(atom == back_atom) return SDL_GPU_CULLMODE_BACK;
	else return SDL_GPU_CULLMODE_NONE;
}

int atom2primitive_type(JSAtom atom)
{
	if(atom == point_atom) return SDL_GPU_PRIMITIVETYPE_POINTLIST;
	else if(atom == line_atom) return SDL_GPU_PRIMITIVETYPE_LINELIST;
	else if(atom == linestrip_atom) return SDL_GPU_PRIMITIVETYPE_LINESTRIP;
	else if(atom == trianglestrip_atom) return SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP;
	else return SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
}

int atom2compare_op(JSAtom atom)
{
	if(atom == never_atom) return SDL_GPU_COMPAREOP_NEVER;
	else if(atom == less_atom) return SDL_GPU_COMPAREOP_LESS;
	else if(atom == equal_atom) return SDL_GPU_COMPAREOP_EQUAL;
	else if(atom == less_equal_atom) return SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
	else if(atom == greater_atom) return SDL_GPU_COMPAREOP_GREATER;
	else if(atom == not_equal_atom) return SDL_GPU_COMPAREOP_NOT_EQUAL;
	else if(atom == greater_equal_atom) return SDL_GPU_COMPAREOP_GREATER_OR_EQUAL;
	else return SDL_GPU_COMPAREOP_ALWAYS;
}

int atom2stencil_op(JSAtom atom)
{
	if(atom == keep_atom) return SDL_GPU_STENCILOP_KEEP;
	else if(atom == zero_stencil_atom) return SDL_GPU_STENCILOP_ZERO;
	else if(atom == replace_atom) return SDL_GPU_STENCILOP_REPLACE;
	else if(atom == incr_clamp_atom) return SDL_GPU_STENCILOP_INCREMENT_AND_CLAMP;
	else if(atom == decr_clamp_atom) return SDL_GPU_STENCILOP_DECREMENT_AND_CLAMP;
	else if(atom == invert_atom) return SDL_GPU_STENCILOP_INVERT;
	else if(atom == incr_wrap_atom) return SDL_GPU_STENCILOP_INCREMENT_AND_WRAP;
	else if(atom == decr_wrap_atom) return SDL_GPU_STENCILOP_DECREMENT_AND_WRAP;
	else return SDL_GPU_STENCILOP_KEEP;
}

int atom2blend_factor(JSAtom atom)
{
	if(atom == zero_atom) return SDL_GPU_BLENDFACTOR_ZERO;
	else if(atom == one_atom) return SDL_GPU_BLENDFACTOR_ONE;
	else if(atom == src_color_atom) return SDL_GPU_BLENDFACTOR_SRC_COLOR;
	else if(atom == one_minus_src_color_atom) return SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_COLOR;
	else if(atom == dst_color_atom) return SDL_GPU_BLENDFACTOR_DST_COLOR;
	else if(atom == one_minus_dst_color_atom) return SDL_GPU_BLENDFACTOR_ONE_MINUS_DST_COLOR;
	else if(atom == src_alpha_atom) return SDL_GPU_BLENDFACTOR_SRC_ALPHA;
	else if(atom == one_minus_src_alpha_atom) return SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	else if(atom == dst_alpha_atom) return SDL_GPU_BLENDFACTOR_DST_ALPHA;
	else if(atom == one_minus_dst_alpha_atom) return SDL_GPU_BLENDFACTOR_ONE_MINUS_DST_ALPHA;
	else if(atom == constant_color_atom) return SDL_GPU_BLENDFACTOR_CONSTANT_COLOR;
	else if(atom == one_minus_constant_color_atom) return SDL_GPU_BLENDFACTOR_ONE_MINUS_CONSTANT_COLOR;
	else if(atom == src_alpha_saturate_atom) return SDL_GPU_BLENDFACTOR_SRC_ALPHA_SATURATE;
	else return SDL_GPU_BLENDFACTOR_ONE;
}

int atom2blend_op(JSAtom atom)
{
	if(atom == add_atom) return SDL_GPU_BLENDOP_ADD;
	else if(atom == sub_atom) return SDL_GPU_BLENDOP_SUBTRACT;
	else if(atom == rev_sub_atom) return SDL_GPU_BLENDOP_REVERSE_SUBTRACT;
	else if(atom == min_atom) return SDL_GPU_BLENDOP_MIN;
	else if(atom == max_atom) return SDL_GPU_BLENDOP_MAX;
	else return SDL_GPU_BLENDOP_ADD;
}

static JSValue js_gpu_make_pipeline(JSContext *js, JSValueConst self, int argc, JSValueConst *argv) {
  SDL_GPUDevice *gpu = js2SDL_GPUDevice(js,self);
  if (argc < 1)
    return JS_ThrowTypeError(js, "gpu_pipeline requires a pipeline object");

  JSValue pipe = argv[0];
  if (!JS_IsObject(pipe))
    return JS_ThrowTypeError(js, "gpu_pipeline argument must be an object");

  SDL_GPUGraphicsPipelineCreateInfo info = {0};

  info.vertex_input_state = (SDL_GPUVertexInputState){
			.num_vertex_buffers = 4,
			.vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[]){{
				.slot = 0,
				.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
				.instance_step_rate = 0,
				.pitch = sizeof(float)*3
			}, {
        .slot = 1,
        .pitch = sizeof(float)*2,
        .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
        .instance_step_rate = 0
      },
      {
        .slot = 2,
        .pitch = sizeof(float)*4,
        .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
        .instance_step_rate=0
      },
      {
        .slot = 3,
        .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
        .instance_step_rate = 0,
        .pitch = sizeof(float)*3
      }},
			.num_vertex_attributes = 4,
			.vertex_attributes = (SDL_GPUVertexAttribute[]){{
				.buffer_slot = 0,
				.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
				.location = 0,
				.offset = 0
			}, {
				.buffer_slot = 1,
				.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
				.location = 1,
				.offset = 0,
			}, {
        .buffer_slot = 2,
        .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
        .location = 2,
        .offset = 0
      }, {
        .buffer_slot = 3,
        .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
        .location = 3,
        .offset = 0
      }
      }
		};


  // Vertex and Fragment Shaders
  JSValue vertex_val = JS_GetPropertyStr(js, pipe, "vertex");
  if (JS_IsUndefined(vertex_val)) {
    JS_FreeValue(js, vertex_val);
    return JS_ThrowTypeError(js, "pipeline object must have a 'vertex' shader");
  }
  
  info.vertex_shader = js2SDL_GPUShader(js, vertex_val);
  JS_FreeValue(js, vertex_val);

  JSValue fragment_val = JS_GetPropertyStr(js, pipe, "fragment");
  if (JS_IsUndefined(fragment_val)) {
    JS_FreeValue(js, fragment_val);
    return JS_ThrowTypeError(js, "pipeline object must have a 'fragment' shader");
  }
  info.fragment_shader = js2SDL_GPUShader(js, fragment_val);
  JS_FreeValue(js, fragment_val);

  if (info.vertex_shader == NULL || info.fragment_shader == NULL) return JS_ThrowInternalError(js, "Failed to retrieve shaders");

  // Vertex Input State
//  info.vertex_input_state = state2d; // Assuming state2d is predefined

  // Primitive Type
  JSValue primitive_val = JS_GetPropertyStr(js, pipe, "primitive");
  JSAtom primitive_atom = JS_ValueToAtom(js, primitive_val);
  info.primitive_type = atom2primitive_type(primitive_atom);
  JS_FreeAtom(js, primitive_atom);
  JS_FreeValue(js, primitive_val);

  // Rasterizer State
  // Fill Mode
  JSValue fill_val = JS_GetPropertyStr(js, pipe, "fill");
  info.rasterizer_state.fill_mode = JS_ToBool(js, fill_val) ? SDL_GPU_FILLMODE_FILL : SDL_GPU_FILLMODE_LINE;
  JS_FreeValue(js, fill_val);

  // Cull Mode
  JSValue cull_val = JS_GetPropertyStr(js, pipe, "cull");
  JSAtom cull_atom = JS_ValueToAtom(js, cull_val);
  info.rasterizer_state.cull_mode = atom2cull_mode(cull_atom);
  JS_FreeAtom(js, cull_atom);
  JS_FreeValue(js, cull_val);

  // Front Face
  JSValue face_val = JS_GetPropertyStr(js, pipe, "face");
  JSAtom face_atom = JS_ValueToAtom(js, face_val);
  info.rasterizer_state.front_face = atom2front_face(face_atom);
  JS_FreeAtom(js, face_atom);
  JS_FreeValue(js, face_val);

  // Depth Bias (assuming defaults if not specified)
  JSValue depth_bias_val = JS_GetPropertyStr(js, pipe, "depth_bias");
  if (!JS_IsUndefined(depth_bias_val)) JS_ToFloat64(js, &info.rasterizer_state.depth_bias_constant_factor, depth_bias_val);
  JS_FreeValue(js, depth_bias_val);

  JSValue depth_bias_slope_scale_val = JS_GetPropertyStr(js, pipe, "depth_bias_slope_scale");
  if (!JS_IsUndefined(depth_bias_slope_scale_val))
    JS_ToFloat64(js, &info.rasterizer_state.depth_bias_slope_factor, depth_bias_slope_scale_val);

  JS_FreeValue(js, depth_bias_slope_scale_val);

  JSValue depth_bias_clamp_val = JS_GetPropertyStr(js, pipe, "depth_bias_clamp");
  if (!JS_IsUndefined(depth_bias_clamp_val))
    JS_ToFloat64(js, &info.rasterizer_state.depth_bias_clamp, depth_bias_clamp_val);
  JS_FreeValue(js, depth_bias_clamp_val);

  // Enable Depth Bias (assuming false if not specified)
  JSValue enable_depth_bias_val = JS_GetPropertyStr(js, pipe, "enable_depth_bias");
  info.rasterizer_state.enable_depth_bias = JS_ToBool(js, enable_depth_bias_val);
  JS_FreeValue(js, enable_depth_bias_val);

  // Enable Depth Clip (assuming true if not specified)
  JSValue enable_depth_clip_val = JS_GetPropertyStr(js, pipe, "enable_depth_clip");
  if (JS_IsUndefined(enable_depth_clip_val))
    info.rasterizer_state.enable_depth_clip = true; // Default
  else
  info.rasterizer_state.enable_depth_clip = JS_ToBool(js, enable_depth_clip_val);
  JS_FreeValue(js, enable_depth_clip_val);

  // Depth Stencil State
  JSValue depth_val = JS_GetPropertyStr(js, pipe, "depth");
  if (JS_IsObject(depth_val)) {
    JSValue compare_val = JS_GetPropertyStr(js, depth_val, "compare");
    if (!JS_IsUndefined(compare_val)) {
      JSAtom compare_atom = JS_ValueToAtom(js, compare_val);
      info.depth_stencil_state.compare_op = atom2compare_op(compare_atom);
      JS_FreeAtom(js, compare_atom);
    }
    JS_FreeValue(js, compare_val);

    // Enable Depth Test
    JSValue test_val = JS_GetPropertyStr(js, depth_val, "test");
    if (!JS_IsUndefined(test_val))
      info.depth_stencil_state.enable_depth_test = JS_ToBool(js, test_val);

    JS_FreeValue(js, test_val);

    // Enable Depth Write
    JSValue write_val = JS_GetPropertyStr(js, depth_val, "write");
    if (!JS_IsUndefined(write_val))
      info.depth_stencil_state.enable_depth_write = JS_ToBool(js, write_val);
    JS_FreeValue(js, write_val);

    // Depth Bias Factors
    JSValue bias_val = JS_GetPropertyStr(js, depth_val, "bias");
    if (!JS_IsUndefined(bias_val))
      JS_ToFloat64(js, &info.rasterizer_state.depth_bias_constant_factor, bias_val);
    JS_FreeValue(js, bias_val);

    JSValue bias_slope_scale_val = JS_GetPropertyStr(js, depth_val, "bias_slope_scale");
    if (!JS_IsUndefined(bias_slope_scale_val))
      JS_ToFloat64(js, &info.rasterizer_state.depth_bias_slope_factor, bias_slope_scale_val);
    JS_FreeValue(js, bias_slope_scale_val);

    JSValue bias_clamp_val = JS_GetPropertyStr(js, depth_val, "bias_clamp");
    if (!JS_IsUndefined(bias_clamp_val))
      JS_ToFloat64(js, &info.rasterizer_state.depth_bias_clamp, bias_clamp_val);
    JS_FreeValue(js, bias_clamp_val);
  }
  JS_FreeValue(js, depth_val);

  // Stencil State
  JSValue stencil_val = JS_GetPropertyStr(js, pipe, "stencil");
  if (JS_IsObject(stencil_val)) {
    JSValue enabled_val = JS_GetPropertyStr(js, stencil_val, "enabled");
    if (!JS_IsUndefined(enabled_val))
      info.depth_stencil_state.enable_stencil_test = JS_ToBool(js, enabled_val);
    JS_FreeValue(js, enabled_val);

    // Front Stencil
    JSValue front_val = JS_GetPropertyStr(js, stencil_val, "front");
    if (JS_IsObject(front_val)) {
      JSValue compare_val = JS_GetPropertyStr(js, front_val, "compare");
      if (!JS_IsUndefined(compare_val)) {
        JSAtom compare_atom = JS_ValueToAtom(js, compare_val);
        info.depth_stencil_state.front_stencil_state.compare_op = atom2compare_op(compare_atom);
        JS_FreeAtom(js, compare_atom);
      }
      JS_FreeValue(js, compare_val);

      JSValue fail_val = JS_GetPropertyStr(js, front_val, "fail");
      if (!JS_IsUndefined(fail_val)) {
        JSAtom fail_atom = JS_ValueToAtom(js, fail_val);
        info.depth_stencil_state.front_stencil_state.fail_op = atom2stencil_op(fail_atom);
        JS_FreeAtom(js, fail_atom);
      }
      JS_FreeValue(js, fail_val);

      JSValue depth_fail_val = JS_GetPropertyStr(js, front_val, "depth_fail");
      if (!JS_IsUndefined(depth_fail_val)) {
        JSAtom depth_fail_atom = JS_ValueToAtom(js, depth_fail_val);
        info.depth_stencil_state.front_stencil_state.depth_fail_op = atom2stencil_op(depth_fail_atom);
        JS_FreeAtom(js, depth_fail_atom);
      }
      JS_FreeValue(js, depth_fail_val);

            JSValue pass_val = JS_GetPropertyStr(js, front_val, "pass");
            if (!JS_IsUndefined(pass_val)) {
                JSAtom pass_atom = JS_ValueToAtom(js, pass_val);
                info.depth_stencil_state.front_stencil_state.pass_op = atom2stencil_op(pass_atom);
                JS_FreeAtom(js, pass_atom);
            }
            JS_FreeValue(js, pass_val);
        }
        JS_FreeValue(js, front_val);

        // Back Stencil
        JSValue back_val = JS_GetPropertyStr(js, stencil_val, "back");
        if (JS_IsObject(back_val)) {
            JSValue compare_val = JS_GetPropertyStr(js, back_val, "compare");
            if (!JS_IsUndefined(compare_val)) {
                JSAtom compare_atom = JS_ValueToAtom(js, compare_val);
                info.depth_stencil_state.back_stencil_state.compare_op = atom2compare_op(compare_atom);
                JS_FreeAtom(js, compare_atom);
            }
            JS_FreeValue(js, compare_val);

            JSValue fail_val = JS_GetPropertyStr(js, back_val, "fail");
            if (!JS_IsUndefined(fail_val)) {
                JSAtom fail_atom = JS_ValueToAtom(js, fail_val);
                info.depth_stencil_state.back_stencil_state.fail_op = atom2stencil_op(fail_atom);
                JS_FreeAtom(js, fail_atom);
            }
            JS_FreeValue(js, fail_val);

            JSValue depth_fail_val = JS_GetPropertyStr(js, back_val, "depth_fail");
            if (!JS_IsUndefined(depth_fail_val)) {
                JSAtom depth_fail_atom = JS_ValueToAtom(js, depth_fail_val);
                info.depth_stencil_state.back_stencil_state.depth_fail_op = atom2stencil_op(depth_fail_atom);
                JS_FreeAtom(js, depth_fail_atom);
            }
            JS_FreeValue(js, depth_fail_val);

            JSValue pass_val = JS_GetPropertyStr(js, back_val, "pass");
            if (!JS_IsUndefined(pass_val)) {
                JSAtom pass_atom = JS_ValueToAtom(js, pass_val);
                info.depth_stencil_state.back_stencil_state.pass_op = atom2stencil_op(pass_atom);
                JS_FreeAtom(js, pass_atom);
            }
            JS_FreeValue(js, pass_val);
        }
        JS_FreeValue(js, back_val);

        // Compare Mask
        JSValue compare_mask_val = JS_GetPropertyStr(js, stencil_val, "compare_mask");
        if (!JS_IsUndefined(compare_mask_val)) {
            JS_ToUint32(js, &info.depth_stencil_state.compare_mask, compare_mask_val);
        }
        JS_FreeValue(js, compare_mask_val);

        // Write Mask
        JSValue write_mask_val = JS_GetPropertyStr(js, stencil_val, "write_mask");
        if (!JS_IsUndefined(write_mask_val)) {
            JS_ToUint32(js, &info.depth_stencil_state.write_mask, write_mask_val);
        }
        JS_FreeValue(js, write_mask_val);
    }
    JS_FreeValue(js, stencil_val);

    // Blend State
    JSValue blend_val = JS_GetPropertyStr(js, pipe, "blend");
    if (JS_IsObject(blend_val)) {
        // Enable Blend
        JSValue enabled_val = JS_GetPropertyStr(js, blend_val, "enabled");
        if (!JS_IsUndefined(enabled_val)) {
            info.target_info.has_depth_stencil_target = true; // Assuming blend requires depth stencil
            // Depending on SDL_GPU's API, adjust accordingly
            // For now, we'll set blend_state fields
            // But since target_info is separate, we need to handle it appropriately
            // This depends on SDL_GPU's actual structure, which isn't fully detailed here
        }
        JS_FreeValue(js, enabled_val);

        // Blend Factors and Operations
        JSValue src_factor_rgb_val = JS_GetPropertyStr(js, blend_val, "src_factor_rgb");
        if (!JS_IsUndefined(src_factor_rgb_val)) {
            JSAtom src_factor_rgb_atom = JS_ValueToAtom(js, src_factor_rgb_val);
            info.target_info.color_target_descriptions = NULL; // Placeholder
            // info.blend_state.src_factor_rgb = atom2blend_factor(src_factor_rgb_atom);
            // Similarly set other blend factors and operations
            // This depends on how SDL_GPU handles blending in target_info
            // Since SDL_GPUGraphicsPipelineCreateInfo has target_info, blending might be part of color_target_descriptions
            // Adjust accordingly based on SDL_GPU's API
            // For demonstration, we'll skip detailed blend state handling here
            JS_FreeAtom(js, src_factor_rgb_atom);
        }
        JS_FreeValue(js, src_factor_rgb_val);

        // Similarly handle other blend factors and operations
        // Skipping detailed implementation due to lack of SDL_GPU specifics
    }
    JS_FreeValue(js, blend_val);

    // Alpha to Coverage
    JSValue atc_val = JS_GetPropertyStr(js, pipe, "alpha_to_coverage");
//    info.multisample_state.sample_mask = 0xFFFFFFFF; // Default
//    info.multisample_state.enable_mask = JS_ToBool(js, JS_GetPropertyStr(js, pipe, "multisample.domask")) ? 1 : 0;
//    info.multisample_state.sample_count = 1; // Default
    // Adjust based on JS object
/*    JSValue multisample_val = JS_GetPropertyStr(js, pipe, "multisample");
    if (JS_IsObject(multisample_val)) {
        JSValue count_val = JS_GetPropertyStr(js, multisample_val, "count");
        if (!JS_IsUndefined(count_val)) {
            JS_ToInt32(js, (int32_t*)&info.multisample_state.sample_count, count_val);
        }
        JS_FreeValue(js, count_val);

        JSValue mask_val = JS_GetPropertyStr(js, multisample_val, "mask");
        if (!JS_IsUndefined(mask_val)) {
            JS_ToUint32(js, &info.multisample_state.sample_mask, mask_val);
        }
        JS_FreeValue(js, mask_val);

        JSValue domask_val = JS_GetPropertyStr(js, multisample_val, "domask");
        if (!JS_IsUndefined(domask_val)) {
            info.multisample_state.enable_mask = JS_ToBool(js, domask_val);
        }
        JS_FreeValue(js, domask_val);
    }
    JS_FreeValue(js, multisample_val);
*/
//    info.blend_state.alpha_to_coverage = JS_ToBool(js, atc_val);
    JS_FreeValue(js, atc_val);

    // Target Info
    // For simplicity, assume a single color target with a default format
    // You can expand this to handle multiple targets based on your needs
//    SDL_GPUColorTargetDescription color_target_desc = { SDL_GPU_RGBA8 }; // Default format
//    info.target_info.color_target_descriptions = &color_target_desc;
    info.target_info.num_color_targets = 1;
//    info.target_info.depth_stencil_format = SDL_GPU_DEPTH24_STENCIL8; // Default depth-stencil format
    JSValue has_depth_stencil_target_val = JS_GetPropertyStr(js, pipe, "has_depth_stencil_target");
    if (!JS_IsUndefined(has_depth_stencil_target_val)) {
        info.target_info.has_depth_stencil_target = JS_ToBool(js, has_depth_stencil_target_val);
    } else {
        info.target_info.has_depth_stencil_target = true; // Default
    }
    JS_FreeValue(js, has_depth_stencil_target_val);

    // Label (optional)
    JSValue label_val = JS_GetPropertyStr(js, pipe, "label");
    if (JS_IsString(label_val)) {
        const char *label_str = JS_ToCString(js, label_val);
        if (label_str) {
            // Assuming SDL_GPUGraphicsPipelineCreateInfo has a label field
            // Modify the struct if necessary
            // Example:
            // strncpy(info.label, label_str, sizeof(info.label) - 1);
            // info.label[sizeof(info.label) - 1] = '\0';
            // For demonstration, we'll ignore it
            JS_FreeCString(js, label_str);
        }
    }
    JS_FreeValue(js, label_val);

    JSValue jswin = JS_GetPropertyStr(js,self,"window");
    SDL_Window *win = js2SDL_Window(js, jswin);
    info.target_info =(SDL_GPUGraphicsPipelineTargetInfo) {
      .num_color_targets = 1,
      .color_target_descriptions = (SDL_GPUColorTargetDescription[]) {{
        .format = SDL_GetGPUSwapchainTextureFormat(gpu, win)
      }}
    };
    JS_FreeValue(js,jswin);

    // Create the pipeline
    SDL_GPUGraphicsPipeline *pipeline = SDL_CreateGPUGraphicsPipeline(gpu, &info);
    if (!pipeline) return JS_ThrowInternalError(js, "Failed to create GPU pipeline");

    printf("MADE PIPELINE\n");

    return SDL_GPUGraphicsPipeline2js(js, pipeline);
}

JSC_CCALL(gpu_newframe,
  SDL_GPUDevice *gpu = js2SDL_GPUDevice(js,self);
  
  SDL_Window *win = js2SDL_Window(js,argv[0]);
)

// Helper conversion functions:
int atom2filter(JSAtom atom)
{
    if (atom == nearest_atom) return SDL_GPU_FILTER_NEAREST;
    if (atom == linear_atom) return SDL_GPU_FILTER_LINEAR;
    // Default
    return SDL_GPU_FILTER_LINEAR;
}

int atom2mipmap_mode(JSAtom atom)
{
    if (atom == mipmap_nearest_atom) return SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    if (atom == mipmap_linear_atom) return SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    return SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
}

int atom2address_mode(JSAtom atom)
{
    if (atom == repeat_atom) return SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    if (atom == mirror_atom) return SDL_GPU_SAMPLERADDRESSMODE_MIRRORED_REPEAT;
    if (atom == clamp_edge_atom) return SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    return SDL_GPU_SAMPLERADDRESSMODE_REPEAT; // Default
}

// Now the function to create the sampler
static JSValue js_gpu_make_sampler(JSContext *js, JSValueConst self, int argc, JSValueConst *argv) {
    // We need at least 1 argument: the sampler description object
    if (argc < 1)
        return JS_ThrowTypeError(js, "gpu_sampler requires a sampler object");

    JSValue sampler_obj = argv[0];
    if (!JS_IsObject(sampler_obj))
        return JS_ThrowTypeError(js, "gpu_sampler argument must be an object");

    SDL_GPUDevice *device = js2SDL_GPUDevice(js, self);
    if (!device)
        return JS_ThrowInternalError(js, "Invalid GPU device");

    SDL_GPUSamplerCreateInfo info;
    SDL_memset(&info, 0, sizeof(info));

    // Get min_filter
    JSValue min_val = JS_GetPropertyStr(js, sampler_obj, "min_filter");
    if (!JS_IsUndefined(min_val)) {
        JSAtom min_atom = JS_ValueToAtom(js, min_val);
        info.min_filter = atom2filter(min_atom);
        JS_FreeAtom(js, min_atom);
    } else {
        info.min_filter = SDL_GPU_FILTER_LINEAR; // default
    }
    JS_FreeValue(js, min_val);

    // Get mag_filter
    JSValue mag_val = JS_GetPropertyStr(js, sampler_obj, "mag_filter");
    if (!JS_IsUndefined(mag_val)) {
        JSAtom mag_atom = JS_ValueToAtom(js, mag_val);
        info.mag_filter = atom2filter(mag_atom);
        JS_FreeAtom(js, mag_atom);
    } else {
        info.mag_filter = SDL_GPU_FILTER_LINEAR; // default
    }
    JS_FreeValue(js, mag_val);

    // mipmap_mode
    JSValue mipmap_val = JS_GetPropertyStr(js, sampler_obj, "mipmap_mode");
    if (!JS_IsUndefined(mipmap_val)) {
        JSAtom mipmap_atom = JS_ValueToAtom(js, mipmap_val);
        info.mipmap_mode = atom2mipmap_mode(mipmap_atom);
        JS_FreeAtom(js, mipmap_atom);
    } else {
        info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR; // default
    }
    JS_FreeValue(js, mipmap_val);

    // address_mode_u
    JSValue address_u_val = JS_GetPropertyStr(js, sampler_obj, "address_mode_u");
    if (!JS_IsUndefined(address_u_val)) {
        JSAtom address_u_atom = JS_ValueToAtom(js, address_u_val);
        info.address_mode_u = atom2address_mode(address_u_atom);
        JS_FreeAtom(js, address_u_atom);
    } else {
        info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT; // default
    }
    JS_FreeValue(js, address_u_val);

    // address_mode_v
    JSValue address_v_val = JS_GetPropertyStr(js, sampler_obj, "address_mode_v");
    if (!JS_IsUndefined(address_v_val)) {
        JSAtom address_v_atom = JS_ValueToAtom(js, address_v_val);
        info.address_mode_v = atom2address_mode(address_v_atom);
        JS_FreeAtom(js, address_v_atom);
    } else {
        info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT; // default
    }
    JS_FreeValue(js, address_v_val);

    // address_mode_w
    JSValue address_w_val = JS_GetPropertyStr(js, sampler_obj, "address_mode_w");
    if (!JS_IsUndefined(address_w_val)) {
        JSAtom address_w_atom = JS_ValueToAtom(js, address_w_val);
        info.address_mode_w = atom2address_mode(address_w_atom);
        JS_FreeAtom(js, address_w_atom);
    } else {
        info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT; // default
    }
    JS_FreeValue(js, address_w_val);

    // mip_lod_bias
    JSValue mip_lod_bias_val = JS_GetPropertyStr(js, sampler_obj, "mip_lod_bias");
    if (!JS_IsUndefined(mip_lod_bias_val)) {
        JS_ToFloat64(js, &info.mip_lod_bias, mip_lod_bias_val);
    } else {
        info.mip_lod_bias = 0.0f; // default
    }
    JS_FreeValue(js, mip_lod_bias_val);

    // max_anisotropy
    JSValue max_anisotropy_val = JS_GetPropertyStr(js, sampler_obj, "max_anisotropy");
    if (!JS_IsUndefined(max_anisotropy_val)) {
        JS_ToFloat64(js, &info.max_anisotropy, max_anisotropy_val);
    } else {
        info.max_anisotropy = 1.0f; // default
    }
    JS_FreeValue(js, max_anisotropy_val);

    // compare_op
    JSValue compare_op_val = JS_GetPropertyStr(js, sampler_obj, "compare_op");
    if (!JS_IsUndefined(compare_op_val)) {
        JSAtom compare_op_atom = JS_ValueToAtom(js, compare_op_val);
        info.compare_op = atom2compare_op(compare_op_atom);
        JS_FreeAtom(js, compare_op_atom);
    } else {
        info.compare_op = SDL_GPU_COMPAREOP_ALWAYS; // default
    }
    JS_FreeValue(js, compare_op_val);

    // min_lod
    JSValue min_lod_val = JS_GetPropertyStr(js, sampler_obj, "min_lod");
    if (!JS_IsUndefined(min_lod_val)) {
        JS_ToFloat64(js, &info.min_lod, min_lod_val);
    } else {
        info.min_lod = 0.0f; // default
    }
    JS_FreeValue(js, min_lod_val);

    // max_lod
    JSValue max_lod_val = JS_GetPropertyStr(js, sampler_obj, "max_lod");
    if (!JS_IsUndefined(max_lod_val)) {
        JS_ToFloat64(js, &info.max_lod, max_lod_val);
    } else {
        info.max_lod = 1000.0f; // arbitrary large number as default
    }
    JS_FreeValue(js, max_lod_val);

    // enable_anisotropy
    JSValue enable_anisotropy_val = JS_GetPropertyStr(js, sampler_obj, "enable_anisotropy");
    if (!JS_IsUndefined(enable_anisotropy_val)) {
        info.enable_anisotropy = JS_ToBool(js, enable_anisotropy_val);
    } else {
        info.enable_anisotropy = false; // default
    }
    JS_FreeValue(js, enable_anisotropy_val);

    // enable_compare
    JSValue enable_compare_val = JS_GetPropertyStr(js, sampler_obj, "enable_compare");
    if (!JS_IsUndefined(enable_compare_val)) {
        info.enable_compare = JS_ToBool(js, enable_compare_val);
    } else {
        info.enable_compare = false; // default
    }
    JS_FreeValue(js, enable_compare_val);

    // props (extensions)
    // If you don't need extensions, set to 0. Otherwise retrieve from JS.
    info.props = 0;

    // Create the sampler
    SDL_GPUSampler *sampler = SDL_CreateGPUSampler(device, &info);
    if (!sampler) {
        return JS_ThrowInternalError(js, "Failed to create GPU sampler");
    }

    // Convert sampler pointer to JS object or handle as needed
    printf("sampler created was %p\n", sampler);
    return SDL_GPUSampler2js(js, sampler); // You need to implement SDL_GPUSampler2js similar to pipeline
}

/*JSC_CCALL(gpu_present,
for (size_t i = 0; i < arrlen(batches)-1; i++) {
    size_t vert_end   = batches[i+1].start_vert; // one past the last vertex of this batch
    size_t vert_count = vert_end - vert_start;

    size_t idx_end   = batches[i+1].start_indx; // one past the last index of this batch
    size_t idx_count = idx_end - idx_start;

    SDL_DrawGPUIndexedPrimitives(
        pass,
        (int)idx_count,  // How many indices to draw
        1,               // Instance count
        (int)idx_start,  // Starting index offset
        (int)vert_start, // Vertex offset
        0                // First instance (if using instancing)
    );
    vert_start = vert_end;
    idx_start = idx_end;
}
  
  SDL_EndGPURenderPass(pass);

  SDL_SubmitGPUCommandBuffer(cmds);
  SDL_ReleaseGPUBuffer(gpu,vertex_buffer);
  SDL_ReleaseGPUBuffer(gpu,index_buffer);
  arrsetlen(global_verts,0);
  arrsetlen(global_indices,0);
)
*/

JSC_CCALL(gpu_camera,
  SDL_GPUDevice *gpu = js2SDL_GPUDevice(js,self);
  SDL_Rect vp;
  vp.w = 1280;
  vp.h = 718;
  int centered = JS_ToBool(js,argv[1]);

  HMM_Mat4 proj;
if (centered) {
  // World coordinates: (0,0) at screen center, Y up
  proj = HMM_Orthographic_RH_NO(
      -(float)vp.w * 0.5f, (float)vp.w * 0.5f, // left, right
      -(float)vp.h * 0.5f, (float)vp.h * 0.5f, // bottom, top
      -1.0f, 1.0f
  );
} else {
  // UI coordinates: (0,0) at bottom-left corner, Y up
  proj = HMM_Orthographic_RH_NO(
      0.0f, (float)vp.w,   // left, right
      0.0f, (float)vp.h,   // bottom, top
      -1.0f, 1.0f
  );

}

transform *tra = js2transform(js, argv[0]);
HMM_Mat4 view = HMM_Translate((HMM_Vec3){-tra->pos.x, -tra->pos.y, 0.0f});
)

JSC_CCALL(gpu_scale,

)

/*
JSC_CCALL(gpu_geometry,
  SDL_GPUDevice *gpu = js2SDL_GPUDevice(js,self);
  SDL_GPUTexture *tex = js2SDL_GPUTexture(js, argv[0]);

  // Extract geometry data from the JS object in argv[1]
  JSValue geom_obj = argv[1];
  JSValue pos_val = JS_GetPropertyStr(js, geom_obj, "pos");
  JSValue color_val = JS_GetPropertyStr(js, geom_obj, "color");
  JSValue uv_val = JS_GetPropertyStr(js, geom_obj, "uv");
  JSValue indices_val = JS_GetPropertyStr(js, geom_obj, "indices");

  int vertices = js_getnum_str(js, geom_obj, "vertices");
  int count = js_getnum_str(js, geom_obj, "count");

  size_t pos_stride, color_stride, uv_stride, indices_stride;
  void *posdata = get_gpu_buffer(js, pos_val, &pos_stride, NULL);
  void *colordata = get_gpu_buffer(js, color_val, &color_stride, NULL);
  void *uvdata = get_gpu_buffer(js, uv_val, &uv_stride, NULL);
  void *idxdata = get_gpu_buffer(js, indices_val, &indices_stride, NULL);

  // Transform positions by camera matrix
  // Assuming posdata is an array of HMM_Vec2 or {float x, float y}
  HMM_Vec2 *trans_pos = malloc(vertices * sizeof(HMM_Vec2));
  memcpy(trans_pos, posdata, sizeof(HMM_Vec2)*vertices);

  for (int i = 0; i < vertices; i++) {
    HMM_Vec3 p3 = {trans_pos[i].X, trans_pos[i].Y, 1.0f};
    HMM_Vec3 tp = HMM_MulM3V3(cam_mat, p3);
    trans_pos[i].X = tp.X;
    trans_pos[i].Y = tp.Y;
  }

  // We'll read each vertex's data and push into global_verts as texture_vertex.
  // Assume:
  // - posdata: HMM_Vec2 per vertex
  // - uvdata: HMM_Vec2 per vertex
  // - colordata: float[4] per vertex (r,g,b,a)
  // Adjust indexing according to strides if needed. For simplicity, assume contiguous arrays.
  unsigned char *pPos = (unsigned char *)posdata;
  unsigned char *pUV = (unsigned char *)uvdata;
  unsigned char *pColor = (unsigned char *)colordata;

  // z coordinate (for 2D rendering, just 0)
  float z = 0.0f;

  for (int i = 0; i < vertices; i++) {
    // Position
    float px = trans_pos[i].X;
    float py = trans_pos[i].Y;

    // UV
    HMM_Vec2 *uv_ptr = (HMM_Vec2*)(pUV + i * uv_stride);
    float u = uv_ptr->X;
    float v = uv_ptr->Y;

    // Color
    float *c_ptr = (float *)(pColor + i * color_stride);
    uint8_t r = (uint8_t)(c_ptr[0] * 255.0f);
    uint8_t g = (uint8_t)(c_ptr[1] * 255.0f);
    uint8_t b = (uint8_t)(c_ptr[2] * 255.0f);
    uint8_t a = (uint8_t)(c_ptr[3] * 255.0f);

    texture_vertex vert = {
      px, py, z,
      u, v,
      r, g, b, a
    };

    arrput(global_verts, vert);
  }

  // Now handle indices
  // Indices might be Uint16 or Uint32, check indices_stride or known format
  // Suppose they are Uint16
  Uint16 *idx_ptr = (Uint16 *)idxdata;

  for (int i = 0; i < count; i++) {
    // Adjust index by base_vert
    arrput(global_indices, idx_ptr[i]);
  }

  free(trans_pos);

  // Clean up JS values
  JS_FreeValue(js, pos_val);
  JS_FreeValue(js, color_val);
  JS_FreeValue(js, uv_val);
  JS_FreeValue(js, indices_val);
)
*/

JSC_CCALL(gpu_texture,
/*  SDL_GPUDevice *gpu = js2SDL_GPUDevice(js,self);
  SDL_GPUTexture *tex = js2SDL_GPUTexture(js, argv[0]);

  rect dst = js2rect(js,argv[1]);
  
  // Determine the coloring
  uint8_t r = 255, g = 255, b = 255, a = 255;
  if (!JS_IsUndefined(argv[3])) {
    colorf color = js2color(js,argv[3]);
    r = (uint8_t)(color.r * 255.0f);
    g = (uint8_t)(color.g * 255.0f);
    b = (uint8_t)(color.b * 255.0f);
    a = (uint8_t)(color.a * 255.0f);
  }

  // If source rect is provided, compute normalized UVs, else use full texture
  float src_x = 0.0f;
  float src_y = 0.0f;
  float src_w = 1.0;
  float src_h = 1.0;

  if (!JS_IsUndefined(argv[2])) {
    rect src = js2rect(js, argv[2]);
    src_x = (float)src.x;
    src_y = (float)src.y;
    src_w = (float)src.w;
    src_h = (float)src.h;
  }

  float u0 = src_x;
  float v1 = src_y;
  float u1 = (src_x + src_w);
  float v0 = (src_y + src_h);

  // Set z = 0 for 2D rendering
  float z = 0.0f;
  
  texture_vertex verts[4];

  verts[0] = (texture_vertex){ (float)dst.x,          (float)dst.y,          z, u0, v0, r,g,b,a };
  verts[1] = (texture_vertex){ (float)(dst.x+dst.w),  (float)dst.y,          z, u1, v0, r,g,b,a };
  verts[2] = (texture_vertex){ (float)(dst.x+dst.w),  (float)(dst.y+dst.h),  z, u1, v1, r,g,b,a };
  verts[3] = (texture_vertex){ (float)dst.x,          (float)(dst.y+dst.h),  z, u0, v1, r,g,b,a };

  size_t base_vert = arrlen(global_verts);
  for (int i = 0; i < 4; i++)
    arrput(global_verts,verts[i]);

  Uint16 indices[6] = {0, 1, 2, 2, 3, 0};
  for (int i = 0; i < 6; i++)
    arrput(global_indices, indices[i]);*/
)

JSC_CCALL(gpu_viewport,
  
)

JSC_CCALL(gpu_make_sprite_mesh,
  size_t quads = js_arrlen(js, argv[0]);
  size_t verts = quads*4;
  size_t count = quads*6;

  HMM_Vec3 *posdata = malloc(sizeof(*posdata)*verts);
  HMM_Vec2 *uvdata = malloc(sizeof(*uvdata)*verts);
  HMM_Vec4 *colordata = malloc(sizeof(*colordata)*verts);

  for (int i = 0; i < quads; i++) {
    JSValue sub = JS_GetPropertyUint32(js,argv[0],i);
    JSValue jstransform = JS_GetProperty(js,sub,transform_atom);
    transform *tr = js2transform(js,jstransform);
    JSValue jssrc = JS_GetProperty(js,sub,src_atom);
    JSValue jscolor = JS_GetProperty(js,sub,color_atom);
    HMM_Vec4 color;

    rect src;
    if (JS_IsUndefined(jssrc))
      src = (rect){.x = 0, .y = 0, .w = 1, .h = 1};
    else
      src = js2rect(js,jssrc);
    
    if (JS_IsUndefined(jscolor))
      color = (HMM_Vec4){1,1,1,1};
    else
      color = js2vec4(js,jscolor);

    // Calculate the base index for the current quad
    size_t base = i * 4;

    HMM_Mat3 trmat = js2transform_mat3(js,jstransform);

    HMM_Vec3 base_quad[4] = {
      {0.0,0.0,1.0},
      {1.0,0.0,1.0},
      {0.0,1.0,1.0},
      {1.0,1.0,1.0}
    };
    for (int j = 0; j < 4; j++)
      posdata[base+j] = HMM_MulM3V3(trmat, base_quad[j]);

    // Define the UV coordinates based on the source rectangle
    uvdata[base + 0] = (HMM_Vec2){ src.x,                  src.y + src.h };
    uvdata[base + 1] = (HMM_Vec2){ src.x + src.w,      src.y + src.h };    
    uvdata[base + 2] = (HMM_Vec2){ src.x,                  src.y };
    uvdata[base + 3] = (HMM_Vec2){ src.x + src.w,      src.y };

    colordata[4*i + 0] = color;
    colordata[4*i + 1] = color;
    colordata[4*i + 2] = color;
    colordata[4*i + 3] = color;

    JS_FreeValue(js,sub);
    JS_FreeValue(js,jscolor);
    JS_FreeValue(js,jssrc);
  }

  ret = JS_NewObject(js);
  JS_SetProperty(js, ret, pos_atom, make_gpu_buffer(js, posdata, sizeof(*posdata) * verts, JS_TYPED_ARRAY_FLOAT32, 2, 1));
  JS_SetProperty(js, ret, uv_atom, make_gpu_buffer(js, uvdata, sizeof(*uvdata) * verts, JS_TYPED_ARRAY_FLOAT32, 2, 1));
  JS_SetProperty(js, ret, color_atom, make_gpu_buffer(js, colordata, sizeof(*colordata) * verts, JS_TYPED_ARRAY_FLOAT32, 0, 1));
  JS_SetProperty(js, ret, indices_atom, make_quad_indices_buffer(js, quads));
  JS_SetProperty(js, ret, vertices_atom, number2js(js, verts));
  JS_SetProperty(js, ret, count_atom, number2js(js, count));
)

JSC_CCALL(gpu_driver,
  SDL_GPUDevice *gpu = js2SDL_GPUDevice(js,self);
  ret = JS_NewString(js, SDL_GetGPUDeviceDriver(gpu));
)

JSC_CCALL(gpu_set_pipeline,
  JSValue pipeline = JS_GetPropertyStr(js, argv[0], "gpu");
  if (JS_IsUndefined(pipeline))
    return JS_ThrowReferenceError(js, "Must use make_pipeline before setting.");
)

JSC_CCALL(gpu_make_shader,
  SDL_GPUDevice *gpu = js2SDL_GPUDevice(js,self);
  SDL_GPUShaderStage stage = !JS_ToBool(js,argv[1]);;

  size_t size;
  void *code = JS_GetArrayBuffer(js, &size, argv[0]);

  SDL_GPUShader *shader = SDL_CreateGPUShader(gpu, &(SDL_GPUShaderCreateInfo) {
    .code_size = size,
    .code = code,
    .entrypoint = "main",
    .format = SDL_GPU_SHADERFORMAT_SPIRV,
    .stage = stage,
    .num_samplers = js2number(js,argv[2]),
    .num_storage_textures = js2number(js,argv[3]),
    .num_storage_buffers = js2number(js,argv[4]),
    .num_uniform_buffers = js2number(js,argv[5])
  });
  if (!shader) return JS_ThrowReferenceError(js, "Unable to create shader: %s", SDL_GetError());
  return SDL_GPUShader2js(js,shader);
)

JSC_CCALL(gpu_acquire_cmd_buffer,
  SDL_GPUDevice *gpu = js2SDL_GPUDevice(js, self);
  SDL_GPUCommandBuffer *cb = SDL_AcquireGPUCommandBuffer(gpu);
  if (!cb) return JS_ThrowReferenceError(js,"Unable to acquire command buffer: %s", SDL_GetError());
  // Wrap cb in a JS object
  return SDL_GPUCommandBuffer2js(js, cb);
)

static Uint32 atom2usage(JSContext *js, JSAtom atom) {
    if (atom == vertex_atom) {
        return SDL_GPU_BUFFERUSAGE_VERTEX;
    } else if (atom == index_atom) {
        return SDL_GPU_BUFFERUSAGE_INDEX;
    } else if (atom == indirect_atom) {
        return SDL_GPU_BUFFERUSAGE_INDIRECT;
    }
    // Default fallback if unrecognized
    return SDL_GPU_BUFFERUSAGE_VERTEX;
}

JSC_CCALL(gpu_upload,
  SDL_GPUDevice *gpu = js2SDL_GPUDevice(js,self);
  SDL_GPUCommandBuffer *cmds = SDL_AcquireGPUCommandBuffer(gpu);
  SDL_GPUCopyPass *copypass = SDL_BeginGPUCopyPass(cmds);
  
  JSAtom usage_atom = JS_ValueToAtom(js, argv[1]);
  Uint32 flag = atom2usage(js, usage_atom);
  JS_FreeAtom(js, usage_atom);  

  JSValue buffers = argv[0]; // process the entire array of buffers
  size_t len = js_arrlen(js,buffers);
  SDL_GPUTransferBuffer *transfer_buffers[len];

  for (int i = 0; i < len; i++) {
  JSValue js_buf = JS_GetPropertyUint32(js,buffers,i);
  JSValue js_idx = JS_GetPropertyStr(js,js_buf, "index");

  Uint32 flag;

  if (JS_IsUndefined(js_idx))
    flag = SDL_GPU_BUFFERUSAGE_VERTEX;
  else {
    flag = SDL_GPU_BUFFERUSAGE_INDEX;
    JS_FreeValue(js,js_idx);
  }
    
  
  size_t stride, size;
  void *data = get_gpu_buffer(js, js_buf, &stride, &size);

  SDL_GPUBuffer *gpu_buffer = SDL_CreateGPUBuffer(
    gpu,
    &(SDL_GPUBufferCreateInfo) {
      .size = size,
      .usage = flag,
    }
  );

  JS_SetPropertyStr(js,js_buf, "gpu", SDL_GPUBuffer2js(js,gpu_buffer));
  JS_FreeValue(js,js_buf);

  transfer_buffers[i] = SDL_CreateGPUTransferBuffer(
    gpu,
    &(SDL_GPUTransferBufferCreateInfo) {
      .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
      .size = size
    }
  );

  void *mapped_data = SDL_MapGPUTransferBuffer(gpu, transfer_buffers[i], false);
  memcpy(mapped_data, data, size);
  SDL_UnmapGPUTransferBuffer(gpu, transfer_buffers[i]);
  
  SDL_UploadToGPUBuffer(copypass,
    &(SDL_GPUTransferBufferLocation) { .transfer_buffer = transfer_buffers[i], .offset = 0 },
    &(SDL_GPUBufferRegion) { .buffer = gpu_buffer, .offset = 0, .size = size },
    false
  );
  }
  SDL_EndGPUCopyPass(copypass);
  SDL_SubmitGPUCommandBuffer(cmds);
  
  for (int i = 0; i < len; i++)
    SDL_ReleaseGPUTransferBuffer(gpu, transfer_buffers[i]);
)


static const JSCFunctionListEntry js_SDL_GPUDevice_funcs[] = {
  MIST_FUNC_DEF(gpu, claim_window, 1),
  MIST_FUNC_DEF(gpu, make_pipeline, 1), // loads pipeline state into an object
  MIST_FUNC_DEF(gpu, make_sampler,1),
  MIST_FUNC_DEF(gpu, set_pipeline, 1), // grabs the gpu property off a pipeline value to load
  MIST_FUNC_DEF(gpu, load_texture, 2),
  MIST_FUNC_DEF(gpu, logical_size, 1),
  MIST_FUNC_DEF(gpu, newframe, 1),
  MIST_FUNC_DEF(gpu, camera, 2),
  MIST_FUNC_DEF(gpu, scale, 1),
  MIST_FUNC_DEF(gpu, texture, 4),
//  MIST_FUNC_DEF(gpu, geometry, 2),
  MIST_FUNC_DEF(gpu, viewport, 1),
  MIST_FUNC_DEF(gpu, make_sprite_mesh, 2),
  MIST_FUNC_DEF(gpu, driver, 0),
  MIST_FUNC_DEF(gpu, make_shader, 6),
  MIST_FUNC_DEF(gpu, load_gltf_model, 1),
  MIST_FUNC_DEF(gpu, acquire_cmd_buffer, 0),
  MIST_FUNC_DEF(gpu, upload, 1),
};

JSC_CCALL(renderpass_bind_pipeline,
  SDL_GPURenderPass *r = js2SDL_GPURenderPass(js,self);
  SDL_GPUGraphicsPipeline *pipe = js2SDL_GPUGraphicsPipeline(js,argv[0]);
  SDL_BindGPUGraphicsPipeline(r,pipe);
)

JSC_CCALL(renderpass_draw,
  SDL_GPURenderPass *pass = js2SDL_GPURenderPass(js,self);
  SDL_DrawGPUIndexedPrimitives(pass, js2number(js,argv[0]), js2number(js,argv[1]), js2number(js,argv[2]), js2number(js,argv[3]), js2number(js,argv[4]));
)

JSC_CCALL(renderpass_bind_model,
  SDL_GPURenderPass *pass = js2SDL_GPURenderPass(js, self);
  JSValue model = argv[0];

  // Helper macro to get and convert a GPU buffer from a property if it exists
  #define GET_GPU_BUFFER_IF_EXISTS(NAME) \
    JSValue NAME##_val = JS_GetPropertyStr(js, model, #NAME); \
    SDL_GPUBuffer *NAME##_buf = NULL; \
    if (!JS_IsUndefined(NAME##_val)) { \
      JSValue NAME##_gpu_val = JS_GetPropertyStr(js, NAME##_val, "gpu"); \
      if (!JS_IsUndefined(NAME##_gpu_val)) { \
        NAME##_buf = js2SDL_GPUBuffer(js, NAME##_gpu_val); \
      } \
      JS_FreeValue(js, NAME##_gpu_val); \
    } \
    JS_FreeValue(js, NAME##_val);

  // Attempt to retrieve each attribute buffer
  GET_GPU_BUFFER_IF_EXISTS(pos)
  GET_GPU_BUFFER_IF_EXISTS(uv)
  GET_GPU_BUFFER_IF_EXISTS(color)
  GET_GPU_BUFFER_IF_EXISTS(normal)

  // Indices (we assume these must always be present for indexed draws)
  JSValue indices_val = JS_GetPropertyStr(js, model, "indices");
  SDL_GPUBuffer *indices_buf = NULL;
  if (!JS_IsUndefined(indices_val)) {
    JSValue indices_gpu_val = JS_GetPropertyStr(js, indices_val, "gpu");
    if (!JS_IsUndefined(indices_gpu_val)) {
      indices_buf = js2SDL_GPUBuffer(js, indices_gpu_val);
      JS_FreeValue(js, indices_gpu_val);
    } else {
      JS_FreeValue(js, indices_val);
      return JS_ThrowInternalError(js, "Model has indices but no GPU buffer property.");
    }
  } else {
    JS_FreeValue(js, indices_val);
    return JS_ThrowReferenceError(js, "Model must have an 'indices' property.");
  }
  JS_FreeValue(js, indices_val);

  // Bind each buffer if it exists. We know our pipeline expects:
  // slot 0: pos (float3)
  // slot 1: uv (float2)
  // slot 2: color (float4)
  // slot 3: normal (float3)
  //
  // If a buffer is missing, we just don't bind anything for that slot.

  SDL_GPUBufferBinding binding;

  if (pos_buf) {
    binding.buffer = pos_buf;
    binding.offset = 0;
    SDL_BindGPUVertexBuffers(pass, 0, &binding, 1); // pos at slot 0
  }

  if (uv_buf) {
    binding.buffer = uv_buf;
    binding.offset = 0;
    SDL_BindGPUVertexBuffers(pass, 1, &binding, 1); // uv at slot 1
  }

  if (color_buf) {
    binding.buffer = color_buf;
    binding.offset = 0;
    SDL_BindGPUVertexBuffers(pass, 2, &binding, 1); // color at slot 2
  }

  if (normal_buf) {
    binding.buffer = normal_buf;
    binding.offset = 0;
    SDL_BindGPUVertexBuffers(pass, 3, &binding, 1); // normal at slot 3
  }

  // Bind the index buffer
  // If your indices are 16-bit, pass SDL_GPU_INDEXELEMENTSIZE_16BIT
  // If they're 32-bit, pass SDL_GPU_INDEXELEMENTSIZE_32BIT.
  // Our model_buffer code used uint32 for indices, so let's assume 32-bit.
  SDL_BindGPUIndexBuffer(pass, &(SDL_GPUBufferBinding){.buffer = indices_buf, .offset = 0}, SDL_GPU_INDEXELEMENTSIZE_16BIT);

  return JS_UNDEFINED;
)

JSC_CCALL(renderpass_end,
  SDL_GPURenderPass *pass = js2SDL_GPURenderPass(js,self);
  SDL_EndGPURenderPass(pass);
)


JSC_CCALL(renderpass_bind_mat,
  SDL_GPURenderPass *pass = js2SDL_GPURenderPass(js, self);
  JSValue mat = argv[0];

  // Look for the diffuse property on the material
  JSValue diffuse_val = JS_GetPropertyStr(js, mat, "diffuse");
  if (!JS_IsUndefined(diffuse_val)) {
    // Retrieve the GPU texture from diffuse.gpu
    JSValue diffuse_gpu_val = JS_GetPropertyStr(js, diffuse_val, "texture");
    SDL_GPUTexture *texture = js2SDL_GPUTexture(js, diffuse_gpu_val);

    // Retrieve the sampler from diffuse.sampler
    JSValue sampler_val = JS_GetPropertyStr(js, diffuse_val, "sampler");
    SDL_GPUSampler *sampler = NULL;
    if (JS_IsUndefined(sampler_val))
      return JS_ThrowReferenceError(js,"Image needs a sampler!");
    sampler = js2SDL_GPUSampler(js, sampler_val);

    // Free JS values now that we have native pointers
    JS_FreeValue(js, sampler_val);
    JS_FreeValue(js, diffuse_gpu_val);
    JS_FreeValue(js, diffuse_val);

    // If both texture and sampler are valid, bind them
    if (texture && sampler) {
      SDL_BindGPUFragmentSamplers(
        pass,
        0,
        &(SDL_GPUTextureSamplerBinding){ .texture = texture, .sampler = sampler },
        1
      );
    }
  } else {
    // No diffuse property, do nothing
    JS_FreeValue(js, diffuse_val);
  }
)

static const JSCFunctionListEntry js_SDL_GPURenderPass_funcs[] = {
  MIST_FUNC_DEF(renderpass, bind_pipeline, 1),
  MIST_FUNC_DEF(renderpass, bind_model, 1),
  MIST_FUNC_DEF(renderpass, bind_mat, 1),
  MIST_FUNC_DEF(renderpass, draw, 5),
  MIST_FUNC_DEF(renderpass, end, 0),
};

JSC_CCALL(copypass_end,
  SDL_GPUCopyPass *pass = js2SDL_GPUCopyPass(js,self);
  SDL_EndGPUCopyPass(pass);
)

float gw, gh;

// On GPUCommandBuffer object
JSC_CCALL(cmd_render_pass,
  SDL_GPUCommandBuffer *cmds = js2SDL_GPUCommandBuffer(js, self);
  SDL_GPUColorTargetInfo info = {0};
  SDL_GPUTexture *swapchain;
  Uint32 w, h;
  SDL_AcquireGPUSwapchainTexture(
    cmds,
    global_window,
    &swapchain,
    &w,
    &h
  );
  gw = w;
  gh = h;
  info.texture = swapchain;
  info.clear_color = (SDL_FColor){1.0,0,0,1.0};
  info.load_op = SDL_GPU_LOADOP_CLEAR;
  info.store_op = SDL_GPU_STOREOP_STORE; 
  SDL_GPURenderPass *pass = SDL_BeginGPURenderPass(cmds, &info, 1, NULL);
  if (!pass) return JS_ThrowInternalError(js, "Failed to begin render pass");
  return SDL_GPURenderPass2js(js,pass);
)

JSC_CCALL(cmd_bind_pipeline,
  SDL_GPUCommandBuffer *cmds = js2SDL_GPUCommandBuffer(js, self);
  SDL_GPUGraphicsPipeline *pipeline = js2SDL_GPUGraphicsPipeline(js, argv[0]);
  SDL_BindGPUGraphicsPipeline(cmds, pipeline);
)

JSC_CCALL(cmd_bind_vertex_buffer,
  SDL_GPUCommandBuffer *cmds = js2SDL_GPUCommandBuffer(js, self);
  int slot;
  JS_ToInt32(js, &slot, argv[0]);
  SDL_GPUBuffer *buffer = js2SDL_GPUBuffer(js, argv[1]);
  size_t offset = 0;
  if (argc > 2) JS_ToIndex(js, &offset, argv[2]); 
  SDL_BindGPUVertexBuffers(cmds, slot, &(SDL_GPUBufferBinding){.buffer = buffer, .offset = offset}, 1);
)

JSC_CCALL(cmd_bind_index_buffer,
  SDL_GPUCommandBuffer *cmds = js2SDL_GPUCommandBuffer(js, self);
  SDL_GPUBuffer *buffer = js2SDL_GPUBuffer(js, argv[0]);
  size_t offset = 0;
  if (argc > 1) JS_ToIndex(js, &offset, argv[1]);
  SDL_BindGPUIndexBuffer(cmds, &(SDL_GPUBufferBinding){.buffer = buffer, .offset = offset}, SDL_GPU_INDEXELEMENTSIZE_16BIT);
)

JSC_CCALL(cmd_bind_fragment_sampler,
  SDL_GPUCommandBuffer *cmds = js2SDL_GPUCommandBuffer(js, self);
  int slot;
  JS_ToInt32(js, &slot, argv[0]);
  SDL_GPUTexture *tex = js2SDL_GPUTexture(js, argv[1]);
  SDL_GPUSampler *sampler = js2SDL_GPUSampler(js, argv[2]);
  SDL_BindGPUFragmentSamplers(cmds, slot, &(SDL_GPUTextureSamplerBinding){.texture = tex, .sampler = sampler}, 1);
)

JSC_CCALL(cmd_push_vertex_uniform_data,
  SDL_GPUCommandBuffer *cmds = js2SDL_GPUCommandBuffer(js, self);
  int slot;
  JS_ToInt32(js, &slot, argv[0]);
  size_t buf_size;
  void *data = JS_GetArrayBuffer(js, &buf_size, argv[1]);
  SDL_PushGPUVertexUniformData(cmds, slot, data, buf_size);
)

JSC_CCALL(cmd_push_fragment_uniform_data,
  SDL_GPUCommandBuffer *cmds = js2SDL_GPUCommandBuffer(js, self);
  int slot;
  JS_ToInt32(js, &slot, argv[0]);
  size_t buf_size;
  void *data = JS_GetArrayBuffer(js, &buf_size, argv[1]);
  SDL_PushGPUFragmentUniformData(cmds, slot, data, buf_size);
)

JSC_CCALL(cmd_submit,
  SDL_GPUCommandBuffer *cmds = js2SDL_GPUCommandBuffer(js,self);
  SDL_SubmitGPUCommandBuffer(cmds);
)

JSC_CCALL(cmd_camera,
  SDL_GPUCommandBuffer *cmds = js2SDL_GPUCommandBuffer(js,self);  
  SDL_Rect vp;
  vp.w = gw;
  vp.h = gh;
  int centered = JS_ToBool(js,argv[1]);

  HMM_Mat4 proj;
if (centered) {
  // World coordinates: (0,0) at screen center, Y up
  proj = HMM_Orthographic_RH_NO(
      -(float)vp.w * 0.5f, (float)vp.w * 0.5f, // left, right
      -(float)vp.h * 0.5f, (float)vp.h * 0.5f, // bottom, top
      -1.0f, 1.0f
  );
} else {
  // UI coordinates: (0,0) at bottom-left corner, Y up
  proj = HMM_Orthographic_RH_NO(
      0.0f, (float)vp.w,   // left, right
      0.0f, (float)vp.h,   // bottom, top
      -1.0f, 1.0f
  );

}



transform *tra = js2transform(js, argv[0]);
HMM_Mat4 view = HMM_Translate((HMM_Vec3){-tra->pos.x, -tra->pos.y, 0.0f});

    // Update uniforms for this batch
    app_data data = {0};
    data.vp = HMM_MulM4(proj, view);
    data.time = SDL_GetTicksNS() / 1000000000.0f;
    SDL_PushGPUVertexUniformData(cmds, 0, &data, sizeof(data));
)

JSC_CCALL(cmd_camera_perspective,
  SDL_GPUCommandBuffer *cmds = js2SDL_GPUCommandBuffer(js, self);
  SDL_Rect vp;
  vp.w = gw;
  vp.h = gh;

  // Get camera object
  JSValue camera_obj = argv[0];

  // Extract camera parameters
  // transform property
  JSValue jstransform = JS_GetPropertyStr(js, camera_obj, "transform");
  if (JS_IsUndefined(jstransform))
    return JS_ThrowReferenceError(js, "Camera object must have a transform property.");

  transform *tra = js2transform(js, jstransform);
  JS_FreeValue(js,jstransform);

  // fov property (assume in degrees, convert to radians)
  JSValue fov_val = JS_GetPropertyStr(js, camera_obj, "fov");
  if (JS_IsUndefined(fov_val))
    return JS_ThrowReferenceError(js, "Camera object must have an fov property.");

  float fov_degrees = js2number(js, fov_val);
  float fov = fov_degrees * (HMM_PI / 180.0f); // convert to radians
  JS_FreeValue(js, fov_val);

  // near property
  JSValue near_val = JS_GetPropertyStr(js, camera_obj, "near");
  if (JS_IsUndefined(near_val))
    return JS_ThrowReferenceError(js, "Camera object must have a near property.");
  float near_plane = js2number(js, near_val);
  JS_FreeValue(js, near_val);

  // far property
  JSValue far_val = JS_GetPropertyStr(js, camera_obj, "far");
  if (JS_IsUndefined(far_val))
    return JS_ThrowReferenceError(js, "Camera object must have a far property.");
  float far_plane = js2number(js, far_val);
  JS_FreeValue(js, far_val);

  // Compute aspect ratio
  float aspect = (float)vp.w / (float)vp.h;

  // Create a perspective projection matrix
  HMM_Mat4 proj = HMM_Perspective_RH_NO(fov, aspect, near_plane, far_plane);

  // Compute a view matrix from the camera transform
  // If transform includes rotation and position, we should create a full transform matrix and invert it.
  // Assuming js2transform_mat4 returns a full world transform matrix of the camera (position & rotation):
  HMM_Mat4 camera_world = transform2mat(tra);
  HMM_Mat4 view = HMM_InvGeneralM4(camera_world);

  // If rotation isn't handled by js2transform_mat4, and only position is known:
  // HMM_Mat4 view = HMM_Translate(HMM_Vec3{-tra->pos.x, -tra->pos.y, -tra->pos.z});
  // For a proper camera, though, you'll want to consider orientation as well.

  // Update uniforms
  app_data data = {0};
  data.vp = HMM_MulM4(proj, view);
  data.time = SDL_GetTicksNS() / 1000000000.0f;
  SDL_PushGPUVertexUniformData(cmds, 0, &data, sizeof(data));
)

static const JSCFunctionListEntry js_SDL_GPUCommandBuffer_funcs[] = {
  MIST_FUNC_DEF(cmd, render_pass, 1),
  MIST_FUNC_DEF(cmd, bind_pipeline, 1),
  MIST_FUNC_DEF(cmd, bind_vertex_buffer, 2),
  MIST_FUNC_DEF(cmd, bind_index_buffer, 1),
  MIST_FUNC_DEF(cmd, bind_fragment_sampler, 3),
  MIST_FUNC_DEF(cmd, push_vertex_uniform_data, 2),
  MIST_FUNC_DEF(cmd, push_fragment_uniform_data, 2),
  MIST_FUNC_DEF(cmd, submit, 0),
  MIST_FUNC_DEF(cmd, camera, 2),
  MIST_FUNC_DEF(cmd, camera_perspective, 1),
};

JSC_CCALL(surface_blit,
  SDL_Surface *dst = js2SDL_Surface(js,self);
  rect dstrect = js2rect(js,argv[0]);
  SDL_Surface *src = js2SDL_Surface(js,argv[1]);
  rect srcrect = js2rect(js,argv[2]);
  SDL_BlitSurfaceScaled(src, &srcrect, dst, &dstrect, SDL_SCALEMODE_LINEAR);
)

JSC_CCALL(surface_scale,
  SDL_Surface *src = js2SDL_Surface(js,self);
  HMM_Vec2 wh = js2vec2(js,argv[0]);
  SDL_Surface *new = SDL_CreateSurface(wh.x,wh.y, SDL_PIXELFORMAT_RGBA32);
  SDL_BlitSurfaceScaled(src, NULL, new, NULL, SDL_SCALEMODE_LINEAR);
  ret = SDL_Surface2js(js,new);
)

static SDL_PixelFormatDetails pdetails = {
    .format = SDL_PIXELFORMAT_RGBA8888, // Standard RGBA8888 format
    .bits_per_pixel = 32,              // 8 bits per channel, 4 channels = 32 bits
    .bytes_per_pixel = 4,              // 4 bytes per pixel
    .padding = {0, 0},                 // Unused padding
    .Rmask = 0xFF000000,               // Red mask
    .Gmask = 0x00FF0000,               // Green mask
    .Bmask = 0x0000FF00,               // Blue mask
    .Amask = 0x000000FF,               // Alpha mask
    .Rbits = 8,                        // 8 bits for Red
    .Gbits = 8,                        // 8 bits for Green
    .Bbits = 8,                        // 8 bits for Blue
    .Abits = 8,                        // 8 bits for Alpha
    .Rshift = 24,                      // Red shift
    .Gshift = 16,                      // Green shift
    .Bshift = 8,                       // Blue shift
    .Ashift = 0                        // Alpha shift
};

JSC_CCALL(surface_fill,
  SDL_Surface *src = js2SDL_Surface(js,self);
  colorf color = js2color(js,argv[0]);
  rect r = {
    .x = 0,
    .y = 0,
    .w = src->w,
    .h = src->h
  };
  SDL_FillSurfaceRect(src, &r, SDL_MapRGBA(&pdetails, NULL, color.r*255,color.g*255,color.b*255,color.a*255));
)

JSC_CCALL(surface_rect,
  SDL_Surface *dst = js2SDL_Surface(js,self);
  rect r = js2rect(js,argv[0]);  
  colorf color = js2color(js,argv[1]);
  SDL_FillSurfaceRect(dst,&r,SDL_MapRGBA(&pdetails,NULL, color.r*255,color.g*255,color.b*255,color.a*255));
) 

JSC_CCALL(surface_dup,
  SDL_Surface *surf = js2SDL_Surface(js,self);
  SDL_Surface *conv = SDL_ConvertSurface(surf, SDL_PIXELFORMAT_RGBA8888);
  if (!conv)
    return JS_ThrowReferenceError(js, "could not blit to dup'd surface: %s", SDL_GetError());

  return SDL_Surface2js(js,conv);
)

static const JSCFunctionListEntry js_SDL_Surface_funcs[] = {
  MIST_FUNC_DEF(surface, blit, 3),
  MIST_FUNC_DEF(surface, scale, 1),
  MIST_FUNC_DEF(surface,fill,1),
  MIST_FUNC_DEF(surface,rect,2),
  MIST_FUNC_DEF(surface, dup, 0),
};

JSC_CCALL(thread_wait,
  SDL_Thread *th = js2SDL_Thread(js,self);
  SDL_WaitThread(th, NULL);
)

static const JSCFunctionListEntry js_SDL_Thread_funcs[] = {
  MIST_FUNC_DEF(thread,wait,0),
};

JSC_CCALL(camera_frame,
  SDL_ClearError();
  SDL_Camera *cam = js2SDL_Camera(js,self);
  if (!cam) return JS_ThrowReferenceError(js,"Self was not a camera: %s", SDL_GetError());
  SDL_Surface *surf = SDL_AcquireCameraFrame(cam, NULL);
  if (!surf) {
    const char *msg = SDL_GetError();
    if (msg[0] != 0)
      return JS_ThrowReferenceError(js,"Could not get camera frame: %s", SDL_GetError());
    else return JS_UNDEFINED;
  }
  return SDL_Surface2js(js,surf);
  SDL_Surface *newsurf = SDL_CreateSurface(surf->w, surf->h, surf->format);
  SDL_ReleaseCameraFrame(cam,surf);

  int didit = SDL_BlitSurface(surf, NULL, newsurf, NULL);
  if (!didit) {
    SDL_DestroySurface(newsurf);
    return JS_ThrowReferenceError(js, "Could not blit: %s", SDL_GetError());
  }

  return SDL_Surface2js(js,newsurf);
)

JSC_CCALL(camera_release_frame,
  SDL_Camera *cam = js2SDL_Camera(js,self);
  SDL_Surface *surf = js2SDL_Surface(js,argv[0]);
  SDL_ReleaseCameraFrame(cam,surf);
)

static const JSCFunctionListEntry js_SDL_Camera_funcs[] = 
{
  MIST_FUNC_DEF(camera, frame, 0),
  MIST_FUNC_DEF(camera, release_frame, 1),
};

static const JSCFunctionListEntry js_SDL_Cursor_funcs[] = {};

JSC_CCALL(texture_mode,
  SDL_Texture *tex = js2SDL_Texture(js,self);
  SDL_SetTextureScaleMode(tex,js2number(js,argv[0]));
)

static const JSCFunctionListEntry js_SDL_Texture_funcs[] = {
  MIST_FUNC_DEF(texture, mode, 1),
};

JSC_CCALL(gputexture_mode,

)

static const JSCFunctionListEntry js_SDL_GPUTexture_funcs[] = {
  MIST_FUNC_DEF(gputexture, mode, 1),
};

JSC_CCALL(input_mouse_lock, SDL_CaptureMouse(JS_ToBool(js,argv[0])))
JSC_CCALL(input_mouse_show,
  if (JS_ToBool(js,argv[0]))
    SDL_ShowCursor();
  else
    SDL_HideCursor();
)

JSC_CCALL(input_cursor_set,
  SDL_Cursor *c = js2SDL_Cursor(js,argv[0]);
  printf("setting cursor to %p\n", c);
  if (!SDL_SetCursor(c))
    return JS_ThrowReferenceError(js, "could not set cursor: %s", SDL_GetError());
)

JSC_CCALL(input_keyname,
  return JS_NewString(js, SDL_GetKeyName(js2number(js,argv[0])));
)

JSC_CCALL(input_keymod,
  return js_keymod(js);
)

static const JSCFunctionListEntry js_input_funcs[] = {
  MIST_FUNC_DEF(input, mouse_show, 1),
  MIST_FUNC_DEF(input, mouse_lock, 1),
  MIST_FUNC_DEF(input, cursor_set, 1),
  MIST_FUNC_DEF(input, keyname, 1),
  MIST_FUNC_DEF(input, keymod, 0),
};

JSC_CCALL(prosperon_guid,
  char guid[33];
  for (int i = 0; i < 4; i++) {
    int r = rand();
    for (int j = 0; j < 8; j++) {
      guid[i*8+j] = "0123456789abcdef"[r%16];
      r /= 16;
    }
  }

  guid[32] = 0;
  return JS_NewString(js,guid);
)

JSC_SCALL(prosperon_openurl,
  if (!SDL_OpenURL(str))
    ret = JS_ThrowReferenceError(js, "unable to open url %s: %s\n", str, SDL_GetError());
)

JSC_CCALL(prosperon_push_event,
  SDL_UserEvent e;
  SDL_zero(e);
  e.type = SDL_EVENT_USER;
  e.timestamp = SDL_GetTicksNS();
  e.code = 0;
  JSValue fn = JS_DupValue(js,argv[0]);
  e.data1 = malloc(sizeof(JSValue));
  *(JSValue*)e.data1 = fn;
  SDL_PushEvent(&e);
)

static const JSCFunctionListEntry js_prosperon_funcs[] = {
  MIST_FUNC_DEF(prosperon, guid, 0),
  MIST_FUNC_DEF(prosperon, openurl, 1),
  MIST_FUNC_DEF(prosperon, push_event, 1),
};

JSC_CCALL(time_now, 
  struct timeval ct;
  gettimeofday(&ct, NULL);
  return number2js(js,(double)ct.tv_sec+(double)(ct.tv_usec/1000000.0));
)

JSValue js_time_computer_dst(JSContext *js, JSValue self) {
  time_t t = time(NULL);
  return JS_NewBool(js,localtime(&t)->tm_isdst);
}

JSValue js_time_computer_zone(JSContext *js, JSValue self) {
  time_t t = time(NULL);
  time_t local_t = mktime(localtime(&t));
  double diff = difftime(t, local_t);
  return number2js(js,diff/3600);
}

static const JSCFunctionListEntry js_time_funcs[] = {
  MIST_FUNC_DEF(time, now, 0),
  MIST_FUNC_DEF(time, computer_dst, 0),
  MIST_FUNC_DEF(time, computer_zone, 0)
};

JSC_SCALL(console_log,
  printf("%s\n", str);
)

static const JSCFunctionListEntry js_console_funcs[] = {
  MIST_FUNC_DEF(console,log,1),
};

static JSValue instr_v = JS_UNDEFINED;
int iiihandle(JSRuntime *rt, void *data)
{
  script_call_sym(instr_v, 0, NULL);
  return 0;
}

JSC_CCALL(profile_gather,
  instr_v = JS_DupValue(js, argv[1]);
  JS_SetInterruptHandler(JS_GetRuntime(js), iiihandle, NULL);
)

JSC_CCALL(profile_gather_rate,
  JS_SetInterruptRate(js2number(js,argv[0]));
)

JSC_CCALL(profile_gather_stop,
  JS_SetInterruptHandler(JS_GetRuntime(js),NULL,NULL);
)

JSC_CCALL(profile_best_t,
  char* result[50];
  double seconds = js2number(js,argv[0]);
  if (seconds < 1e-6)
    snprintf(result, 50, "%.2f ns", seconds * 1e9);
  else if (seconds < 1e-3)
    snprintf(result, 50, "%.2f µs", seconds * 1e6);
  else if (seconds < 1)
    snprintf(result, 50, "%.2f ms", seconds * 1e3);
  else
    snprintf(result, 50, "%.2f s", seconds);

  return JS_NewString(js,result);
)

JSC_CCALL(profile_now, return number2js(js, SDL_GetTicksNS()/1000000000.f))

static const JSCFunctionListEntry js_profile_funcs[] = {
  MIST_FUNC_DEF(profile,now,0),
  MIST_FUNC_DEF(profile,best_t, 1),
  MIST_FUNC_DEF(profile,gather,2),
  MIST_FUNC_DEF(profile,gather_rate,1),
  MIST_FUNC_DEF(profile,gather_stop,0),
};

JSC_CCALL(debug_stack_depth, return number2js(js,js_debugger_stack_depth(js)))
JSC_CCALL(debug_build_backtrace, return js_debugger_build_backtrace(js,NULL))
JSC_CCALL(debug_closure_vars, return js_debugger_closure_variables(js,argv[0]))
JSC_CCALL(debug_local_vars, return js_debugger_local_variables(js, js2number(js,argv[0])))
JSC_CCALL(debug_fn_info, return js_debugger_fn_info(js, argv[0]));
JSC_CCALL(debug_backtrace_fns, return js_debugger_backtrace_fns(js,NULL));
JSC_CCALL(debug_dump_obj, return js_dump_value(js, argv[0]));
static const JSCFunctionListEntry js_debug_funcs[] = {
  MIST_FUNC_DEF(debug, stack_depth, 0),
  MIST_FUNC_DEF(debug, build_backtrace, 0),
  MIST_FUNC_DEF(debug, closure_vars, 1),
  MIST_FUNC_DEF(debug, local_vars, 1),
  MIST_FUNC_DEF(debug,fn_info, 1),
  MIST_FUNC_DEF(debug,backtrace_fns,0),
  MIST_FUNC_DEF(debug, dump_obj, 1),
};

JSC_SCALL(io_rm,
  if (!PHYSFS_delete(str)) ret = JS_ThrowReferenceError(js,"%s", PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
)
JSC_SCALL(io_mkdir,
  if (!PHYSFS_mkdir(str)) ret = JS_ThrowReferenceError(js,"%s", PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));  
)

JSC_SCALL(io_exists, ret = JS_NewBool(js,PHYSFS_exists(str)); )

JSC_SCALL(io_stat,
  PHYSFS_Stat stat;
  if (!PHYSFS_stat(str, &stat))
    return JS_ThrowReferenceError(js, "%s", PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));

  ret = JS_NewObject(js);
  JS_SetPropertyStr(js,ret,"filesize", number2js(js,stat.filesize));
  JS_SetPropertyStr(js,ret,"modtime", number2js(js,stat.modtime));
  JS_SetPropertyStr(js,ret,"createtime", number2js(js,stat.createtime));
  JS_SetPropertyStr(js,ret,"accesstime", number2js(js,stat.accesstime));
)

JSC_SCALL(io_slurp,
  PHYSFS_File *f = PHYSFS_openRead(str);
  if (!f) {
    ret = JS_ThrowReferenceError(js,"physfs error when slurping %s: %s", str, PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
    goto END;
  }
  PHYSFS_Stat stat;
  PHYSFS_stat(str,&stat);
  void *data = malloc(stat.filesize);
  PHYSFS_readBytes(f,data,stat.filesize);
  PHYSFS_close(f);  
  if (JS_ToBool(js,argv[1]))
    ret = JS_NewStringLen(js,data, stat.filesize);
  else
    ret = JS_NewArrayBufferCopy(js,data,stat.filesize);

  END:
)

JSC_SCALL(io_slurpwrite,
  PHYSFS_File *f = PHYSFS_openWrite(str);
  if (!f) {
    ret = JS_ThrowReferenceError(js,"%s", PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
    goto END;
  }
  size_t len;
  unsigned char *data;
  if (JS_IsString(argv[1]))
    data = JS_ToCStringLen(js,&len,argv[1]);
   else
    data = JS_GetArrayBuffer(js,&len, argv[1]);

  size_t wrote = PHYSFS_writeBytes(f,data, len);
  PHYSFS_close(f);  
  if (wrote == -1 || wrote < len)
    ret = JS_ThrowReferenceError(js,"%s", PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
  
  END:
)

JSC_SCALL(io_mount,
  if (!PHYSFS_mount(str,NULL,0)) ret = JS_ThrowReferenceError(js,"%s", PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
)

JSC_SCALL(io_unmount,
  if (!PHYSFS_unmount(str)) ret = JS_ThrowReferenceError(js,"%s", PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
)

JSC_SCALL(io_writepath,
  if (!PHYSFS_setWriteDir(str)) ret = JS_ThrowReferenceError(js,"%s", PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
)

JSC_SSCALL(io_gamemode,
  const char *prefdir = PHYSFS_getPrefDir(str, str2);
  printf("%s\n", prefdir);
  PHYSFS_setWriteDir(prefdir);
)

struct globdata {
  JSContext *js;
  JSValue arr;
  char **globs;
  int idx;
};

int globfs_cb(struct globdata *data, char *dir, char *file)
{
  int needfree = 0;
  char *path;
  if (dir[0] == 0) path = file;
  else {
    path = malloc(strlen(dir) + strlen(file) + 2);
    path[0] = 0;
    strcat(path,dir);
    strcat(path,"/");
    strcat(path,file);
    needfree = 1;
  }

  char **glob = data->globs;
  while (*glob != NULL) {
    if (wildmatch(*glob, path, WM_WILDSTAR) == WM_MATCH)
      goto END;
    *glob++;
  }

  PHYSFS_Stat stat;
  PHYSFS_stat(path, &stat);
  if (stat.filetype == PHYSFS_FILETYPE_DIRECTORY) {
    PHYSFS_enumerate(path, globfs_cb, data);
    goto END;
  }

  JS_SetPropertyUint32(data->js, data->arr, data->idx++, JS_NewString(data->js,path));

  END:
  if (needfree) free(path);
  return 1;
}

JSC_CCALL(io_globfs,
  ret = JS_NewArray(js);
  struct globdata data;
  data.js = js;
  data.arr = ret;
  data.idx = 0;
  int globs_len = js_arrlen(js,argv[0]);
  char *globs[globs_len+1];
  for (int i = 0; i < globs_len; i++) {
    JSValue g = JS_GetPropertyUint32(js,argv[0],i);
    globs[i] = JS_ToCString(js,g);
    JS_FreeValue(js,g);
  }
  globs[globs_len] = NULL;
  data.globs = globs;

  PHYSFS_enumerate("", globfs_cb, &data);

  return data.arr;
)

JSC_CCALL(io_basedir, return JS_NewString(js,PHYSFS_getBaseDir()))

static const JSCFunctionListEntry js_io_funcs[] = {
  MIST_FUNC_DEF(io, rm, 1),
  MIST_FUNC_DEF(io, mkdir, 1),
  MIST_FUNC_DEF(io,stat,1),
  MIST_FUNC_DEF(io, globfs, 1),
  MIST_FUNC_DEF(io, exists, 1),
  MIST_FUNC_DEF(io, mount, 1),
  MIST_FUNC_DEF(io,unmount,1),
  MIST_FUNC_DEF(io,slurp,2),
  MIST_FUNC_DEF(io,slurpwrite,2),
  MIST_FUNC_DEF(io,writepath, 1),
  MIST_FUNC_DEF(io,basedir, 0),
  MIST_FUNC_DEF(io, gamemode, 2),
};

JSC_GETSET(transform, pos, vec3)
JSC_GETSET(transform, scale, vec3f)
JSC_GETSET(transform, rotation, quat)
JSC_CCALL(transform_move,
  transform *t = js2transform(js,self);
  transform_move(t, js2vec3(js,argv[0]));
)

JSC_CCALL(transform_lookat,
  HMM_Vec3 point = js2vec3(js,argv[0]);
  transform *go = js2transform(js,self);
  HMM_Mat4 m = HMM_LookAt_RH(go->pos, point, vUP);
  go->rotation = HMM_M4ToQ_RH(m);
  go->dirty = true;
)

JSC_CCALL(transform_rotate,
  HMM_Vec3 axis = js2vec3(js,argv[0]);
  transform *t = js2transform(js,self);
  HMM_Quat rot = HMM_QFromAxisAngle_RH(axis, js2angle(js,argv[1]));
  t->rotation = HMM_MulQ(t->rotation,rot);
  t->dirty = true;
)

JSC_CCALL(transform_angle,
  HMM_Vec3 axis = js2vec3(js,argv[0]);
  transform *t = js2transform(js,self);
  if (axis.x) return angle2js(js,HMM_Q_Roll(t->rotation));
  if (axis.y) return angle2js(js,HMM_Q_Pitch(t->rotation));
  if (axis.z) return angle2js(js,HMM_Q_Yaw(t->rotation));
  return angle2js(js,0);
)

JSC_CCALL(transform_direction,
  transform *t = js2transform(js,self);
  return vec32js(js, HMM_QVRot(js2vec3(js,argv[0]), t->rotation));
)

JSC_CCALL(transform_phys2d,
  transform *t = js2transform(js,self);
  HMM_Vec2 v = js2vec2(js,argv[0]);
  float av = js2number(js,argv[1]);
  float dt = js2number(js,argv[2]);
  transform_move(t, (HMM_Vec3){v.x*dt,v.y*dt,0});
  HMM_Quat rot = HMM_QFromAxisAngle_RH((HMM_Vec3){0,0,1}, av*dt);
  t->rotation = HMM_MulQ(t->rotation, rot);
)

JSC_CCALL(transform_unit,
  transform *t = js2transform(js,self);
  t->pos = v3zero;
  t->rotation = QUAT1;
  t->scale = v3one;
)

JSC_CCALL(transform_trs,
  transform *t = js2transform(js,self);
  t->pos = JS_IsUndefined(argv[0]) ? v3zero : js2vec3(js,argv[0]);
  t->rotation = JS_IsUndefined(argv[1]) ? QUAT1 : js2quat(js,argv[1]);
  t->scale = JS_IsUndefined(argv[2]) ? v3one : js2vec3(js,argv[2]);
)

JSC_CCALL(transform_rect,
  transform *t = js2transform(js,self);
  rect r = js2rect(js,argv[0]);
  t->pos = (HMM_Vec3){r.x,r.y,0};
  t->scale = (HMM_Vec3){r.w,r.h,1};
  t->rotation = QUAT1;
)

static const JSCFunctionListEntry js_transform_funcs[] = {
  CGETSET_ADD(transform, pos),
  CGETSET_ADD(transform, scale),
  CGETSET_ADD(transform, rotation),
  MIST_FUNC_DEF(transform, trs, 3),
  MIST_FUNC_DEF(transform, phys2d, 3),
  MIST_FUNC_DEF(transform, move, 1),
  MIST_FUNC_DEF(transform, rotate, 2),
  MIST_FUNC_DEF(transform, angle, 1),
  MIST_FUNC_DEF(transform, lookat, 1),
  MIST_FUNC_DEF(transform, direction, 1),
  MIST_FUNC_DEF(transform, unit, 0),
  MIST_FUNC_DEF(transform, rect, 1),
};

JSC_CCALL(datastream_time, return number2js(js,plm_get_time(js2datastream(js,self)->plm)); )
JSC_CCALL(datastream_seek, ds_seek(js2datastream(js,self), js2number(js,argv[0])))
JSC_CCALL(datastream_advance, ds_advance(js2datastream(js,self), js2number(js,argv[0])))
JSC_CCALL(datastream_duration, return number2js(js,ds_length(js2datastream(js,self))))
JSC_CCALL(datastream_framerate, return number2js(js,plm_get_framerate(js2datastream(js,self)->plm)))

JSC_GETSET_CALLBACK(datastream, callback)

static const JSCFunctionListEntry js_datastream_funcs[] = {
  MIST_FUNC_DEF(datastream, time, 0),
  MIST_FUNC_DEF(datastream, seek, 1),
  MIST_FUNC_DEF(datastream, advance, 1),
  MIST_FUNC_DEF(datastream, duration, 0),
  MIST_FUNC_DEF(datastream, framerate, 0),
  CGETSET_ADD(datastream, callback),
};

JSC_GETSET_CALLBACK(timer, fn)
JSC_GETSET(timer, remain, number)

static const JSCFunctionListEntry js_timer_funcs[] = {
  CGETSET_ADD(timer, remain),
  CGETSET_ADD(timer, fn),
};

JSC_GETSET(font, linegap, number)
JSC_GET(font, height, number)
JSC_GET(font, ascent, number)
JSC_GET(font, descent, number)

static const JSCFunctionListEntry js_font_funcs[] = {
  CGETSET_ADD(font, linegap),
  MIST_GET(font, height),
  MIST_GET(font, ascent),
  MIST_GET(font, descent)
};

const char *STRTEST = "TEST STRING";

JSC_CCALL(performance_barecall,)
JSC_CCALL(performance_unpack_array,
  void *v = js2cpvec2arr(js,argv[0]);
  arrfree(v);
)
JSC_CCALL(performance_pack_num, return number2js(js,1.0))
JSC_CCALL(performance_pack_string, return JS_NewStringLen(js, STRTEST, sizeof(*STRTEST)))
JSC_CCALL(performance_unpack_string, JS_ToCString(js, argv[0]))
JSC_CCALL(performance_call_fn_n, 
  for (int i = 0; i < js2number(js,argv[1]); i++)
    script_call_sym(argv[0],0,NULL);
  script_call_sym(argv[2],0,NULL);
)

static const JSCFunctionListEntry js_performance_funcs[] = {
  MIST_FUNC_DEF(performance, barecall,0),
  MIST_FUNC_DEF(performance, unpack_array, 1),
  MIST_FUNC_DEF(performance, pack_num, 0),
  MIST_FUNC_DEF(performance, pack_string, 0),
  MIST_FUNC_DEF(performance, unpack_string, 1),
  MIST_FUNC_DEF(performance, call_fn_n, 3)
};

JSC_CCALL(geometry_rect_intersection,
  rect a = js2rect(js,argv[0]);
  rect b = js2rect(js,argv[1]);
  rect c;
  SDL_GetRectIntersectionFloat(&a, &b, &c);
  return rect2js(js,c);
)

JSC_CCALL(geometry_rect_inside,
  rect inner = js2rect(js,argv[0]);
  rect outer = js2rect(js,argv[1]);
  return JS_NewBool(js,
         inner.x >= outer.x &&
         inner.x + inner.w <= outer.x + outer.w &&
         inner.y >= outer.y &&
         inner.y + inner.h <= outer.y + outer.h
         );
)

JSC_CCALL(geometry_rect_random,
  rect a = js2rect(js,argv[0]);
  return vec22js(js,(HMM_Vec2){
    a.x + rand_range(-0.5,0.5)*a.w,
    a.y + rand_range(-0.5,0.5)*a.h
  });
)

JSC_CCALL(geometry_rect_point_inside,
  rect a = js2rect(js,argv[0]);
  HMM_Vec2 p = js2vec2(js,argv[1]);
  return JS_NewBool(js,p.x >= a.x && p.x <= a.x+a.w && p.y <= a.y+a.h && p.y >= a.y);
)

JSC_CCALL(geometry_cwh2rect,
  HMM_Vec2 c = js2vec2(js,argv[0]);
  HMM_Vec2 wh = js2vec2(js,argv[1]);
  rect r;
  r.x = c.x;
  r.y = c.y;
  r.w = wh.x;
  r.h = wh.y;
  return rect2js(js,r);
)

JSC_CCALL(geometry_rect_pos,
  rect r = js2rect(js,argv[0]);
  return vec22js(js,(HMM_Vec2){
    .x = r.x,
    .y = r.y
  });
)

JSC_CCALL(geometry_rect_move,
  rect r = js2rect(js,argv[0]);
  HMM_Vec2 move = js2vec2(js,argv[1]);
//  cblas_saxpy(2, 1.0f, move.e, 1, &r, 1);
  r.x += move.x;
  r.y += move.y;
  return rect2js(js,r);
)

static const JSCFunctionListEntry js_geometry_funcs[] = {
  MIST_FUNC_DEF(geometry, rect_intersection, 2),
  MIST_FUNC_DEF(geometry, rect_inside, 2),
  MIST_FUNC_DEF(geometry, rect_random, 1),
  MIST_FUNC_DEF(geometry, cwh2rect, 2),
  MIST_FUNC_DEF(geometry, rect_point_inside, 2),
  MIST_FUNC_DEF(geometry, rect_pos, 1),
  MIST_FUNC_DEF(geometry, rect_move, 2),
};

JSValue js_os_cwd(JSContext *js, JSValue self, int argc, JSValue *argv)
{
  char cwd[PATH_MAX];
  #ifndef __EMSCRIPTEN__
  getcwd(cwd, sizeof(cwd));
  #else
  cwd[0] = '.';
  cwd[1] = 0;
  #endif
  return JS_NewString(js,cwd);
}

JSC_SCALL(os_env,
  char *env = getenv(str);
  if (env) ret = JS_NewString(js,env);
)

JSValue js_os_sys(JSContext *js, JSValue self, int argc, JSValue *argv)
{
  return JS_NewString(js, SDL_GetPlatform());
}

JSC_CCALL(os_exit, exit(js2number(js,argv[0]));)
JSC_CCALL(os_gc,
  return JS_RunGC(JS_GetRuntime(js), js)
)
JSC_CCALL(os_mem_limit, JS_SetMemoryLimit(JS_GetRuntime(js), js2number(js,argv[0])))
JSC_CCALL(os_gc_threshold, JS_SetGCThreshold(JS_GetRuntime(js), js2number(js,argv[0])))
JSC_CCALL(os_max_stacksize, JS_SetMaxStackSize(JS_GetRuntime(js), js2number(js,argv[0])))
JSC_CCALL(os_rt_info, return JS_GetRTInfo(JS_GetRuntime(js),js))

static JSValue tmp2js(JSContext *js,FILE *tmp)
{
  size_t size = ftell(tmp);
  rewind(tmp);
  char *buffer = calloc(size+1, sizeof(char));
  fread(buffer, sizeof(char),size, tmp);
  JSValue ret = JS_NewString(js,buffer);
  free(buffer);
  return ret;
}

JSC_CCALL(os_dump_atoms,
  FILE *tmp = tmpfile();
  quickjs_set_dumpout(tmp);
  JS_PrintAtoms(JS_GetRuntime(js));
  ret = tmp2js(js,tmp);
)

JSC_CCALL(os_dump_shapes,
  FILE *tmp = tmpfile();
  quickjs_set_dumpout(tmp);
  JS_PrintShapes(JS_GetRuntime(js));
  size_t size = ftell(tmp);
  rewind(tmp);
  char buffer[size];
  fgets(buffer, sizeof(char)*size, tmp);
  fclose(tmp);
  ret = JS_NewString(js,buffer);
)

JSC_CCALL(os_calc_mem,
  return number2js(js,JS_MyValueSize(JS_GetRuntime(js), argv[0]));
)

#define JSOBJ_ADD_FIELD(OBJ, STRUCT, FIELD, TYPE) \
JS_SetPropertyStr(js, OBJ, #FIELD, TYPE##2js(js,STRUCT.FIELD));\

#define JSJMEMRET(FIELD) JSOBJ_ADD_FIELD(ret, jsmem, FIELD, number)

JSC_CCALL(os_memstate,
  JSMemoryUsage jsmem;
  JS_FillMemoryState(JS_GetRuntime(js), &jsmem);
  ret = JS_NewObject(js);
  JSJMEMRET(malloc_size)
  JSJMEMRET(malloc_limit)
)

JSC_CCALL(os_mallinfo,
  ret = JS_UNDEFINED;
  /*struct mallinfo jsmem = mallinfo();
  ret = JS_NewObject(js);
  JSJMEMRET(arena);
  JSJMEMRET(ordblks);
  JSJMEMRET(smblks);
  JSJMEMRET(hblks);
  JSJMEMRET(hblkhd);
  JSJMEMRET(usmblks);
  JSJMEMRET(uordblks);
  JSJMEMRET(fordblks);
  JSJMEMRET(keepcost);*/
)

JSC_CCALL(os_rusage,
  ret = JS_NewObject(js);

#ifndef _WIN32
  struct rusage jsmem;
  getrusage(RUSAGE_SELF, &jsmem);
  JSJMEMRET(ru_maxrss);
  JSJMEMRET(ru_ixrss);
  JSJMEMRET(ru_idrss);
  JSJMEMRET(ru_isrss);
  JSJMEMRET(ru_minflt);
  JSJMEMRET(ru_majflt);
  JSJMEMRET(ru_nswap);
  JSJMEMRET(ru_inblock);
  JSJMEMRET(ru_oublock);
  JSJMEMRET(ru_msgsnd);
  JSJMEMRET(ru_msgrcv);
  JSJMEMRET(ru_nsignals);
  JSJMEMRET(ru_nvcsw);
  JSJMEMRET(ru_nivcsw);
#endif
)

JSC_CCALL(os_mem,
  JSMemoryUsage jsmem;
  JS_ComputeMemoryUsage(JS_GetRuntime(js), &jsmem);
  ret = JS_NewObject(js);
  JSJMEMRET(malloc_size)
  JSJMEMRET(malloc_limit)
  JSJMEMRET(memory_used_size)
  JSJMEMRET(memory_used_count)
  JSJMEMRET(atom_count)
  JSJMEMRET(atom_size)
  JSJMEMRET(str_count)
  JSJMEMRET(str_size)
  JSJMEMRET(obj_count)
  JSJMEMRET(prop_count)
  JSJMEMRET(prop_size)
  JSJMEMRET(shape_count)
  JSJMEMRET(shape_size)
  JSJMEMRET(js_func_count)
  JSJMEMRET(js_func_size)
  JSJMEMRET(js_func_code_size)
  JSJMEMRET(js_func_pc2line_count)
  JSJMEMRET(js_func_pc2line_size)
  JSJMEMRET(c_func_count)
  JSJMEMRET(array_count)
  JSJMEMRET(fast_array_count)
  JSJMEMRET(fast_array_elements)
  JSJMEMRET(binary_object_count)
  JSJMEMRET(binary_object_size)
)

JSC_CCALL(os_dump_mem,
  FILE *tmp = tmpfile();
  JSMemoryUsage mem = {0};
  JS_DumpMemoryUsage(tmp, &mem, JS_GetRuntime(js));
  ret = tmp2js(js,tmp);
)

JSC_CCALL(os_value_id,
  return number2js(js,(intptr_t)JS_VALUE_GET_PTR(argv[0]));
)

static double gc_t = 0;
static double gc_mem = 0;
static double gc_startmem = 0;
void script_report_gc_time(double t, double startmem, double mem)
{
  gc_t = t;
  gc_mem = mem;
  gc_startmem = startmem;
}

static FILE *cycles;

JSC_CCALL(os_check_cycles,
  if (ftell(cycles) == 0) return JS_UNDEFINED;
  cycles = tmpfile();
  quickjs_set_cycleout(cycles);
  ret = tmp2js(js,cycles);  
)

JSC_CCALL(os_check_gc,
  if (gc_t == 0) return JS_UNDEFINED;

  JSValue gc = JS_NewObject(js);
  JS_SetPropertyStr(js, gc, "time", number2js(js,gc_t));
  JS_SetPropertyStr(js, gc, "mem", number2js(js,gc_mem));
  JS_SetPropertyStr(js, gc, "startmem", number2js(js,gc_startmem));
  gc_t = 0;
  gc_mem = 0;
  gc_startmem = 0;
  return gc;
)

JSC_SSCALL(os_eval, ret = script_eval(str, str2))
JSC_CCALL(os_make_timer, return timer2js(js,timer_make(js,argv[0])))
JSC_CCALL(os_update_timers, timer_update(js, js2number(js,argv[0])))

JSC_CCALL(os_make_texture,
  size_t len;
  void *raw = JS_GetArrayBuffer(js, &len, argv[0]);
  if (!raw) return JS_ThrowReferenceError(js, "could not load texture with array buffer");

  int n, width, height;
  void *data = stbi_load_from_memory(raw, len, &width, &height, &n, 0);

  if (data == NULL)
    return JS_ThrowReferenceError(js, "no known image type from pixel data");

  int FMT = 0;
  if (n == 4)
    FMT = SDL_PIXELFORMAT_RGBA32;
  else if (n == 3)
    FMT = SDL_PIXELFORMAT_RGB24;
  else {
    free(data);
    return JS_ThrowReferenceError(js, "unknown pixel format. got %d channels", n);
  }
    
  SDL_Surface *surf = SDL_CreateSurfaceFrom(width,height,FMT, data, width*n);
  if (!surf) {
    free(data);
    return JS_ThrowReferenceError(js, "Error creating surface from data: %s",SDL_GetError());
  }

  ret = SDL_Surface2js(js,surf);
)

JSC_CCALL(os_make_gif,
  size_t rawlen;
  void *raw = JS_GetArrayBuffer(js, &rawlen, argv[0]);
  if (!raw) return JS_ThrowReferenceError(js, "could not load gif from supplied array buffer");

  int n;
  int frames;
  int *delays;
  int width;
  int height;
  void *pixels = stbi_load_gif_from_memory(raw, rawlen, &delays, &width, &height, &frames, &n, 4);

  JSValue gif = JS_NewObject(js);
  ret = gif;

  if (frames == 1) {
    // still image, so return just that
    JS_SetPropertyStr(js, gif, "surface", SDL_Surface2js(js,SDL_CreateSurfaceFrom(width,height,SDL_PIXELFORMAT_RGBA32, pixels, width*4)));
    return gif;
  }

  JSValue delay_arr = JS_NewArray(js);

  for (int i = 0; i < frames; i++) {
    JSValue frame = JS_NewObject(js);
    JS_SetPropertyStr(js, frame, "time", number2js(js,(float)delays[i]/1000.0));
    void *frame_pixels = malloc(width*height*4);
    if (!frame_pixels) {
      JS_FreeValue(js,gif);
      ret = JS_ThrowOutOfMemory(js);
      goto CLEANUP;
    }
    memcpy(frame_pixels, (unsigned char*)pixels+(width*height*4*i), width*height*4);
    SDL_Surface *framesurf = SDL_CreateSurfaceFrom(width,height,SDL_PIXELFORMAT_RGBA32,frame_pixels, width*4);
    if (!framesurf) {
      ret = JS_ThrowReferenceError(js, "failed to create SDL_Surface: %s", SDL_GetError());
      goto CLEANUP;
    }
    JS_SetPropertyStr(js, frame, "surface", SDL_Surface2js(js,framesurf));
    JS_SetPropertyUint32(js, delay_arr, i, frame);
  }

  JS_SetPropertyStr(js, gif, "frames", delay_arr);

CLEANUP:
  free(delays);
  free(pixels);
)

JSValue aseframe2js(JSContext *js, ase_frame_t aframe)
{
  JSValue frame = JS_NewObject(js);
  void *frame_pixels = malloc(aframe.ase->w*aframe.ase->h*4);
  memcpy(frame_pixels, aframe.pixels, aframe.ase->w*aframe.ase->h*4);
  SDL_Surface *surf = SDL_CreateSurfaceFrom(aframe.ase->w, aframe.ase->h, SDL_PIXELFORMAT_RGBA32, frame_pixels, aframe.ase->w*4);
  JS_SetPropertyStr(js, frame, "surface", SDL_Surface2js(js,surf));
  JS_SetPropertyStr(js, frame, "rect", rect2js(js,(rect){.x=0,.y=0,.w=1,.h=1}));
  JS_SetPropertyStr(js, frame, "time", number2js(js,(float)aframe.duration_milliseconds/1000.0));
  return frame;
}

JSC_CCALL(os_make_aseprite,
  size_t rawlen;
  void *raw = JS_GetArrayBuffer(js,&rawlen,argv[0]);
  
  ase_t *ase = cute_aseprite_load_from_memory(raw, rawlen, NULL);

  if (!ase)
    return JS_ThrowReferenceError(js, "could not load aseprite from supplied array buffer: %s", aseprite_GetError());

  int w = ase->w;
  int h = ase->h;

  int pixels = w*h;

  if (ase->tag_count == 0) {
    // we're dealing with a single frame image, or single animation
    if (ase->frame_count == 1) {
      JSValue obj = aseframe2js(js,ase->frames[0]);
      cute_aseprite_free(ase);
      return obj;
    }
  }

  JSValue obj = JS_NewObject(js);

  for (int t = 0; t < ase->tag_count; t++) {
    ase_tag_t tag = ase->tags[t];
    JSValue anim = JS_NewObject(js);
    JS_SetPropertyStr(js, anim, "repeat", number2js(js,tag.repeat));
    switch(tag.loop_animation_direction) {
      case ASE_ANIMATION_DIRECTION_FORWARDS:
        JS_SetPropertyStr(js, anim, "loop", JS_NewString(js,"forward"));
        break;
      case ASE_ANIMATION_DIRECTION_BACKWORDS:
        JS_SetPropertyStr(js, anim, "loop", JS_NewString(js,"backward"));
        break;
      case ASE_ANIMATION_DIRECTION_PINGPONG:
        JS_SetPropertyStr(js, anim, "loop", JS_NewString(js,"pingpong"));
        break;
    }

    int _frame = 0;
    JSValue frames = JS_NewArray(js);
    for (int f = tag.from_frame; f <= tag.to_frame; f++) {
      JSValue frame = aseframe2js(js,ase->frames[f]);
      JS_SetPropertyUint32(js, frames, _frame, frame);
      _frame++;
    }
    JS_SetPropertyStr(js, anim, "frames", frames);
    JS_SetPropertyStr(js, obj, tag.name, anim);
  }

  ret = obj;

  cute_aseprite_free(ase);
)

JSC_CCALL(os_make_surface,
  HMM_Vec2 wh = js2vec2(js,argv[0]);
  SDL_Surface *surface = SDL_CreateSurface(wh.x, wh.y, SDL_PIXELFORMAT_RGBA32);
  ret = SDL_Surface2js(js, surface);
)

JSC_CCALL(os_make_cursor,
  SDL_Surface *s = js2SDL_Surface(js,argv[0]);
  HMM_Vec2 hot = js2vec2(js,argv[1]);
  SDL_Cursor *c = SDL_CreateColorCursor(s, hot.x, hot.y);
  if (!c) return JS_ThrowReferenceError("couldn't make cursor: %s", SDL_GetError());
  printf("made cursor %p\n", c);
  return SDL_Cursor2js(js,c);
)

JSC_CCALL(os_make_font,
  size_t len;
  void *data = JS_GetArrayBuffer(js,&len,argv[0]);
  if (!data) return JS_ThrowReferenceError(js, "could not get array buffer data");
  font *f = MakeFont(data, len, js2number(js,argv[1]));
  if (!f) return JS_ThrowReferenceError(js, "could not create font");
  ret = font2js(js,f);
  JS_SetPropertyStr(js, ret, "surface", SDL_Surface2js(js,f->surface));
)

JSC_CCALL(os_make_transform, return transform2js(js,make_transform()))

JSC_SCALL(os_system, return number2js(js,system(str)); )

JSC_SCALL(os_model_buffer,
  int mesh_idx = 0;
  const struct aiScene *scene = aiImportFile(
    str,
    aiProcess_Triangulate |
    aiProcess_GenNormals |
    aiProcess_GenUVCoords |
    aiProcess_FlipUVs     |
    aiProcess_OptimizeMeshes |
    aiProcess_JoinIdenticalVertices
  );

  if (!scene) {
    JS_ThrowInternalError(js, "Failed to load model: %s", aiGetErrorString());
    return JS_UNDEFINED;
  }

  if (mesh_idx < 0 || mesh_idx >= (int)scene->mNumMeshes) {
    JS_ThrowInternalError(js, "Invalid mesh index");
    aiReleaseImport(scene);
    return JS_UNDEFINED;
  }

  struct aiMesh *mesh = scene->mMeshes[mesh_idx];

  if (!mesh->mVertices) {
    JS_ThrowInternalError(js, "Mesh has no vertices");
    aiReleaseImport(scene);
    return JS_UNDEFINED;
  }

  size_t num_verts = mesh->mNumVertices;
  float *posdata = NULL;
  float *normdata = NULL;
  float *uvdata = NULL;
  float *colordata = NULL; // New: To provide a default color
  Uint16 *indicesdata = NULL;

  // Positions (float3)
  posdata = malloc(sizeof(float)*3*num_verts);
  for (size_t i = 0; i < num_verts; i++) {
    posdata[i*3+0] = mesh->mVertices[i].x;
    posdata[i*3+1] = mesh->mVertices[i].y;
    posdata[i*3+2] = mesh->mVertices[i].z;
  }

  // Normals (float3) - guaranteed by aiProcess_GenNormals, but check anyway
  if (mesh->mNormals) {
    normdata = malloc(sizeof(float)*3*num_verts);
    for (size_t i = 0; i < num_verts; i++) {
      normdata[i*3+0] = mesh->mNormals[i].x;
      normdata[i*3+1] = mesh->mNormals[i].y;
      normdata[i*3+2] = mesh->mNormals[i].z;
    }
  } else {
    // Provide a default normal if none are available
    normdata = malloc(sizeof(float)*3*num_verts);
    for (size_t i = 0; i < num_verts; i++) {
      normdata[i*3+0] = 0.0f;
      normdata[i*3+1] = 0.0f;
      normdata[i*3+2] = 1.0f;
    }
  }

  // UVs (float2)
  if (mesh->mTextureCoords[0]) {
    uvdata = malloc(sizeof(float)*2*num_verts);
    for (size_t i = 0; i < num_verts; i++) {
      uvdata[i*2+0] = mesh->mTextureCoords[0][i].x;
      uvdata[i*2+1] = mesh->mTextureCoords[0][i].y;
    }
  } else {
    // Default UVs if not present
    uvdata = malloc(sizeof(float)*2*num_verts);
    for (size_t i = 0; i < num_verts; i++) {
      uvdata[i*2+0] = 0.0f;
      uvdata[i*2+1] = 0.0f;
    }
  }

  // Colors (float4) - Assimp may provide vertex colors, but if not, default to white
  // Check if there's a color channel
  if (mesh->mColors[0]) {
    colordata = malloc(sizeof(float)*4*num_verts);
    for (size_t i = 0; i < num_verts; i++) {
      colordata[i*4+0] = mesh->mColors[0][i].r;
      colordata[i*4+1] = mesh->mColors[0][i].g;
      colordata[i*4+2] = mesh->mColors[0][i].b;
      colordata[i*4+3] = mesh->mColors[0][i].a;
    }
  } else {
    colordata = malloc(sizeof(float)*4*num_verts);
    for (size_t i = 0; i < num_verts; i++) {
      colordata[i*4+0] = 1.0f;
      colordata[i*4+1] = 1.0f;
      colordata[i*4+2] = 1.0f;
      colordata[i*4+3] = 1.0f;
    }
  }

  // Indices
  size_t num_faces = mesh->mNumFaces;
  size_t index_count = num_faces * 3; 
  indicesdata = malloc(sizeof(Uint16)*index_count);
  for (size_t i = 0; i < num_faces; i++) {
    const struct aiFace *face = &mesh->mFaces[i];
    indicesdata[i*3+0] = face->mIndices[0];
    indicesdata[i*3+1] = face->mIndices[1];
    indicesdata[i*3+2] = face->mIndices[2];
  }

  // Create a JS object to hold the buffers
  ret = JS_NewObject(js);

  // Positions: float3
  JS_SetProperty(js, ret, pos_atom,
    make_gpu_buffer(js, posdata, sizeof(float)*3*num_verts, JS_TYPED_ARRAY_FLOAT32, 3, 1));

  // UV: float2
  JS_SetProperty(js, ret, uv_atom,
    make_gpu_buffer(js, uvdata, sizeof(float)*2*num_verts, JS_TYPED_ARRAY_FLOAT32, 2, 1));

  // Color: float4
  JS_SetProperty(js, ret, color_atom,
    make_gpu_buffer(js, colordata, sizeof(float)*4*num_verts, JS_TYPED_ARRAY_FLOAT32, 4, 1));

  // Normal: float3
  JS_SetProperty(js, ret, norm_atom,
    make_gpu_buffer(js, normdata, sizeof(float)*3*num_verts, JS_TYPED_ARRAY_FLOAT32, 3, 1));

  // Indices: uint32
  JS_SetProperty(js, ret, indices_atom,
    make_gpu_buffer(js, indicesdata, sizeof(unsigned int)*index_count, JS_TYPED_ARRAY_UINT16, 0, 1));

  // Metadata
  JS_SetProperty(js, ret, vertices_atom, number2js(js, (double)num_verts));
  JS_SetProperty(js, ret, count_atom, number2js(js, (double)index_count));

  // Cleanup
  free(posdata);
  free(normdata);
  free(uvdata);
  free(colordata);
  free(indicesdata);

  aiReleaseImport(scene);
)

JSC_SCALL(os_gltf_buffer,
  int buffer_idx = js2number(js,argv[1]);
  int type = js2number(js,argv[2]);
  cgltf_options options = {0};
  cgltf_data *data = NULL;
  cgltf_result result = cgltf_parse_file(&options, str, &data);
  result = cgltf_load_buffers(&options, data, str);

  SDL_GPUBuffer *b = SDL_CreateGPUBuffer(NULL,NULL);
//  *b = accessor2buffer(&data->accessors[buffer_idx], type);
  cgltf_free(data);

//  ret = sg_buffer2js(js,b);
)

JSC_SCALL(os_gltf_skin,
  cgltf_options options = {0};
  cgltf_data *data = NULL;
  cgltf_parse_file(&options,str,&data);
  cgltf_load_buffers(&options,data,str);

  if (data->skins_count <= 0) {
    ret = (JS_UNDEFINED);
    goto CLEANUP;
  }

  ret = skin2js(js,make_gltf_skin(data->skins+0, data));

  CLEANUP:
  cgltf_free(data);
)

JSC_CCALL(os_make_color_buffer,
  int count = js2number(js,argv[1]);
  HMM_Vec4 color = js2vec4(js,argv[0]);
  HMM_Vec4 *buffer = malloc(sizeof(HMM_Vec4) * count);
  for (int i =  0; i < count; i++)
    memcpy(buffer+i, &color, sizeof(HMM_Vec4));
  ret = make_gpu_buffer(js,buffer,sizeof(HMM_Vec4)*count,JS_TYPED_ARRAY_FLOAT32,4,0);
)

JSC_CCALL(os_make_line_prim,
  JSValue prim = JS_NewObject(js);
  HMM_Vec2 *v = js2cpvec2arr(js,argv[0]);
  
  parsl_context *par_ctx = parsl_create_context((parsl_config){
    .thickness = js2number(js,argv[1]),
    .flags= PARSL_FLAG_ANNOTATIONS,
    .u_mode = js2number(js,argv[2])
  });
  
  uint16_t spine_lens[] = {arrlen(v)};
  
  parsl_mesh *m = parsl_mesh_from_lines(par_ctx, (parsl_spine_list){
    .num_vertices = arrlen(v),
    .num_spines = 1,
    .vertices = v,
    .spine_lengths = spine_lens,
    .closed = JS_ToBool(js,argv[3])
  });

  JS_SetPropertyStr(js, prim, "pos", make_gpu_buffer(js,m->positions,sizeof(*m->positions)*m->num_vertices, JS_TYPED_ARRAY_FLOAT32, 2,1));
  
  JS_SetPropertyStr(js, prim, "indices", make_gpu_buffer(js,m->triangle_indices,sizeof(*m->triangle_indices)*m->num_triangles*3, JS_TYPED_ARRAY_UINT32, 1,1));
  
  float uv[m->num_vertices*2];
  for (int i = 0; i < m->num_vertices; i++) {
    uv[i*2] = m->annotations[i].u_along_curve;
    uv[i*2+1] = m->annotations[i].v_across_curve;
  }

  JS_SetPropertyStr(js, prim, "uv", make_gpu_buffer(js, uv, sizeof(uv), JS_TYPED_ARRAY_FLOAT32,2,1));
  JS_SetPropertyStr(js,prim,"vertices", number2js(js,m->num_vertices));
  JS_SetPropertyStr(js,prim,"count", number2js(js,m->num_triangles*3));

  parsl_destroy_context(par_ctx);
  
  return prim;
)

JSValue parmesh2js(JSContext *js,par_shapes_mesh *m)
{
  return JS_UNDEFINED;
/*  JSValue obj = JS_NewObject(js);
  sg_buffer *pos = malloc(sizeof(*pos));
  *pos = float_buffer(m->points, 3*m->npoints);
  JS_SetPropertyStr(js, obj, "pos", sg_buffer2js(js,pos));
  
  if (m->tcoords) {
    sg_buffer *uv = malloc(sizeof(*uv));
    *uv = texcoord_floats(m->tcoords, 2*m->npoints);
    JS_SetPropertyStr(js, obj, "uv", sg_buffer2js(js,uv));
  }
  
  if (m->normals) {
    sg_buffer *norm = malloc(sizeof(*norm));
    *norm = normal_floats(m->normals, 3*m->npoints);
    JS_SetPropertyStr(js, obj, "norm", sg_buffer2js(js,norm));
  }
  
  sg_buffer *index = malloc(sizeof(*index));
  *index = sg_make_buffer(&(sg_buffer_desc){
    .data = {
      .ptr = m->triangles,
      .size = sizeof(*m->triangles)*3*m->ntriangles
    },
    .type = SG_BUFFERTYPE_INDEXBUFFER
  });
  JS_SetPropertyStr(js, obj, "index", sg_buffer2js(js,index));
  
  JS_SetPropertyStr(js, obj, "count", number2js(js,3*m->ntriangles));
  
  par_shapes_free_mesh(m);
  return obj;
*/
}

JSC_CCALL(os_make_cylinder,
  return parmesh2js(js,par_shapes_create_cylinder(js2number(js,argv[0]), js2number(js,argv[1])));
)

JSC_CCALL(os_make_cone,
  return parmesh2js(js,par_shapes_create_cone(js2number(js,argv[0]), js2number(js,argv[1])));
)

JSC_CCALL(os_make_disk,
  return parmesh2js(js,par_shapes_create_parametric_disk(js2number(js,argv[0]), js2number(js,argv[1])));
)

JSC_CCALL(os_make_torus,
  return parmesh2js(js,par_shapes_create_torus(js2number(js,argv[0]), js2number(js,argv[1]), js2number(js,argv[2])));
)

JSC_CCALL(os_make_sphere,
  return parmesh2js(js,par_shapes_create_parametric_sphere(js2number(js,argv[0]), js2number(js,argv[1])));
)

JSC_CCALL(os_make_klein_bottle,
  return parmesh2js(js,par_shapes_create_klein_bottle(js2number(js,argv[0]), js2number(js,argv[1])));
)

JSC_CCALL(os_make_trefoil_knot,
  return parmesh2js(js,par_shapes_create_trefoil_knot(js2number(js,argv[0]), js2number(js,argv[1]), js2number(js,argv[2])));
)

JSC_CCALL(os_make_hemisphere,
  return parmesh2js(js,par_shapes_create_hemisphere(js2number(js,argv[0]), js2number(js,argv[1])));
)

JSC_CCALL(os_make_plane,
  return parmesh2js(js,par_shapes_create_plane(js2number(js,argv[0]), js2number(js,argv[1])));
)

static void render_frame(plm_t *mpeg, plm_frame_t *frame, datastream *ds) {
  if (JS_IsUndefined(ds->callback)) return;
  uint8_t *rgb = malloc(frame->height*frame->width*4);
  memset(rgb,255,frame->height*frame->width*4);
  plm_frame_to_rgba(frame, rgb, frame->width*4);
  SDL_Surface *surf = SDL_CreateSurfaceFrom(frame->width,frame->height, SDL_PIXELFORMAT_RGBA32, rgb, frame->width*4);
  JSValue s[1];
  s[0] = SDL_Surface2js(ds->js,surf);
  JSValue cb = JS_DupValue(ds->js,ds->callback);
  JSValue ret = JS_Call(ds->js, cb, JS_UNDEFINED, 1, s);
  JS_FreeValue(ds->js,ret);
  JS_FreeValue(ds->js,cb);
  free(rgb);
}

JSC_CCALL(os_make_video,
  size_t len;
  void *data = JS_GetArrayBuffer(js,&len,argv[0]);
  datastream *ds = ds_openvideo(data, len);
  if (!ds) return JS_ThrowReferenceError(js, "Video file was not valid.");
  ds->js = js;
  ds->callback = JS_UNDEFINED;
  plm_set_video_decode_callback(ds->plm, render_frame, ds);
  return datastream2js(js,ds);  
)

JSC_CCALL(os_skin_calculate,
  skin *sk = js2skin(js,argv[0]);
  skin_calculate(sk);
)

JSC_CCALL(os_rectpack,
  int width = js2number(js,argv[0]);
  int height = js2number(js,argv[1]);
  int num = js_arrlen(js,argv[2]);
  stbrp_context ctx[1];
  stbrp_rect rects[num];
  
  for (int i = 0; i < num; i++) {
    HMM_Vec2 wh = js2vec2(js,js_getpropertyuint32(js, argv[2], i));
    rects[i].w = wh.x;
    rects[i].h = wh.y;
    rects[i].id = i;
  }
  
  stbrp_node nodes[width];
  stbrp_init_target(ctx, width, height, nodes, width);
  int packed = stbrp_pack_rects(ctx, rects, num);
  
  if (!packed) {
    return JS_UNDEFINED;
  }
  
  ret = JS_NewArray(js);
  for (int i = 0; i < num; i++) {
    HMM_Vec2 pos;
    pos.x = rects[i].x;
    pos.y = rects[i].y;
    JS_SetPropertyUint32(js, ret, i, vec22js(js,pos));
  }
)

JSC_CCALL(os_perlin,
  HMM_Vec3 coord = js2vec3(js,argv[0]);
  HMM_Vec3 wrap = js2vec3(js,argv[2]);
  return number2js(js,stb_perlin_noise3_seed(coord.x, coord.y, coord.z, wrap.x, wrap.y, wrap.z, js2number(js,argv[1])));
)

JSC_CCALL(os_ridge,
  HMM_Vec3 c = js2vec3(js,argv[0]);
  return number2js(js,stb_perlin_ridge_noise3(c.x, c.y, c.z, js2number(js,argv[1]), js2number(js,argv[2]), js2number(js,argv[3]), js2number(js,argv[4])));
)

JSC_CCALL(os_fbm,
  HMM_Vec3 c = js2vec3(js,argv[0]);
  return number2js(js,stb_perlin_fbm_noise3(c.x, c.y, c.z, js2number(js,argv[1]), js2number(js,argv[2]), js2number(js,argv[3])));
)

JSC_CCALL(os_turbulence,
  HMM_Vec3 c = js2vec3(js,argv[0]);
  return number2js(js,stb_perlin_turbulence_noise3(c.x, c.y, c.z, js2number(js,argv[1]), js2number(js,argv[2]), js2number(js,argv[3])));  
)

JSC_SCALL(os_kill,
  int sig = 0;
  if (!strcmp(str, "SIGABRT")) sig = SIGABRT;
  else if (!strcmp(str, "SIGFPE")) sig = SIGFPE;
  else if (!strcmp(str, "SIGILL")) sig = SIGILL;
  else if (!strcmp(str, "SIGINT")) sig = SIGINT;
  else if (!strcmp(str, "SIGSEGV")) sig = SIGSEGV;
  else if (!strcmp(str, "SIGTERM")) sig = SIGTERM;

  if (!sig) return JS_ThrowReferenceError(js, "string %s is not a valid signal", str);
  raise(sig);
  return JS_UNDEFINED;
)
int detectImageInWebcam(SDL_Surface *a, SDL_Surface *b);
/*JSC_CCALL(os_match_img,
  SDL_Surface *img1 = js2SDL_Surface(js,argv[0]);
  SDL_Surface *img2 = js2SDL_Surface(js,argv[1]);
  int n = detectImageInWebcam(img1,img2);
  return number2js(js,n);
)
*/
JSC_CCALL(os_sleep,
  double time = js2number(js,argv[0]);
  time *= 1000000000.;
  SDL_DelayNS(time);
)

static const JSCFunctionListEntry js_os_funcs[] = {
  MIST_FUNC_DEF(os, turbulence, 4),
  MIST_FUNC_DEF(os, model_buffer, 1),
  MIST_FUNC_DEF(os, fbm, 4),
  MIST_FUNC_DEF(os, make_color_buffer, 2),
  MIST_FUNC_DEF(os, ridge, 5),
  MIST_FUNC_DEF(os, perlin, 3),
  MIST_FUNC_DEF(os, rectpack, 3),
  MIST_FUNC_DEF(os, cwd, 0),
  MIST_FUNC_DEF(os, rusage, 0),
  MIST_FUNC_DEF(os, mallinfo, 0),
  MIST_FUNC_DEF(os, env, 1),
  MIST_FUNC_DEF(os, sys, 0),
  MIST_FUNC_DEF(os, system, 1),
  MIST_FUNC_DEF(os, exit, 1),
  MIST_FUNC_DEF(os, gc, 0),
  MIST_FUNC_DEF(os, eval, 2),
  MIST_FUNC_DEF(os, make_texture, 1),
  MIST_FUNC_DEF(os, make_gif, 1),
  MIST_FUNC_DEF(os, make_aseprite, 1),
  MIST_FUNC_DEF(os, make_surface, 1),
  MIST_FUNC_DEF(os, make_cursor, 1),
  MIST_FUNC_DEF(os, make_font, 2),
  MIST_FUNC_DEF(os, make_transform, 0),
  MIST_FUNC_DEF(os, make_line_prim, 4),
  MIST_FUNC_DEF(os, make_cylinder, 2),
  MIST_FUNC_DEF(os, make_cone, 2),
  MIST_FUNC_DEF(os, make_disk, 2),
  MIST_FUNC_DEF(os, make_torus, 3),
  MIST_FUNC_DEF(os, make_sphere, 2),
  MIST_FUNC_DEF(os, make_klein_bottle, 2),
  MIST_FUNC_DEF(os, make_trefoil_knot, 3),
  MIST_FUNC_DEF(os, make_hemisphere, 2),
  MIST_FUNC_DEF(os, make_plane, 2),
  MIST_FUNC_DEF(os, make_video, 1),
  MIST_FUNC_DEF(os, make_timer, 1),
  MIST_FUNC_DEF(os, make_text_buffer, 6),
  MIST_FUNC_DEF(os, update_timers, 1),
  MIST_FUNC_DEF(os, mem, 1),
  MIST_FUNC_DEF(os, mem_limit, 1),
  MIST_FUNC_DEF(os, gc_threshold, 1),
  MIST_FUNC_DEF(os, max_stacksize, 1),
  MIST_FUNC_DEF(os, rt_info, 0),
  MIST_FUNC_DEF(os, dump_mem, 0),
  MIST_FUNC_DEF(os, dump_shapes, 0),
  MIST_FUNC_DEF(os, dump_atoms,0),
  MIST_FUNC_DEF(os, calc_mem, 1),
  MIST_FUNC_DEF(os, check_gc, 0),
  MIST_FUNC_DEF(os, check_cycles, 0),
  MIST_FUNC_DEF(os, memstate, 0),
  MIST_FUNC_DEF(os, value_id, 1),
  MIST_FUNC_DEF(os, gltf_buffer, 3),
  MIST_FUNC_DEF(os, gltf_skin, 1),
  MIST_FUNC_DEF(os, skin_calculate, 1),
  MIST_FUNC_DEF(os, kill, 1),
//  MIST_FUNC_DEF(os, match_img, 2),
  MIST_FUNC_DEF(os, sleep, 1),
};

#define JSSTATIC(NAME, PARENT) \
js_##NAME = JS_NewObject(js); \
JS_SetPropertyFunctionList(js, js_##NAME, js_##NAME##_funcs, countof(js_##NAME##_funcs)); \
JS_SetPrototype(js, js_##NAME, PARENT); \

JSValue js_layout_use(JSContext *js);
JSValue js_miniz_use(JSContext *js);
JSValue js_soloud_use(JSContext *js);
JSValue js_chipmunk2d_use(JSContext *js);

#ifdef TRACY_ENABLE
JSValue js_tracy_use(JSContext *js);
#endif

#ifndef NEDITOR
JSValue js_imgui(JSContext *js);
JSValue js_dmon_use(JSContext *js);
#endif

static void signal_handler(int sig) {
  const char *str = NULL;
  switch(sig) {
    case SIGABRT:
      str = "SIGABRT";
      break;
    case SIGFPE:
      str = "SIGFPE";
      break;
    case SIGILL:
      str = "SIGILL";
      break;
    case SIGINT:
      str = "SIGINT";
      break;
    case SIGSEGV:
      str = "SIGSEGV";
      break;
    case SIGTERM:
      str = "SIGTERM";
      break;
   }
  if (!str) return;
  script_evalf("prosperon.%s?.();", str);
}

static void exit_handler()
{
  script_evalf("prosperon.exit?.();");
  script_stop();
}

void ffi_load(JSContext *js) {
  cycles = tmpfile();
  quickjs_set_cycleout(cycles);
  
  JSValue globalThis = JS_GetGlobalObject(js);

  QJSCLASSPREP_FUNCS(SDL_Window)
  QJSCLASSPREP_FUNCS(SDL_Surface)
  QJSCLASSPREP_FUNCS(SDL_Thread)
  QJSCLASSPREP_FUNCS(SDL_Texture)
  QJSCLASSPREP_FUNCS(SDL_Renderer)
  QJSCLASSPREP_FUNCS(SDL_Camera)
  QJSCLASSPREP_FUNCS(SDL_Cursor)
  QJSCLASSPREP_FUNCS(SDL_GPUDevice)
  QJSCLASSPREP_FUNCS(SDL_GPUTexture)
  QJSCLASSPREP_FUNCS(SDL_GPUCommandBuffer)
  QJSCLASSPREP_FUNCS(SDL_GPURenderPass)
//  QJSCLASSPREP_FUNCS(SDL_GPUGraphicsPipeline)
//  QJSCLASSPREP_FUNCS(SDL_GPUSampler)
//  QJSCLASSPREP_FUNCS(SDL_GPUShader)
//  QJSCLASSPREP_FUNCS(SDL_GPUBuffer)
//  QJSCLASSPREP_FUNCS(SDL_GPUTransferBuffer)
  

  QJSGLOBALCLASS(os);
  
  QJSCLASSPREP_FUNCS(transform);
//  QJSCLASSPREP_FUNCS(warp_gravity);
//  QJSCLASSPREP_FUNCS(warp_damp);
  QJSCLASSPREP_FUNCS(font);
  QJSCLASSPREP_FUNCS(datastream);
  QJSCLASSPREP_FUNCS(timer);

  QJSGLOBALCLASS(input);
  QJSGLOBALCLASS(io);
  QJSGLOBALCLASS(prosperon);
  QJSGLOBALCLASS(time);
  QJSGLOBALCLASS(console);
  QJSGLOBALCLASS(profile);
  QJSGLOBALCLASS(debug);
  QJSGLOBALCLASS(game);
  QJSGLOBALCLASS(render);
  QJSGLOBALCLASS(vector);
  QJSGLOBALCLASS(spline);
  QJSGLOBALCLASS(performance);
  QJSGLOBALCLASS(geometry);

  JS_SetPropertyStr(js, prosperon, "version", JS_NewString(js,"ver"));
  JS_SetPropertyStr(js, prosperon, "revision", JS_NewString(js,"com"));
  JS_SetPropertyStr(js, prosperon, "date", JS_NewString(js,"date"));

  JSValue array_proto = js_getpropertystr(js,globalThis, "Array");
  array_proto = js_getpropertystr(js,array_proto, "prototype");
  JS_SetPropertyFunctionList(js, array_proto, js_array_funcs, countof(js_array_funcs));

  JS_SetPropertyStr(js, globalThis, "layout", js_layout_use(js));
  JS_SetPropertyStr(js, globalThis, "miniz", js_miniz_use(js));
  JS_SetPropertyStr(js, globalThis, "soloud", js_soloud_use(js));
  JS_SetPropertyStr(js, globalThis, "chipmunk2d", js_chipmunk2d_use(js));

#ifdef TRACY_ENABLE
  JS_SetPropertyStr(js, globalThis, "tracy", js_tracy_use(js));
#endif

  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);
  signal(SIGSEGV, signal_handler);
  signal(SIGABRT, signal_handler);
  atexit(exit_handler);

#ifndef NEDITOR
  JS_SetPropertyStr(js, globalThis, "dmon", js_dmon_use(js));
  JS_SetPropertyStr(js, globalThis, "imgui", js_imgui(js));
#endif

  x_atom = JS_NewAtom(js,"x");
  y_atom = JS_NewAtom(js,"y");
  width_atom = JS_NewAtom(js,"width");
  height_atom = JS_NewAtom(js,"height");
  anchor_x_atom = JS_NewAtom(js, "anchor_x");
  anchor_y_atom = JS_NewAtom(js,"anchor_y");
  src_atom = JS_NewAtom(js,"src");
  pos_atom = JS_NewAtom(js, "pos");
  uv_atom = JS_NewAtom(js, "uv");
  color_atom = JS_NewAtom(js, "color");
  indices_atom = JS_NewAtom(js, "indices");
  vertices_atom = JS_NewAtom(js, "vertices");
  dst_atom = JS_NewAtom(js, "dst");
  count_atom = JS_NewAtom(js, "count");
  transform_atom = JS_NewAtom(js,"transform");

  cw_atom = JS_NewAtom(js,"cw");
  ccw_atom = JS_NewAtom(js,"ccw");
  zero_atom = JS_NewAtom(js, "zero");
  one_atom = JS_NewAtom(js, "one");
  add_atom = JS_NewAtom(js, "add");
  sub_atom = JS_NewAtom(js, "sub");
  rev_sub_atom = JS_NewAtom(js, "rev_sub");
  min_atom = JS_NewAtom(js, "min");
  max_atom = JS_NewAtom(js, "max");
  none_atom = JS_NewAtom(js, "none");
  front_atom = JS_NewAtom(js, "front");
  back_atom = JS_NewAtom(js, "back");
  never_atom = JS_NewAtom(js, "never");
  less_atom = JS_NewAtom(js, "less");
  equal_atom = JS_NewAtom(js, "equal");
  less_equal_atom = JS_NewAtom(js, "less_equal");
  greater_atom = JS_NewAtom(js, "greater");
  not_equal_atom = JS_NewAtom(js, "not_equal");
  greater_equal_atom = JS_NewAtom(js, "greater_equal");
  always_atom = JS_NewAtom(js, "always");
  keep_atom = JS_NewAtom(js, "keep");
  zero_stencil_atom = JS_NewAtom(js, "zero");
  replace_atom = JS_NewAtom(js, "replace");
  incr_clamp_atom = JS_NewAtom(js, "incr_clamp");
  decr_clamp_atom = JS_NewAtom(js, "decr_clamp");
  invert_atom = JS_NewAtom(js, "invert");
  incr_wrap_atom = JS_NewAtom(js, "incr_wrap");
  decr_wrap_atom = JS_NewAtom(js, "decr_wrap");
  point_atom = JS_NewAtom(js, "point");
  line_atom = JS_NewAtom(js, "line");
  linestrip_atom = JS_NewAtom(js, "linestrip");
  triangle_atom = JS_NewAtom(js, "triangle");
  trianglestrip_atom = JS_NewAtom(js, "trianglestrip");
  src_color_atom = JS_NewAtom(js, "src_color");
  one_minus_src_color_atom = JS_NewAtom(js, "one_minus_src_color");
  dst_color_atom = JS_NewAtom(js, "dst_color");
  one_minus_dst_color_atom = JS_NewAtom(js, "one_minus_dst_color");
  src_alpha_atom = JS_NewAtom(js, "src_alpha");
  one_minus_src_alpha_atom = JS_NewAtom(js, "one_minus_src_alpha");
  dst_alpha_atom = JS_NewAtom(js, "dst_alpha");
  one_minus_dst_alpha_atom = JS_NewAtom(js, "one_minus_dst_alpha");
  constant_color_atom = JS_NewAtom(js, "constant_color");
  one_minus_constant_color_atom = JS_NewAtom(js, "one_minus_constant_color");
  src_alpha_saturate_atom = JS_NewAtom(js, "src_alpha_saturate");
  none_cull_atom = JS_NewAtom(js, "none");
  front_cull_atom = JS_NewAtom(js, "front");
  back_cull_atom = JS_NewAtom(js, "back");
  norm_atom = JS_NewAtom(js,"norm");

  nearest_atom = JS_NewAtom(js, "nearest");
  linear_atom = JS_NewAtom(js, "linear");

  mipmap_nearest_atom = JS_NewAtom(js, "nearest");  // For mipmap mode "nearest"
  mipmap_linear_atom = JS_NewAtom(js, "linear");    // For mipmap mode "linear"

  repeat_atom = JS_NewAtom(js, "repeat");
  mirror_atom = JS_NewAtom(js, "mirror");
  clamp_edge_atom = JS_NewAtom(js, "clamp_edge");
  clamp_border_atom = JS_NewAtom(js, "clamp_border");
  vertex_atom = JS_NewAtom(js, "vertex");
  index_atom = JS_NewAtom(js, "index");
  indirect_atom = JS_NewAtom(js, "indirect");

  fill_event_atoms(js);

  JS_FreeValue(js,globalThis);  
}
