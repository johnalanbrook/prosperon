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
#include <assert.h>
#include "resources.h"
#include <sokol/sokol_time.h>

#include "render.h"

#include "model.h"

#include "HandmadeMath.h"

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


QJSCLASS(dsp_node)
QJSCLASS(gameobject)

#define QJSCLASSPREP(TYPE) \
JS_NewClassID(&js_##TYPE##_id);\
JS_NewClass(JS_GetRuntime(js), js_##TYPE##_id, &js_##TYPE##_class);\

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)     \
  (byte & 0x80 ? '1' : '0'),     \
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
  return num2js(diff/3600);
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
  JSValue jst = num2js(t);
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

uint64_t js2uint64(JSValue v)
{
  int64_t i;
  JS_ToInt64(js, &i, v);
  uint64_t n = i;
  return n;
}

int js2int(JSValue v) {
  int32_t i;
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

JSValue float2js(double g) { return JS_NewFloat64(js, g);}
JSValue num2js(double g) { return float2js(g); }
struct sprite *js2sprite(JSValue v) { return id2sprite(js2int(v)); }

void *js2ptr(JSValue v) { return JS_GetOpaque(v,js_ptr_id); }

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

/* Does not need to be freed by returning; but not reentrant */
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
  js_setprop_num(array,0,num2js(v.x));
  js_setprop_num(array,1,num2js(v.y));
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

  switch (cmd) {
  case 0:
    str = JS_ToCString(js, argv[1]);
    ret = JS_NewInt64(js, script_dofile(str));
    break;

  case 1:
    YughWarn("Do not set pawns here anymore; Do it entirely in script.");
    // set_pawn(js2ptrduk_get_heapptr(duk, 1));
    break;

  case 2:
//    gameobject_free(js2gameobject(argv[1]));
    break;

  case 3:
    set_timescale(js2number(argv[1]));
    break;

  case 4:
    debug_draw_phys(JS_ToBool(js, argv[1]));
    break;

  case 5:
//    renderMS = js2number(argv[1]);
    break;

  case 6:
    updateMS = js2number(argv[1]);
    break;

  case 7:
    physMS = js2number(argv[1]);
    break;

  case 8:
    phys2d_set_gravity(js2vec2(argv[1]).cp);
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

  case 14:
    str = JS_ToCString(js, argv[1]);
    ret = ptr2js(dsp_source(str));
    ((sound*)((dsp_node*)js2ptr(ret))->data)->hook = argv[2];
    break;

  case 15:
  gameobject_draw_debug(js2gameobject(argv[1]));
  break;

  case 16:
//    dbg_color = js2color(argv[1]);
    break;

  case 17:
//    trigger_color = js2color(argv[1]);
    break;

  case 18:
    shape_set_sensor(js2ptr(argv[1]), JS_ToBool(js, argv[2]));
    break;

  case 19:
//    mini_master(js2number(argv[1]));
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

  case 30:
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

  case 38:
    str = JS_ToCString(js, argv[1]);
    d1 = slurp_text(str,NULL);
    ret = JS_NewString(js, d1);
    break;

  case 39:
    str = JS_ToCString(js, argv[1]);
    str2 = JS_ToCString(js, argv[2]);
    ret = JS_NewInt64(js, slurp_write(str, str2));
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

  case 55:
    break;
//    js2gameobject(argv[1])->flipx = JS_ToBool(js, argv[2]) ? -1 : 1;

  case 56:
//    js2gameobject(argv[1])->flipy = JS_ToBool(js, argv[2]) ? -1 : 1;
    break;

  case 57:
//    ret = JS_NewBool(js, js2gameobject(argv[1])->flipx == -1 ? 1 : 0);
    break;

  case 58:
//    ret = JS_NewBool(js, js2gameobject(argv[1])->flipy == -1 ? 1 : 0);
    break;

  case 59:
    v1 = js2cpvec2arr(argv[2]);
    ret = JS_NewInt64(js, point2segindex(js2vec2(argv[1]), v1, js2number(argv[3])));
    break;

  case 60:
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

  case 65:
    str = JS_ToCString(js, argv[1]);
    ret = JS_NewBool(js, fexists(str));
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

  case 69:
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

  case 78:
    break;

  case 79:
    ret = JS_NewBool(js, phys_stepping());
    break;

  case 80:
    ids = phys2d_query_shape(js2ptr(argv[1]));
    ret = gos2ref(ids);
    arrfree(ids);
    break;

  case 81:
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

  case 87:
//    str = JS_ToCString(js, argv[1]);
//    mini_music_play(str);
    break;

  case 88:
    ret = num2js(HMM_DotV2(js2vec2(argv[1]), js2vec2(argv[2])));
    break;

  case 89:
//    mini_music_stop();
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

  case 94:
    str = JS_ToCString(js, argv[1]);
    texture_pullfromfile(str)->opts.mips = js2bool(argv[2]);
    texture_sync(str);
    break;

  case 95:
    str = JS_ToCString(js, argv[1]);
    texture_pullfromfile(str)->opts.sprite = js2bool(argv[2]);
    texture_sync(str);
    break;

  case 96:
    id2sprite(js2int(argv[1]))->color = js2color(argv[2]);
    break;

  case 97:
    eye = HMM_AddV3(eye, (HMM_Vec3){0, 0.01, 0});
    break;

  case 98:
    eye = HMM_AddV3(eye, (HMM_Vec3){0, -0.01, 0});
    break;

  case 99:
    eye = HMM_AddV3(eye, (HMM_Vec3){-0.01, 0, 0});
    break;

  case 100:
    eye = HMM_AddV3(eye, (HMM_Vec3){0.01, 0,0});
    break;

  case 101:
    eye = HMM_AddV3(eye, (HMM_Vec3){0,0,-0.01});
    break;

  case 102:
    eye = HMM_AddV3(eye,(HMM_Vec3){0,0,0.01});
    break;
    
  case 103:
    ret = vec2js(js2gameobject(argv[1])->scale.XY);
    break;

  case 104:
//    ret = bool2js(js2gameobject(argv[1])->flipx == -1 ? 1 : 0);
    break;

  case 105:
//    ret = bool2js(js2gameobject(argv[1])->flipy == -1 ? 1 : 0);
    break;

  case 106:
    js2gameobject(argv[1])->e = js2number(argv[2]);
    break;

  case 107:
    ret = num2js(js2gameobject(argv[1])->e);
    break;

  case 108:
    js2gameobject(argv[1])->f = js2number(argv[2]);
    break;
    
  case 109:
    ret = num2js(js2gameobject(argv[1])->f);
    break;
    
  case 110:
    ret = num2js(js2gameobject(argv[1])->e);
    break;

    case 111:
      ret = vec2js(js2sprite(argv[1])->t.pos);
      break;

    case 112:
      ret = num2js(((struct phys2d_edge*)js2ptr(argv[1]))->thickness);
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

    case 120:
      ret = str2js(engine_info());
      break;
    case 121:
      ret = num2js(get_timescale());
      break;
    case 122:
      break;

    case 123:
      str = JS_ToCString(js, argv[1]);
      str2 = JS_ToCString(js, argv[3]);
      script_eval_w_env(str, argv[2], str2);
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
      ret = JS_NewFloat64(js, stm_ns(js2uint64(argv[1])));
      break;

    case 129:
      ret = JS_NewFloat64(js, stm_us(js2uint64(argv[1])));
      break;

    case 130:
      ret = JS_NewFloat64(js, stm_ms(js2uint64(argv[1])));
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
      ret = float2js(cam_zoom());
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
      system(str);
      break;

    case 144:
      ret = str2js(DATA_PATH);
      break;

    case 145:
      if (js2bool(argv[1])) window_makefullscreen(&mainwin);
      else window_unfullscreen(&mainwin);
      break;
    case 146:
      log_clear();
      break;

    case 147:
      YughWarn("EXITING");
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
     ret = num2js(js2gameobject(argv[1])->maxvelocity);
     break;
    case 153:
     cpBodySetTorque(js2gameobject(argv[1])->body, js2number(argv[2]));
     break;
    case 154:
      js2gameobject(argv[1])->maxangularvelocity = js2number(argv[2]);
      break;
    case 155:
      ret = num2js(js2gameobject(argv[1])->maxangularvelocity);
      break;

    case 156:
      js2gameobject(argv[1])->damping = js2number(argv[2]);
      break;
    case 157:
      ret = num2js(js2gameobject(argv[1])->damping);
      break;
    case 158:
      break;
    case 159:
      ret = vec2js(js2gameobject(argv[1])->gravity);
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
    case 163:
      str = JS_ToCString(js,argv[1]);
      str2 = JS_ToCString(js,argv[2]);
      ret = int2js(rename(str, str2));
      break;
    case 164:
      unplug_node(js2ptr(argv[1]));
      break;
    case 165:
//      ret = bool2js(sound_paused(js2ptr(argv[1])));
      break;
    case 166:
      str = js2str(argv[1]);
      str2 = js2str(argv[2]);
      ret = int2js(cp(str, str2));
      break;
    case 167:
      js2gameobject(argv[1])->gravity = js2vec2(argv[2]);
      break;
    case 168:
      js2gameobject(argv[1])->timescale = js2number(argv[2]);
      break;
    case 169:
      ret = num2js(js2gameobject(argv[1])->timescale);
      break;
    case 170:
      id2sprite(js2int(argv[1]))->emissive = js2color(argv[2]);
      break;
    case 171:
      ret = num2js(js2gameobject(argv[1])->drawlayer);
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
    case 175:
      ret = num2js((js2dsp_node(argv[1]))->gain);
      break;
    case 176:
      js2dsp_node(argv[1])->gain = js2number(argv[2]);
      break;
    case 177:
      plugin_node(js2dsp_node(argv[1]), js2dsp_node(argv[2]));
      break;
    case 178:
      ret = num2js(js2dsp_node(argv[1])->pan);
      break;
    case 179:
      js2dsp_node(argv[1])->pan=js2number(argv[2]);
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
      ((sound*)js2dsp_node(ret)->data)->hook = JS_DupValue(js,argv[2]);
      break;
    case 183:
      js2dsp_node(argv[1])->off = js2bool(argv[2]);
      break;
    case 184:
      js2dsp_node(argv[1])->pass = js2bool(argv[2]);
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
    case 193:
//      node_free(js2ptr(argv[1]));
      break;
    case 194:
      ret = bool2js(((sound*)js2dsp_node(argv[1])->data)->loop);
      break;
    case 195:
      ((sound*)js2dsp_node(argv[1])->data)->loop = js2bool(argv[2]);
      break;
    case 196:
      ret = num2js(((sound*)js2dsp_node(argv[1])->data)->frame);
      break;
    case 197:
      ret = num2js(((sound*)js2dsp_node(argv[1])->data)->data->frames);    
      break;
    case 198:
      ret = num2js(SAMPLERATE);
      break;
    case 199:
      ((sound*)js2dsp_node(argv[1])->data)->frame = js2number(argv[2]);
      break;
    case 200:
      ret = dsp_node2js(dsp_pitchshift(js2number(argv[1])));
      break;
    case 201:
      ret = num2js(((sound*)js2dsp_node(argv[1])->data)->timescale);
      break;
    case 202:
      YughWarn("%g", js2number(argv[2]));
      ((sound*)js2dsp_node(argv[1])->data)->timescale = js2number(argv[2]);
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
    case 208:
//      dag_set(js2gameobject(argv[1]), js2gameobject(argv[2]));
      break;
    case 209:
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
      ret = num2js(js2sprite(argv[1])->t.angle);
      break;
    case 218:
      js2sprite(argv[1])->t.angle = js2number(argv[2]);
      break;
    case 219:
      js2sprite(argv[1])->drawmode = js2number(argv[2]);
      break;
    case 220:
      ret = num2js(js2sprite(argv[1])->drawmode);
      break;
  }

  if (str)
    JS_FreeCString(js, str);

  if (str2)
    JS_FreeCString(js, str2);

  if (d1) free(d1);
  if (d2) free(d2);

  if (v1) arrfree(v1);

  if (!JS_IsNull(ret)) {
    return ret;
  }

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

  case 4:
    //       unregister_obj(obj);
    break;

  case 5:
    //       unregister_gui(c);
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

void gameobject_add_shape_collider(gameobject *go, struct callee c, struct phys2d_shape *shape) {
  struct shape_cb shapecb;
  shapecb.shape = shape;
  shapecb.cbs.begin = c;
  arrpush(go->shape_cbs, shapecb);
}

JSValue duk_register_collide(JSContext *js, JSValueConst this, int argc, JSValueConst *argv) {
  int cmd = js2int(argv[0]);
  gameobject *go = js2gameobject(argv[3]);
  struct callee c;
  c.fn = argv[1];
  c.obj = argv[2];

  switch (cmd) {
  case 0:
    go->cbs.begin = c;
    break;

  case 1:
    gameobject_add_shape_collider(go, c, js2ptr(argv[4]));
    break;

  case 2:
    phys2d_rm_go_handlers(go);
    break;

  case 3:
    go->cbs.separate = c;
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

  case 7:
    return gameobject2js(MakeGameobject());

  case 8:
    return JS_NewInt64(js, frame_fps());

  case 9: /* Clear the level out */
    break;

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

  case 3:
    break;

  case 4:
    cpBodyApplyImpulseAtWorldPoint(go->body, js2vec2(argv[2]).cp, cpBodyGetPosition(go->body));
    return JS_UNDEFINED;

  case 5:
    break;

  case 6:
    break;

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
      return num2js(circle->radius);

    case 3:
      return vec2js(circle->offset);
  }
  
  phys2d_applycircle(circle);
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

JSValue duk_profile(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
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
    return num2js(1.0);
    case 4:
    js2str(argv[1]);
    break;
    case 5:
    jsfloat2vec(argv[1]);
    break;
    case 6:
    return JS_NewStringLen(js, STRTEST, sizeof(*STRTEST));
  }
  return JS_UNDEFINED;
}

#define DUK_FUNC(NAME, ARGS) JS_SetPropertyStr(js, globalThis, #NAME, JS_NewCFunction(js, duk_##NAME, #NAME, ARGS));

void ffi_load() {
  globalThis = JS_GetGlobalObject(js);

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

  DUK_FUNC(profile, 2)

  JS_FreeValue(js,globalThis);

  JS_NewClassID(&js_ptr_id);
  JS_NewClass(JS_GetRuntime(js), js_ptr_id, &js_ptr_class);

  QJSCLASSPREP(dsp_node);
  QJSCLASSPREP(gameobject);
}
