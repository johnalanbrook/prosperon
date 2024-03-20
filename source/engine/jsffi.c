#include "jsffi.h"

#include "script.h"

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

#include <time.h>
#include <sys/time.h>
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

JSValue str2js(const char *c) { return JS_NewString(js, c); }
const char *js2str(JSValue v) {
  return JS_ToCString(js, v);
}

#define QJSCLASS(TYPE)\
static JSClassID js_ ## TYPE ## _id;\
static void js_##TYPE##_finalizer(JSRuntime *rt, JSValue val){\
TYPE *n = JS_GetOpaque(val, js_##TYPE##_id);\
YughSpam("Freeing " #TYPE " at %p", n); \
TYPE##_free(n);}\
static JSClassDef js_##TYPE##_class = {\
  #TYPE,\
  .finalizer = js_##TYPE##_finalizer,\
};\
static TYPE *js2##TYPE (JSValue val) { return JS_GetOpaque(val,js_##TYPE##_id); }\
static JSValue TYPE##2js(TYPE *n) { \
  JSValue j = JS_NewObjectClass(js,js_##TYPE##_id);\
  YughSpam("Created " #TYPE " at %p", n); \
  JS_SetOpaque(j,n);\
  return j; }\

QJSCLASS(gameobject)
QJSCLASS(emitter)
QJSCLASS(dsp_node)
QJSCLASS(texture)
QJSCLASS(sprite)
QJSCLASS(warp_gravity)
QJSCLASS(warp_damp)
QJSCLASS(material)
QJSCLASS(mesh)
QJSCLASS(window)
QJSCLASS(drawmodel)

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

/* Defines a class and uses its function list as its prototype */
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

int js2bool(JSValue v) { return JS_ToBool(js, v); }
JSValue bool2js(int b) { return JS_NewBool(js,b); }

void js_setprop_num(JSValue obj, uint32_t i, JSValue v) { JS_SetPropertyUint32(js, obj, i, v); }

JSValue js_getpropidx(JSValue v, uint32_t i)
{
  JSValue p = JS_GetPropertyUint32(js, v, i);
  JS_FreeValue(js,p);
  return p;
}

JSValue gos2ref(gameobject **go)
{
  JSValue array = JS_NewArray(js);
  for (int i = 0; i < arrlen(go); i++)
    js_setprop_num(array,i,JS_DupValue(js,go[i]->ref));
  return array;
}

void js_setprop_str(JSValue obj, const char *prop, JSValue v) { JS_SetPropertyStr(js, obj, prop, v); }
JSValue js_getpropstr(JSValue v, const char *str)
{
  JSValue p = JS_GetPropertyStr(js,v,str);
  JS_FreeValue(js,p);
  return p;
}

static inline cpBody *js2body(JSValue v) { return js2gameobject(v)->body; }


JSValue strarr2js(char **c)
{
  JSValue arr = JS_NewArray(js);
  for (int i = 0; i < arrlen(c); i++)
    js_setprop_num(arr,i,JS_NewString(js, c[i]));

  return arr;
}

JSValue number2js(double g) { return JS_NewFloat64(js,g); }
double js2number(JSValue v) {
  double g;
  JS_ToFloat64(js, &g, v);
  return g;
}

void *js2ptr(JSValue v) { return JS_GetOpaque(v,js_ptr_id); }

JSValue ptr2js(void *ptr) {
  JSValue obj = JS_NewObjectClass(js, js_ptr_id);
  JS_SetOpaque(obj, ptr);
  return obj;
}

int js_arrlen(JSValue v) {
  int len;
  JS_ToInt32(js, &len, js_getpropstr( v, "length"));
  return len;
}

char *js_do_nota_decode(JSValue *tmp, char *nota)
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
        nota = js_do_nota_decode(&ret2, nota);
        JS_SetPropertyInt64(js, *tmp, i, ret2);
      }
      break;
    case NOTA_REC:
      nota = nota_read_record(&n, nota);
      *tmp = JS_NewObject(js);
      for (int i = 0; i < n; i++) {
        nota = nota_read_text(&str, nota);
	nota = js_do_nota_decode(&ret2, nota);
	JS_SetPropertyStr(js, *tmp, str, ret2);
	free(str);
      }
      break;
    case NOTA_INT:
      nota = nota_read_int(&n, nota);
      *tmp = number2js(n);
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

char *js_do_nota_encode(JSValue v, char *nota)
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
          nota = js_do_nota_encode(js_getpropidx(v, i), nota);
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
	
        nota = js_do_nota_encode(val, nota);
	      JS_FreeValue(js,val);
      }
      js_free(js, ptab);
      return nota;
    default:
      return nota;
  }
  return nota;
}

struct rgba js2color(JSValue v) {
  JSValue c[4];
  for (int i = 0; i < 4; i++)
    c[i] = js_getpropidx(v,i);
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
  js_setprop_num(arr,0,number2js((double)color.r/255));
  js_setprop_num(arr,1,number2js((double)color.g/255));  
  js_setprop_num(arr,2,number2js((double)color.b/255));
  js_setprop_num(arr,3,number2js((double)color.a/255));
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

cpVect js2cvec2(JSValue v) { return js2vec2(v).cp; }
JSValue cvec22js(cpVect v) { return vec22js((HMM_Vec2)v); }

HMM_Vec2 js2vec2(JSValue v)
{
  HMM_Vec2 v2;
  v2.X = js2number(js_getpropidx(v,0));
  v2.Y = js2number(js_getpropidx(v,1));
  return v2;
}

JSValue vec22js(HMM_Vec2 v)
{
  JSValue array = JS_NewArray(js);
  js_setprop_num(array,0,number2js(v.x));
  js_setprop_num(array,1,number2js(v.y));
  return array;
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
  return arr;
}

JSValue bitmask2js(cpBitmask mask) {
  JSValue arr = JS_NewArray(js);
  for (int i = 0; i < 11; i++)
    js_setprop_num(arr,i,JS_NewBool(js,mask & 1 << i));

  return arr;
}

JSValue vecarr2js(HMM_Vec2 *points, int n) {
  JSValue array = JS_NewArray(js);
  for (int i = 0; i < n; i++)
    js_setprop_num(array,i,vec22js(points[i]));
    
  return array;
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
  js_setprop_str(obj, "x", number2js(rect.x));
  js_setprop_str(obj, "y", number2js(rect.y));
  js_setprop_str(obj, "w", number2js(rect.w));
  js_setprop_str(obj, "h", number2js(rect.h));
  return obj;
}

JSValue bb2js(struct boundingbox bb)
{
  JSValue obj = JS_NewObject(js);
  js_setprop_str(obj,"t", number2js(bb.t));
  js_setprop_str(obj,"b", number2js(bb.b));
  js_setprop_str(obj,"r", number2js(bb.r));
  js_setprop_str(obj,"l", number2js(bb.l));
  return obj;
}

JSValue ints2js(int *ints) {
  JSValue arr = JS_NewArray(js);
  for (int i = 0; i < arrlen(ints); i++)
    js_setprop_num(arr,i, number2js(ints[i]));

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

void gameobject_add_shape_collider(gameobject *go, JSValue fn, struct phys2d_shape *shape) {
  struct shape_cb shapecb;
  shapecb.shape = shape;
  shapecb.cbs.begin = fn;
  arrpush(go->shape_cbs, shapecb);
}

JSValue duk_register_collide(JSContext *js, JSValue this, int argc, JSValue *argv) {
  int cmd = js2number(argv[0]);
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

JSValue duk_make_circle2d(JSContext *js, JSValue this, int argc, JSValue *argv) {
  gameobject *go = js2gameobject(argv[0]);

  struct phys2d_circle *circle = Make2DCircle(go);

  JSValue circleval = JS_NewObject(js);
  js_setprop_str(circleval, "id", ptr2js(circle));
  js_setprop_str(circleval, "shape", ptr2js(&circle->shape));
  return circleval;
}

JSValue duk_make_model(JSContext *js, JSValue this, int argc, JSValue *argv)
{
  gameobject *go = js2gameobject(argv[0]);
  struct drawmodel *dm = make_drawmodel(go);
  JSValue ret = JS_NewObject(js);
  js_setprop_str(ret, "id", ptr2js(dm));
  return ret;
}

JSValue duk_cmd_circle2d(JSContext *js, JSValue this, int argc, JSValue *argv) {
  int cmd = js2number(argv[0]);
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
      return vec22js(circle->offset);
  }
  phys2d_shape_apply(&circle->shape);

  return JS_UNDEFINED;
}

JSValue duk_make_poly2d(JSContext *js, JSValue this, int argc, JSValue *argv) {
  gameobject *go = js2gameobject(argv[0]);
  struct phys2d_poly *poly = Make2DPoly(go);
  phys2d_poly_setverts(poly, NULL);
  JSValue polyval = JS_NewObject(js);
  js_setprop_str(polyval, "id", ptr2js(poly));
  js_setprop_str(polyval, "shape", ptr2js(&poly->shape));
  return polyval;
}

JSValue duk_cmd_poly2d(JSContext *js, JSValue this, int argc, JSValue *argv) {
  int cmd = js2number(argv[0]);
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

JSValue duk_make_edge2d(JSContext *js, JSValue this, int argc, JSValue *argv) {
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

#define GETSET_PAIR_APPLY(ID, ENTRY, TYPE) \
JSValue ID##_set_##ENTRY (JSContext *js, JSValue this, JSValue val) { \
  ID *id = js2##ID (this); \
  id->ENTRY = js2##TYPE (val); \
  ID##_apply(id); \
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


#define DEF_FN_STR(NAME) JSValue js_##NAME (JSContext *js, JSValue this, int argc, JSValue *argv) { \
  char *str = js2str(argv[0]); \
  NAME (str); \
  JS_FreeCString(js,str); \
  return JS_UNDEFINED; \
} \

GETSET_PAIR(warp_gravity, strength, number)
GETSET_PAIR(warp_gravity, decay, number)
GETSET_PAIR(warp_gravity, spherical, bool)
GETSET_PAIR(warp_gravity, mask, bitmask)
GETSET_PAIR(warp_gravity, planar_force, vec3)

#define MIST_CFUNC_DEF(name, length, func1) { name, JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE, JS_DEF_CFUNC, 0, .u = { .func = { length, JS_CFUNC_generic, { .generic = func1 } } } }

#define MIST_FUNC_DEF(TYPE, FN, LEN) MIST_CFUNC_DEF(#FN, LEN, js_##TYPE##_##FN)

#define MIST_FN_STR(NAME) MIST_CFUNC_DEF(#NAME, 1, js_##NAME)

#define MIST_CGETSET_DEF(name, fgetter, fsetter) { name, JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE, JS_DEF_CGETSET, 0, .u = { .getset = { .get = { .getter = fgetter }, .set = { .setter = fsetter } } } }

#define CGETSET_ADD(ID, ENTRY) MIST_CGETSET_DEF(#ENTRY, ID##_get_##ENTRY, ID##_set_##ENTRY)

#define GGETSET_ADD(ENTRY)

#define JSC_CCALL(NAME, FN) JSValue js_##NAME (JSContext *js, JSValue this, int argc, JSValue *argv) { \
  {FN;} \
  return JS_UNDEFINED; \
} \

#define JSC_RET(NAME, FN) JSValue js_##NAME (JSContext *js, JSValue this, int argc, JSValue *argv) { \
  FN; } \

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

JSC_CCALL(drawmodel_draw, draw_drawmodel(js2drawmodel(this)))
JSC_CCALL(drawmodel_path,
  char *s = js2str(argv[0]);
  js2drawmodel(this)->model = GetExistingModel(s);
  JS_FreeCString(js,s);
)

static const JSCFunctionListEntry js_drawmodel_funcs[] = {
  MIST_FUNC_DEF(drawmodel, draw, 0),
  MIST_FUNC_DEF(drawmodel, path, 1)
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
GETSET_PAIR(emitter, texture, texture)

JSValue js_emitter_start (JSContext *js, JSValue this, int argc, JSValue *argv)
{
  emitter *n = js2emitter(this);
  start_emitter(n);
  return JS_UNDEFINED;
}

JSValue js_emitter_stop(JSContext *js, JSValue this, int argc, JSValue *argv)
{
  emitter *n = js2emitter(this);
  stop_emitter(n);
  return JS_UNDEFINED;
}

JSValue js_emitter_emit(JSContext *js, JSValue this, int argc, JSValue *argv)
{
  emitter *n = js2emitter(this);
  emitter_emit(n, js2number(argv[0]));
  return JS_UNDEFINED;
}

JSValue js_os_cwd(JSContext *js, JSValue this, int argc, JSValue *argv)
{
  char cwd[PATH_MAX];
  #ifndef __EMSCRIPTEN__
  getcwd(cwd, sizeof(cwd));
  #else
  cwd[0] = '.';
  cwd[1] = 0;
  #endif
  return str2js(cwd);
}

JSValue js_os_env(JSContext *js, JSValue this, int argc, JSValue *argv)
{
  char *str = js2str(argv[0]);
  JSValue ret = JS_UNDEFINED;
  if (getenv(str)) ret = str2js(getenv(str));
  JS_FreeCString(js,str);
  return ret;
}

JSValue js_os_sys(JSContext *js, JSValue this, int argc, JSValue *argv)
{
  #ifdef __linux__
  return str2js("linux");
  #elif defined(_WIN32) || defined(_WIN64)
  return str2js("windows");
  #elif defined(__APPLE__)
  return str2js("macos");
  #endif
  return JS_UNDEFINED;
}

JSC_CCALL(os_quit, quit();)
JSC_CCALL(os_reindex_static, cpSpaceReindexStatic(space));
JSC_CCALL(os_gc, script_gc());
JSC_CCALL(os_eval_env,
  char *s1 = js2str(argv[0]);
  char *s2 = js2str(argv[2]);
  JSValue ret = eval_script_env(s1, argv[1], s2);
  JS_FreeCString(js,s1);
  JS_FreeCString(js,s2);
  return ret;
)

#define RETUN return JS_UNDEFINED

JSValue js_os_capture(JSContext *js, JSValue this, int argc, JSValue *argv) {
  char *str = js2str(argv[0]);
  capture_screen(js2number(argv[1]), js2number(argv[2]), js2number(argv[4]), js2number(argv[5]), str);
  JS_FreeCString(js,str);
  RETUN;
}

JSC_CCALL(os_sprite,
  if (js2bool(argv[0])) return JS_GetClassProto(js,js_sprite_id);
  return sprite2js(sprite_make());
)

static const JSCFunctionListEntry js_os_funcs[] = {
  MIST_FUNC_DEF(os,sprite,1),
  MIST_FUNC_DEF(os, cwd, 0),
  MIST_FUNC_DEF(os, env, 1),
  MIST_FUNC_DEF(os, sys, 0),
  MIST_FUNC_DEF(os, quit, 0),
  MIST_FUNC_DEF(os, reindex_static, 0),
  MIST_FUNC_DEF(os, gc, 0),
  MIST_FUNC_DEF(os, capture, 5),
  MIST_FUNC_DEF(os, eval_env, 3),
};

JSC_CCALL(render_normal, opengl_rendermode(LIT))
JSC_CCALL(render_wireframe, opengl_rendermode(WIREFRAME))
JSC_CCALL(render_grid, draw_grid(js2number(argv[0]), js2number(argv[1]), js2color(argv[2]));)
JSC_CCALL(render_point, draw_cppoint(js2vec2(argv[0]), js2number(argv[1]), js2color(argv[2])))
JSC_CCALL(render_circle, draw_circle(js2vec2(argv[0]), js2number(argv[1]), js2number(argv[2]), js2color(argv[3]), -1);)
JSC_CCALL(render_poly, 
  int n = js_arrlen(argv[0]);
  HMM_Vec2 points[n];
  for (int i = 0; i < n; i++)
    points[i] = js2vec2(js_getpropidx(argv[0], i));
  draw_poly(points, n, js2color(argv[1]));
)

JSC_CCALL(render_line, 
  void *v1 = js2cpvec2arr(argv[0]);
  draw_edge(v1, js_arrlen(argv[0]), js2color(argv[1]), js2number(argv[2]), 0, js2color(argv[1]), 10);
)

JSC_CCALL(render_sprites, sprite_draw_all())
JSC_CCALL(render_models, model_draw_all())
JSC_CCALL(render_emitters, emitters_draw())
JSC_CCALL(render_flush, debug_flush(&projection); text_flush(&projection))
JSC_CCALL(render_flush_hud, debug_flush(&hudproj); debug_flush(&hudproj); sprite_flush();)
JSC_CCALL(render_pass, debug_nextpass())
JSC_CCALL(render_end_pass, sg_end_pass())
JSC_CCALL(render_commit, sg_commit(); debug_newframe();)
JSC_CCALL(render_text_size,
  char *s = js2str(argv[0]);
  JSValue ret = bb2js(text_bb(s, js2number(argv[1]), js2number(argv[2]), 1));
  JS_FreeCString(js,s);
  return ret;
)
JSC_CCALL(render_gif_times,
  char *s = js2str(argv[0]);
  JSValue ret = ints2js(gif_delays(s));
  JS_FreeCString(js,s);
  return ret;
)

JSC_CCALL(render_gif_frames,
  char *s = js2str(argv[0]);
  JSValue ret = number2js(gif_nframes(s));
  JS_FreeCString(js,s);
  return ret;
)

JSC_CCALL(render_cam_body, set_cam_body(js2gameobject(argv[0])->body))
JSC_CCALL(render_add_zoom, add_zoom(js2number(argv[0])))
JSC_CCALL(render_get_zoom, return number2js(cam_zoom()))
JSC_CCALL(render_world2screen, return vec22js(world2screen(js2vec2(argv[0]))))
JSC_CCALL(render_screen2world, return vec22js(screen2world(js2vec2(argv[0]))))

static const JSCFunctionListEntry js_render_funcs[] = {
  MIST_FUNC_DEF(render,cam_body, 1),
  MIST_FUNC_DEF(render,add_zoom,1),
  MIST_FUNC_DEF(render,get_zoom,0),
  MIST_FUNC_DEF(render,world2screen,1),
  MIST_FUNC_DEF(render,screen2world,1),
  MIST_FUNC_DEF(render,gif_frames,1),
  MIST_FUNC_DEF(render, gif_times, 1),
  MIST_FUNC_DEF(render, normal, 0),
  MIST_FUNC_DEF(render, wireframe, 0),
  MIST_FUNC_DEF(render, grid, 3),
  MIST_FUNC_DEF(render, point, 3),
  MIST_FUNC_DEF(render, circle, 3),
  MIST_FUNC_DEF(render, poly, 2),
  MIST_FUNC_DEF(render, line, 3),
  MIST_FUNC_DEF(render, sprites, 0),
  MIST_FUNC_DEF(render, models, 0),
  MIST_FUNC_DEF(render, emitters, 0),
  MIST_FUNC_DEF(render, flush, 0),
  MIST_FUNC_DEF(render, flush_hud, 0),
  MIST_FUNC_DEF(render, pass, 0),
  MIST_FUNC_DEF(render, end_pass, 0),
  MIST_FUNC_DEF(render, commit, 0),
  MIST_FUNC_DEF(render, text_size, 3)
};

JSC_CCALL(gui_flush, text_flush(&hudproj));
JSC_CCALL(gui_scissor, sg_apply_scissor_rectf(js2number(argv[0]), js2number(argv[1]), js2number(argv[2]), js2number(argv[3]), 0))
JSC_CCALL(gui_text,
  const char *s = JS_ToCString(js, argv[0]);
  HMM_Vec2 pos = js2vec2(argv[1]);

  float size = js2number(argv[2]);
  struct rgba c = js2color(argv[3]);
  int wrap = js2number(argv[4]);
  int cursor = js2number(argv[5]);
  JSValue ret = JS_NewInt64(js, renderText(s, pos, size, c, wrap, cursor, 1.0));
  JS_FreeCString(js, s);
  return ret;
)

JSC_CCALL(gui_img,
  const char *img = JS_ToCString(js, argv[0]);
  transform2d t;
  t.pos = js2vec2(argv[1]);
  t.scale = js2vec2(argv[2]);
  t.angle = js2number(argv[3]);
  gui_draw_img(img, t, js2bool(argv[4]), js2vec2(argv[5]), 1.0, js2color(argv[6]));
  JS_FreeCString(js, img);
)


DEF_FN_STR(font_set)

static const JSCFunctionListEntry js_gui_funcs[] = {
  MIST_FUNC_DEF(gui, flush, 0),
  MIST_FUNC_DEF(gui, scissor, 4),
  MIST_FUNC_DEF(gui, text, 6),
  MIST_FUNC_DEF(gui, img, 7),
  MIST_FN_STR(font_set),
};

JSC_CCALL(spline_catmull,
  HMM_Vec2 *points = js2cpvec2arr(argv[0]);
  float param = js2number(argv[1]);
  HMM_Vec2 *samples = catmull_rom_ma_v2(points,param);
  arrfree(points);
  if (!samples) return JS_UNDEFINED;
  JSValue arr = vecarr2js(samples, arrlen(samples));
  arrfree(samples);
  return arr;
)

JSC_CCALL(spline_bezier,
  HMM_Vec2 *points = js2cpvec2arr(argv[0]);
  float param = js2number(argv[1]);
  HMM_Vec2 *samples = catmull_rom_ma_v2(points,param);
  arrfree(points);
  if (!samples) return JS_UNDEFINED;
  JSValue arr = vecarr2js(samples, arrlen(samples));
  arrfree(samples);
  return arr;
)

static const JSCFunctionListEntry js_spline_funcs[] = {
  MIST_FUNC_DEF(spline, catmull, 2),
  MIST_FUNC_DEF(spline, bezier, 2)
};

JSValue js_vector_dot(JSContext *js, JSValue this, int argc, JSValue *argv) { return number2js(HMM_DotV2(js2vec2(argv[0]), js2vec2(argv[1]))) ; };

JSC_RET(vector_project, return vec22js(HMM_ProjV2(js2vec2(argv[0]), js2vec2(argv[1])));)

static const JSCFunctionListEntry js_vector_funcs[] = {
  MIST_FUNC_DEF(vector,dot,2),
  MIST_FUNC_DEF(vector,project,2),
};

JSValue js_game_engine_start(JSContext *js, JSValue this, int argc, JSValue *argv) {
  engine_start(argv[0],argv[1]);
  RETUN;
}
JSValue js_game_object_count(JSContext *js, JSValue this) { return number2js(go_count()); }

static const JSCFunctionListEntry js_game_funcs[] = {
  MIST_FUNC_DEF(game, engine_start, 2),
  MIST_FUNC_DEF(game, object_count, 0)
};

JSC_CCALL(input_show_keyboard, sapp_show_keyboard(js2bool(argv[0])))
JSValue js_input_keyboard_shown(JSContext *js, JSValue this) { return bool2js(sapp_keyboard_shown()); }
JSC_CCALL(input_mouse_mode, set_mouse_mode(js2number(argv[0])))
JSC_CCALL(input_mouse_cursor, sapp_set_mouse_cursor(js2number(argv[0])))

DEF_FN_STR(cursor_img)

static const JSCFunctionListEntry js_input_funcs[] = {
  MIST_FUNC_DEF(input, show_keyboard, 1),
  MIST_FUNC_DEF(input, keyboard_shown, 0),
  MIST_FUNC_DEF(input, mouse_mode, 1),
  MIST_FUNC_DEF(input, mouse_cursor, 1),
  MIST_FN_STR(cursor_img)
};

#define GETSET_GLOBAL(ENTRY, TYPE) \
JSValue global_set_##ENTRY (JSContext *js, JSValue this, JSValue val) { \
  ENTRY = js2##TYPE (val); \
  return JS_UNDEFINED; \
} \
\
JSValue global_get_##ENTRY (JSContext *js, JSValue this) { \
  return TYPE##2js(ENTRY); \
} \

JSC_CCALL(prosperon_emitters_step, emitters_step(js2number(argv[0])))
JSC_CCALL(prosperon_phys2d_step, phys2d_update(js2number(argv[0])))
JSC_CCALL(prosperon_window_render, openglRender(&mainwin))

static const JSCFunctionListEntry js_prosperon_funcs[] = {
  MIST_FUNC_DEF(prosperon, emitters_step, 1),
  MIST_FUNC_DEF(prosperon, phys2d_step, 1),
  MIST_FUNC_DEF(prosperon, window_render, 0)
};

JSValue js_time_now(JSContext *js, JSValue this) {
  struct timeval ct;
  gettimeofday(&ct, NULL);
  return number2js((double)ct.tv_sec+(double)(ct.tv_usec/1000000.0));
}

JSValue js_time_computer_dst(JSContext *js, JSValue this) {
  time_t t = time(NULL);
  return bool2js(localtime(&t)->tm_isdst);
}

JSValue js_time_computer_zone(JSContext *js, JSValue this) {
  time_t t = time(NULL);
  time_t local_t = mktime(localtime(&t));
  double diff = difftime(t, local_t);
  return number2js(diff/3600);
}

static const JSCFunctionListEntry js_time_funcs[] = {
  MIST_FUNC_DEF(time, now, 0),
  MIST_FUNC_DEF(time, computer_dst, 0),
  MIST_FUNC_DEF(time, computer_zone, 0)
};

JSValue js_console_print(JSContext *js, JSValue this, int argc, JSValue *argv)
{
  char *s = JS_ToCString(js,argv[0]);
  log_print(s);
  JS_FreeCString(js,s);
  return JS_UNDEFINED;
}

JSValue js_console_rec(JSContext *js, JSValue this, int argc, JSValue *argv)
{
  int level = js2number(argv[0]);
  const char *msg = JS_ToCString(js, argv[1]);
  const char *file = JS_ToCString(js, argv[2]);
  int line = js2number(argv[3]);

  mYughLog(LOG_SCRIPT, level, line, file, msg);

  JS_FreeCString(js, msg);
  JS_FreeCString(js, file);

  return JS_UNDEFINED;
}

GETSET_GLOBAL(stdout_lvl, number)

static const JSCFunctionListEntry js_console_funcs[] = {
  MIST_FUNC_DEF(console,print,1),
  MIST_FUNC_DEF(console,rec,4),
  CGETSET_ADD(global, stdout_lvl)
};

JSValue js_profile_now(JSContext *js, JSValue this) { return number2js(stm_now()); }

static const JSCFunctionListEntry js_profile_funcs[] = {
  MIST_FUNC_DEF(profile,now,0),
};

JSValue js_io_exists(JSContext *js, JSValue this, int argc, JSValue *argv)
{
  char *file = JS_ToCString(js, argv[0]);
  JSValue ret = JS_NewBool(js, fexists(file));
  JS_FreeCString(js,file);
  return ret;
}

JSValue js_io_ls(JSContext *js, JSValue this, int argc, JSValue *argv)
{
  return strarr2js(ls("."));
}

JSValue js_io_cp(JSContext *js, JSValue this, int argc, JSValue *argv)
{
  char *f1, *f2;
  f1 = JS_ToCString(js, argv[0]);
  f2 = JS_ToCString(js, argv[1]);
  JSValue ret = number2js(cp(f1,f2));
  JS_FreeCString(js,f1);
  JS_FreeCString(js,f2);
  return ret;
}

JSValue js_io_mv(JSContext *js, JSValue this, int argc, JSValue *argv)
{
  char *f1, *f2;
  f1 = JS_ToCString(js, argv[0]);
  f2 = JS_ToCString(js, argv[1]);
  JSValue ret = number2js(rename(f1,f2));
  JS_FreeCString(js,f1);
  JS_FreeCString(js,f2);
  return ret;
}

JSValue js_io_chdir(JSContext *js, JSValue this, int argc, JSValue *argv)
{
  char *path = JS_ToCString(js, argv[0]);
  JSValue ret = number2js(chdir(path));
  JS_FreeCString(js,path);
  return ret;
}

JSValue js_io_rm(JSContext *js, JSValue this, int argc, JSValue *argv)
{
  char *file = JS_ToCString(js, argv[0]);
  JSValue ret = number2js(remove(file));
  JS_FreeCString(js,file);
  return JS_UNDEFINED;
}

JSValue js_io_mkdir(JSContext *js, JSValue this, int argc, JSValue *argv)
{
  char *f = js2str(argv[0]);
  JSValue ret = number2js(mkdir(f,0777));
  JS_FreeCString(js,f);
  return ret;
}

JSValue js_io_slurpbytes(JSContext *js, JSValue this, int argc, JSValue *argv)
{
  char *f = js2str(argv[0]);
  size_t len;
  char *d = slurp_file(f,&len);
  JSValue ret = JS_NewArrayBufferCopy(js,d,len);
  JS_FreeCString(js,f);
  free(d);
  return ret;
}

JSValue js_io_slurp(JSContext *js, JSValue this, int argc, JSValue *argv)
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

JSValue js_io_slurpwrite(JSContext *js, JSValue this, int argc, JSValue *argv)
{
  char *f = js2str(argv[0]);
  size_t len;
  char *data;
  JSValue ret;
  if (JS_IsString(argv[1])) {
    data = JS_ToCStringLen(js, &len, argv[1]);
    ret = number2js(slurp_write(data, f, len));
    JS_FreeCString(js,data);
  } else {
    data = JS_GetArrayBuffer(js, &len, argv[1]);
    ret = number2js(slurp_write(data, f, len));
  }

  return ret;
}

JSValue js_io_chmod(JSContext *js, JSValue this, int argc, JSValue *argv)
{
  char *f = js2str(argv[0]);
  int mod = js2number(argv[1]);
  chmod(f, mod);
  return JS_UNDEFINED;
}

DEF_FN_STR(save_qoa)

JSValue js_io_compile(JSContext *js, JSValue this, int argc, JSValue *argv) {
  size_t len;
  char *str = js2str(argv[0]);
  void *d = script_compile(str, &len);
  JSValue ret = JS_NewArrayBufferCopy(js,d,len);
  JS_FreeCString(js,str);
  free(d);
  return ret;
}

JSValue js_io_run_bytecode(JSContext *js, JSValue this, int argc, JSValue *argv) {
  size_t len;
  char *str = js2str(argv[0]);
  void *d = slurp_file(str, &len);
  JSValue ret = script_run_bytecode(d, len);
  JS_FreeCString(js,str);
  free(d);
  return ret;
}

DEF_FN_STR(pack_engine)

static const JSCFunctionListEntry js_io_funcs[] = {
  MIST_FUNC_DEF(io, exists,1),
  MIST_FUNC_DEF(io, ls, 0),
  MIST_FUNC_DEF(io, cp, 2),
  MIST_FUNC_DEF(io, mv, 2),
  MIST_FUNC_DEF(io, rm, 1),
  MIST_FUNC_DEF(io, chdir, 1),
  MIST_FUNC_DEF(io, mkdir, 1),
  MIST_FUNC_DEF(io, chmod, 2),
  MIST_FUNC_DEF(io, slurp, 1),
  MIST_FUNC_DEF(io, slurpbytes, 1),
  MIST_FUNC_DEF(io, slurpwrite, 2),
  MIST_FN_STR(save_qoa),
  MIST_FN_STR(pack_engine)
};

JSValue js_debug_draw_gameobject(JSContext *js, JSValue this, int argc, JSValue *argv) { gameobject_draw_debug(js2gameobject(argv[0])); RETUN; }

static const JSCFunctionListEntry js_debug_funcs[] = {
  MIST_FUNC_DEF(debug, draw_gameobject, 1)
};

JSValue js_physics_sgscale(JSContext *js, JSValue this, int argc, JSValue *argv) {
  js2gameobject(argv[0])->scale.xy = js2vec2(argv[1]);
  gameobject_apply(js2gameobject(argv[0]));
  cpSpaceReindexShapesForBody(space, js2gameobject(argv[0])->body);
  RETUN;
}

JSValue js_physics_set_cat_mask(JSContext *js, JSValue this, int argc, JSValue *argv) { set_cat_mask(js2number(argv[0]), js2bitmask(argv[1])); RETUN; }

JSC_CCALL(physics_box_query,
  int *ids = phys2d_query_box(js2vec2(argv[0]), js2vec2(argv[1]));
  JSValue ret = gos2ref(ids);
  arrfree(ids);
  return ret;
)

JSC_CCALL(physics_pos_query,
  gameobject *go = pos2gameobject(js2vec2(argv[0]), js2number(argv[1]));
  JSValue ret = go ? JS_DupValue(js,go->ref) : JS_UNDEFINED;
  return ret;
)

JSC_CCALL(physics_box_point_query,
  void *v = js2cpvec2arr(argv[2]);
  int *intids = phys2d_query_box_points(js2vec2(argv[0]), js2vec2(argv[1]), v, js_arrlen(argv[2]));
  JSValue ret = ints2js(intids);
  arrfree(intids);
  return ret;
)

JSC_CCALL(physics_query_shape,
  int *ids = phys2d_query_shape(js2ptr(argv[0]));
  JSValue ret = gos2ref(ids);
  arrfree(ids);
  return ret;
)

JSC_CCALL(physics_closest_point,
  void *v1 = js2cpvec2arr(argv[1]);
  JSValue ret = number2js(point2segindex(js2vec2(argv[0]), v1, js2number(argv[2])));
  arrfree(v1);
  return ret;
)

JSC_CCALL(physics_make_gravity, return warp_gravity2js(warp_gravity_make()))
JSC_CCALL(physics_make_damp, return warp_damp2js(warp_damp_make()))
JSC_CCALL(physics_edge_thickness, return number2js(((struct phys2d_edge*)js2ptr(argv[0]))->thickness))

static const JSCFunctionListEntry js_physics_funcs[] = {
  MIST_FUNC_DEF(physics,edge_thickness,1),
  MIST_FUNC_DEF(physics, sgscale, 2),
  MIST_FUNC_DEF(physics, set_cat_mask, 2),
  MIST_FUNC_DEF(physics, box_query, 2),
  MIST_FUNC_DEF(physics, pos_query, 2),
  MIST_FUNC_DEF(physics, box_point_query, 3),
  MIST_FUNC_DEF(physics, query_shape, 1),
  MIST_FUNC_DEF(physics, closest_point, 3),
  MIST_FUNC_DEF(physics, make_damp, 0),
  MIST_FUNC_DEF(physics, make_gravity, 0),
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
  MIST_FUNC_DEF(emitter, start, 0),
  MIST_FUNC_DEF(emitter, stop, 0),
  MIST_FUNC_DEF(emitter, emit, 1),
  CGETSET_ADD(emitter, texture),
};

GETSET_PAIR(dsp_node, pass, bool)
GETSET_PAIR(dsp_node, off, bool)
GETSET_PAIR(dsp_node, gain, number)
GETSET_PAIR(dsp_node, pan, number)

JSValue js_dsp_node_plugin(JSContext *js, JSValue this, int argc, JSValue *argv)
{
  plugin_node(js2dsp_node(this), js2dsp_node(argv[0]));
  return JS_DupValue(js,this);
}

JSValue js_dsp_node_unplug(JSContext *js, JSValue this, int argc, JSValue *argv)
{
  JS_FreeValue(js,this);
  unplug_node(js2dsp_node(this));
  return JS_UNDEFINED;
}

static const JSCFunctionListEntry js_dsp_node_funcs[] = {
  CGETSET_ADD(dsp_node, pass),
  CGETSET_ADD(dsp_node, off),
  CGETSET_ADD(dsp_node, gain),
  CGETSET_ADD(dsp_node, pan),
  MIST_FUNC_DEF(dsp_node, plugin, 1),
  MIST_FUNC_DEF(dsp_node, unplug, 0),
};

GETSET_PAIR(sound, loop, bool)
GETSET_PAIR(sound, timescale, number)
GETSET_PAIR(sound, frame, number)
GETSET_PAIR_HOOK(sound, hook)
JSC_CCALL(sound_frames, return number2js(js2sound(this)->data->frames))

static const JSCFunctionListEntry js_sound_funcs[] = {
  CGETSET_ADD(sound, loop),
  CGETSET_ADD(sound, timescale),
  CGETSET_ADD(sound, frame),
  CGETSET_ADD(sound, hook),
  MIST_FUNC_DEF(sound, frames, 0),
};

static JSValue window_get_size(JSContext *js, JSValue this) { return vec22js(js2window(this)->size); }
static JSValue window_set_size(JSContext *js, JSValue this, JSValue v) {
  window *w = js2window(this);
  if (!w->start)
    w->size = js2vec2(v);
    
  return JS_UNDEFINED;
}

GETSET_PAIR_APPLY(window, rendersize, vec2)
GETSET_PAIR(window, mode, number)

static JSValue window_get_fullscreen(JSContext *js, JSValue this)
{
  return number2js(js2window(this)->fullscreen);
}

static JSValue window_set_fullscreen(JSContext *js, JSValue this, JSValue v)
{
  window_setfullscreen(js2window(this), js2bool(v));
  return JS_UNDEFINED;
}

static JSValue window_set_title(JSContext *js, JSValue this, JSValue v)
{
  window *w = js2window(this);
  if (w->title) JS_FreeCString(js, w->title);
  w->title = js2str(v);
  if (w->start)
    sapp_set_window_title(w->title);
  return JS_UNDEFINED;
}

static JSValue window_get_title(JSContext *js, JSValue this, JSValue v)
{
  if (!js2window(this)->title) return JS_UNDEFINED;
  return str2js(js2window(this)->title);
}

DEF_FN_STR(set_icon)

JSValue js_window_icon(JSContext *js, JSValue this, int argc, JSValue *argv) { 
  char *str = js2str(argv[0]);
  set_icon(str);
  JS_FreeCString(js,str);
  RETUN;
}

static const JSCFunctionListEntry js_window_funcs[] = {
  CGETSET_ADD(window, size),
  CGETSET_ADD(window, rendersize),
  CGETSET_ADD(window, mode),
  CGETSET_ADD(window, fullscreen),
  CGETSET_ADD(window, title),
  MIST_FN_STR(set_icon),
};

#define GETSET_PAIR_BODY(ID, ENTRY, TYPE) \
JSValue ID##_set_##ENTRY (JSContext *js, JSValue this, JSValue val) { \
  js2##ID (this)->ENTRY = js2##TYPE (val); \
  ID##_apply(js2##ID (this)); \
  return JS_UNDEFINED; \
} \
\
JSValue ID##_get_##ENTRY (JSContext *js, JSValue this) { \
  return TYPE##2js(js2##ID (this)->ENTRY); \
} \

#define BODY_GETSET(ENTRY, CPENTRY, TYPE) \
JSValue gameobject_set_##ENTRY (JSContext *js, JSValue this, JSValue val) { \
  cpBody *b = js2gameobject(this)->body; \
  cpBodySet##CPENTRY (b, js2##TYPE (val)); \
  return JS_UNDEFINED; \
} \
\
JSValue gameobject_get_##ENTRY (JSContext *js, JSValue this) { \
  cpBody *b = js2gameobject(this)->body; \
  return TYPE##2js (cpBodyGet##CPENTRY (b)); \
} \

BODY_GETSET(pos, Position, cvec2)
BODY_GETSET(angle, Angle, number)
GETSET_PAIR(gameobject, scale, vec3)
BODY_GETSET(velocity, Velocity, cvec2)
BODY_GETSET(angularvelocity, AngularVelocity, number)
BODY_GETSET(moi, Moment, number)
BODY_GETSET(phys, Type, number)
BODY_GETSET(torque, Torque, number)
JSC_CCALL(gameobject_impulse, cpBodyApplyImpulseAtWorldPoint(js2gameobject(this)->body, js2vec2(argv[0]).cp, cpBodyGetPosition(js2gameobject(this)->body)))
JSC_CCALL(gameobject_force, cpBodyApplyForceAtWorldPoint(js2gameobject(this)->body, js2vec2(argv[0]).cp, cpBodyGetPosition(js2gameobject(this)->body)))
JSC_CCALL(gameobject_force_local, cpBodyApplyForceAtLocalPoint(js2gameobject(this)->body, js2vec2(argv[0]).cp, js2vec2(argv[1]).cp))
JSC_CCALL(gameobject_in_air, return bool2js(phys2d_in_air(js2gameobject(this)->body)))
JSC_CCALL(gameobject_world2this, return vec22js(world2go(js2gameobject(this), js2vec2(argv[0]))))
JSC_CCALL(gameobject_this2world, return vec22js(go2world(js2gameobject(this), js2vec2(argv[0]))))
JSC_CCALL(gameobject_dir_world2this, return vec22js(mat_t_dir(t_world2go(js2gameobject(this)), js2vec2(argv[0]))))
JSC_CCALL(gameobject_dir_this2world, return vec22js(mat_t_dir(t_go2world(js2gameobject(this)), js2vec2(argv[0]))))
GETSET_PAIR_BODY(gameobject, f, number)
GETSET_PAIR_BODY(gameobject, e, number)
GETSET_PAIR_BODY(gameobject, mass, number)
GETSET_PAIR(gameobject, damping, number)
GETSET_PAIR(gameobject, timescale, number)
GETSET_PAIR(gameobject, maxvelocity, number)
GETSET_PAIR(gameobject, maxangularvelocity, number)
GETSET_PAIR_BODY(gameobject, layer, number)
GETSET_PAIR(gameobject, warp_filter, bitmask)
GETSET_PAIR(gameobject, drawlayer, number)
JSC_CCALL(gameobject_setref, js2gameobject(this)->ref = argv[0]);

JSValue duk_make_gameobject(JSContext *js, JSValue this) { return gameobject2js(MakeGameobject()); }

static const JSCFunctionListEntry js_gameobject_funcs[] = {
  CGETSET_ADD(gameobject, f),
  CGETSET_ADD(gameobject, e),
  CGETSET_ADD(gameobject,mass),
  CGETSET_ADD(gameobject,damping),
  CGETSET_ADD(gameobject, scale),
  CGETSET_ADD(gameobject,timescale),
  CGETSET_ADD(gameobject,maxvelocity),
  CGETSET_ADD(gameobject,maxangularvelocity),
  CGETSET_ADD(gameobject,layer),
  CGETSET_ADD(gameobject,warp_filter),
  CGETSET_ADD(gameobject,scale),
  CGETSET_ADD(gameobject,drawlayer),
  CGETSET_ADD(gameobject, pos),
  CGETSET_ADD(gameobject, angle),
  CGETSET_ADD(gameobject, velocity),
  CGETSET_ADD(gameobject, angularvelocity),
  CGETSET_ADD(gameobject, moi),
  CGETSET_ADD(gameobject, phys),
  CGETSET_ADD(gameobject, torque),
  MIST_FUNC_DEF(gameobject, impulse, 1),
  MIST_FUNC_DEF(gameobject, force, 1),
  MIST_FUNC_DEF(gameobject, force_local, 2),
  MIST_FUNC_DEF(gameobject, in_air, 0),
  MIST_FUNC_DEF(gameobject, world2this, 1),
  MIST_FUNC_DEF(gameobject, this2world, 1),
  MIST_FUNC_DEF(gameobject, dir_world2this, 1),
  MIST_FUNC_DEF(gameobject, dir_this2world, 1),
  MIST_FUNC_DEF(gameobject,setref,1),
};

JSC_CCALL(joint_pin, return constraint2js(constraint_make(cpPinJointNew(js2gameobject(argv[0])->body, js2gameobject(argv[1])->body, cpvzero,cpvzero))))
JSC_CCALL(joint_pivot, return constraint2js(constraint_make(cpPivotJointNew(js2gameobject(argv[0])->body, js2gameobject(argv[1])->body,js2vec2(argv[2]).cp))))
JSC_CCALL(joint_gear, return constraint2js(constraint_make(cpGearJointNew(js2gameobject(argv[0])->body, js2gameobject(argv[1])->body, js2number(argv[2]), js2number(argv[3])))))
JSC_CCALL(joint_rotary, return constraint2js(constraint_make(cpRotaryLimitJointNew(js2gameobject(argv[0])->body, js2gameobject(argv[1])->body, js2number(argv[2]), js2number(argv[3])))))
JSC_CCALL(joint_damped_rotary, return constraint2js(constraint_make(cpDampedRotarySpringNew(js2gameobject(argv[0])->body, js2gameobject(argv[1])->body, js2number(argv[2]), js2number(argv[3]), js2number(argv[4])))))
JSC_CCALL(joint_damped_spring, return constraint2js(constraint_make(cpDampedSpringNew(js2gameobject(argv[0])->body, js2gameobject(argv[1])->body, js2vec2(argv[2]).cp, js2vec2(argv[3]).cp, js2number(argv[4]), js2number(argv[5]), js2number(argv[6])))))
JSC_CCALL(joint_groove, return constraint2js(constraint_make(cpGrooveJointNew(js2gameobject(argv[0])->body, js2gameobject(argv[1])->body, js2vec2(argv[2]).cp, js2vec2(argv[3]).cp, js2vec2(argv[4]).cp))))
JSC_CCALL(joint_slide, return constraint2js(constraint_make(cpSlideJointNew(js2gameobject(argv[0])->body, js2gameobject(argv[1])->body, js2vec2(argv[2]).cp, js2vec2(argv[3]).cp, js2number(argv[4]), js2number(argv[5])))))
JSC_CCALL(joint_ratchet, return constraint2js(constraint_make(cpRatchetJointNew(js2body(argv[0]), js2body(argv[1]), js2number(argv[2]), js2number(argv[3])))))
JSC_CCALL(joint_motor, return constraint2js(constraint_make(cpSimpleMotorNew(js2body(argv[0]), js2body(argv[1]), js2number(argv[2])))))

static const JSCFunctionListEntry js_joint_funcs[] = {
  MIST_FUNC_DEF(joint, pin, 2),
  MIST_FUNC_DEF(joint, pivot, 3),
  MIST_FUNC_DEF(joint, gear, 4),
  MIST_FUNC_DEF(joint, rotary, 4),
  MIST_FUNC_DEF(joint, damped_rotary, 5),
  MIST_FUNC_DEF(joint, damped_spring, 7),
  MIST_FUNC_DEF(joint, groove, 5),
  MIST_FUNC_DEF(joint, slide, 6),
  MIST_FUNC_DEF(joint, ratchet, 4),
  MIST_FUNC_DEF(joint, motor, 3)
};

JSC_CCALL(dspsound_noise, return dsp_node2js(dsp_whitenoise()))
JSC_CCALL(dspsound_pink, return dsp_node2js(dsp_pinknoise()))
JSC_CCALL(dspsound_red, return dsp_node2js(dsp_rednoise()))
JSC_CCALL(dspsound_pitchshift, return dsp_node2js(dsp_pitchshift(js2number(argv[0]))))
JSC_CCALL(dspsound_noise_gate, return dsp_node2js(dsp_noise_gate(js2number(argv[0]))))
JSC_CCALL(dspsound_limiter, return dsp_node2js(dsp_limiter(js2number(argv[0]))))
JSC_CCALL(dspsound_compressor, return dsp_node2js(dsp_compressor()))
JSC_CCALL(dspsound_crush, return dsp_node2js(dsp_bitcrush(js2number(argv[0]), js2number(argv[1]))))
JSC_CCALL(dspsound_lpf, return dsp_node2js(dsp_lpf(js2number(argv[0]))))
JSC_CCALL(dspsound_hpf, return dsp_node2js(dsp_hpf(js2number(argv[0]))))
JSC_CCALL(dspsound_delay, return dsp_node2js(dsp_delay(js2number(argv[0]), js2number(argv[1]))))
JSC_CCALL(dspsound_fwd_delay, return dsp_node2js(dsp_fwd_delay(js2number(argv[0]), js2number(argv[1]))))
JSC_CCALL(dspsound_source,
  char *s = js2str(argv[0]);
  JSValue ret = dsp_node2js(dsp_source(s));
  JS_FreeCString(js,s);
  JS_SetPrototype(js, ret, sound_proto);
  return ret;
)
JSC_CCALL(dspsound_mix, return dsp_node2js(make_node(NULL,NULL,NULL)))
JSC_CCALL(dspsound_master, return dsp_node2js(masterbus))
JSC_CCALL(dspsound_plugin_node, plugin_node(js2dsp_node(argv[0]), js2dsp_node(argv[1])))
JSC_CCALL(dspsound_samplerate, return number2js(SAMPLERATE))
JSC_CCALL(dspsound_mod,
  char *s = js2str(argv[0]);
  JSValue ret = dsp_node2js(dsp_mod(s));
  JS_FreeCString(js,s);
  return ret;
)
JSC_CCALL(dspsound_midi,
  char *s1;
  char *s2;
  s1 = js2str(argv[0]);
  s2 = js2str(argv[1]);
  JSValue ret = dsp_node2js(dsp_midi(s1, make_soundfont(s2)));
  JS_FreeCString(js,s1);
  JS_FreeCString(js,s2);
  return ret;
)

static const JSCFunctionListEntry js_dspsound_funcs[] = {
  MIST_FUNC_DEF(dspsound, noise, 0),
  MIST_FUNC_DEF(dspsound, pink, 0),
  MIST_FUNC_DEF(dspsound, red, 0),
  MIST_FUNC_DEF(dspsound, pitchshift, 1),
  MIST_FUNC_DEF(dspsound, noise_gate, 1),
  MIST_FUNC_DEF(dspsound, limiter, 1),
  MIST_FUNC_DEF(dspsound, compressor, 0),
  MIST_FUNC_DEF(dspsound, crush, 2),
  MIST_FUNC_DEF(dspsound, lpf, 1),
  MIST_FUNC_DEF(dspsound, hpf, 1),
  MIST_FUNC_DEF(dspsound, delay, 2),
  MIST_FUNC_DEF(dspsound, fwd_delay, 2),
  MIST_FUNC_DEF(dspsound, source, 1),
  MIST_FUNC_DEF(dspsound, mix, 0),
  MIST_FUNC_DEF(dspsound, master, 0),
  MIST_FUNC_DEF(dspsound, plugin_node, 2),
  MIST_FUNC_DEF(dspsound, samplerate, 0),
  MIST_FUNC_DEF(dspsound, midi, 2),
  MIST_FUNC_DEF(dspsound, mod, 1)
};

JSC_CCALL(pshape_set_sensor, shape_set_sensor(js2ptr(argv[0]), js2bool(argv[1])))
JSC_CCALL(pshape_get_sensor, shape_get_sensor(js2ptr(argv[0])))
JSC_CCALL(pshape_set_enabled, shape_enabled(js2ptr(argv[0]), js2bool(argv[1])))
JSC_CCALL(pshape_get_enabled, shape_is_enabled(js2ptr(argv[0])))

static const JSCFunctionListEntry js_pshape_funcs[] = {
  MIST_FUNC_DEF(pshape, set_sensor, 2),
  MIST_FUNC_DEF(pshape, get_sensor, 1),
  MIST_FUNC_DEF(pshape, set_enabled, 2),
  MIST_FUNC_DEF(pshape, get_enabled, 1)
};

GETSET_PAIR(sprite, color, color)
GETSET_PAIR(sprite, emissive, color)
GETSET_PAIR(sprite, enabled, bool)
GETSET_PAIR(sprite, parallax, number)
GETSET_PAIR(sprite, tex, texture)
GETSET_PAIR(sprite, pos, vec2)
GETSET_PAIR(sprite, scale, vec2)
GETSET_PAIR(sprite, angle, number)
GETSET_PAIR(sprite, frame, rect)
GETSET_PAIR(sprite,go,gameobject)

static const JSCFunctionListEntry js_sprite_funcs[] = {
  CGETSET_ADD(sprite,pos),
  CGETSET_ADD(sprite,scale),
  CGETSET_ADD(sprite,angle),
  CGETSET_ADD(sprite,tex),
  CGETSET_ADD(sprite,color),
  CGETSET_ADD(sprite,emissive),
  CGETSET_ADD(sprite,enabled),
  CGETSET_ADD(sprite,parallax),
  CGETSET_ADD(sprite,frame),
  CGETSET_ADD(sprite,go)
};

#define GETFN(ID, ENTRY, TYPE) \
JSValue js_##ID##_##ENTRY (JSContext *js, JSValue this, JSValue val) {\
  return TYPE##2js(js2##ID (this)->ENTRY); \
} \

GETFN(texture,width,number)
GETFN(texture,height,number)
JSValue js_texture_path(JSContext *js, JSValue this, JSValue val)
{
  return str2js(tex_get_path(js2texture(this)));
}
JSValue js_texture_find(JSContext *js, JSValue this, int argc, JSValue *argv) { 
  char *str = js2str(argv[0]);
  JSValue ret = texture2js(texture_from_file(str));
  JS_FreeCString(js,str);
  return ret;
}

static const JSCFunctionListEntry js_texture_funcs[] = {
  MIST_FUNC_DEF(texture, width, 0),
  MIST_FUNC_DEF(texture, height, 0),
  MIST_FUNC_DEF(texture, path, 0),
  MIST_FUNC_DEF(texture, find, 1),
//  MIST_FUNC_DEF(texture, dimensions, 1),
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
     
JSValue duk_cmd_edge2d(JSContext *js, JSValue this, int argc, JSValue *argv) {
  int cmd = js2number(argv[0]);
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

JSValue duk_inflate_cpv(JSContext *js, JSValue this, int argc, JSValue *argv) {
  HMM_Vec2 *points = js2cpvec2arr(argv[0]);
  int n = js2number(argv[1]);
  double d = js2number(argv[2]);
  HMM_Vec2 *infl = inflatepoints(points,d,n);
  JSValue arr = vecarr2js(infl,arrlen(infl));
  
  arrfree(infl);
  arrfree(points);
  return arr;
}

const char *STRTEST = "TEST STRING";

JSValue duk_performance(JSContext *js, JSValue this, int argc, JSValue *argv)
{
  int cmd = js2number(argv[0]);
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
        script_call_sym(argv[1],0,NULL);
      script_call_sym(argv[3],0,NULL);
      break;
  }
  return JS_UNDEFINED;
}

JSValue js_nota_encode(JSContext *js, JSValue this, int argc, JSValue *argv)
{
  if (argc < 1) return JS_UNDEFINED;
  
  JSValue obj = argv[0];
  char nota[1024*1024]; // 1MB
  char *e = js_do_nota_encode(obj, nota);

  return JS_NewArrayBufferCopy(js, nota, e-nota);
}

JSValue js_nota_decode(JSContext *js, JSValue this, int argc, JSValue *argv)
{
  if (argc < 1) return JS_UNDEFINED;

  size_t len;
  char *nota = JS_GetArrayBuffer(js, &len, argv[0]);
  JSValue ret;
  js_do_nota_decode(&ret, nota);
  return ret;
}

static const JSCFunctionListEntry js_nota_funcs[] = {
  MIST_FUNC_DEF(nota, encode, 1),
  MIST_FUNC_DEF(nota, decode, 1)
};

#define DUK_FUNC(NAME, ARGS) JS_SetPropertyStr(js, globalThis, #NAME, JS_NewCFunction(js, duk_##NAME, #NAME, ARGS));

void ffi_load() {
  globalThis = JS_GetGlobalObject(js);

  DUK_FUNC(make_gameobject, 0)
  DUK_FUNC(make_circle2d, 1)
  DUK_FUNC(cmd_circle2d, 6)
  DUK_FUNC(make_poly2d, 1)
  DUK_FUNC(cmd_poly2d, 6)
  DUK_FUNC(make_edge2d, 3)
  DUK_FUNC(cmd_edge2d, 6)
  DUK_FUNC(make_model,2);
  DUK_FUNC(register_collide, 6)

  DUK_FUNC(inflate_cpv, 3)
  DUK_FUNC(performance, 2)
  
  QJSCLASSPREP(ptr);
  QJSCLASSPREP_FUNCS(gameobject);
  QJSCLASSPREP_FUNCS(dsp_node);
  
  sound_proto = JS_NewObject(js);
  JS_SetPropertyFunctionList(js, sound_proto, js_sound_funcs, countof(js_sound_funcs));
  JS_SetPrototype(js, sound_proto, dsp_node_proto);
  
  QJSCLASSPREP_FUNCS(emitter);
  QJSCLASSPREP_FUNCS(warp_gravity);
  QJSCLASSPREP_FUNCS(warp_damp);
  QJSCLASSPREP_FUNCS(sprite);
  QJSCLASSPREP_FUNCS(texture);
  QJSCLASSPREP_FUNCS(constraint);
  QJSCLASSPREP_FUNCS(window);
  QJSCLASSPREP_FUNCS(drawmodel);

  QJSGLOBALCLASS(os);
  QJSGLOBALCLASS(nota);
  QJSGLOBALCLASS(input);
  QJSGLOBALCLASS(io);
  QJSGLOBALCLASS(prosperon);
  QJSGLOBALCLASS(time);
  QJSGLOBALCLASS(console);
  QJSGLOBALCLASS(profile);
  QJSGLOBALCLASS(game);
  QJSGLOBALCLASS(gui);
  QJSGLOBALCLASS(debug);
  QJSGLOBALCLASS(render);
  QJSGLOBALCLASS(physics);
  QJSGLOBALCLASS(vector);
  QJSGLOBALCLASS(spline);
  QJSGLOBALCLASS(joint);
  QJSGLOBALCLASS(dspsound);
  QJSGLOBALCLASS(pshape);
  
  JS_SetPropertyStr(js, prosperon, "version", str2js(VER));
  JS_SetPropertyStr(js, prosperon, "revision", str2js(COM));
  JS_SetPropertyStr(js, globalThis, "window", window2js(&mainwin));
  JS_SetPropertyStr(js, globalThis, "texture", JS_DupValue(js,texture_proto));

  JS_FreeValue(js,globalThis);
}

void ffi_stop()
{
  JS_FreeValue(js, sound_proto);
}
