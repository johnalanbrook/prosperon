#include "jsffi.h"

#include "script.h"

#include "anim.h"
#include "timer.h"
#include "debug.h"
#include "debugdraw.h"
#include "font.h"
#include "gameobject.h"
#include "input.h"
#include "log.h"
#include "dsp.h"
#include "music.h"
#include "2dphysics.h"
#include "datastream.h"
#include "sound.h"
#include "sprite.h"
#include "stb_ds.h"
#include "string.h"
#include "window.h"
#include "spline.h"
#include "yugine.h"
#include "particle.h"
#include <assert.h>
#include "resources.h"
#include <sokol/sokol_time.h>
#include <sokol/sokol_app.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>

#if (defined(_WIN32) || defined(__WIN32__))
#include <direct.h>
#define mkdir(x,y) _mkdir(x)
#endif

#include "nota.h"

#include "render.h"

#include "model.h"

#include "HandmadeMath.h"

#define countof(x) (sizeof(x)/sizeof((x)[0]))

static JSValue globalThis;

static JSClassID js_ptr_id;
static JSClassDef js_ptr_class = { "POINTER" };

#define QJSCLASS(TYPE)\
static JSClassID js_ ## TYPE ## _id;\
static void js_##TYPE##_finalizer(JSRuntime *rt, JSValue val){\
TYPE *n = JS_GetOpaque(val, js_##TYPE##_id);\
TYPE##_free(n);}\
static JSClassDef js_##TYPE##_class = {\
  #TYPE,\
  .finalizer = js_##TYPE##_finalizer,\
};\
static TYPE *js2##TYPE (JSValue val) { return JS_GetOpaque(val,js_##TYPE##_id); }\
static JSValue TYPE##2js(TYPE *n) { \
  JSValue j = JS_NewObjectClass(js,js_##TYPE##_id);\
  JS_SetOpaque(j,n);\
  return j; }\


QJSCLASS(gameobject)
QJSCLASS(emitter)
QJSCLASS(dsp_node)
QJSCLASS(warp_gravity)
QJSCLASS(warp_damp)
QJSCLASS(material)
QJSCLASS(mesh)

/* qjs class colliders and constraints */
/* constraint works for all constraints - 2d or 3d */
static JSClassID js_constraint_id;
static void js_constraint_finalizer(JSRuntime *rt, JSValue val) {
  constraint *c = JS_GetOpaque(val, js_constraint_id);
  constraint_free(c);
}
static JSClassDef js_constraint_class = {
  "constraint",
  .finalizer = js_constraint_finalizer
};
static constraint *js2constraint(JSValue val) { return JS_GetOpaque(val, js_constraint_id); }
static JSValue constraint2js(constraint *c)
{
  JSValue j = JS_NewObjectClass(js, js_constraint_id);
  JS_SetOpaque(j, c);
  return j;
}

static JSValue sound_proto;
sound *js2sound(JSValue v) { return js2dsp_node(v)->data; }

#define QJSGLOBALCLASS(NAME) \
JSValue NAME = JS_NewObject(js); \
JS_SetPropertyFunctionList(js, NAME, js_##NAME##_funcs, countof(js_##NAME##_funcs)); \
JS_SetPropertyStr(js, globalThis, #NAME, NAME); \

#define QJSCLASSPREP(TYPE) \
JS_NewClassID(&js_##TYPE##_id);\
JS_NewClass(JS_GetRuntime(js), js_##TYPE##_id, &js_##TYPE##_class);\

#define QJSCLASSPREP_FUNCS(TYPE) \
QJSCLASSPREP(TYPE); \
JSValue TYPE##_proto = JS_NewObject(js); \
JS_SetPropertyFunctionList(js, TYPE##_proto, js_##TYPE##_funcs, countof(js_##TYPE##_funcs)); \
JS_SetClassProto(js, js_##TYPE##_id, TYPE##_proto); \

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)     \
      (byte & 0x80 ? '1' : '0'), \
      (byte & 0x40 ? '1' : '0'), \
      (byte & 0x20 ? '1' : '0'), \
      (byte & 0x10 ? '1' : '0'), \
      (byte & 0x08 ? '1' : '0'), \
      (byte & 0x04 ? '1' : '0'), \
      (byte & 0x02 ? '1' : '0'), \
      (byte & 0x01 ? '1' : '0')

void js_setprop_str(JSValue obj, const char *prop, JSValue v) { JS_SetPropertyStr(js, obj, prop, v); }

JSValue jstzone()
{
  time_t t = time(NULL);
  time_t local_t = mktime(localtime(&t));
  double diff = difftime(t, local_t);
  return number2js(diff/3600);
}

int js2bool(JSValue v) { return JS_ToBool(js, v); }

JSValue bool2js(int b) { return JS_NewBool(js,b); }

JSValue jsdst()
{
  time_t t = time(NULL);
  return bool2js(localtime(&t)->tm_isdst);
}

JSValue jscurtime()
{
  time_t t;
  time(&t);
  JSValue jst = number2js(t);
  return jst;
}

void js_setprop_num(JSValue obj, uint32_t i, JSValue v) { JS_SetPropertyUint32(js, obj, i, v); }

JSValue gos2ref(gameobject **go)
{
  JSValue array = JS_NewArray(js);
  for (int i = 0; i < arrlen(go); i++)
    js_setprop_num(array,i,JS_DupValue(js,go[i]->ref));
  return array;
}

JSValue js_getpropstr(JSValue v, const char *str)
{
  JSValue p = JS_GetPropertyStr(js,v,str);
  JS_FreeValue(js,p);
  return p;
}

JSValue js_getpropidx(JSValue v, uint32_t i)
{
  JSValue p = JS_GetPropertyUint32(js,v,i);
  JS_FreeValue(js,p);
  return p;
}

static inline cpBody *js2body(JSValue v) { return js2gameobject(v)->body; }

int64_t js2int64(JSValue v) {
  int64_t i;
  JS_ToInt64(js, &i, v);
  return i;
}

int js2int(JSValue v) {
  int i;
  JS_ToInt32(js, &i, v);
  return i;
}

JSValue int2js(int i) { return JS_NewInt64(js, i); }

JSValue str2js(const char *c) { return JS_NewString(js, c); }
const char *js2str(JSValue v) { return JS_ToCString(js, v); }

JSValue strarr2js(char **c)
{
  JSValue arr = JS_NewArray(js);
  for (int i = 0; i < arrlen(c); i++)
    js_setprop_num(arr,i,JS_NewString(js, c[i]));

  return arr;
}

double js2number(JSValue v) {
  double g;
  JS_ToFloat64(js, &g, v);
  return g;
}

void *js2ptr(JSValue v) { return JS_GetOpaque(v,js_ptr_id); }

JSValue number2js(double g) {
  return JS_NewFloat64(js,g);
}
struct sprite *js2sprite(JSValue v) { return id2sprite(js2int(v)); }

JSValue ptr2js(void *ptr) {
  JSValue obj = JS_NewObjectClass(js, js_ptr_id);
  JS_SetOpaque(obj, ptr);
  return obj;
}

struct timer *js2timer(JSValue v) { return id2timer(js2int(v)); }

double js_get_prop_number(JSValue v, const char *p) {
  double num;
  JS_ToFloat64(js, &num, js_getpropstr(v,p));
  return num;
}

struct glrect js2glrect(JSValue v) {
  struct glrect rect;
  rect.s0 = js_get_prop_number(v, "s0");
  rect.s1 = js_get_prop_number(v, "s1");
  rect.t0 = js_get_prop_number(v, "t0");
  rect.t1 = js_get_prop_number(v, "t1");
  return rect;
}

JSValue js_arridx(JSValue v, int idx) { return js_getpropidx(v, idx); }

int js_arrlen(JSValue v) {
  int len;
  JS_ToInt32(js, &len, js_getpropstr( v, "length"));
  return len;
}

char *js_nota_decode(JSValue *tmp, char *nota)
{
  int type = nota_type(nota);
  JSValue ret2;
  long long n;
  double d;
  int b;
  char *str;

  switch(type) {
    case NOTA_BLOB:
      break;
    case NOTA_TEXT:
      nota = nota_read_text(&str, nota);
      *tmp = str2js(str);
      /* TODO: Avoid malloc and free here */
      free(str);
      break;
    case NOTA_ARR:
      nota = nota_read_array(&n, nota);
      *tmp = JS_NewArray(js);
      for (int i = 0; i < n; i++) {
        nota = js_nota_decode(&ret2, nota);
        JS_SetPropertyInt64(js, *tmp, i, ret2);
      }
      break;
    case NOTA_REC:
      nota = nota_read_record(&n, nota);
      *tmp = JS_NewObject(js);
      for (int i = 0; i < n; i++) {
        nota = nota_read_text(&str, nota);
	nota = js_nota_decode(&ret2, nota);
	JS_SetPropertyStr(js, *tmp, str, ret2);
	free(str);
      }
      break;
    case NOTA_INT:
      nota = nota_read_int(&n, nota);
      *tmp = int2js(n);
      break;
    case NOTA_SYM:
      nota = nota_read_sym(&b, nota);
      if (b == NOTA_NULL) *tmp = JS_UNDEFINED;
      else
        *tmp = bool2js(b);
      break;
    default:
    case NOTA_FLOAT:
      nota = nota_read_float(&d, nota);
      *tmp = number2js(d);
      break;
  }

  return nota;
}

char *js_nota_encode(JSValue v, char *nota)
{
  int tag = JS_VALUE_GET_TAG(v);
  char *str = NULL;
  JSPropertyEnum *ptab;
  uint32_t plen;
  int n;
  JSValue val;
  
  switch(tag) {
    case JS_TAG_FLOAT64:
      return nota_write_float(JS_VALUE_GET_FLOAT64(v), nota);
    case JS_TAG_INT:
      return nota_write_int(JS_VALUE_GET_INT(v), nota);
    case JS_TAG_STRING:
      str = js2str(v);
      nota = nota_write_text(str, nota);
      JS_FreeCString(js, str);
      return nota;
    case JS_TAG_BOOL:
      return nota_write_sym(JS_VALUE_GET_BOOL(v), nota);
    case JS_TAG_UNDEFINED:
      return nota_write_sym(NOTA_NULL, nota);
    case JS_TAG_NULL:
      return nota_write_sym(NOTA_NULL, nota);
    case JS_TAG_OBJECT:
      if (JS_IsArray(js, v)) {
        int n = js_arrlen(v);
        nota = nota_write_array(n, nota);
        for (int i = 0; i < n; i++)
          nota = js_nota_encode(js_arridx(v, i), nota);
        return nota;
      }
      n = JS_GetOwnPropertyNames(js, &ptab, &plen, v, JS_GPN_ENUM_ONLY | JS_GPN_STRING_MASK);
      nota = nota_write_record(plen, nota);
      
      for (int i = 0; i < plen; i++) {
        val = JS_GetProperty(js,v,ptab[i].atom);
        str = JS_AtomToCString(js, ptab[i].atom);
	JS_FreeAtom(js, ptab[i].atom);
	
        nota = nota_write_text(str, nota);
	JS_FreeCString(js, str);
	
        nota = js_nota_encode(val, nota);
	JS_FreeValue(js,val);
      }
      
      js_free(js, ptab);
      return nota;
  }
}

struct rgba js2color(JSValue v) {
  JSValue c[4];
  for (int i = 0; i < 4; i++)
    c[i] = js_arridx(v,i);
  float a = JS_IsUndefined(c[3]) ? 1.0 : js2number(c[3]);
  struct rgba color = {
    .r = js2number(c[0])*RGBA_MAX,
    .g = js2number(c[1])*RGBA_MAX,
    .b = js2number(c[2])*RGBA_MAX,
    .a = a*RGBA_MAX,
  };

  for (int i = 0; i < 4; i++)
    JS_FreeValue(js,c[i]);

  return color;
}

JSValue color2js(struct rgba color)
{
  JSValue arr = JS_NewArray(js);
  js_setprop_num(arr,0,JS_NewFloat64(js,(double)color.r/255));
  js_setprop_num(arr,1,JS_NewFloat64(js,(double)color.g/255));  
  js_setprop_num(arr,2,JS_NewFloat64(js,(double)color.b/255));
  js_setprop_num(arr,3,JS_NewFloat64(js,(double)color.a/255));
  return arr;
}

struct boundingbox js2bb(JSValue v)
{
  struct boundingbox bb;
  bb.t = js2number(js_getpropstr(v,"t"));
  bb.b = js2number(js_getpropstr(v,"b"));
  bb.r = js2number(js_getpropstr(v,"r"));
  bb.l = js2number(js_getpropstr(v,"l"));
  return bb;
}


HMM_Vec2 js2vec2(JSValue v)
{
  HMM_Vec2 v2;
  v2.X = js2number(js_getpropidx(v,0));
  v2.Y = js2number(js_getpropidx(v,1));
  return v2;
}

HMM_Vec3 js2vec3(JSValue v)
{
  HMM_Vec3 v3;
  v3.x = js2number(js_getpropidx(v,0));
  v3.y = js2number(js_getpropidx(v,1));
  v3.z = js2number(js_getpropidx(v,2));
  return v3;
}

JSValue vec32js(HMM_Vec3 v)
{
  JSValue array = JS_NewArray(js);
  js_setprop_num(array,0,number2js(v.x));
  js_setprop_num(array,1,number2js(v.y));
  js_setprop_num(array,2,number2js(v.z));
  return array;
}

cpBitmask js2bitmask(JSValue v) {
  cpBitmask mask = 0;
  int len = js_arrlen(v);

  for (int i = 0; i < len; i++) {
    int val = JS_ToBool(js, js_getpropidx( v, i));
    if (!val) continue;

    mask |= 1 << i;
  }

  return mask;
}

HMM_Vec2 *js2cpvec2arr(JSValue v) {
  HMM_Vec2 *arr = NULL;
  int n = js_arrlen(v);
  arrsetlen(arr,n);
  
  for (int i = 0; i < n; i++)
    arr[i] = js2vec2(js_getpropidx( v, i));
    
  return arr;
}

HMM_Vec2 *jsfloat2vec(JSValue v)
{
  size_t s;
  void *buf = JS_GetArrayBuffer(js, &s, v);
  HMM_Vec2 *arr = NULL;
  int n = s/2;
  n /= sizeof(float);
//  arrsetcap(arr,n);
//  memcpy(arr,buf,s);
  return arr;
}

JSValue bitmask2js(cpBitmask mask) {
  JSValue arr = JS_NewArray(js);
  for (int i = 0; i < 11; i++)
    js_setprop_num(arr,i,JS_NewBool(js,mask & 1 << i));

  return arr;
}

void vec2float(HMM_Vec2 v, float *f) {
  f[0] = v.x;
  f[1] = v.y;
}

JSValue vec2js(HMM_Vec2 v) {
  JSValue array = JS_NewArray(js);
  js_setprop_num(array,0,number2js(v.x));
  js_setprop_num(array,1,number2js(v.y));
  return array;
}

JSValue vecarr2js(HMM_Vec2 *points, int n) {
  JSValue array = JS_NewArray(js);
  for (int i = 0; i < n; i++)
    js_setprop_num(array,i,vec2js(points[i]));
    
  return array;
}

JSValue duk_ui_text(JSContext *js, JSValueConst this, int argc, JSValueConst *argv) {
  const char *s = JS_ToCString(js, argv[0]);
  HMM_Vec2 pos = js2vec2(argv[1]);

  float size = js2number(argv[2]);
  struct rgba c = js2color(argv[3]);
  int wrap = js2int(argv[4]);
  int cursor = js2int(argv[5]);
  JSValue ret = JS_NewInt64(js, renderText(s, pos, size, c, wrap, cursor, 1.0));
  JS_FreeCString(js, s);
  return ret;
}

int js_print_exception(JSValue v)
{
#ifndef NDEBUG
  if (!JS_IsException(v))
    return 0;

  JSValue ex = JS_GetException(js);
    
    const char *name = JS_ToCString(js, js_getpropstr(ex, "name"));
    const char *msg = js2str(js_getpropstr(ex, "message"));
    const char *stack = js2str(js_getpropstr(ex, "stack"));
    int line = 0;
    mYughLog(LOG_SCRIPT, LOG_ERROR, line, "script", "%s :: %s\n%s", name, msg, stack);

    JS_FreeCString(js, name);
    JS_FreeCString(js, msg);
    JS_FreeCString(js, stack);
    JS_FreeValue(js,ex);
    return 1;
#endif
  return 0;
}

JSValue duk_gui_img(JSContext *js, JSValueConst this, int argc, JSValueConst *argv) {
  const char *img = JS_ToCString(js, argv[0]);
  transform2d t;
  t.pos = js2vec2(argv[1]);
  t.scale = js2vec2(argv[2]);
  t.angle = js2number(argv[3]);
  gui_draw_img(img, t, js2bool(argv[4]), js2vec2(argv[5]), 1.0, js2color(argv[6]));
  JS_FreeCString(js, img);
  return JS_UNDEFINED;
}

struct rect js2rect(JSValue v) {
  struct rect rect;
  rect.x = js2number(js_getpropstr(v, "x"));
  rect.y = js2number(js_getpropstr(v, "y"));
  rect.w = js2number(js_getpropstr(v, "w"));
  rect.h = js2number(js_getpropstr(v, "h"));
  return rect;
}

JSValue rect2js(struct rect rect) {
  JSValue obj = JS_NewObject(js);
  js_setprop_str(obj, "x", JS_NewFloat64(js, rect.x));
  js_setprop_str(obj, "y", JS_NewFloat64(js, rect.y));
  js_setprop_str(obj, "w", JS_NewFloat64(js, rect.w));
  js_setprop_str(obj, "h", JS_NewFloat64(js, rect.h));
  return obj;
}

JSValue bb2js(struct boundingbox bb)
{
  JSValue obj = JS_NewObject(js);
  js_setprop_str(obj,"t", JS_NewFloat64(js,bb.t));
  js_setprop_str(obj,"b", JS_NewFloat64(js,bb.b));
  js_setprop_str(obj,"r", JS_NewFloat64(js,bb.r));
  js_setprop_str(obj,"l", JS_NewFloat64(js,bb.l));
  return obj;
}

JSValue duk_spline_cmd(JSContext *js, JSValueConst this, int argc, JSValueConst *argv) {
  int cmd = js2int(argv[0]);
  /*
    0: catmull-rom
    1: bezier
  */

  int type = js2int(argv[1]);
  int d = js2int(argv[2]); /* dimensions: 1d, 2d, 3d ...*/
  HMM_Vec2 *points = js2cpvec2arr(argv[3]);
  float param = js2number(argv[4]);
  HMM_Vec2 *samples = NULL;
  switch(type) {
    case 0:
      samples = catmull_rom_ma_v2(points, param);
      break;
    case 1:
      samples = bezier_cb_ma_v2(points, param);
      break;
  }

  arrfree(points);
  
  if (!samples)
    return JS_UNDEFINED;

  JSValue arr = vecarr2js(samples, arrlen(samples));  
  arrfree(samples);

  return arr;
}

JSValue ints2js(int *ints) {
  JSValue arr = JS_NewArray(js);
  for (int i = 0; i < arrlen(ints); i++)
    js_setprop_num(arr,i,int2js(ints[i]));

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

JSValue duk_cmd(JSContext *js, JSValueConst this, int argc, JSValueConst *argv) {
  int cmd = js2int(argv[0]);
  const char *str = NULL;
  const char *str2 = NULL;
  void *d1 = NULL;
  void *d2 = NULL;
  void *v1 = NULL;
  gameobject **ids = NULL;
  int *intids = NULL;
  gameobject *go = NULL;
  JSValue ret = JS_UNDEFINED;
  size_t plen = 0;

  switch (cmd) {
  case 0:
    str = JS_ToCString(js, argv[1]);
    ret = JS_NewInt64(js, script_dofile(str));
    break;

  case 1:
    YughWarn("Do not set pawns here anymore; Do it entirely in script.");
    // set_pawn(js2ptrduk_get_heapptr(duk, 1));
    break;

  case 3:
    set_timescale(js2number(argv[1]));
    break;

  case 4:
    debug_draw_phys(JS_ToBool(js, argv[1]));
    break;

  case 6:
    updateMS = js2number(argv[1]);
    break;

  case 7:
    physMS = js2number(argv[1]);
    break;

  case 8:
    phys2d_set_gravity(js2vec2(argv[1]));
    break;

  case 9:
    sprite_delete(js2int(argv[1]));
    break;

  case 10:
    YughWarn("Pawns are handled in script only now.");
    break;

  case 11:
    str = JS_ToCString(js, argv[1]);
    ret = JS_NewInt64(js, file_mod_secs(str));
    break;

  case 12:
    str = JS_ToCString(js, argv[2]);
    sprite_loadtex(id2sprite(js2int(argv[1])), str, js2glrect(argv[3]));
    break;

  case 13:
    str = JS_ToCString(js, argv[1]);
    str2 = JS_ToCString(js, argv[2]);
    play_song(str, str2);
    break;
  case 15:
  gameobject_draw_debug(js2gameobject(argv[1]));
  break;

  case 16:
    str = js2str(argv[1]);
    file_eval_env(str,argv[2]);
    break;

  case 17:
    sapp_set_mouse_cursor(js2int(argv[1]));
    break;

  case 18:
    shape_set_sensor(js2ptr(argv[1]), JS_ToBool(js, argv[2]));
    break;

  case 20:
    sprite_enabled(js2int(argv[1]), JS_ToBool(js, argv[2]));
    break;

  case 21:
    ret = JS_NewBool(js, shape_get_sensor(js2ptr(argv[1])));
    break;

  case 22:
    shape_enabled(js2ptr(argv[1]), JS_ToBool(js, argv[2]));
    break;

  case 23:
    ret = JS_NewBool(js, shape_is_enabled(js2ptr(argv[1])));
    break;

  case 24:
    timer_pause(js2timer(argv[1]));
    break;

  case 25:
    timer_stop(js2timer(argv[1]));
    break;

  case 26:
    timer_start(js2timer(argv[1]));
    break;

  case 27:
    timer_remove(js2int(argv[1]));
    break;

  case 28:
    timerr_settime(js2timer(argv[1]), js2number(argv[2]));
    break;

  case 29:
    ret = JS_NewFloat64(js, js2timer(argv[1])->interval);
    break;

  case 31:
    free(js2ptr(argv[1]));
    break;

  case 32:
    ret = JS_NewFloat64(js, js2timer(argv[1])->remain_time);
    break;

  case 33:
    ret = JS_NewBool(js, js2timer(argv[1])->on);
    break;

  case 34:
    ret = JS_NewBool(js, js2timer(argv[1])->repeat);
    break;

  case 35:
    js2timer(argv[1])->repeat = JS_ToBool(js, argv[2]);
    break;

  case 36:
    js2gameobject(argv[1])->scale.XY = js2vec2(argv[2]);
    gameobject_apply(js2gameobject(argv[1]));
    cpSpaceReindexShapesForBody(space, js2gameobject(argv[1])->body);
    break;

  case 37:
    if (!id2sprite(js2int(argv[1]))) break;
    id2sprite(js2int(argv[1]))->t.pos = js2vec2(argv[2]);
    break;

  case 40:
    js2gameobject(argv[1])->filter.categories = js2bitmask(argv[2]);
    gameobject_apply(js2gameobject(argv[1]));
    break;

  case 41:
    js2gameobject(argv[1])->filter.mask = js2bitmask(argv[2]);
    gameobject_apply(js2gameobject(argv[1]));    
    break;

  case 42:
    ret = bitmask2js(js2gameobject(argv[1])->filter.categories);
    break;

  case 43:
    ret = bitmask2js(js2gameobject(argv[1])->filter.mask);
    break;

  case 44:
    go = pos2gameobject(js2vec2(argv[1]), js2number(argv[2]));
    ret = go ? JS_DupValue(js,go->ref) : JS_UNDEFINED;
    break;

  case 45:
    ret = vec2js(mouse_pos);
    break;

  case 46:
    set_mouse_mode(js2int(argv[1]));
    break;

  case 47:
    draw_grid(js2number(argv[1]), js2number(argv[2]), js2color(argv[3]));
    break;

  case 48:
    ret = JS_NewInt64(js, mainwin.width);
    break;

  case 49:
    ret = JS_NewInt64(js, mainwin.height);
    break;

  case 50:
    ret = JS_NewBool(js, action_down(js2int(argv[1])));
    break;

  case 51:
    draw_cppoint(js2vec2(argv[1]), js2number(argv[2]), js2color(argv[3]));
    break;

  case 52:
    ids = phys2d_query_box(js2vec2(argv[1]), js2vec2(argv[2]));
    ret = gos2ref(ids);
    arrfree(ids);
    break;

  case 53:
    draw_box(js2vec2(argv[1]), js2vec2(argv[2]), js2color(argv[3]));
    break;

  case 54:
    gameobject_apply(js2gameobject(argv[1]));
    break;

  case 59:
    v1 = js2cpvec2arr(argv[2]);
    ret = JS_NewInt64(js, point2segindex(js2vec2(argv[1]), v1, js2number(argv[3])));
    break;

  case 61:
    set_cam_body(js2gameobject(argv[1])->body);
    break;

  case 62:
    add_zoom(js2number(argv[1]));
    break;

  case 63:
    set_cam_body(NULL);
    break;

  case 64:
    str = JS_ToCString(js, argv[1]);
    ret = vec2js(tex_get_dimensions(texture_pullfromfile(str)));
    break;

  case 66:
    ret = strarr2js(ls(","));
    break;

  case 67:
    opengl_rendermode(LIT);
    break;

  case 68:
    opengl_rendermode(WIREFRAME);
    break;

  case 70:
    ret = vec2js(world2go(js2gameobject(argv[1]), js2vec2(argv[2])));
    break;

  case 71:
    ret = vec2js(go2world(js2gameobject(argv[1]), js2vec2(argv[2])));
    break;

  case 72:
    ret = vec2js((HMM_Vec2)cpSpaceGetGravity(space));
    break;

  case 73:
    cpSpaceSetDamping(space, js2number(argv[1]));
    break;

  case 74:
    ret = JS_NewFloat64(js, cpSpaceGetDamping(space));
    break;

  case 75:
    js2gameobject(argv[1])->layer = js2int(argv[2]);
    break;

  case 76:
    set_cat_mask(js2int(argv[1]), js2bitmask(argv[2]));
    break;

  case 77:
    ret = int2js(js2gameobject(argv[1])->layer);
    break;

  case 79:
    ret = JS_NewBool(js, phys_stepping());
    break;

  case 80:
    ids = phys2d_query_shape(js2ptr(argv[1]));
    ret = gos2ref(ids);
    arrfree(ids);
    break;

  case 82:
    gameobject_draw_debug(js2gameobject(argv[1]));
    break;

  case 83:
    v1 = js2cpvec2arr(argv[1]);
    draw_edge(v1, js_arrlen(argv[1]), js2color(argv[2]), js2number(argv[3]), 0, js2color(argv[2]), 10);
    break;

  case 84:
    ret = consolelog ? JS_NewString(js, consolelog) : JS_NewString(js,"");
    break;

  case 85:
    ret = vec2js(HMM_ProjV2(js2vec2(argv[1]), js2vec2(argv[2])));
    break;

  case 86:
    v1 = js2cpvec2arr(argv[3]);
    intids = phys2d_query_box_points(js2vec2(argv[1]), js2vec2(argv[2]), v1, js2int(argv[4]));
    ret = ints2js(intids);
    arrfree(intids);
    break;

  case 88:
    ret = number2js(HMM_DotV2(js2vec2(argv[1]), js2vec2(argv[2])));
    break;

  case 90:
    str = JS_ToCString(js, argv[1]);
    window_set_icon(str);
    break;

  case 91:
    str = JS_ToCString(js, argv[1]);
    log_print(str);
    break;

  case 92:
    logLevel = js2int(argv[1]);
    break;

  case 93:
    ret = int2js(logLevel);
    break;

  case 96:
    id2sprite(js2int(argv[1]))->color = js2color(argv[2]);
    break;

  case 97:
    str = js2str(argv[1]);
    cursor_img(str);
    break;
  case 103:
    ret = vec2js(js2gameobject(argv[1])->scale.XY);
    break;
  case 106:
    js2gameobject(argv[1])->e = js2number(argv[2]);
    break;
  case 107:
    ret = number2js(js2gameobject(argv[1])->e);
    break;
  case 108:
    js2gameobject(argv[1])->f = js2number(argv[2]);
    break;
  case 109:
    ret = number2js(js2gameobject(argv[1])->f);
    break;
  case 110:
    ret = number2js(js2gameobject(argv[1])->e);
    break;

    case 111:
      ret = vec2js(js2sprite(argv[1])->t.pos);
      break;

    case 112:
      ret = number2js(((struct phys2d_edge*)js2ptr(argv[1]))->thickness);
      break;

    case 113:
      js2gameobject(argv[1])->ref = argv[2];
      break;

    case 114:
      ret = bool2js(js2sprite(argv[1])->enabled);
      break;

    case 115:
      draw_circle(js2vec2(argv[1]), js2number(argv[2]), js2number(argv[2]), js2color(argv[3]), -1);
      break;

    case 116:
      ret = str2js(tex_get_path(js2sprite(argv[1])->tex));
      break;
      
    case 117:
      str = JS_ToCString(js, argv[1]);
      ret = script_runfile(str);
      break;

    case 118:
      str = JS_ToCString(js,argv[1]);
      ret = bb2js(text_bb(str, js2number(argv[2]), js2number(argv[3]), 1.0));
      break;

    case 119:
      str = JS_ToCString(js, argv[1]);
      ret = JS_NewInt64(js, file_mod_secs(str));
      break;

    case 121:
      ret = number2js(get_timescale());
      break;

    case 122:
      str = JS_ToCString(js, argv[1]);
      ret = file_eval_env(str, argv[2]);
      break;

    case 123:
      str = JS_ToCString(js, argv[1]);
      str2 = JS_ToCString(js, argv[3]);
      ret = eval_file_env(str, str2, argv[2]);
      break;

    case 124:
      str = JS_ToCString(js, argv[1]);
      pack_engine(str);
      break;

    case 125:
      mainwin.width = js2int(argv[1]);
      break;

    case 126:
      mainwin.height = js2int(argv[1]);
      break;

    case 127:
      ret = JS_NewInt64(js, stm_now());
      break;

    case 128:
      ret = JS_NewFloat64(js, stm_ns(js2int64(argv[1])));
      break;

    case 129:
      ret = JS_NewFloat64(js, stm_us(js2int64(argv[1])));
      break;

    case 130:
      ret = JS_NewFloat64(js, stm_ms(js2int64(argv[1])));
      break;

    case 131:
      gif_rec_start(js2int(argv[1]), js2int(argv[2]), js2int(argv[3]), js2int(argv[4]));
      break;
    case 132:
      str = JS_ToCString(js, argv[1]);
      gif_rec_end(str);
      break;
    case 133:
      ret = JS_NewFloat64(js, apptime());
      break;

    case 134:
      str = JS_ToCString(js,argv[1]);
      app_name(str);
      break;
      
    case 135:
      ret = number2js(cam_zoom());
      break;

    case 136:
      ret = vec2js(world2screen(js2vec2(argv[1])));
      break;

    case 137:
      ret = vec2js(screen2world(js2vec2(argv[1])));
      break;

    case 138:
      str = JS_ToCString(js, argv[1]);
      ret = JS_NewInt64(js, jso_file(str));
      break;

    case 139:
      str = JS_ToCString(js,argv[1]);
      ret = JS_NewInt64(js, gif_nframes(str));
      break;

    case 140:
      sg_apply_scissor_rectf(js2number(argv[1]), js2number(argv[2]), js2number(argv[3]), js2number(argv[4]), 0);
      break;

    case 141:
      text_flush(&hudproj);
      break;

    case 142:
      str = JS_ToCString(js, argv[1]);
      console_print(str);
      break;

    case 143:
      str = JS_ToCString(js, argv[1]);
      if (!getenv(str)) ret = JS_UNDEFINED;
      else
        ret = str2js(getenv(str));
      break;

    case 145:
      if (js2bool(argv[1])) window_makefullscreen(&mainwin);
      else window_unfullscreen(&mainwin);
      break;
    case 146:
      log_clear();
      break;

    case 147:
      exit(js2int(argv[1]));
      break;
    case 148:
      ret = color2js(id2sprite(js2int(argv[1]))->color);
      break;
    case 149:
      ((struct drawmodel *)js2ptr(argv[1]))->model = GetExistingModel(js2str(argv[2]));
      break;

    case 150:
      draw_drawmodel(js2ptr(argv[1]));
      break;

   case 151:
     js2gameobject(argv[1])->maxvelocity = js2number(argv[2]);
     break;
   case 152:
     ret = number2js(js2gameobject(argv[1])->maxvelocity);
     break;
    case 153:
     cpBodySetTorque(js2gameobject(argv[1])->body, js2number(argv[2]));
     break;
    case 154:
      js2gameobject(argv[1])->maxangularvelocity = js2number(argv[2]);
      break;
    case 155:
      ret = number2js(js2gameobject(argv[1])->maxangularvelocity);
      break;

    case 156:
      js2gameobject(argv[1])->damping = js2number(argv[2]);
      break;
    case 157:
      ret = number2js(js2gameobject(argv[1])->damping);
      break;
    case 160:
      ret = vec2js(mat_t_dir(t_world2go(js2gameobject(argv[1])), js2vec2(argv[2])));
      break;
    case 161:
      ret = vec2js(mat_t_dir(t_go2world(js2gameobject(argv[1])), js2vec2(argv[2])));
      break;
    case 162:
      str = JS_ToCString(js, argv[1]);
      ret = int2js(remove(str));
      break;

    case 164:
      unplug_node(js2ptr(argv[1]));
      break;

    case 168:
      js2gameobject(argv[1])->timescale = js2number(argv[2]);
      break;
    case 169:
      ret = number2js(js2gameobject(argv[1])->timescale);
      break;
    case 170:
      id2sprite(js2int(argv[1]))->emissive = js2color(argv[2]);
      break;
    case 171:
      ret = number2js(js2gameobject(argv[1])->drawlayer);
      break;
    case 172:
     js2gameobject(argv[1])->drawlayer = js2number(argv[2]);
     break;
   case 173:
     str = js2str(argv[1]);
     capture_screen(js2number(argv[2]), js2number(argv[3]), js2number(argv[4]), js2number(argv[5]), str);
     break;
   case 174:
     str = js2str(argv[1]);
     ds_openvideo(str);
     break;
    case 177:
      plugin_node(js2dsp_node(argv[1]), js2dsp_node(argv[2]));
      break;
    case 180:
      ret = dsp_node2js(masterbus);
      break;
    case 181:
      ret = dsp_node2js(make_node(NULL,NULL,NULL));
      break;
    case 182:
      str = js2str(argv[1]);
      ret = dsp_node2js(dsp_source(str));
      JS_SetPrototype(js, ret, sound_proto);
      break;
    case 185:
      ret = dsp_node2js(dsp_delay(js2number(argv[1]), js2number(argv[2])));
      break;
    case 186:
      ret = dsp_node2js(dsp_lpf(js2number(argv[1])));
      break;
    case 187:
      ret = dsp_node2js(dsp_hpf(js2number(argv[1])));
      break;
    case 188:
      str = js2str(argv[1]);
      ret = dsp_node2js(dsp_mod(str));
      break;
    case 189:
      ret = dsp_node2js(dsp_bitcrush(js2number(argv[1]), js2number(argv[2])));
      break;
    case 190:
      ret = dsp_node2js(dsp_compressor());
      break;
    case 191:
      ret = dsp_node2js(dsp_limiter(js2number(argv[1])));
      break;
    case 192:
      ret = dsp_node2js(dsp_noise_gate(js2number(argv[1])));
      break;
    case 197:
      ret = number2js(js2sound(argv[1])->data->frames);    
      break;
    case 198:
      ret = number2js(SAMPLERATE);
      break;
    case 200:
      ret = dsp_node2js(dsp_pitchshift(js2number(argv[1])));
      break;
    case 203:
      ret = dsp_node2js(dsp_whitenoise());
      break;
    case 204:
      ret = dsp_node2js(dsp_pinknoise());
      break;
    case 205:
      ret = dsp_node2js(dsp_rednoise());
      break;
    case 206:
      str = js2str(argv[1]);
      str2 = js2str(argv[2]);
      ret = dsp_node2js(dsp_midi(str, make_soundfont(str2)));
      break;
    case 207:
      ret = dsp_node2js(dsp_fwd_delay(js2number(argv[1]), js2number(argv[2])));
      break;
    case 210:
      ret = jscurtime();
      break;
    case 211:
      ret = jstzone();
      break;
    case 212:
      ret = jsdst();
      break;
    case 213:
      free_drawmodel(js2ptr(argv[1]));
      break;
    case 214:
      ret = int2js(go_count());
      break;
    case 215:
      ret = vec2js(js2sprite(argv[1])->t.scale);
      break;
    case 216:
      js2sprite(argv[1])->t.scale = js2vec2(argv[2]);
      break;
    case 217:
      ret = number2js(js2sprite(argv[1])->t.angle);
      break;
    case 218:
      js2sprite(argv[1])->t.angle = js2number(argv[2]);
      break;
    case 219:
      js2sprite(argv[1])->drawmode = js2number(argv[2]);
      break;
    case 220:
      ret = number2js(js2sprite(argv[1])->drawmode);
      break;
    case 221:
      ret = constraint2js(constraint_make(cpPivotJointNew(js2gameobject(argv[1])->body, js2gameobject(argv[2])->body,js2vec2(argv[3]).cp)));
      break;
    case 222:
      ret = constraint2js(constraint_make(cpPinJointNew(js2gameobject(argv[1])->body, js2gameobject(argv[2])->body, cpvzero,cpvzero)));
      break;
    case 223:
      ret = constraint2js(constraint_make(cpGearJointNew(js2gameobject(argv[1])->body, js2gameobject(argv[2])->body, js2number(argv[3]), js2number(argv[4]))));
      break;
    case 224:
      str = js2str(argv[1]);
      ret = ints2js(gif_delays(str));
      break;
    case 225:
      ret = constraint2js(constraint_make(cpRotaryLimitJointNew(js2gameobject(argv[1])->body, js2gameobject(argv[2])->body, js2number(argv[3]), js2number(argv[4]))));
      break;
    case 226:
      ret = constraint2js(constraint_make(cpDampedRotarySpringNew(js2gameobject(argv[1])->body, js2gameobject(argv[2])->body, js2number(argv[3]), js2number(argv[4]), js2number(argv[5]))));
      break;
    case 227:
      ret = constraint2js(constraint_make(cpDampedSpringNew(js2gameobject(argv[1])->body, js2gameobject(argv[2])->body, js2vec2(argv[3]).cp, js2vec2(argv[4]).cp, js2number(argv[5]), js2number(argv[6]), js2number(argv[7]))));
      break;
    case 228:
      ret = constraint2js(constraint_make(cpGrooveJointNew(js2gameobject(argv[1])->body, js2gameobject(argv[2])->body, js2vec2(argv[3]).cp, js2vec2(argv[4]).cp, js2vec2(argv[5]).cp)));
      break;
    case 229:
      ret = constraint2js(constraint_make(cpSlideJointNew(js2gameobject(argv[1])->body, js2gameobject(argv[2])->body, js2vec2(argv[3]).cp, js2vec2(argv[4]).cp, js2number(argv[5]), js2number(argv[6]))));
      break;
    case 230:
      ret = constraint2js(constraint_make(cpRatchetJointNew(js2body(argv[1]), js2body(argv[2]), js2number(argv[3]), js2number(argv[4]))));
      break;
    case 231:
      ret = constraint2js(constraint_make(cpSimpleMotorNew(js2body(argv[1]), js2body(argv[2]), js2number(argv[3]))));
      break;
    case 232:
      ret = number2js(js2sprite(argv[1])->parallax);
      break;
    case 233:
      js2sprite(argv[1])->parallax = js2number(argv[2]);
      break;
    case 234:
      ret = emitter2js(make_emitter());
      break;
    case 249:
      str = JS_ToCString(js,argv[2]);
      js2emitter(argv[1])->texture = texture_pullfromfile(str);
      break;
    case 251:
      js2gameobject(argv[1])->warp_filter = js2bitmask(argv[2]);
      break;
    case 252:
      ret = bitmask2js(js2gameobject(argv[1])->warp_filter);
      break;
    case 253:
      ret = warp_gravity2js(warp_gravity_make());
      break;
    case 254:
      ret = warp_damp2js(warp_damp_make());
      break;

    case 255:
      ret = str2js(VER);
      break;

    case 256:
      ret = str2js(COM);
      break;
    case 257:
      engine_start(argv[1]);
      break;

    case 259:
      script_gc();
      break;
    case 260:
      str = js2str(argv[1]);
      d1 = script_compile(str, &plen);
      ret = JS_NewArrayBufferCopy(js, d1, plen);
      break;
    case 261:
      str = js2str(argv[1]);
      d1 = slurp_file(str, &plen);
      return script_run_bytecode(d1, plen);
      break;
    case 262:
      str = js2str(argv[1]);
      save_qoa(str);
      break;
  }

  if (str) JS_FreeCString(js, str);
  if (str2) JS_FreeCString(js, str2);
  if (d1) free(d1);
  if (d2) free(d2);
  if (v1) arrfree(v1);

  if (!JS_IsNull(ret)) return ret;
  return JS_UNDEFINED;
}

JSValue duk_register(JSContext *js, JSValueConst this, int argc, JSValueConst *argv) {
  int cmd = js2int(argv[0]);

  struct callee c;
  c.fn = argv[1];
  c.obj = argv[2];

  switch (cmd) {
  case 0:
    register_update(c);
    break;

  case 1:
    register_physics(c);
    break;

  case 2:
    register_gui(c);
    break;

  case 3:
    register_nk_gui(c);
    break;
  case 6:
    register_debug(c);
    break;
  case 7:
    register_pawn(c);
    break;

  case 8:
    register_gamepad(c);
    break;

  case 9:
    stacktrace_callee = c;
    break;

  case 10:
    register_draw(c);
    break;
  }

  return JS_UNDEFINED;
}

void gameobject_add_shape_collider(gameobject *go, JSValue fn, struct phys2d_shape *shape) {
  struct shape_cb shapecb;
  shapecb.shape = shape;
  shapecb.cbs.begin = fn;
  arrpush(go->shape_cbs, shapecb);
}

JSValue duk_register_collide(JSContext *js, JSValueConst this, int argc, JSValueConst *argv) {
  int cmd = js2int(argv[0]);
  gameobject *go = js2gameobject(argv[2]);
  JSValue fn = argv[1];

  switch (cmd) {
  case 0:
    go->cbs.begin = JS_DupValue(js,fn);
    break;

  case 1:
    gameobject_add_shape_collider(go, JS_DupValue(js,fn), js2ptr(argv[3]));
    break;

  case 2:
    phys2d_rm_go_handlers(go);
    break;

  case 3:
    go->cbs.separate = JS_DupValue(js,fn);
    break;
  }

  return JS_UNDEFINED;
}

JSValue duk_sys_cmd(JSContext *js, JSValueConst this, int argc, JSValueConst *argv) {
  int cmd = js2int(argv[0]);

  switch (cmd) {
  case 0:
    quit();
    break;

  case 1:
    sim_start();
    cpSpaceReindexStatic(space);
    break;

  case 2:
    sim_pause();
    break;

  case 3:
    sim_pause();
    break;

  case 4:
    sim_step();
    break;

  case 5:
    return JS_NewBool(js, sim_playing());

  case 6:
    return JS_NewBool(js, sim_paused());

  case 8:
    return JS_NewInt64(js, frame_fps());

  case 10:
    editor_mode = js2bool(argv[1]);
    break;
  }

  return JS_UNDEFINED;
}

JSValue duk_make_gameobject(JSContext *js, JSValueConst this, int argc, JSValueConst *argv) {
  return gameobject2js(MakeGameobject());
}

JSValue duk_yughlog(JSContext *js, JSValueConst this, int argc, JSValueConst *argv) {
  int cmd = js2int(argv[0]);
  const char *s = JS_ToCString(js, argv[1]);
  const char *f = JS_ToCString(js, argv[2]);
  int line = js2int(argv[3]);

  mYughLog(1, cmd, line, f, s);

  JS_FreeCString(js, s);
  JS_FreeCString(js, f);

  return JS_UNDEFINED;
}

JSValue duk_set_body(JSContext *js, JSValueConst this, int argc, JSValueConst *argv) {
  int cmd = js2int(argv[0]);
  gameobject *go = js2gameobject(argv[1]);
  
  if (!go) return JS_UNDEFINED;

  /* TODO: Possible that reindexing shapes only needs done for static shapes? */
  switch (cmd) {
  case 0:
    gameobject_setangle(go, js2number(argv[2]));
    break;

  case 1:
    go->bodytype = js2int(argv[2]);
    cpBodySetType(go->body, go->bodytype);
    gameobject_apply(go);
    break;

  case 2:
    cpBodySetPosition(go->body, js2vec2(argv[2]).cp);
    break;

  case 4:
    cpBodyApplyImpulseAtWorldPoint(go->body, js2vec2(argv[2]).cp, cpBodyGetPosition(go->body));
    return JS_UNDEFINED;

  case 7:
    go->mass = js2number(argv[2]);
    if (go->bodytype == CP_BODY_TYPE_DYNAMIC)
      cpBodySetMass(go->body, go->mass);
    break;

  case 8:
    cpBodySetAngularVelocity(go->body, js2number(argv[2]));
    return JS_UNDEFINED;

  case 9:
    cpBodySetVelocity(go->body, js2vec2(argv[2]).cp);
    return JS_UNDEFINED;

  case 10:
    go->e = fmax(js2number(argv[2]), 0);
    break;

  case 11:
    go->f = fmax(js2number(argv[2]), 0);
    break;

  case 12:
    cpBodyApplyForceAtWorldPoint(go->body, js2vec2(argv[2]).cp, cpBodyGetPosition(go->body));
    return JS_UNDEFINED;

  case 13:
    cpBodySetMoment(go->body, js2number(argv[2]));
    return JS_UNDEFINED;
  case 14:
    cpBodyApplyForceAtLocalPoint(go->body, js2vec2(argv[2]).cp, js2vec2(argv[3]).cp);
    return JS_UNDEFINED;
  }

  cpSpaceReindexShapesForBody(space, go->body);

  return JS_UNDEFINED;
}

JSValue duk_q_body(JSContext *js, JSValueConst this, int argc, JSValueConst *argv) {
  int q = js2int(argv[0]);
  gameobject *go = js2gameobject(argv[1]);

  if (!go) return JS_UNDEFINED;

  switch (q) {
  case 0:
    return JS_NewInt64(js, cpBodyGetType(go->body));

  case 1:
    return vec2js((HMM_Vec2)cpBodyGetPosition(go->body));

  case 2:
    return JS_NewFloat64(js, cpBodyGetAngle(go->body));

  case 3:
    return vec2js((HMM_Vec2)cpBodyGetVelocity(go->body));

  case 4:
    return JS_NewFloat64(js, cpBodyGetAngularVelocity(go->body));

  case 5:
    return JS_NewFloat64(js, cpBodyGetMass(go->body));

  case 6:
    return JS_NewFloat64(js, cpBodyGetMoment(go->body));

  case 7:
    return JS_NewBool(js, phys2d_in_air(go->body));

  case 8:
    gameobject_free(go);
    break;
  }

  return JS_UNDEFINED;
}

JSValue duk_make_sprite(JSContext *js, JSValueConst this, int argc, JSValueConst *argv) {
  JSValue sprite = JS_NewObject(js);
  js_setprop_str(sprite,"id",JS_NewInt64(js, make_sprite(js2gameobject(argv[0]))));
  return sprite;
}

JSValue duk_make_circle2d(JSContext *js, JSValueConst this, int argc, JSValueConst *argv) {
  gameobject *go = js2gameobject(argv[0]);

  struct phys2d_circle *circle = Make2DCircle(go);

  JSValue circleval = JS_NewObject(js);
  js_setprop_str(circleval, "id", ptr2js(circle));
  js_setprop_str(circleval, "shape", ptr2js(&circle->shape));
  return circleval;
}

JSValue duk_make_model(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
  gameobject *go = js2gameobject(argv[0]);
  struct drawmodel *dm = make_drawmodel(go);
  JSValue ret = JS_NewObject(js);
  js_setprop_str(ret, "id", ptr2js(dm));
  return ret;
}

JSValue duk_cmd_circle2d(JSContext *js, JSValueConst this, int argc, JSValueConst *argv) {
  int cmd = js2int(argv[0]);
  struct phys2d_circle *circle = js2ptr(argv[1]);

  if (!circle) return JS_UNDEFINED;

  switch (cmd) {
  case 0:
    circle->radius = js2number(argv[2]);
    break;

  case 1:
    circle->offset = js2vec2(argv[2]);
    break;

    case 2:
      return number2js(circle->radius);

    case 3:
      return vec2js(circle->offset);
  }
  phys2d_shape_apply(&circle->shape);

  return JS_UNDEFINED;
}

JSValue duk_make_poly2d(JSContext *js, JSValueConst this, int argc, JSValueConst *argv) {
  gameobject *go = js2gameobject(argv[0]);
  struct phys2d_poly *poly = Make2DPoly(go);
  phys2d_poly_setverts(poly, NULL);
  JSValue polyval = JS_NewObject(js);
  js_setprop_str(polyval, "id", ptr2js(poly));
  js_setprop_str(polyval, "shape", ptr2js(&poly->shape));
  return polyval;
}

JSValue duk_cmd_poly2d(JSContext *js, JSValueConst this, int argc, JSValueConst *argv) {
  int cmd = js2int(argv[0]);
  struct phys2d_poly *poly = js2ptr(argv[1]);
  HMM_Vec2 *v1 = NULL;

  if (!poly) return JS_UNDEFINED;

  switch (cmd) {
  case 0:
    v1 = js2cpvec2arr(argv[2]);
    phys2d_poly_setverts(poly, v1);
    break;
  }
  if (v1) arrfree(v1);

  return JS_UNDEFINED;
}

JSValue duk_make_edge2d(JSContext *js, JSValueConst this, int argc, JSValueConst *argv) {
  gameobject *go = js2gameobject(argv[0]);
  struct phys2d_edge *edge = Make2DEdge(go);
  HMM_Vec2 *points = js2cpvec2arr(argv[1]);
  phys2d_edge_update_verts(edge, points);
  arrfree(points);

  JSValue edgeval = JS_NewObject(js);
  js_setprop_str(edgeval, "id", ptr2js(edge));
  js_setprop_str(edgeval, "shape", ptr2js(&edge->shape));
  return edgeval;
}

#define GETSET_PAIR(ID, ENTRY, TYPE) \
JSValue ID##_set_##ENTRY (JSContext *js, JSValue this, JSValue val) { \
  js2##ID (this)->ENTRY = js2##TYPE (val); \
  return JS_UNDEFINED; \
} \
\
JSValue ID##_get_##ENTRY (JSContext *js, JSValue this) { \
  return TYPE##2js(js2##ID (this)->ENTRY); \
} \

#define GETSET_PAIR_HOOK(ID, ENTRY) \
JSValue ID##_set_##ENTRY (JSContext *js, JSValue this, JSValue val) { \
  ID *n = js2##ID (this); \
  n->ENTRY = val; \
  return JS_UNDEFINED; \
} \
\
JSValue ID##_get_##ENTRY (JSContext *js, JSValue this) { \
  ID *n = js2##ID (this); \
  return n->ENTRY; \
} \


GETSET_PAIR(warp_gravity, strength, number)
GETSET_PAIR(warp_gravity, decay, number)
GETSET_PAIR(warp_gravity, spherical, bool)
GETSET_PAIR(warp_gravity, mask, bitmask)
GETSET_PAIR(warp_gravity, planar_force, vec3)

#define MIST_CFUNC_DEF(name, length, func1) { name, JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE, JS_DEF_CFUNC, 0, .u = { .func = { length, JS_CFUNC_generic, { .generic = func1 } } } }

#define MIST_CGETSET_DEF(name, fgetter, fsetter) { name, JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE, JS_DEF_CGETSET, 0, .u = { .getset = { .get = { .getter = fgetter }, .set = { .setter = fsetter } } } }

#define CGETSET_ADD(ID, ENTRY) MIST_CGETSET_DEF(#ENTRY, ID##_get_##ENTRY, ID##_set_##ENTRY)

static const JSCFunctionListEntry js_warp_gravity_funcs [] = {
  CGETSET_ADD(warp_gravity, strength),
  CGETSET_ADD(warp_gravity, decay),
  CGETSET_ADD(warp_gravity, spherical),
  CGETSET_ADD(warp_gravity, mask),
  CGETSET_ADD(warp_gravity, planar_force),  
};

GETSET_PAIR(warp_damp, damp, vec3)

static const JSCFunctionListEntry js_warp_damp_funcs [] = {
  CGETSET_ADD(warp_damp, damp)
};

GETSET_PAIR(emitter, life, number)
GETSET_PAIR(emitter, life_var, number)
GETSET_PAIR(emitter, speed, number)
GETSET_PAIR(emitter, variation, number)
GETSET_PAIR(emitter, divergence, number)
GETSET_PAIR(emitter, scale, number)
GETSET_PAIR(emitter, scale_var, number)
GETSET_PAIR(emitter, grow_for, number)
GETSET_PAIR(emitter, shrink_for, number)
GETSET_PAIR(emitter, max, number)
GETSET_PAIR(emitter, explosiveness, number)
GETSET_PAIR(emitter, go, gameobject)
GETSET_PAIR(emitter, bounce, number)
GETSET_PAIR(emitter, collision_mask, bitmask)
GETSET_PAIR(emitter, die_after_collision, bool)
GETSET_PAIR(emitter, persist, number)
GETSET_PAIR(emitter, persist_var, number)
GETSET_PAIR(emitter, warp_mask, bitmask)

JSValue js_emitter_start (JSContext *js, JSValue this)
{
  emitter *n = js2emitter(this);
  start_emitter(n);
  return JS_UNDEFINED;
}

JSValue js_emitter_stop(JSContext *js, JSValue this)
{
  emitter *n = js2emitter(this);
  stop_emitter(n);
  return JS_UNDEFINED;
}

JSValue js_emitter_emit(JSContext *js, JSValueConst this, int argc, JSValue *argv)
{
  emitter *n = js2emitter(this);
  emitter_emit(n, js2number(argv[0]));
  return JS_UNDEFINED;
}

JSValue js_os_cwd(JSContext *js, JSValueConst this)
{
  char cwd[PATH_MAX];
  getcwd(cwd, sizeof(cwd));
  return str2js(cwd);
}

JSValue js_os_env(JSContext *js, JSValueConst this, int argc, JSValue *argv)
{
  char *str = js2str(argv[0]);
  JSValue ret = JS_UNDEFINED;
  if (getenv(str)) ret = str2js(getenv(str));
  JS_FreeCString(js,str);
  return ret;
}

static const JSCFunctionListEntry js_os_funcs[] = {
  MIST_CFUNC_DEF("cwd", 0, js_os_cwd),
  MIST_CFUNC_DEF("env", 1, js_os_env),
};

JSValue js_io_exists(JSContext *js, JSValueConst this, int argc, JSValue *argv)
{
  char *file = JS_ToCString(js, argv[0]);
  JSValue ret = JS_NewBool(js, fexists(file));
  JS_FreeCString(js,file);
  return ret;
}

JSValue js_io_ls(JSContext *js, JSValueConst this)
{
  return strarr2js(ls(","));
}

JSValue js_io_cp(JSContext *js, JSValueConst this, int argc, JSValue *argv)
{
  char *f1, f2;
  f1 = JS_ToCString(js, argv[0]);
  f2 = JS_ToCString(js, argv[1]);
  JSValue ret = int2js(cp(f1,f2));
  JS_FreeCString(js,f1);
  JS_FreeCString(js,f2);
  return ret;
}

JSValue js_io_mv(JSContext *js, JSValueConst this, int argc, JSValue *argv)
{
  char *f1, f2;
  f1 = JS_ToCString(js, argv[0]);
  f2 = JS_ToCString(js, argv[1]);
  JSValue ret = int2js(rename(f1,f2));
  JS_FreeCString(js,f1);
  JS_FreeCString(js,f2);
  return ret;
}

JSValue js_io_rm(JSContext *js, JSValueConst this, int argc, JSValue *argv)
{
  char *file = JS_ToCString(js, argv[0]);
  JS_FreeCString(js,file);
  return JS_UNDEFINED;
}

JSValue js_io_mkdir(JSContext *js, JSValueConst this, int argc, JSValue *argv)
{
  char *f = js2str(argv[0]);
  JSValue ret = int2js(mkdir(f,0777));
  JS_FreeCString(js,f);
  return ret;
}

JSValue js_io_slurpbytes(JSContext *js, JSValueConst this, int argc, JSValue *argv)
{
  char *f = js2str(argv[0]);
  size_t len;
  char *d = slurp_file(f,&len);
  JSValue ret = JS_NewArrayBufferCopy(js,d,len);
  JS_FreeCString(js,f);
  free(d);
  return ret;
}

JSValue js_io_slurp(JSContext *js, JSValueConst this, int argc, JSValue *argv)
{
  char *f = js2str(argv[0]);
  size_t len;
  char *s = slurp_text(f,&len);
  JS_FreeCString(js,f);

  if (!s) return JS_UNDEFINED;
  
  JSValue ret = JS_NewStringLen(js, s, len);
  free(s);
  return ret;
}

JSValue js_io_slurpwrite(JSContext *js, JSValueConst this, int argc, JSValue *argv)
{
  char *f = js2str(argv[0]);
  size_t len;
  char *data;
  JSValue ret;
  if (JS_IsString(argv[1])) {
    data = JS_ToCStringLen(js, &len, argv[1]);
    ret = int2js(slurp_write(data, f, len));
    JS_FreeCString(js,data);
  } else {
    data = JS_GetArrayBuffer(js, &len, argv[1]);
    ret = int2js(slurp_write(data, f, len));
  }

  return ret;
}

JSValue js_io_chmod(JSContext *js, JSValueConst this, int argc, JSValue *argv)
{
  char *f = js2str(argv[0]);
  int mod = js2int(argv[1]);
  chmod(f, mod);
  return JS_UNDEFINED;
}

static const JSCFunctionListEntry js_io_funcs[] = {
  MIST_CFUNC_DEF("exists", 1, js_io_exists),
  MIST_CFUNC_DEF("ls", 0, js_io_ls),
  MIST_CFUNC_DEF("cp", 2, js_io_cp),
  MIST_CFUNC_DEF("mv", 2, js_io_mv),
  MIST_CFUNC_DEF("rm", 1, js_io_rm),
  MIST_CFUNC_DEF("mkdir", 1, js_io_mkdir),
  MIST_CFUNC_DEF("chmod", 2, js_io_chmod),
  MIST_CFUNC_DEF("slurp", 1, js_io_slurp),
  MIST_CFUNC_DEF("slurpbytes", 1, js_io_slurpbytes),
  MIST_CFUNC_DEF("slurpwrite", 2, js_io_slurpwrite),
};

static const JSCFunctionListEntry js_emitter_funcs[] = {
  CGETSET_ADD(emitter, life),
  CGETSET_ADD(emitter, life_var),
  CGETSET_ADD(emitter, speed),
  CGETSET_ADD(emitter, variation),
  CGETSET_ADD(emitter, divergence),
  CGETSET_ADD(emitter, scale),
  CGETSET_ADD(emitter, scale_var),
  CGETSET_ADD(emitter, grow_for),
  CGETSET_ADD(emitter, shrink_for),
  CGETSET_ADD(emitter, max),
  CGETSET_ADD(emitter, explosiveness),
  CGETSET_ADD(emitter, go),
  CGETSET_ADD(emitter, bounce),
  CGETSET_ADD(emitter, collision_mask),
  CGETSET_ADD(emitter, die_after_collision),
  CGETSET_ADD(emitter, persist),
  CGETSET_ADD(emitter, persist_var),
  CGETSET_ADD(emitter, warp_mask),  
  MIST_CFUNC_DEF("start", 0, js_emitter_start),
  MIST_CFUNC_DEF("stop", 0, js_emitter_stop),
  MIST_CFUNC_DEF("emit", 1, js_emitter_emit)
};

GETSET_PAIR(dsp_node, pass, bool)
GETSET_PAIR(dsp_node, off, bool)
GETSET_PAIR(dsp_node, gain, number)
GETSET_PAIR(dsp_node, pan, number)

JSValue js_dsp_node_plugin(JSContext *js, JSValueConst this, int argc, JSValue *argv)
{
  plugin_node(js2dsp_node(this), js2dsp_node(argv[0]));
  return JS_UNDEFINED;
}

JSValue js_dsp_node_unplug(JSContext *js, JSValueConst this)
{
  unplug_node(js2dsp_node(this));
  return JS_UNDEFINED;
}

static const JSCFunctionListEntry js_dsp_node_funcs[] = {
  CGETSET_ADD(dsp_node, pass),
  CGETSET_ADD(dsp_node, off),
  CGETSET_ADD(dsp_node, gain),
  CGETSET_ADD(dsp_node, pan),
  MIST_CFUNC_DEF("plugin", 1, js_dsp_node_plugin),
  MIST_CFUNC_DEF("unplug", 0, js_dsp_node_unplug)
};

GETSET_PAIR(sound, loop, bool)
GETSET_PAIR(sound, timescale, number)
GETSET_PAIR(sound, frame, number)
GETSET_PAIR_HOOK(sound, hook)

static const JSCFunctionListEntry js_sound_funcs[] = {
  CGETSET_ADD(sound, loop),
  CGETSET_ADD(sound, timescale),
  CGETSET_ADD(sound, frame),
  CGETSET_ADD(sound, hook)
};

JSValue constraint_set_max_force (JSContext *js, JSValue this, JSValue val) {
  cpConstraintSetMaxForce(js2constraint(this)->c, js2number(val));
  return JS_UNDEFINED;
}

JSValue constraint_get_max_force(JSContext *js, JSValue this) { return number2js(cpConstraintGetMaxForce(js2constraint(this)->c));
}

JSValue constraint_set_collide (JSContext *js, JSValue this, JSValue val) {
  cpConstraintSetCollideBodies(js2constraint(this)->c, js2bool(val));
  return JS_UNDEFINED;
}

JSValue constraint_get_collide(JSContext *js, JSValue this) { return bool2js(cpConstraintGetCollideBodies(js2constraint(this)->c));
}

static const JSCFunctionListEntry js_constraint_funcs[] = {
  CGETSET_ADD(constraint, max_force),
  CGETSET_ADD(constraint, collide),
};
     
JSValue duk_cmd_edge2d(JSContext *js, JSValueConst this, int argc, JSValueConst *argv) {
  int cmd = js2int(argv[0]);
  struct phys2d_edge *edge = js2ptr(argv[1]);

  if (!edge) {
    YughError("Attempted to do a cmd on edge %p. Not found.", edge);
    return JS_UNDEFINED;
  }

  HMM_Vec2 *v1 = NULL;
  
  switch (cmd) {
  case 0:
    v1 = js2cpvec2arr(argv[2]);
    phys2d_edge_update_verts(edge,v1);
    break;

  case 1:
    edge->thickness = js2number(argv[2]);
    break;
  }
  if (v1) arrfree(v1);

  return JS_UNDEFINED;
}

JSValue duk_inflate_cpv(JSContext *js, JSValueConst this, int argc, JSValueConst *argv) {
  HMM_Vec2 *points = js2cpvec2arr(argv[0]);
  int n = js2int(argv[1]);
  double d = js2number(argv[2]);
  HMM_Vec2 *infl = inflatepoints(points,d,n);
  JSValue arr = vecarr2js(infl,arrlen(infl));
  
  arrfree(infl);
  arrfree(points);
  return arr;
}

JSValue duk_cmd_points(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
  int n = js_arrlen(argv[1]);
  HMM_Vec2 points[n];

  for (int i = 0; i < n; i++)
    points[i] = js2vec2(js_arridx(argv[1], i));

  int cmd = js2int(argv[0]);

  switch(cmd) {
    case 0:
      draw_poly(points, n, js2color(argv[2]));
      break;
  }

  return JS_UNDEFINED;
}

const char *STRTEST = "TEST STRING";

JSValue duk_performance_js2num(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
  
}

JSValue duk_performance(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
  int cmd = js2int(argv[0]);
  switch(cmd) {
    case 0:
    break;
    case 1:
    js2number(argv[1]);
    break;
    case 2:
    js2cpvec2arr(argv[1]);
    break;
    case 3:
    return number2js(1.0);
    case 4:
    js2str(argv[1]);
    break;
    case 5:
    jsfloat2vec(argv[1]);
    break;
    case 6:
    return JS_NewStringLen(js, STRTEST, sizeof(*STRTEST));
    case 7:
      for (int i = 0; i < js2number(argv[2]); i++)
        script_call_sym(argv[1]);
      script_call_sym(argv[3]);
      break;
  }
  return JS_UNDEFINED;
}

JSValue nota_encode(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
  if (argc < 1) return JS_UNDEFINED;
  
  JSValue obj = argv[0];
  char nota[1024*1024]; // 1MB
  char *e = js_nota_encode(obj, nota);

  return JS_NewArrayBufferCopy(js, nota, e-nota);
}

JSValue nota_decode(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
  if (argc < 1) return JS_UNDEFINED;

  size_t len;
  char *nota = JS_GetArrayBuffer(js, &len, argv[0]);
  JSValue ret;
  js_nota_decode(&ret, nota);
  return ret;
}

static const JSCFunctionListEntry nota_funcs[] = {
  MIST_CFUNC_DEF("encode", 1, nota_encode),
  MIST_CFUNC_DEF("decode", 1, nota_decode)
};

#define DUK_FUNC(NAME, ARGS) JS_SetPropertyStr(js, globalThis, #NAME, JS_NewCFunction(js, duk_##NAME, #NAME, ARGS));

void ffi_load() {
  globalThis = JS_GetGlobalObject(js);

  JSValue nota = JS_NewObject(js);
  JS_SetPropertyFunctionList(js, nota, nota_funcs, countof(nota_funcs));
  JS_SetPropertyStr(js, globalThis, "nota", nota);

  DUK_FUNC(yughlog, 4)
  DUK_FUNC(make_gameobject, 0)
  DUK_FUNC(set_body, 3)
  DUK_FUNC(q_body, 2)
  DUK_FUNC(sys_cmd, 1)
  DUK_FUNC(make_sprite, 1)
  DUK_FUNC(spline_cmd, 6)
  DUK_FUNC(make_circle2d, 1)
  DUK_FUNC(cmd_circle2d, 6)
  DUK_FUNC(make_poly2d, 1)
  DUK_FUNC(cmd_poly2d, 6)
  DUK_FUNC(make_edge2d, 3)
  DUK_FUNC(cmd_edge2d, 6)
  DUK_FUNC(make_model,2);
  DUK_FUNC(cmd_points, 5);
  DUK_FUNC(cmd, 6)
  DUK_FUNC(register, 3)
  DUK_FUNC(register_collide, 6)

  DUK_FUNC(ui_text, 8)
  DUK_FUNC(gui_img, 10)

  DUK_FUNC(inflate_cpv, 3)

  DUK_FUNC(performance, 2)

  JS_FreeValue(js,globalThis);
  
  QJSCLASSPREP(ptr);
  QJSCLASSPREP(gameobject);
  QJSCLASSPREP_FUNCS(dsp_node);
  
  sound_proto = JS_NewObject(js);
  JS_SetPropertyFunctionList(js, sound_proto, js_sound_funcs, countof(js_sound_funcs));
  JS_SetPrototype(js, sound_proto, dsp_node_proto);
  
  QJSCLASSPREP_FUNCS(emitter);
  QJSCLASSPREP_FUNCS(warp_gravity);
  QJSCLASSPREP_FUNCS(warp_damp);
  
  QJSCLASSPREP_FUNCS(constraint);

  QJSGLOBALCLASS(os);
  QJSGLOBALCLASS(io);
}

void ffi_stop()
{
  JS_FreeValue(js, sound_proto);
}
