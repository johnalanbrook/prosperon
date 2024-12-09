#include "jsffi.h"

#include "script.h" 
#include "font.h"
#include "datastream.h"
#include "stb_ds.h"
#include "stb_image.h"
#include "stb_rect_pack.h"
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

#include "physfs.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_error.h>

#include <cv.hpp>

#ifdef __APPLE__
#include <Accelerate/Accelerate.h>
#else
#include <cblas.h>
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
static HMM_Vec2 campos = (HMM_Vec2){0,0};
static HMM_Vec2 logical = {0};

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

void *get_gpu_buffer(JSContext *js, JSValue argv, size_t *stride)
{
  size_t o, len, bytes, size;
  JSValue buf = JS_GetTypedArrayBuffer(js, argv, &o, &len, &bytes);
  void *data = JS_GetArrayBuffer(js, &size, buf);
  JS_FreeValue(js,buf); 
  *stride = js_getnum_str(js, argv, "stride");
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

void SDL_Texture_free(JSRuntime *rt, SDL_Texture *t){
  TracyCFreeN(t, "vram");
  SDL_DestroyTexture(t);
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

QJSCLASS(SDL_Cursor)

int js_arrlen(JSContext *js,JSValue v) {
  if (JS_IsUndefined(v)) return 0;
  int len;
  len = js_getnum_str(js,v,"length");
  return len;
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

rect transform_rect(rect in, HMM_Mat3 *t)
{
  in.y *= -1;
  in.y += logical.y;
  in.y -= in.h;
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
  JSValue ret = number2js(js, cblas_sdot(alen, a, 1, b,1));
  free(a);
  free(b);
  return ret;
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
  cblas_saxpy(2, -1.0f, pivot.e, 1, vec.e, 1);

  // Length of the vector (r)
  float r = sqrtf(cblas_sdot(2, vec.e, 1, vec.e, 1));
  
  // Update angle
  angle += atan2f(vec.y, vec.x);

  // Update vec to rotated position
  vec.x = r * cosf(angle);
  vec.y = r * sinf(angle);

  // vec = vec + pivot
  cblas_saxpy(2, 1.0f, pivot.e, 1, vec.e, 1);

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
  sum = cblas_sasum(len, a,1);
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
  cblas_saxpy(len,1.0f,vec_b,1,vec_a,1);
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

  return SDL_Window2js(js,new);
)

static JSValue event2js(JSContext *js, SDL_Event event)
{
  JSValue e = JS_NewObject(js);
  JS_SetPropertyStr(js,e,"timestamp", number2js(js,event.common.timestamp));
  
  switch(event.type) {
    case SDL_EVENT_QUIT:
      JS_SetPropertyStr(js,e,"type", JS_NewString(js,"quit"));
      break;
    case SDL_EVENT_MOUSE_MOTION:
      JS_SetPropertyStr(js, e, "type", JS_NewString(js, "mouse"));
      JS_SetPropertyStr(js,e,"window", number2js(js,event.motion.windowID));
      JS_SetPropertyStr(js,e,"which", number2js(js,event.motion.which));
      JS_SetPropertyStr(js, e, "state", number2js(js,event.motion.state));
      JS_SetPropertyStr(js,e, "mouse", vec22js(js,(HMM_Vec2){event.motion.x,event.motion.y}));
      JS_SetPropertyStr(js,e,"mouse_d", vec22js(js,(HMM_Vec2){event.motion.xrel, event.motion.yrel}));
      break;
    case SDL_EVENT_MOUSE_WHEEL:
      JS_SetPropertyStr(js,e,"type",JS_NewString(js,"wheel"));
      JS_SetPropertyStr(js,e,"window", number2js(js,event.wheel.windowID));
      JS_SetPropertyStr(js,e,"which", number2js(js,event.wheel.which));
      JS_SetPropertyStr(js,e,"scroll", vec22js(js,(HMM_Vec2){event.wheel.x,event.wheel.y}));
      JS_SetPropertyStr(js,e, "mouse", vec22js(js,(HMM_Vec2){event.wheel.mouse_x,event.wheel.mouse_y}));
      break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
      JS_SetPropertyStr(js,e,"type",JS_NewString(js,"mouse_button"));
      JS_SetPropertyStr(js,e,"window", number2js(js,event.button.windowID));
      JS_SetPropertyStr(js,e,"which", number2js(js,event.button.which));
      JS_SetPropertyStr(js,e,"down", JS_NewBool(js,event.button.down));
      JS_SetPropertyStr(js,e,"clicks", number2js(js,event.button.clicks));
      JS_SetPropertyStr(js,e,"mouse", vec22js(js,(HMM_Vec2){event.button.x,event.button.y}));
      break;
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
      JS_SetPropertyStr(js,e,"type", JS_NewString(js,"key"));
      JS_SetPropertyStr(js,e,"window", number2js(js,event.key.windowID));
      JS_SetPropertyStr(js,e,"which", number2js(js,event.key.which));
      JS_SetPropertyStr(js,e,"down", JS_NewBool(js,event.key.down));
      JS_SetPropertyStr(js,e,"repeat", JS_NewBool(js,event.key.repeat));
      JS_SetPropertyStr(js,e,"key", number2js(js,event.key.key));
      JS_SetPropertyStr(js,e,"scancode", number2js(js,event.key.scancode));
      JS_SetPropertyStr(js,e,"mod", number2js(js,event.key.mod));
      break;
    case SDL_EVENT_FINGER_MOTION:
    case SDL_EVENT_FINGER_DOWN:
    case SDL_EVENT_FINGER_UP:
      JS_SetPropertyStr(js,e,"type", JS_NewString(js,"touch"));
      JS_SetPropertyStr(js,e,"touch", number2js(js,event.tfinger.touchID));
      JS_SetPropertyStr(js,e,"finger", number2js(js,event.tfinger.fingerID));
      JS_SetPropertyStr(js,e,"pos", vec22js(js, (HMM_Vec2){event.tfinger.x, event.tfinger.y}));
      JS_SetPropertyStr(js,e,"pos_d", vec22js(js,(HMM_Vec2){event.tfinger.x, event.tfinger.dy}));
      JS_SetPropertyStr(js,e,"pressure", number2js(js,event.tfinger.pressure));
      JS_SetPropertyStr(js,e,"window", number2js(js,event.key.windowID));
      break;
    case SDL_EVENT_DROP_BEGIN:
    case SDL_EVENT_DROP_FILE:
    case SDL_EVENT_DROP_TEXT:
    case SDL_EVENT_DROP_COMPLETE:
    case SDL_EVENT_DROP_POSITION:
      JS_SetPropertyStr(js,e,"type", JS_NewString(js,"drop"));
      JS_SetPropertyStr(js,e,"window", number2js(js,event.drop.windowID));
      JS_SetPropertyStr(js,e,"pos", vec22js(js, (HMM_Vec2){event.drop.x,event.drop.y}));
      JS_SetPropertyStr(js,e,"data", JS_NewString(js,event.drop.data));
      JS_SetPropertyStr(js,e,"source",JS_NewString(js,event.drop.source));
      break;
    case SDL_EVENT_TEXT_INPUT:
      JS_SetPropertyStr(js,e,"type", JS_NewString(js,"text"));
      JS_SetPropertyStr(js,e,"window", number2js(js,event.text.windowID));
      JS_SetPropertyStr(js,e,"text", JS_NewString(js,event.text.text));
      break;
    case SDL_EVENT_CAMERA_DEVICE_APPROVED:
      JS_SetPropertyStr(js,e,"type", JS_NewString(js, "camera approved"));
      break;
  }
  return e;
}

// Polls and handles all input events
JSC_CCALL(game_engine_input,
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
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

JSC_CCALL(SDL_Renderer_present,
  SDL_RenderPresent(js2SDL_Renderer(js,self));
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
      rects[i] = transform_rect(js2rect(js,val), &cam_mat);
      JS_FreeValue(js,val);
    }
    SDL_RenderRects(r,rects,len);
    return JS_UNDEFINED;
  }
  
  rect rect = js2rect(js,argv[0]);
  rect = transform_rect(rect, &cam_mat);
  
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
  rect rect = transform_rect(js2rect(js,argv[0]),&cam_mat);
  
  if (!SDL_RenderFillRect(r, &rect))
    return JS_ThrowReferenceError("Could not render rectangle: %s", SDL_GetError());
)

JSC_CCALL(renderer_texture,
  SDL_Renderer *renderer = js2SDL_Renderer(js,self);
  SDL_Texture *tex = js2SDL_Texture(js,argv[0]);
  rect dst = transform_rect(js2rect(js,argv[1]), &cam_mat);
  
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
  src = transform_rect(js2rect(js,argv[3]),&cam_mat);
  dst = transform_rect(js2rect(js,argv[1]), &cam_mat);

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
  pos.y *= -1;
  pos.y += logical.y;
  SDL_RenderDebugText(r, pos.x, pos.y, str);
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

  HMM_Vec2 point = js2vec2(js,argv[0]);
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
  void *posdata = get_gpu_buffer(js,pos, &pos_stride);
  void *idxdata = get_gpu_buffer(js,indices, &indices_stride);
  void *uvdata = get_gpu_buffer(js,uv, &uv_stride);
  void *colordata = get_gpu_buffer(js,color,&color_stride);

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
  logical = v;  
  SDL_SetRenderLogicalPresentation(r,v.x,v.y,SDL_LOGICAL_PRESENTATION_LETTERBOX);
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

JSC_CCALL(renderer_campos,
  campos = js2vec2(js,argv[0]);
)

JSC_CCALL(renderer_vsync,
  SDL_Renderer *r = js2SDL_Renderer(js,self);
  SDL_SetRenderVSync(r,js2number(js,argv[0]));
)

JSC_CCALL(renderer_coords,
  SDL_Renderer *r = js2SDL_Renderer(js,self);
  HMM_Vec2 pos, coord;
  pos = js2vec2(js,argv[0]);
  SDL_RenderCoordinatesFromWindow(r,pos.x,pos.y, &coord.x, &coord.y);
  return vec22js(js,coord);
)

JSC_CCALL(renderer_camera,
  HMM_Mat3 t;
  t.Columns[0] = (HMM_Vec3){1,0,0};
  t.Columns[1] = (HMM_Vec3){0,-1,0};
  t.Columns[2] = (HMM_Vec3){0,logical.y,1};

  transform *tra = js2transform(js,argv[0]);
  tra->pos.x -= logical.x/2;
  tra->pos.y -= logical.y/2;
  HMM_Mat3 T = transform2mat3(tra);
  cam_mat = HMM_MulM3(t,T);
  tra->pos.x += logical.x/2;
  tra->pos.y += logical.y/2;
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
  MIST_FUNC_DEF(renderer, campos, 1),
  MIST_FUNC_DEF(renderer,logical_size,1),
  MIST_FUNC_DEF(renderer,viewport,1),
  MIST_FUNC_DEF(renderer,clip,1),
  MIST_FUNC_DEF(renderer,vsync,1),
  MIST_FUNC_DEF(renderer, coords, 1),
  MIST_FUNC_DEF(renderer, camera, 1),
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

static const JSCFunctionListEntry js_input_funcs[] = {
  MIST_FUNC_DEF(input, mouse_show, 1),
  MIST_FUNC_DEF(input, mouse_lock, 1),
  MIST_FUNC_DEF(input, cursor_set, 1),
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

static const JSCFunctionListEntry js_prosperon_funcs[] = {
  MIST_FUNC_DEF(prosperon, guid, 0),
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
  double seconds = SDL_GetTicksNS()/1000000000.f;
  if (seconds < 1e-6)
    snprintf(result, 50, "%.2f ns", seconds * 1e9);
  else if (seconds < 1e-3)
    snprintf(result, 50, "%.2f µs", seconds * 1e6);
  else if (seconds < 1)
    snprintf(result, 50, "%.2f ms", seconds * 1e3);
  else
    snprintf(result, 50, "%.2f s", seconds);

  ret = JS_NewString(js,result);
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
  cblas_saxpy(2, 1.0f, move.e, 1, &r, 1);
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
  SDL_Surface *surf = SDL_CreateSurfaceFrom(aframe.ase->w, aframe.ase->h, SDL_PIXELFORMAT_RGBA32, aframe.pixels, aframe.ase->w*4);
  JS_SetPropertyStr(js, frame, "surface", SDL_Surface2js(js,surf));
  JS_SetPropertyStr(js, frame, "rect", rect2js(js,(rect){.x=0,.y=0,.w=1,.h=1}));
  JS_SetPropertyStr(js, frame, "time", number2js(js,(float)aframe.duration_milliseconds/1000.0));
  return frame;
}

JSC_CCALL(os_make_aseprite,
  size_t rawlen;
  void *raw = JS_GetArrayBuffer(js,&rawlen,argv[0]);
  if (!raw) return JS_ThrowReferenceError(js, "could not load aseprite from supplied array buffer");
  
  ase_t *ase = cute_aseprite_load_from_memory(raw, rawlen, NULL);

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

JSC_CCALL(os_make_sprite_mesh,
  JSValue sprites = argv[0];
  JSValue old = argv[1];
  size_t quads = js_arrlen(js,argv[0]);
  size_t verts = quads*4;
  size_t count = quads*6;

  HMM_Vec2 *posdata = malloc(sizeof(*posdata)*verts);
  HMM_Vec2 *uvdata = malloc(sizeof(*uvdata)*verts);
  HMM_Vec4 *colordata = malloc(sizeof(*colordata)*verts);

  for (int i = 0; i < quads; i++) {
    JSValue sub = JS_GetPropertyUint32(js,sprites,i);
    JSValue jsdst = JS_GetProperty(js,sub,dst_atom);
    JSValue jssrc = JS_GetProperty(js,sub,src_atom);
    JSValue jscolor = JS_GetProperty(js,sub,color_atom);
    HMM_Vec4 color;
    rect dst = js2rect(js,jsdst);

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

    // Define the four corners of the destination rectangle
    posdata[base + 0] = (HMM_Vec2){ dst.x,             dst.y + dst.h };
    posdata[base + 1] = (HMM_Vec2){ dst.x + dst.w, dst.y + dst.h };
    posdata[base + 2] = (HMM_Vec2){ dst.x,             dst.y };
    posdata[base + 3] = (HMM_Vec2){ dst.x + dst.w, dst.y };

    // Define the UV coordinates based on the source rectangle
    uvdata[base + 0] = (HMM_Vec2){ src.x,                  src.y + src.h };
    uvdata[base + 1] = (HMM_Vec2){ src.x + src.w,      src.y + src.h };    
    uvdata[base + 2] = (HMM_Vec2){ src.x,                  src.y };
    uvdata[base + 3] = (HMM_Vec2){ src.x + src.w,      src.y };

    cblas_scopy(4, color.e, 1, &(colordata[base+0].x),1);
    cblas_scopy(4, color.e, 1, &(colordata[base+1].x),1);
    cblas_scopy(4, color.e, 1, &(colordata[base+2].x),1);
    cblas_scopy(4, color.e, 1, &(colordata[base+3].x),1);
/*    colordata[base + 0] = color;
    colordata[base + 1] = color;
    colordata[base + 2] = color;
    colordata[base + 3] = color;
    */
    JS_FreeValue(js,sub);
    JS_FreeValue(js,jscolor);
    JS_FreeValue(js,jsdst);
    JS_FreeValue(js,jssrc);
  }

  ret = JS_NewObject(js);
  JS_SetProperty(js, ret, pos_atom, make_gpu_buffer(js, posdata, sizeof(*posdata) * verts, JS_TYPED_ARRAY_FLOAT32, 2, 0));
  JS_SetProperty(js, ret, uv_atom, make_gpu_buffer(js, uvdata, sizeof(*uvdata) * verts, JS_TYPED_ARRAY_FLOAT32, 2, 0));
  JS_SetProperty(js, ret, color_atom, make_gpu_buffer(js, colordata, sizeof(*colordata) * verts, JS_TYPED_ARRAY_FLOAT32, 2, 0));
  JS_SetProperty(js, ret, indices_atom, make_quad_indices_buffer(js, quads));
  JS_SetProperty(js, ret, vertices_atom, number2js(js, verts));
  JS_SetProperty(js, ret, count_atom, number2js(js, count));
)

int detectImageInWebcam(SDL_Surface *a, SDL_Surface *b);
JSC_CCALL(os_match_img,
  SDL_Surface *img1 = js2SDL_Surface(js,argv[0]);
  SDL_Surface *img2 = js2SDL_Surface(js,argv[1]);
  int n = detectImageInWebcam(img1,img2);
  return number2js(js,n);
)

static const JSCFunctionListEntry js_os_funcs[] = {
  MIST_FUNC_DEF(os, turbulence, 4),
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
  MIST_FUNC_DEF(os, make_sprite_mesh, 2),
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
  MIST_FUNC_DEF(os, match_img, 2),
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
//JSValue js_imgui(JSContext *js);
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
  QJSCLASSPREP_FUNCS(SDL_Texture)
  QJSCLASSPREP_FUNCS(SDL_Renderer)
  QJSCLASSPREP_FUNCS(SDL_Camera)
  QJSCLASSPREP_FUNCS(SDL_Cursor)

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
//  JS_SetPropertyStr(js, globalThis, "imgui", js_imgui(js));
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

  JS_FreeValue(js,globalThis);  
}
