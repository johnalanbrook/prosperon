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
#include "nota.h"
#include "render.h"
#include "model.h"
#include "HandmadeMath.h"

#if (defined(_WIN32) || defined(__WIN32__))
#include <direct.h>
#define mkdir(x,y) _mkdir(x)
#endif

static JSValue globalThis;

static JSClassID js_ptr_id;
static JSClassDef js_ptr_class = { "POINTER" };

JSValue str2js(const char *c) {
  if (!c) return JS_UNDEFINED;
  return JS_NewString(js, c);
}

const char *js2str(JSValue v) {
  return JS_ToCString(js, v);
}

/* support functions for these macros when they involve a JSValue */
JSValue js2JSValue(JSValue v) { return v; }
JSValue JSValue2js(JSValue v) { return v; }

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

int js2boolean(JSValue v) { return JS_ToBool(js, v); }
JSValue boolean2js(int b) { return JS_NewBool(js,b); }

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
        *tmp = boolean2js(b);
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
    int val = js2boolean(js_getpropidx( v, i));
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
    js_setprop_num(arr,i,boolean2js(mask & 1 << i));

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

circle2d *js2circle2d(JSValue v) { return js2ptr(v); }

JSC_CCALL(circle2d_set_radius, js2circle2d(argv[0])->radius = js2number(argv[1]))
JSC_CCALL(circle2d_get_radius, return number2js(js2circle2d(argv[0])->radius))
JSC_CCALL(circle2d_set_offset, js2circle2d(argv[0])->offset = js2vec2(argv[1]))
JSC_CCALL(circle2d_get_offset, return vec22js(js2circle2d(argv[0])->offset))
JSC_CCALL(circle2d_sync, phys2d_shape_apply(&js2circle2d(argv[0])->shape))

static const JSCFunctionListEntry js_circle2d_funcs[] = {
  MIST_FUNC_DEF(circle2d, set_radius, 2),
  MIST_FUNC_DEF(circle2d, get_radius, 1),
  MIST_FUNC_DEF(circle2d, set_offset, 2),
  MIST_FUNC_DEF(circle2d, get_offset, 1),
  MIST_FUNC_DEF(circle2d, sync, 1),
};

struct phys2d_poly *js2poly2d(JSValue v) { return js2ptr(v); }

JSC_CCALL(poly2d_setverts, 
  struct phys2d_poly *p = js2poly2d(argv[0]);
  HMM_Vec2 *v = js2cpvec2arr(argv[1]);
  phys2d_poly_setverts(p,v);
  arrfree(v);
)

static const JSCFunctionListEntry js_poly2d_funcs[] = {
  MIST_FUNC_DEF(poly2d, setverts, 2),
};

JSC_GETSET(warp_gravity, strength, number)
JSC_GETSET(warp_gravity, decay, number)
JSC_GETSET(warp_gravity, spherical, boolean)
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

JSC_CCALL(drawmodel_draw, draw_drawmodel(js2drawmodel(this)))
JSC_SCALL(drawmodel_path, js2drawmodel(this)->model = GetExistingModel(str))

static const JSCFunctionListEntry js_drawmodel_funcs[] = {
  MIST_FUNC_DEF(drawmodel, draw, 0),
  MIST_FUNC_DEF(drawmodel, path, 1)
};

JSC_GETSET(emitter, life, number)
JSC_GETSET(emitter, life_var, number)
JSC_GETSET(emitter, speed, number)
JSC_GETSET(emitter, variation, number)
JSC_GETSET(emitter, divergence, number)
JSC_GETSET(emitter, scale, number)
JSC_GETSET(emitter, scale_var, number)
JSC_GETSET(emitter, grow_for, number)
JSC_GETSET(emitter, shrink_for, number)
JSC_GETSET(emitter, max, number)
JSC_GETSET(emitter, explosiveness, number)
JSC_GETSET(emitter, go, gameobject)
JSC_GETSET(emitter, bounce, number)
JSC_GETSET(emitter, collision_mask, bitmask)
JSC_GETSET(emitter, die_after_collision, boolean)
JSC_GETSET(emitter, persist, number)
JSC_GETSET(emitter, persist_var, number)
JSC_GETSET(emitter, warp_mask, bitmask)
JSC_GETSET(emitter, texture, texture)

JSC_CCALL(emitter_start, start_emitter(js2emitter(this)))
JSC_CCALL(emitter_stop, stop_emitter(js2emitter(this)))
JSC_CCALL(emitter_emit, emitter_emit(js2emitter(this), js2number(argv[0])))

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

JSC_SCALL(os_env, ret = str2js(getenv(str)))

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
JSC_SSCALL(os_eval, ret = script_eval(str, str2))
JSC_SCALL(os_capture, capture_screen(js2number(argv[1]), js2number(argv[2]), js2number(argv[4]), js2number(argv[5]), str))

JSC_CCALL(os_sprite,
  if (js2boolean(argv[0])) return JS_GetClassProto(js,js_sprite_id);
  return sprite2js(sprite_make());
)

JSC_CCALL(os_make_gameobject, return gameobject2js(MakeGameobject()))
JSC_CCALL(os_make_circle2d,
  gameobject *go = js2gameobject(argv[0]);
  struct phys2d_circle *circle = Make2DCircle(go);
  JSValue circleval = JS_NewObject(js);
  js_setprop_str(circleval, "id", ptr2js(circle));
  js_setprop_str(circleval, "shape", ptr2js(&circle->shape));
  return circleval;
)

JSC_CCALL(os_make_model,
  gameobject *go = js2gameobject(argv[0]);
  struct drawmodel *dm = make_drawmodel(go);
  JSValue ret = JS_NewObject(js);
  js_setprop_str(ret, "id", ptr2js(dm));
  return ret;
)

JSC_CCALL(os_make_poly2d,
  gameobject *go = js2gameobject(argv[0]);
  struct phys2d_poly *poly = Make2DPoly(go);
  phys2d_poly_setverts(poly, NULL);
  JSValue polyval = JS_NewObject(js);
  js_setprop_str(polyval, "id", ptr2js(poly));
  js_setprop_str(polyval, "shape", ptr2js(&poly->shape));
  return polyval;
)

JSC_CCALL(os_make_edge2d,
  gameobject *go = js2gameobject(argv[0]);
  struct phys2d_edge *edge = Make2DEdge(go);
  HMM_Vec2 *points = js2cpvec2arr(argv[1]);
  phys2d_edge_update_verts(edge, points);
  arrfree(points);
  
  JSValue edgeval = JS_NewObject(js);
  js_setprop_str(edgeval, "id", ptr2js(edge));
  js_setprop_str(edgeval, "shape", ptr2js(&edge->shape));
  return edgeval;
)

JSC_SCALL(os_make_texture,
  ret = texture2js(texture_from_file(str));
  YughInfo("Made texture with %s", str);
  JS_SetPropertyStr(js, ret, "path", JS_DupValue(js,argv[0]));
)

JSC_SCALL(os_system,
  system(str);
)

static const JSCFunctionListEntry js_os_funcs[] = {
  MIST_FUNC_DEF(os,sprite,1),
  MIST_FUNC_DEF(os, cwd, 0),
  MIST_FUNC_DEF(os, env, 1),
  MIST_FUNC_DEF(os, sys, 0),
  MIST_FUNC_DEF(os, system, 1),
  MIST_FUNC_DEF(os, quit, 0),
  MIST_FUNC_DEF(os, reindex_static, 0),
  MIST_FUNC_DEF(os, gc, 0),
  MIST_FUNC_DEF(os, capture, 5),
  MIST_FUNC_DEF(os, eval, 2),
  MIST_FUNC_DEF(os, make_gameobject, 0),
  MIST_FUNC_DEF(os, make_circle2d, 1),
  MIST_FUNC_DEF(os, make_poly2d, 1),
  MIST_FUNC_DEF(os, make_edge2d, 1),
  MIST_FUNC_DEF(os, make_model, 2),
  MIST_FUNC_DEF(os, make_texture, 1),
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
JSC_SCALL(render_text_size, ret = bb2js(text_bb(str, js2number(argv[1]), js2number(argv[2]), 1)))
JSC_CCALL(render_world2screen, return vec22js(world2screen(js2vec2(argv[0]))))
JSC_CCALL(render_screen2world, return vec22js(screen2world(js2vec2(argv[0]))))

static const JSCFunctionListEntry js_render_funcs[] = {
  MIST_FUNC_DEF(render,world2screen,1),
  MIST_FUNC_DEF(render,screen2world,1),
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
  transform2d t;
  t.pos = js2vec2(argv[1]);
  t.scale = js2vec2(argv[2]);
  t.angle = js2number(argv[3]);
  gui_draw_img(js2texture(argv[0]), t, js2boolean(argv[4]), js2vec2(argv[5]), 1.0, js2color(argv[6]));
)

JSC_SCALL(gui_font_set, font_set(str))

static const JSCFunctionListEntry js_gui_funcs[] = {
  MIST_FUNC_DEF(gui, flush, 0),
  MIST_FUNC_DEF(gui, scissor, 4),
  MIST_FUNC_DEF(gui, text, 6),
  MIST_FUNC_DEF(gui, img, 7),
  MIST_FUNC_DEF(gui, font_set,1)
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

JSC_CCALL(vector_project, return vec22js(HMM_ProjV2(js2vec2(argv[0]), js2vec2(argv[1]))))

JSC_CCALL(vector_inflate,
  HMM_Vec2 *p = js2cpvec2arr(argv[0]);
  double d = js2number(argv[1]);
  HMM_Vec2 *infl = inflatepoints(p,d, js_arrlen(argv[0]));
  ret = vecarr2js(infl, arrlen(infl));
  arrfree(infl);
  arrfree(p);
)

static const JSCFunctionListEntry js_vector_funcs[] = {
  MIST_FUNC_DEF(vector,dot,2),
  MIST_FUNC_DEF(vector,project,2),
  MIST_FUNC_DEF(vector, inflate, 2)
};

JSC_CCALL(game_engine_start, engine_start(argv[0],argv[1]))

JSValue js_game_object_count(JSContext *js, JSValue this) { return number2js(go_count()); }

static const JSCFunctionListEntry js_game_funcs[] = {
  MIST_FUNC_DEF(game, engine_start, 2),
  MIST_FUNC_DEF(game, object_count, 0)
};

JSC_CCALL(input_show_keyboard, sapp_show_keyboard(js2boolean(argv[0])))
JSValue js_input_keyboard_shown(JSContext *js, JSValue this) { return boolean2js(sapp_keyboard_shown()); }
JSC_CCALL(input_mouse_mode, set_mouse_mode(js2number(argv[0])))
JSC_CCALL(input_mouse_cursor, sapp_set_mouse_cursor(js2number(argv[0])))
JSC_SCALL(input_cursor_img, cursor_img(str))

static const JSCFunctionListEntry js_input_funcs[] = {
  MIST_FUNC_DEF(input, show_keyboard, 1),
  MIST_FUNC_DEF(input, keyboard_shown, 0),
  MIST_FUNC_DEF(input, mouse_mode, 1),
  MIST_FUNC_DEF(input, mouse_cursor, 1),
  MIST_FUNC_DEF(input, cursor_img, 1)
};

JSC_CCALL(prosperon_emitters_step, emitters_step(js2number(argv[0])))
JSC_CCALL(prosperon_phys2d_step, phys2d_update(js2number(argv[0])))
JSC_CCALL(prosperon_window_render, openglRender(&mainwin, js2gameobject(argv[0]), js2number(argv[1])))

static const JSCFunctionListEntry js_prosperon_funcs[] = {
  MIST_FUNC_DEF(prosperon, emitters_step, 1),
  MIST_FUNC_DEF(prosperon, phys2d_step, 1),
  MIST_FUNC_DEF(prosperon, window_render, 0)
};

JSC_CCALL(time_now, 
  struct timeval ct;
  gettimeofday(&ct, NULL);
  return number2js((double)ct.tv_sec+(double)(ct.tv_usec/1000000.0));
)

JSValue js_time_computer_dst(JSContext *js, JSValue this) {
  time_t t = time(NULL);
  return boolean2js(localtime(&t)->tm_isdst);
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

JSC_SCALL(console_print, log_print(str))

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

JSC_GETSET_GLOBAL(stdout_lvl, number)

static const JSCFunctionListEntry js_console_funcs[] = {
  MIST_FUNC_DEF(console,print,1),
  MIST_FUNC_DEF(console,rec,4),
  CGETSET_ADD(global, stdout_lvl)
};

JSC_CCALL(profile_now, return number2js(stm_now()))

static const JSCFunctionListEntry js_profile_funcs[] = {
  MIST_FUNC_DEF(profile,now,0),
};

JSC_SCALL(io_exists, ret = boolean2js(fexists(str)))
JSC_CCALL(io_ls, return strarr2js(ls(".")))
JSC_SSCALL(io_cp, ret = number2js(cp(str,str2)))
JSC_SSCALL(io_mv, ret = number2js(rename(str,str2)))
JSC_SCALL(io_chdir, ret = number2js(chdir(str)))
JSC_SCALL(io_rm, ret = number2js(remove(str)))
JSC_SCALL(io_mkdir, ret = number2js(mkdir(str,0777)))

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
  const char *data;
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

JSC_SCALL(io_save_qoa, save_qoa(str))

JSC_SCALL(io_pack_engine, pack_engine(str))

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
  MIST_FUNC_DEF(io, save_qoa,1),
  MIST_FUNC_DEF(io, pack_engine, 1),
};

JSC_CCALL(debug_draw_gameobject, gameobject_draw_debug(js2gameobject(argv[0]));)

static const JSCFunctionListEntry js_debug_funcs[] = {
  MIST_FUNC_DEF(debug, draw_gameobject, 1)
};

JSC_CCALL(physics_sgscale,
  js2gameobject(argv[0])->scale.xy = js2vec2(argv[1]);
  gameobject_apply(js2gameobject(argv[0]));
  cpSpaceReindexShapesForBody(space, js2gameobject(argv[0])->body);
)

JSC_CCALL(physics_set_cat_mask, set_cat_mask(js2number(argv[0]), js2bitmask(argv[1])))

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

JSC_CCALL(physics_collide_begin, js2gameobject(argv[1])->cbs.begin = JS_DupValue(js,argv[0]))
JSC_CCALL(physics_collide_rm, phys2d_rm_go_handlers(js2gameobject(argv[0])))
JSC_CCALL(physics_collide_separate, js2gameobject(argv[1])->cbs.separate = JS_DupValue(js,argv[0]))
JSC_CCALL(physics_collide_shape, gameobject_add_shape_collider(js2gameobject(argv[1]), JS_DupValue(js,argv[0]), js2ptr(argv[2])))

static const JSCFunctionListEntry js_physics_funcs[] = {
  MIST_FUNC_DEF(physics, sgscale, 2),
  MIST_FUNC_DEF(physics, set_cat_mask, 2),
  MIST_FUNC_DEF(physics, box_query, 2),
  MIST_FUNC_DEF(physics, pos_query, 2),
  MIST_FUNC_DEF(physics, box_point_query, 3),
  MIST_FUNC_DEF(physics, query_shape, 1),
  MIST_FUNC_DEF(physics, closest_point, 3),
  MIST_FUNC_DEF(physics, make_damp, 0),
  MIST_FUNC_DEF(physics, make_gravity, 0),
  MIST_FUNC_DEF(physics, collide_begin, 2),
  MIST_FUNC_DEF(physics, collide_rm, 1),
  MIST_FUNC_DEF(physics, collide_separate, 2),
  MIST_FUNC_DEF(physics, collide_shape, 3)
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

JSC_GETSET(dsp_node, pass, boolean)
JSC_GETSET(dsp_node, off, boolean)
JSC_GETSET(dsp_node, gain, number)
JSC_GETSET(dsp_node, pan, number)

JSC_CCALL(dsp_node_plugin, plugin_node(js2dsp_node(this), js2dsp_node(argv[0])))
JSC_CCALL(dsp_node_unplug, unplug_node(js2dsp_node(this)))

static const JSCFunctionListEntry js_dsp_node_funcs[] = {
  CGETSET_ADD(dsp_node, pass),
  CGETSET_ADD(dsp_node, off),
  CGETSET_ADD(dsp_node, gain),
  CGETSET_ADD(dsp_node, pan),
  MIST_FUNC_DEF(dsp_node, plugin, 1),
  MIST_FUNC_DEF(dsp_node, unplug, 0),
};

JSC_GETSET(sound, loop, boolean)
JSC_GETSET(sound, timescale, number)
JSC_GETSET(sound, frame, number)
JSC_GETSET_HOOK(sound, hook)
JSC_CCALL(sound_frames, return number2js(js2sound(this)->data->frames))

static const JSCFunctionListEntry js_sound_funcs[] = {
  CGETSET_ADD(sound, loop),
  CGETSET_ADD(sound, timescale),
  CGETSET_ADD(sound, frame),
  CGETSET_ADD(sound, hook),
  MIST_FUNC_DEF(sound, frames, 0),
};

static JSValue js_window_get_size(JSContext *js, JSValue this) { return vec22js(js2window(this)->size); }
static JSValue js_window_set_size(JSContext *js, JSValue this, JSValue v) {
  window *w = js2window(this);
  if (!w->start)
    w->size = js2vec2(v);
    
  return JS_UNDEFINED;
}

JSC_GETSET_APPLY(window, rendersize, vec2)
JSC_GETSET(window, mode, number)
static JSValue js_window_get_fullscreen(JSContext *js, JSValue this) { return boolean2js(js2window(this)->fullscreen); }
static JSValue js_window_set_fullscreen(JSContext *js, JSValue this, JSValue v) { window_setfullscreen(js2window(this), js2boolean(v)); }

static JSValue js_window_set_title(JSContext *js, JSValue this, JSValue v)
{
  window *w = js2window(this);
  if (w->title) JS_FreeCString(js, w->title);
  w->title = js2str(v);
  if (w->start)
    sapp_set_window_title(w->title);
  return JS_UNDEFINED;
}
JSC_CCALL(window_get_title, return str2js(js2window(this)->title))
JSC_CCALL(window_set_icon, window_seticon(&mainwin, js2texture(argv[0])))

static const JSCFunctionListEntry js_window_funcs[] = {
  CGETSET_ADD(window, size),
  CGETSET_ADD(window, rendersize),
  CGETSET_ADD(window, mode),
  CGETSET_ADD(window, fullscreen),
  CGETSET_ADD(window, title),
  MIST_FUNC_DEF(window, set_icon, 1)
};

JSC_GETSET_BODY(pos, Position, cvec2)
JSC_GETSET_BODY(angle, Angle, number)
JSC_GETSET_BODY(velocity, Velocity, cvec2)
JSC_GETSET_BODY(angularvelocity, AngularVelocity, number)
JSC_GETSET_BODY(moi, Moment, number)
JSC_GETSET_BODY(torque, Torque, number)
JSC_CCALL(gameobject_impulse, cpBodyApplyImpulseAtWorldPoint(js2gameobject(this)->body, js2vec2(argv[0]).cp, cpBodyGetPosition(js2gameobject(this)->body)))
JSC_CCALL(gameobject_force, cpBodyApplyForceAtWorldPoint(js2gameobject(this)->body, js2vec2(argv[0]).cp, cpBodyGetPosition(js2gameobject(this)->body)))
JSC_CCALL(gameobject_force_local, cpBodyApplyForceAtLocalPoint(js2gameobject(this)->body, js2vec2(argv[0]).cp, js2vec2(argv[1]).cp))
JSC_GETSET_APPLY(gameobject, friction, number)
JSC_GETSET_APPLY(gameobject, elasticity, number)
JSC_GETSET_APPLY(gameobject, mass, number)
JSC_GETSET_APPLY(gameobject, phys, number)
JSC_GETSET_APPLY(gameobject, layer, number)
JSC_GETSET(gameobject, scale, vec3)
JSC_GETSET(gameobject, damping, number)
JSC_GETSET(gameobject, timescale, number)
JSC_GETSET(gameobject, maxvelocity, number)
JSC_GETSET(gameobject, maxangularvelocity, number)
JSC_GETSET(gameobject, warp_filter, bitmask)
JSC_GETSET(gameobject, drawlayer, number)
JSC_CCALL(gameobject_setref, js2gameobject(this)->ref = argv[0]);
JSC_CCALL(gameobject_sync, gameobject_apply(js2gameobject(this)))
JSC_CCALL(gameobject_in_air, return boolean2js(phys2d_in_air(js2gameobject(this)->body)))
JSC_CCALL(gameobject_world2this, return vec22js(world2go(js2gameobject(this), js2vec2(argv[0]))))
JSC_CCALL(gameobject_this2world, return vec22js(go2world(js2gameobject(this), js2vec2(argv[0]))))
JSC_CCALL(gameobject_dir_world2this, return vec22js(mat_t_dir(t_world2go(js2gameobject(this)), js2vec2(argv[0]))))
JSC_CCALL(gameobject_dir_this2world, return vec22js(mat_t_dir(t_go2world(js2gameobject(this)), js2vec2(argv[0]))))

static const JSCFunctionListEntry js_gameobject_funcs[] = {
  CGETSET_ADD(gameobject, friction),
  CGETSET_ADD(gameobject, elasticity),
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
  MIST_FUNC_DEF(gameobject, sync, 0),
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
JSC_SCALL(dspsound_source,
  ret = dsp_node2js(dsp_source(str));
  JS_SetPrototype(js, ret, sound_proto);
)
JSC_CCALL(dspsound_mix, return dsp_node2js(make_node(NULL,NULL,NULL)))
JSC_CCALL(dspsound_master, return dsp_node2js(masterbus))
JSC_CCALL(dspsound_plugin_node, plugin_node(js2dsp_node(argv[0]), js2dsp_node(argv[1]));)
JSC_CCALL(dspsound_samplerate, return number2js(SAMPLERATE))
JSC_SCALL(dspsound_mod, ret = dsp_node2js(dsp_mod(str))) 
JSC_SSCALL(dspsound_midi, ret = dsp_node2js(dsp_midi(str, make_soundfont(str2))))

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

JSC_CCALL(pshape_set_sensor, shape_set_sensor(js2ptr(argv[0]), js2boolean(argv[1])))
JSC_CCALL(pshape_get_sensor, shape_get_sensor(js2ptr(argv[0])))
JSC_CCALL(pshape_set_enabled, shape_enabled(js2ptr(argv[0]), js2boolean(argv[1])))
JSC_CCALL(pshape_get_enabled, shape_is_enabled(js2ptr(argv[0])))

static const JSCFunctionListEntry js_pshape_funcs[] = {
  MIST_FUNC_DEF(pshape, set_sensor, 2),
  MIST_FUNC_DEF(pshape, get_sensor, 1),
  MIST_FUNC_DEF(pshape, set_enabled, 2),
  MIST_FUNC_DEF(pshape, get_enabled, 1)
};

JSC_GETSET(sprite, color, color)
JSC_GETSET(sprite, emissive, color)
JSC_GETSET(sprite, enabled, boolean)
JSC_GETSET(sprite, parallax, number)
JSC_GETSET(sprite, tex, texture)
JSC_GETSET(sprite, pos, vec2)
JSC_GETSET(sprite, scale, vec2)
JSC_GETSET(sprite, angle, number)
JSC_GETSET(sprite, frame, rect)
JSC_GETSET(sprite,go,gameobject)
JSC_CCALL(sprite_tex, js2sprite(this)->tex = js2texture(argv[0]))

static const JSCFunctionListEntry js_sprite_funcs[] = {
  CGETSET_ADD(sprite,pos),
  CGETSET_ADD(sprite,scale),
  CGETSET_ADD(sprite,angle),
  MIST_FUNC_DEF(sprite, tex, 1),
  CGETSET_ADD(sprite,color),
  CGETSET_ADD(sprite,emissive),
  CGETSET_ADD(sprite,enabled),
  CGETSET_ADD(sprite,parallax),
  CGETSET_ADD(sprite,frame),
  CGETSET_ADD(sprite,go)
};

JSC_GET(texture, width, number)
JSC_GET(texture, height, number)
JSC_GET(texture, frames, number)
JSC_GET(texture, delays, ints)

static const JSCFunctionListEntry js_texture_funcs[] = {
  MIST_GET(texture, width),
  MIST_GET(texture, height),
  MIST_GET(texture, frames),
  MIST_GET(texture, delays),
};

JSValue js_constraint_set_max_force (JSContext *js, JSValue this, JSValue val) {
  cpConstraintSetMaxForce(js2constraint(this)->c, js2number(val));
  return JS_UNDEFINED;
}

JSValue js_constraint_get_max_force(JSContext *js, JSValue this) { return number2js(cpConstraintGetMaxForce(js2constraint(this)->c));
}

JSValue js_constraint_set_collide (JSContext *js, JSValue this, JSValue val) {
  cpConstraintSetCollideBodies(js2constraint(this)->c, js2boolean(val));
  return JS_UNDEFINED;
}

JSValue js_constraint_get_collide(JSContext *js, JSValue this) { return boolean2js(cpConstraintGetCollideBodies(js2constraint(this)->c));
}

static const JSCFunctionListEntry js_constraint_funcs[] = {
  CGETSET_ADD(constraint, max_force),
  CGETSET_ADD(constraint, collide),
};

struct phys2d_edge *js2edge2d(JSValue v) { return js2ptr(v); }
JSC_CCALL(edge2d_setverts,
  struct phys2d_edge *edge = js2edge2d(argv[0]);
  HMM_Vec2 *v = js2cpvec2arr(argv[1]);
  phys2d_edge_update_verts(edge,v);
  arrfree(v);
)
JSC_CCALL(edge2d_set_thickness, js2edge2d(argv[0])->thickness = js2number(argv[1]))
JSC_CCALL(edge2d_get_thickness, return number2js(js2edge2d(argv[0])->thickness))

static const JSCFunctionListEntry js_edge2d_funcs[] = {
  MIST_FUNC_DEF(edge2d, setverts, 2),
  MIST_FUNC_DEF(edge2d, set_thickness, 2),
  MIST_FUNC_DEF(edge2d, get_thickness, 1),
};

const char *STRTEST = "TEST STRING";

JSC_CCALL(performance_barecall,)
JSC_CCALL(performance_unpack_num, int i = js2number(argv[0]))
JSC_CCALL(performance_unpack_array, js2cpvec2arr(argv[0]))
JSC_CCALL(performance_pack_num, return number2js(1.0))
JSC_CCALL(performance_pack_string, return JS_NewStringLen(js, STRTEST, sizeof(*STRTEST)))
JSC_CCALL(performance_unpack_string, js2str(argv[0]))
JSC_CCALL(performance_unpack_32farr, jsfloat2vec(argv[0]))
JSC_CCALL(performance_call_fn_n, 
  for (int i = 0; i < js2number(argv[1]); i++)
    script_call_sym(argv[0],0,NULL);
  script_call_sym(argv[2],0,NULL);
)

static const JSCFunctionListEntry js_performance_funcs[] = {
  MIST_FUNC_DEF(performance, barecall,0),
  MIST_FUNC_DEF(performance, unpack_num, 1),
  MIST_FUNC_DEF(performance, unpack_array, 1),
  MIST_FUNC_DEF(performance, pack_num, 0),
  MIST_FUNC_DEF(performance, pack_string, 0),
  MIST_FUNC_DEF(performance, unpack_string, 1),
  MIST_FUNC_DEF(performance, unpack_32farr, 1),
  MIST_FUNC_DEF(performance, call_fn_n, 3)
};

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

#include "steam.h"

void ffi_load() {
  globalThis = JS_GetGlobalObject(js);
  
  QJSCLASSPREP(ptr);
  
  QJSGLOBALCLASS(os);
  
  QJSCLASSPREP_FUNCS(gameobject);
  QJSCLASSPREP_FUNCS(dsp_node);
  QJSCLASSPREP_FUNCS(emitter);
  QJSCLASSPREP_FUNCS(warp_gravity);
  QJSCLASSPREP_FUNCS(warp_damp);
  QJSCLASSPREP_FUNCS(sprite);
  QJSCLASSPREP_FUNCS(texture);
  QJSCLASSPREP_FUNCS(constraint);
  QJSCLASSPREP_FUNCS(window);
  QJSCLASSPREP_FUNCS(drawmodel);

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
  QJSGLOBALCLASS(performance);
  
  QJSGLOBALCLASS(circle2d);
  QJSGLOBALCLASS(poly2d);
  QJSGLOBALCLASS(edge2d);
  
  JS_SetPropertyStr(js, prosperon, "version", str2js(VER));
  JS_SetPropertyStr(js, prosperon, "revision", str2js(COM));
  JS_SetPropertyStr(js, globalThis, "window", window2js(&mainwin));
  JS_SetPropertyStr(js, globalThis, "texture", JS_DupValue(js,texture_proto));
  
  JS_SetPropertyStr(js, globalThis, "steam", js_init_steam(js));

  sound_proto = JS_NewObject(js);
  JS_SetPropertyStr(js,globalThis, "sound_proto", sound_proto);
  JS_SetPropertyFunctionList(js, sound_proto, js_sound_funcs, countof(js_sound_funcs));
  JS_SetPrototype(js, sound_proto, dsp_node_proto);
  
  JS_FreeValue(js,globalThis);  
}

void ffi_stop()
{
}