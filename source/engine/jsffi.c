#include "jsffi.h"

#include "script.h"
#include "font.h"
#include "gameobject.h"
#include "input.h"
#include "log.h"
#include "dsp.h"
#include "music.h"
#include "2dphysics.h"
#include "datastream.h"
#include "sound.h"
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
#include "par/par_streamlines.h"
#include "par/par_shapes.h"
#include "sokol_glue.h"

#if (defined(_WIN32) || defined(__WIN32__))
#include <direct.h>
#define mkdir(x,y) _mkdir(x)
#endif

static JSValue globalThis;

static JSClassID js_ptr_id;
static JSClassDef js_ptr_class = { "POINTER" };

static JSValue JSUNDEF;

JSValue str2js(const char *c, ...) {
  if (!c) return JS_UNDEFINED;
  char *result = NULL;
  va_list args;
  va_start(args, c);
  va_list args2;
  va_copy(args2,args);
  char buf[1+vsnprintf(NULL, 0, c, args)];
  va_end(args);
  vsnprintf(buf, sizeof buf, c, args2);
  va_end(args2);
  return JS_NewString(js, buf);
}

const char *js2str(JSValue v) {
  return JS_ToCString(js, v);
}

void sg_buffer_free(sg_buffer *b)
{
  sg_destroy_buffer(*b);
  free(b);
}

void jsfreestr(const char *s) { JS_FreeCString(js, s); }
QJSCLASS(gameobject)
QJSCLASS(transform)
QJSCLASS(emitter)
QJSCLASS(dsp_node)
QJSCLASS(texture)
QJSCLASS(font)
QJSCLASS(warp_gravity)
QJSCLASS(warp_damp)
QJSCLASS(material)
QJSCLASS(model)
QJSCLASS(window)
QJSCLASS(constraint)
QJSCLASS(sg_buffer)
QJSCLASS(datastream)

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

void js_setprop_str(JSValue obj, const char *prop, JSValue v) { JS_SetPropertyStr(js, obj, prop, v); }
JSValue js_getpropstr(JSValue v, const char *str)
{
  JSValue p = JS_GetPropertyStr(js,v,str);
  JS_FreeValue(js,p);
  return p;
}

void js_setpropstr(JSValue v, const char *str, JSValue p)
{
  JS_SetPropertyStr(js, v, str, p);
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
  if (isnan(g)) g = 0;
  return g;
}

void *js2ptr(JSValue v) { return JS_GetOpaque(v,js_ptr_id); }

JSValue ptr2js(void *ptr) {
  JSValue obj = JS_NewObjectClass(js, js_ptr_id);
  JS_SetOpaque(obj, ptr);
  return obj;
}

int js_arrlen(JSValue v) {
  if (JS_IsUndefined(v)) return 0;
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

JSValue quat2js(HMM_Quat q)
{
  JSValue arr = JS_NewArray(js);
  js_setprop_num(arr, 0, number2js(q.x));
  js_setprop_num(arr,1,number2js(q.y));
  js_setprop_num(arr,2,number2js(q.z));
  js_setprop_num(arr,3,number2js(q.w));
  return arr;
}

HMM_Vec4 js2vec4(JSValue v)
{
  HMM_Vec4 v4;
  for (int i = 0; i < 4; i++)
    v4.e[i] = js2number(js_getpropidx(v,i));
  return v4;
}

HMM_Quat js2quat(JSValue v)
{
  return js2vec4(v).quat;
}

JSValue vec42js(HMM_Vec4 v)
{
  JSValue array = JS_NewArray(js);
  for (int i = 0; i < 4; i++)
    js_setprop_num(array,i,number2js(v.e[i]));
  return array;
}

cpBitmask js2bitmask(JSValue v) {
  cpBitmask a;
  JS_ToUint32(js, &a, v);
  return a;
}
JSValue bitmask2js(cpBitmask mask) { return JS_NewUint32(js, mask); }

HMM_Vec2 *js2cpvec2arr(JSValue v) {
  HMM_Vec2 *arr = NULL;
  int n = js_arrlen(v);
  arrsetlen(arr,n);
  
  for (int i = 0; i < n; i++)
    arr[i] = js2vec2(js_getpropidx( v, i));
    
  return arr;
}

HMM_Vec3 *js2cpvec3arr(JSValue v)
{
  HMM_Vec3 *arr = NULL;
  int n = js_arrlen(v);
  arrsetlen(arr,n);
  for (int i = 0; i < n; i++)
    arr[i] = js2vec3(js_getpropidx(v,i));
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

JSValue vecarr2js(HMM_Vec2 *points, int n) {
  JSValue array = JS_NewArray(js);
  for (int i = 0; i < n; i++)
    js_setprop_num(array,i,vec22js(points[i]));
    
  return array;
}

int js_print_exception(JSValue v)
{
  if (!js) return 0;
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

sg_bindings js2bind(JSValue v)
{
  sg_bindings bind = {0};
  JSValue attrib = js_getpropstr(v, "attrib");
  for (int i = 0; i < js_arrlen(attrib); i++)
    bind.vertex_buffers[i] = *js2sg_buffer(js_getpropidx(attrib,i));
    
  JSValue index = js_getpropstr(v, "index");
  if (!JS_IsUndefined(index))
    bind.index_buffer = *js2sg_buffer(index);
  
  JSValue imgs = js_getpropstr(v, "images");
  for (int i = 0; i < js_arrlen(imgs); i++) {
    bind.fs.images[i] = js2texture(js_getpropidx(imgs, i))->id;
    bind.fs.samplers[i] = std_sampler; 
  }
  
  JSValue ssbo = js_getpropstr(v, "ssbo");
  for (int i = 0; i < js_arrlen(ssbo); i++) {
    bind.vs.storage_buffers[i] = *js2sg_buffer(js_getpropidx(ssbo,i));
  }
  
  return bind;
}

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
JSC_GETSET(emitter, bounce, number)
JSC_GETSET(emitter, collision_mask, bitmask)
JSC_GETSET(emitter, die_after_collision, boolean)
JSC_GETSET(emitter, persist, number)
JSC_GETSET(emitter, persist_var, number)
JSC_GETSET(emitter, warp_mask, bitmask)
JSC_CCALL(emitter_emit, emitter_emit(js2emitter(this), js2number(argv[0]), js2transform(argv[1])))
JSC_CCALL(emitter_step, emitter_step(js2emitter(this), js2number(argv[0]), js2transform(argv[1])))
JSC_CCALL(emitter_draw,
  emitter_draw(js2emitter(this));
  return number2js(arrlen(js2emitter(this)->verts));
)

JSC_CCALL(render_flushtext,
  return number2js(text_flush());
)

JSC_CCALL(render_end_pass,
  sg_end_pass();
  
  sg_begin_pass(&(sg_pass){
    .swapchain = sglue_swapchain(),
    .action = (sg_pass_action){
      .colors[0] = {
        .load_action = SG_LOADACTION_CLEAR,
        .clear_value = (sg_color){0,0,0,1}
      }
    }
  });
  sg_pipeline p = {0};
  
  switch(mainwin.mode) {
    case MODE_STRETCH:
      sg_apply_viewportf(0,0,mainwin.size.x,mainwin.size.y,0);
      break;
    case MODE_WIDTH:
      sg_apply_viewportf(0, mainwin.top, mainwin.size.x, mainwin.psize.y,0); // keep width
      break;
    case MODE_HEIGHT:
      sg_apply_viewportf(mainwin.left,0,mainwin.psize.x, mainwin.size.y,0); // keep height
      break;
    case MODE_KEEP:
      sg_apply_viewportf(0,0,mainwin.rendersize.x, mainwin.rendersize.y, 0); // no scaling
      break;
    case MODE_EXPAND:
      if (mainwin.aspect < mainwin.raspect)
        sg_apply_viewportf(0, mainwin.top, mainwin.size.x, mainwin.psize.y,0); // keep width
      else
        sg_apply_viewportf(mainwin.left,0,mainwin.psize.x, mainwin.size.y,0); // keep height
      break;
  }
  p.id = js2number(argv[0]);
  sg_apply_pipeline(p);
  sg_bindings bind = js2bind(argv[1]);
  bind.fs.images[0] = screencolor;
  bind.fs.samplers[0] = std_sampler;
  sg_apply_bindings(&bind);
  int c = js2number(js_getpropstr(argv[1], "count"));
  sg_draw(0,c,1);
  
  sg_end_pass();
  sg_commit();
)

JSC_CCALL(render_commit, sg_commit())

JSC_SCALL(render_text_size, ret = bb2js(text_bb(str, js2number(argv[1]), js2number(argv[2]), 1)))

JSC_CCALL(render_set_camera,
  JSValue cam = argv[0];
  int ortho = js2boolean(js_getpropstr(cam, "ortho"));
  float near = js2number(js_getpropstr(cam, "near"));
  float far = js2number(js_getpropstr(cam, "far"));
  float fov = js2number(js_getpropstr(cam, "fov"))*HMM_DegToRad;
  
  transform *t = js2transform(js_getpropstr(cam, "transform"));
  globalview.v = transform2mat(*t);
  HMM_Vec2 size = mainwin.mode == MODE_FULL ? mainwin.size : mainwin.rendersize;
  
  if (ortho)
    globalview.p = HMM_Orthographic_LH_ZO(
      -size.x/2,
      size.x/2,
      -size.y/2,
      size.y/2,
      near,
      far
    );
  else
    globalview.p = HMM_Perspective_Metal(fov, size.x/size.y, near, far);
    
  globalview.vp = HMM_MulM4(globalview.p, globalview.v);
)

sg_shader js2shader(JSValue v)
{
  sg_shader_desc desc = {0};
  JSValue prog = v;
  JSValue vs = js_getpropstr(prog, "vs");
  JSValue fs = js_getpropstr(prog, "fs");
  char *vsf = js2str(js_getpropstr(vs, "code"));
  char *fsf = js2str(js_getpropstr(fs, "code"));
  desc.vs.source = vsf;
  desc.fs.source = fsf;
  char *vsmain = js2str(js_getpropstr(vs, "entry_point"));
  char *fsmain = js2str(js_getpropstr(fs, "entry_point"));
  desc.vs.entry = vsmain;
  desc.fs.entry = fsmain;
  JSValue vsu = js_getpropstr(vs, "uniform_blocks");
  int unin = js_arrlen(vsu);
  for (int i = 0; i < unin; i++) {
    JSValue u = js_getpropidx(vsu, i);
    int slot = js2number(js_getpropstr(u, "slot"));
    desc.vs.uniform_blocks[slot].size = js2number(js_getpropstr(u, "size"));
    desc.vs.uniform_blocks[slot].layout = SG_UNIFORMLAYOUT_STD140;
  }
  
  JSValue fsu = js_getpropstr(fs, "uniform_blocks");
  unin = js_arrlen(fsu);
  for (int i = 0; i < unin; i++) {
    JSValue u = js_getpropidx(fsu, i);
    int slot = js2number(js_getpropstr(u, "slot"));
    desc.fs.uniform_blocks[slot].size = js2number(js_getpropstr(u, "size"));
    desc.fs.uniform_blocks[slot].layout = SG_UNIFORMLAYOUT_STD140;
  }
  
  JSValue imgs = js_getpropstr(fs, "images");
  unin = js_arrlen(imgs);
  for (int i = 0; i < unin; i++) {
    JSValue u = js_getpropidx(imgs, i);
    int slot = js2number(js_getpropstr(u, "slot"));
    desc.fs.images[i].used = true;
    desc.fs.images[i].multisampled = js2boolean(js_getpropstr(u, "multisampled"));
    desc.fs.images[i].image_type = SG_IMAGETYPE_2D;
    desc.fs.images[i].sample_type = SG_IMAGESAMPLETYPE_FLOAT;
  }
  
  JSValue samps = js_getpropstr(fs, "samplers");
  unin = js_arrlen(samps);
  for (int i = 0; i < unin; i++) {
    desc.fs.samplers[0].used = true;
    desc.fs.samplers[0].sampler_type = SG_SAMPLERTYPE_FILTERING;
  }
  
  JSValue pairs = js_getpropstr(fs, "image_sampler_pairs");
  unin = js_arrlen(pairs);
  for (int i = 0; i < unin; i++) {
    JSValue pair = js_getpropidx(pairs, i);
    desc.fs.image_sampler_pairs[0].used = true;
    desc.fs.image_sampler_pairs[0].image_slot = js2number(js_getpropstr(pair, "slot"));
    desc.fs.image_sampler_pairs[0].sampler_slot = 0;
  }
  
  JSValue ssbos = js_getpropstr(vs, "storage_buffers");
  unin = js_arrlen(ssbos);
  for (int i = 0; i < unin; i++) {
    desc.vs.storage_buffers[i].used = true;
    desc.vs.storage_buffers[i].readonly = true;
  }
  
  sg_shader sh = sg_make_shader(&desc);
  
  jsfreestr(vsf);
  jsfreestr(fsf);
  jsfreestr(vsmain);
  jsfreestr(fsmain);
  
  return sh;
}

sg_vertex_layout_state js2layout(JSValue v)
{
  sg_vertex_layout_state st = {0};
  JSValue inputs = js_getpropstr(js_getpropstr(v, "vs"), "inputs");
  for (int i = 0; i < js_arrlen(inputs); i++) {
    JSValue attr = js_getpropidx(inputs, i);
    int slot = js2number(js_getpropstr(attr, "slot"));
    int mat = js2number(js_getpropstr(attr, "mat"));
    st.attrs[slot].format = mat2type(mat);
    st.attrs[slot].buffer_index = slot;
  }
  
  return st;
}

JSC_CCALL(render_pipeline,
  sg_pipeline_desc p = {0};
  p.shader = js2shader(argv[0]);
  p.layout = js2layout(argv[0]);
  p.cull_mode = js2number(js_getpropstr(argv[0], "cull"));
  p.primitive_type = js2number(js_getpropstr(argv[0], "primitive"));
  //p.face_winding = js2number(js_getpropstr(argv[0], "face"));
  p.face_winding = 1;
  p.index_type = SG_INDEXTYPE_UINT16;
  if (js2boolean(js_getpropstr(argv[0], "blend")))
    p.colors[0].blend = blend_trans;
    
  int depth = js2boolean(js_getpropstr(argv[0], "depth"));
  if (depth) {
    p.depth.write_enabled = true;
    p.depth.compare = SG_COMPAREFUNC_LESS;
  }
  
  sg_pipeline pipe = sg_make_pipeline(&p);

  return number2js(pipe.id);
)

JSC_CCALL(render_setuniv,
  HMM_Vec4 f = {0};
  f.x = js2number(argv[2]);
  sg_apply_uniforms(js2number(argv[0]), js2number(argv[1]), SG_RANGE_REF(f));
)

JSC_CCALL(render_setuniv2,
  HMM_Vec4 v;
  v.xy = js2vec2(argv[2]);
  sg_apply_uniforms(js2number(argv[0]), js2number(argv[1]), SG_RANGE_REF(v.e));
)

JSC_CCALL(render_setuniv3,
  HMM_Vec4 f = {0};
  f.xyz = js2vec3(argv[2]);
  sg_apply_uniforms(js2number(argv[0]), js2number(argv[1]), SG_RANGE_REF(f.e));
)

JSC_CCALL(render_setuniv4,
  HMM_Vec4 v = {0};
  if (JS_IsArray(js, argv[2])) {
    for (int i = 0; i < js_arrlen(argv[2]); i++)
     v.e[i] = js2number(js_getpropidx(argv[2], i));
  } else
    v.x = js2number(argv[2]);

  sg_apply_uniforms(js2number(argv[0]), js2number(argv[1]), SG_RANGE_REF(v.e));
)

JSC_CCALL(render_setuniproj,
  sg_apply_uniforms(js2number(argv[0]), js2number(argv[1]), SG_RANGE_REF(globalview.p));
)

JSC_CCALL(render_setuniview,
  sg_apply_uniforms(js2number(argv[0]), js2number(argv[1]), SG_RANGE_REF(globalview.v));
)

JSC_CCALL(render_setunivp,
  sg_apply_uniforms(js2number(argv[0]), js2number(argv[1]), SG_RANGE_REF(globalview.vp));
)

JSC_CCALL(render_setunim4,
  HMM_Mat4 m = MAT1;
  if (JS_IsArray(js, argv[2])) {
    JSValue arr = argv[2];
    int n = js_arrlen(arr);
    if (n == 1)
      m = transform2mat(*js2transform(js_getpropidx(arr,0)));
    else {
      for (int i = 0; i < n; i++) {
        HMM_Mat4 p = transform2mat(*js2transform(js_getpropidx(arr, i)));
        m = HMM_MulM4(p,m);
      }
    }
  } else
    m = transform2mat(*js2transform(argv[2]));
    
  sg_apply_uniforms(js2number(argv[0]), js2number(argv[1]), SG_RANGE_REF(m.e));
);

JSC_CCALL(render_spdraw,
  sg_bindings bind = js2bind(argv[0]);
  sg_apply_bindings(&bind);
  int p = js2number(js_getpropstr(argv[0], "count"));
  int n = js2number(js_getpropstr(argv[0], "inst"));
  sg_draw(0,p,n);
)

JSC_CCALL(render_setpipeline,
  sg_pipeline p = {0};
  p.id = js2number(argv[0]);
  sg_apply_pipeline(p);
)

JSC_CCALL(render_text_ssbo,
  return sg_buffer2js(&text_ssbo);
)

static const JSCFunctionListEntry js_render_funcs[] = {
  MIST_FUNC_DEF(render, flushtext, 0),
  MIST_FUNC_DEF(render, end_pass, 2),
  MIST_FUNC_DEF(render, text_size, 3),
  MIST_FUNC_DEF(render, text_ssbo, 0),
  MIST_FUNC_DEF(render, set_camera, 1),
  MIST_FUNC_DEF(render, pipeline, 1),
  MIST_FUNC_DEF(render, setuniv3, 2),
  MIST_FUNC_DEF(render, setuniv, 2),
  MIST_FUNC_DEF(render, spdraw, 1),
  MIST_FUNC_DEF(render, setuniproj, 2),
  MIST_FUNC_DEF(render, setuniview, 2),
  MIST_FUNC_DEF(render, setunivp, 2),
  MIST_FUNC_DEF(render, setunim4, 3),
  MIST_FUNC_DEF(render, setuniv2, 2),
  MIST_FUNC_DEF(render, setuniv4, 2),
  MIST_FUNC_DEF(render, setpipeline, 1),
  MIST_FUNC_DEF(render, commit, 0),
};

JSC_CCALL(gui_flush, text_flush());
JSC_CCALL(gui_scissor, sg_apply_scissor_rect(js2number(argv[0]), js2number(argv[1]), js2number(argv[2]), js2number(argv[3]), 0))
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

JSC_CCALL(gui_font_set, font_set(js2font(argv[0])))

static const JSCFunctionListEntry js_gui_funcs[] = {
  MIST_FUNC_DEF(gui, flush, 0),
  MIST_FUNC_DEF(gui, scissor, 4),
  MIST_FUNC_DEF(gui, text, 6),
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

static const JSCFunctionListEntry js_game_funcs[] = {
  MIST_FUNC_DEF(game, engine_start, 2),
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

JSC_CCALL(prosperon_phys2d_step, phys2d_update(js2number(argv[0])))
JSC_CCALL(prosperon_window_render, openglRender(&mainwin))
JSC_CCALL(prosperon_guid,
  uint8_t bytes[16];
  for (int i = 0; i < 16; i++) bytes[i] = rand()%256;
  char uuid[37];
  snprintf(uuid, 37, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
  bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7],
  bytes[8], bytes[9], bytes[10], bytes[11], bytes[12], bytes[13], bytes[14], bytes[15]);
  return str2js(uuid);
)

static const JSCFunctionListEntry js_prosperon_funcs[] = {
  MIST_FUNC_DEF(prosperon, phys2d_step, 1),
  MIST_FUNC_DEF(prosperon, window_render, 0),
  MIST_FUNC_DEF(prosperon, guid, 0),
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

JSC_GETSET_GLOBAL(CHANNELS, number)
JSC_GETSET_GLOBAL(BUF_FRAMES, number)
JSC_GETSET_GLOBAL(SAMPLERATE, number)

static const JSCFunctionListEntry js_audio_funcs[] = {
  CGETSET_ADD_NAME(global, CHANNELS, channels),
  CGETSET_ADD_NAME(global, BUF_FRAMES, buffer_frames),
  CGETSET_ADD_NAME(global, SAMPLERATE, samplerate),
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

JSC_SCALL(io_pack_start, pack_start(str))
JSC_SCALL(io_pack_add, pack_add(str))
JSC_CCALL(io_pack_end, pack_end())
JSC_SCALL(io_mod, ret = number2js(file_mod_secs(str));)

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
  MIST_FUNC_DEF(io, pack_start, 1),
  MIST_FUNC_DEF(io, pack_add, 1),
  MIST_FUNC_DEF(io, pack_end, 0),
  MIST_FUNC_DEF(io, mod,1)
};

JSC_GETSET_GLOBAL(disabled_color, color)
JSC_GETSET_GLOBAL(sleep_color, color)
JSC_GETSET_GLOBAL(dynamic_color, color)
JSC_GETSET_GLOBAL(kinematic_color, color)
JSC_GETSET_GLOBAL(static_color, color)

static const JSCFunctionListEntry js_debug_funcs[] = {
  CGETSET_ADD(global, disabled_color),
  CGETSET_ADD(global, sleep_color),
  CGETSET_ADD(global, dynamic_color),
  CGETSET_ADD(global, kinematic_color),
  CGETSET_ADD(global, static_color),
};

JSC_CCALL(physics_sgscale,
  js2gameobject(argv[0])->scale.xy = js2vec2(argv[1]);
  gameobject_apply(js2gameobject(argv[0]));
  cpSpaceReindexShapesForBody(space, js2gameobject(argv[0])->body);
)

JSC_CCALL(physics_closest_point,
  void *v1 = js2cpvec2arr(argv[1]);
  JSValue ret = number2js(point2segindex(js2vec2(argv[0]), v1, js2number(argv[2])));
  arrfree(v1);
  return ret;
)

JSC_CCALL(physics_make_gravity, return warp_gravity2js(warp_gravity_make()))
JSC_CCALL(physics_make_damp, return warp_damp2js(warp_damp_make()))

void bb_query_fn(cpShape *shape, JSValue *cb)
{
  JSValue go = JS_DupValue(js,shape2go(shape)->ref);
  script_call_sym(*cb, 1, &go);
  JS_FreeValue(js, go);
}

JSC_CCALL(physics_box_query,
  HMM_Vec2 pos = js2vec2(argv[0]);
  HMM_Vec2 wh = js2vec2(argv[1]);
  cpBB bbox;
  bbox.l = pos.x-wh.x/2;
  bbox.r = pos.x+wh.x/2;
  bbox.t = pos.y+wh.y/2;
  bbox.b = pos.y-wh.y/2;
  cpSpaceBBQuery(space, bbox, CP_SHAPE_FILTER_ALL, bb_query_fn, &argv[3]);
  return ret;
)

static void shape_query_fn(cpShape *shape, cpContactPointSet *points, JSValue *cb)
{
  JSValue go = JS_DupValue(js,shape2go(shape)->ref);
  script_call_sym(*cb, 1, &go);
  JS_FreeValue(js, go);
}

JSC_CCALL(physics_shape_query,
  cpSpaceShapeQuery(space, ((struct phys2d_shape*)js2ptr(argv[0]))->shape, shape_query_fn, &argv[1]);
)

static void ray_query_fn(cpShape *shape, float t, cpVect n, float a, JSValue *cb)
{
  JSValue argv[3] = {
    JS_DupValue(js, shape2go(shape)->ref),
    vec22js((HMM_Vec2)n),
    number2js(a)
  };
  script_call_sym(*cb, 3, argv);
  for (int i = 0; i < 3; i++) JS_FreeValue(js, argv[i]);
}

JSC_CCALL(physics_ray_query,
  cpSpaceSegmentQuery(space, js2vec2(argv[0]).cp, js2vec2(argv[1]).cp, js2number(argv[2]), CP_SHAPE_FILTER_ALL, ray_query_fn, &argv[3]);
);

static void point_query_fn(cpShape *shape, float dist, cpVect point, JSValue *cb)
{
  JSValue argv[3] = {
    JS_DupValue(js, shape2go(shape)->ref),
    vec22js((HMM_Vec2)point),
    number2js(dist)
  };
  script_call_sym(*cb, 3, argv);
  for (int i = 0; i < 3; i++) JS_FreeValue(js, argv[i]);
}

JSC_CCALL(physics_point_query,
  cpSpacePointQuery(space, js2vec2(argv[0]).cp, js2number(argv[1]), CP_SHAPE_FILTER_ALL, point_query_fn, &argv[2]);
);

JSValue pointinfo2js(cpPointQueryInfo info)
{
  JSValue o = JS_NewObject(js);
  JS_SetPropertyStr(js, o, "distance", number2js(info.distance));
  JS_SetPropertyStr(js, o, "point", vec22js((HMM_Vec2)info.point));
  JS_SetPropertyStr(js, o, "entity", JS_DupValue(js, shape2go(info.shape)->ref));
  return o;
}

JSC_CCALL(physics_point_query_nearest,
  cpPointQueryInfo info;
  cpShape *sh = cpSpacePointQueryNearest(space, js2vec2(argv[0]).cp, js2number(argv[1]), CP_SHAPE_FILTER_ALL, &info);
  if (!sh) return JS_UNDEFINED;
  return pointinfo2js(info);
)

static const JSCFunctionListEntry js_physics_funcs[] = {
  MIST_FUNC_DEF(physics, sgscale, 2),
  MIST_FUNC_DEF(physics, point_query, 3),
  MIST_FUNC_DEF(physics, point_query_nearest, 2),
  MIST_FUNC_DEF(physics, ray_query, 4),
  MIST_FUNC_DEF(physics, box_query, 2),
  MIST_FUNC_DEF(physics, shape_query, 1),
  MIST_FUNC_DEF(physics, closest_point, 3),
  MIST_FUNC_DEF(physics, make_damp, 0),
  MIST_FUNC_DEF(physics, make_gravity, 0),
};

JSC_CCALL(model_draw_go,
  model_draw_go(js2model(this), js2gameobject(argv[0]))
);

static const JSCFunctionListEntry js_model_funcs[] = {
  MIST_FUNC_DEF(model, draw_go, 1)
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
  CGETSET_ADD(emitter, bounce),
  CGETSET_ADD(emitter, collision_mask),
  CGETSET_ADD(emitter, die_after_collision),
  CGETSET_ADD(emitter, persist),
  CGETSET_ADD(emitter, persist_var),
  CGETSET_ADD(emitter, warp_mask),
  MIST_FUNC_DEF(emitter, emit, 1),
  MIST_FUNC_DEF(emitter, step, 1),
  MIST_FUNC_DEF(emitter, draw, 1)
};

JSC_GETSET(transform, pos, vec3)
JSC_GETSET(transform, scale, vec3)
JSC_GETSET(transform, rotation, quat)
JSC_CCALL(transform_lookat,
  HMM_Vec3 point = js2vec3(argv[0]);
  transform *go = js2transform(this);
  HMM_Mat4 m = HMM_LookAt_LH(go->pos, point, vUP);
  go->rotation = HMM_M4ToQ_LH(m);
)

JSC_CCALL(transform_rotate,
  HMM_Vec3 axis = js2vec3(argv[0]);
  transform *t = js2transform(this);
  HMM_Quat rot = HMM_QFromAxisAngle_LH(axis, js2number(argv[1]));
  t->rotation = HMM_MulQ(rot, t->rotation);
)

static const JSCFunctionListEntry js_transform_funcs[] = {
  CGETSET_ADD(transform, pos),
  CGETSET_ADD(transform, scale),
  CGETSET_ADD(transform, rotation),
  MIST_FUNC_DEF(transform, rotate, 2),
  MIST_FUNC_DEF(transform, lookat, 1)
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
JSC_CCALL(sound_frames, return number2js(js2sound(this)->data->frames))

static const JSCFunctionListEntry js_sound_funcs[] = {
  CGETSET_ADD(sound, loop),
  CGETSET_ADD(sound, timescale),
  CGETSET_ADD(sound, frame),
  MIST_FUNC_DEF(sound, frames, 0),
};

static JSValue js_window_get_size(JSContext *js, JSValue this) { return vec22js(js2window(this)->size); }
static JSValue js_window_set_size(JSContext *js, JSValue this, JSValue v) {
  window *w = js2window(this);
  if (!w->start)
    w->size = js2vec2(v);
    
  return JS_UNDEFINED;
}
static JSValue js_window_get_rendersize(JSContext *js, JSValue this) {
  window *w = js2window(this);
  if (w->rendersize.x == 0 || w->rendersize.y == 0) return vec22js(w->size);
  return vec22js(w->rendersize);
}
static JSValue js_window_set_rendersize(JSContext *js, JSValue this, JSValue v) {
  js2window(this)->rendersize = js2vec2(v);
  return JS_UNDEFINED;
}
JSC_GETSET(window, mode, number)
static JSValue js_window_get_fullscreen(JSContext *js, JSValue this) { return boolean2js(js2window(this)->fullscreen); }
static JSValue js_window_set_fullscreen(JSContext *js, JSValue this, JSValue v) { window_setfullscreen(js2window(this), js2boolean(v)); return JS_UNDEFINED; }

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
JSC_GETSET(window, vsync, boolean)
JSC_GETSET(window, enable_clipboard, boolean)
JSC_GETSET(window, enable_dragndrop, boolean)
JSC_GETSET(window, high_dpi, boolean)
JSC_GETSET(window, sample_count, number)

static const JSCFunctionListEntry js_window_funcs[] = {
  CGETSET_ADD(window, size),
  CGETSET_ADD(window, rendersize),
  CGETSET_ADD(window, mode),
  CGETSET_ADD(window, fullscreen),
  CGETSET_ADD(window, title),
  CGETSET_ADD(window, vsync),
  CGETSET_ADD(window, enable_clipboard),
  CGETSET_ADD(window, enable_dragndrop),
  CGETSET_ADD(window, high_dpi),
  CGETSET_ADD(window, sample_count),
  MIST_FUNC_DEF(window, set_icon, 1)
};

JSValue js_gameobject_set_rpos(JSContext *js, JSValue this, JSValue val) {
  cpBody *b = js2gameobject(this)->body;
  cpBodySetPosition(b, js2cvec2(val));
  if (cpBodyGetType(b) == CP_BODY_TYPE_STATIC)
    cpSpaceReindexShapesForBody(space, b);
  return JS_UNDEFINED;
}
JSValue js_gameobject_get_rpos(JSContext *js, JSValue this) { return cvec22js(cpBodyGetPosition(js2gameobject(this)->body)); }
JSValue js_gameobject_set_rangle (JSContext *js, JSValue this, JSValue val) { cpBodySetAngle(js2gameobject(this)->body, HMM_TurnToRad*js2number(val)); return JS_UNDEFINED; }
JSValue js_gameobject_get_rangle (JSContext *js, JSValue this) { return number2js(HMM_RadToTurn*cpBodyGetAngle(js2gameobject(this)->body)); }
JSValue js_gameobject_get_rscale(JSContext *js, JSValue this) { return vec32js(js2gameobject(this)->scale); }
JSValue js_gameobject_set_rscale(JSContext *js, JSValue this, JSValue val) { js2gameobject(this)->scale = js2vec3(val); return JS_UNDEFINED; }
JSC_GETSET_BODY(velocity, Velocity, cvec2)
JSValue js_gameobject_set_angularvelocity (JSContext *js, JSValue this, JSValue val) { cpBodySetAngularVelocity(js2gameobject(this)->body, HMM_TurnToRad*js2number(val)); return JS_UNDEFINED;}
JSValue js_gameobject_get_angularvelocity (JSContext *js, JSValue this) { return number2js(HMM_RadToTurn*cpBodyGetAngularVelocity(js2gameobject(this)->body)); }
//JSC_GETSET_BODY(moi, Moment, number)
JSC_GETSET_BODY(torque, Torque, number)
JSC_CCALL(gameobject_impulse, cpBodyApplyImpulseAtWorldPoint(js2gameobject(this)->body, js2vec2(argv[0]).cp, cpBodyGetPosition(js2gameobject(this)->body)))
JSC_CCALL(gameobject_force, cpBodyApplyForceAtWorldPoint(js2gameobject(this)->body, js2vec2(argv[0]).cp, cpBodyGetPosition(js2gameobject(this)->body)))
JSC_CCALL(gameobject_force_local, cpBodyApplyForceAtLocalPoint(js2gameobject(this)->body, js2vec2(argv[0]).cp, js2vec2(argv[1]).cp))
JSC_GETSET_APPLY(gameobject, friction, number)
JSC_GETSET_APPLY(gameobject, elasticity, number)
JSC_GETSET_APPLY(gameobject, mass, number)
JSC_GETSET_APPLY(gameobject, phys, number)
JSC_GETSET_APPLY(gameobject, layer, number)
JSC_GETSET(gameobject, damping, number)
JSC_GETSET(gameobject, timescale, number)
JSC_GETSET(gameobject, maxvelocity, number)
JSC_GETSET(gameobject, maxangularvelocity, number)
JSC_GETSET(gameobject, warp_mask, bitmask)
JSC_GETSET(gameobject, categories, bitmask)
JSC_GETSET(gameobject, mask, bitmask)
JSC_CCALL(gameobject_selfsync, gameobject_apply(js2gameobject(this)))

static const JSCFunctionListEntry js_gameobject_funcs[] = {
  CGETSET_ADD(gameobject, friction),
  CGETSET_ADD(gameobject, elasticity),
  CGETSET_ADD(gameobject,mass),
  CGETSET_ADD(gameobject,damping),
  CGETSET_ADD(gameobject,timescale),
  CGETSET_ADD(gameobject,maxvelocity),
  CGETSET_ADD(gameobject,maxangularvelocity),
  CGETSET_ADD(gameobject,layer),
  CGETSET_ADD(gameobject,warp_mask),
  CGETSET_ADD(gameobject, categories),
  CGETSET_ADD(gameobject, mask),
  CGETSET_ADD(gameobject, velocity),
  CGETSET_ADD(gameobject, angularvelocity),
//  CGETSET_ADD(gameobject, moi),
  CGETSET_ADD(gameobject, phys),
  CGETSET_ADD(gameobject, torque),
  MIST_FUNC_DEF(gameobject, impulse, 1),
  MIST_FUNC_DEF(gameobject, force, 1),
  MIST_FUNC_DEF(gameobject, force_local, 2),
  MIST_FUNC_DEF(gameobject, selfsync, 0),
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
  MIST_FUNC_DEF(dspsound, midi, 2),
  MIST_FUNC_DEF(dspsound, mod, 1)
};

JSC_CCALL(datastream_time, return number2js(plm_get_time(js2datastream(this)->plm)); )

JSC_CCALL(datastream_advance_frames,  ds_advanceframes(js2datastream(this), js2number(argv[0])))

JSC_CCALL(datastream_seek, ds_seek(js2datastream(this), js2number(argv[0])))

JSC_CCALL(datastream_advance, ds_advance(js2datastream(this), js2number(argv[0])))

JSC_CCALL(datastream_duration, return number2js(ds_length(js2datastream(this))))

JSC_CCALL(datastream_framerate, return number2js(plm_get_framerate(js2datastream(this)->plm)))

static const JSCFunctionListEntry js_datastream_funcs[] = {
  MIST_FUNC_DEF(datastream, time, 0),
  MIST_FUNC_DEF(datastream, advance_frames, 1),
  MIST_FUNC_DEF(datastream, seek, 1),
  MIST_FUNC_DEF(datastream, advance, 1),
  MIST_FUNC_DEF(datastream, duration, 0),
  MIST_FUNC_DEF(datastream, framerate, 0),
};

JSC_CCALL(pshape_set_sensor, shape_set_sensor(js2ptr(argv[0]), js2boolean(argv[1])))
JSC_CCALL(pshape_get_sensor, return boolean2js(shape_get_sensor(js2ptr(argv[0]))))
JSC_CCALL(pshape_set_enabled, shape_enabled(js2ptr(argv[0]), js2boolean(argv[1])))
JSC_CCALL(pshape_get_enabled, return boolean2js(shape_is_enabled(js2ptr(argv[0]))))

static const JSCFunctionListEntry js_pshape_funcs[] = {
  MIST_FUNC_DEF(pshape, set_sensor, 2),
  MIST_FUNC_DEF(pshape, get_sensor, 1),
  MIST_FUNC_DEF(pshape, set_enabled, 2),
  MIST_FUNC_DEF(pshape, get_enabled, 1)
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

JSC_GETSET(font, linegap, number)
JSC_GET(font, height, number)

static const JSCFunctionListEntry js_font_funcs[] = {
  CGETSET_ADD(font, linegap),
  MIST_GET(font, height),
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
JSC_CCALL(performance_unpack_array,
  void *v = js2cpvec2arr(argv[0]);
  arrfree(v);
)
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
  #elif defined(IOS)
  return str2js("ios");
  #elif defined(__APPLE__)
  return str2js("macos");
  #elif define(__EMSCRIPTEN__)
  return str2js("web");
  #endif
  return JS_UNDEFINED;
}

JSC_CCALL(os_quit, quit();)
JSC_CCALL(os_exit, exit(js2number(argv[0]));)
JSC_CCALL(os_reindex_static, cpSpaceReindexStatic(space));
JSC_CCALL(os_gc, script_gc());
JSC_SSCALL(os_eval, ret = script_eval(str, str2))

JSC_CCALL(os_make_gameobject,
  ret = gameobject2js(MakeGameobject());
  JS_SetPropertyFunctionList(js, ret, js_gameobject_funcs, countof(js_gameobject_funcs));
  js2gameobject(ret)->ref = ret;
  return ret;
)
JSC_CCALL(os_make_circle2d,
  gameobject *go = js2gameobject(argv[0]);
  struct phys2d_circle *circle = Make2DCircle(go);
  JSValue circleval = JS_NewObject(js);
  js_setprop_str(circleval, "id", ptr2js(circle));
  js_setprop_str(circleval, "shape", ptr2js(&circle->shape));
  circle->shape.ref = JS_DupValue(js,argv[1]);
  return circleval;
)

JSC_CCALL(os_make_poly2d,
  gameobject *go = js2gameobject(argv[0]);
  struct phys2d_poly *poly = Make2DPoly(go);
  phys2d_poly_setverts(poly, NULL);
  JSValue polyval = JS_NewObject(js);
  js_setprop_str(polyval, "id", ptr2js(poly));
  js_setprop_str(polyval, "shape", ptr2js(&poly->shape));
  poly->shape.ref = JS_DupValue(js,argv[1]);
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
  edge->shape.ref = JS_DupValue(js, argv[1]);
  return edgeval;
)

JSC_SCALL(os_make_texture,
  ret = texture2js(texture_from_file(str));
  YughInfo("Made texture with %s", str);
  JS_SetPropertyStr(js, ret, "path", JS_DupValue(js,argv[0]));
)

JSC_CCALL(os_make_font,
  font *f = MakeFont(js2str(argv[0]), js2number(argv[1]));
  ret = font2js(f);
  js_setpropstr(ret, "texture", texture2js(f->texture));
)

JSC_CCALL(os_make_transform, return transform2js(make_transform()))

JSC_SCALL(os_system, return number2js(system(str)); )

#define PRIMCHECK(VAR, JS, VAL) \
if (VAR->VAL.id) js_setpropstr(JS, #VAL, sg_buffer2js(&VAR->VAL)); \

JSValue primitive2js(primitive *p)
{
  JSValue v = JS_NewObject(js);
  PRIMCHECK(p, v, pos)
  PRIMCHECK(p,v,norm)
  PRIMCHECK(p,v,uv)
  PRIMCHECK(p,v,bone)
  PRIMCHECK(p,v,weight)
  PRIMCHECK(p,v,color)
  PRIMCHECK(p,v,index)
  js_setpropstr(v, "count", number2js(p->idx_count));
  
  printf("new primitive with count %d\n", p->idx_count);
  
  return v;
}

JSC_SCALL(os_make_model,
  model *m = model_make(str);
  if (arrlen(m->meshes) != 1) return JSUNDEF;
  mesh *me = m->meshes+0;
  
  return primitive2js(me->primitives+0);
)
JSC_CCALL(os_make_emitter,
  emitter *e = make_emitter();
  ret = emitter2js(e);
  js_setpropstr(ret, "buffer", sg_buffer2js(&e->buffer));
)

JSC_CCALL(os_make_buffer,
  int type = js2number(argv[1]);
  float *b = malloc(sizeof(float)*js_arrlen(argv[0]));
  for (int i = 0; i < js_arrlen(argv[0]); i++)
    b[i] = js2number(js_getpropidx(argv[0],i));
    
  sg_buffer *p = malloc(sizeof(sg_buffer));
  
  switch(type) {
    case 0:
      *p = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(float)*js_arrlen(argv[0]),
        .data = b
      });
      break;
    case 1:
      *p = index_buffer(b, js_arrlen(argv[0]));
      break;
    case 2:
      *p = texcoord_floats(b, js_arrlen(argv[0]));
      break;
  }

  free(b);
  return sg_buffer2js(p);
)

JSC_CCALL(os_make_line_prim,
  JSValue prim = JS_NewObject(js);
  HMM_Vec2 *v = js2cpvec2arr(argv[0]);
  parsl_position par_v[arrlen(v)];
  for (int i = 0; i < arrlen(v); i++) {
    par_v[i].x = v[i].x;
    par_v[i].y = v[i].y;
  }
  
  parsl_context *par_ctx = parsl_create_context((parsl_config){
    .thickness = js2number(argv[1]),
    .flags= PARSL_FLAG_ANNOTATIONS,
    .u_mode = js2number(argv[2])
  });
  
  uint16_t spine_lens[] = {arrlen(v)};
  
  parsl_mesh *m = parsl_mesh_from_lines(par_ctx, (parsl_spine_list){
    .num_vertices = arrlen(v),
    .num_spines = 1,
    .vertices = par_v,
    .spine_lengths = spine_lens,
    .closed = js2boolean(argv[3])
  });
  HMM_Vec3 a_pos[m->num_vertices];
  
  for (int i = 0; i < m->num_vertices; i++) {
    a_pos[i].x = m->positions[i].x;
    a_pos[i].y = m->positions[i].y;
    a_pos[i].z = 0;
  }
  
  sg_buffer *pos = malloc(sizeof(*pos));
  *pos = sg_make_buffer(&(sg_buffer_desc){
    .data = SG_RANGE(a_pos),
  });
  js_setpropstr(prim, "pos", sg_buffer2js(pos));
  js_setpropstr(prim, "count", number2js(m->num_triangles*3));
  sg_buffer *idx = malloc(sizeof(*idx));
  *idx = par_idx_buffer(m->triangle_indices, m->num_triangles*3);
  js_setpropstr(prim, "index", sg_buffer2js(idx));
  
  float uv[m->num_vertices*2];
  for (int i = 0; i < m->num_vertices; i++) {
    uv[i*2] = m->annotations[i].u_along_curve;
    uv[i*2+1] = m->annotations[i].v_across_curve;
  }
  sg_buffer *buv = malloc(sizeof(*buv));
  *buv = texcoord_floats(uv, m->num_vertices*2);
  js_setpropstr(prim, "uv", sg_buffer2js(buv));
  
  return prim;
)

JSValue parmesh2js(par_shapes_mesh *m)
{
  JSValue obj = JS_NewObject(js);
  sg_buffer *pos = malloc(sizeof(*pos));
  *pos = float_buffer(m->points, 3*m->npoints);
  js_setpropstr(obj, "pos", sg_buffer2js(pos));
  
  if (m->tcoords) {
    sg_buffer *uv = malloc(sizeof(*uv));
    *uv = texcoord_floats(m->tcoords, 2*m->npoints);
    js_setpropstr(obj, "uv", sg_buffer2js(uv));
  }
  
  if (m->normals) {
    sg_buffer *norm = malloc(sizeof(*norm));
    *norm = normal_floats(m->normals, 3*m->npoints);
    js_setpropstr(obj, "norm", sg_buffer2js(norm));
  }
  
  sg_buffer *index = malloc(sizeof(*index));
  *index = sg_make_buffer(&(sg_buffer_desc){
    .data = {
      .ptr = m->triangles,
      .size = sizeof(*m->triangles)*3*m->ntriangles
    },
    .type = SG_BUFFERTYPE_INDEXBUFFER
  });
  js_setpropstr(obj, "index", sg_buffer2js(index));
  
  js_setpropstr(obj, "count", number2js(3*m->ntriangles));
  
  par_shapes_free_mesh(m);
  return obj;
}

JSC_CCALL(os_make_cylinder,
  return parmesh2js(par_shapes_create_cylinder(js2number(argv[0]), js2number(argv[1])));
)

JSC_CCALL(os_make_cone,
  return parmesh2js(par_shapes_create_cone(js2number(argv[0]), js2number(argv[1])));
)

JSC_CCALL(os_make_disk,
  return parmesh2js(par_shapes_create_parametric_disk(js2number(argv[0]), js2number(argv[1])));
)

JSC_CCALL(os_make_torus,
  return parmesh2js(par_shapes_create_torus(js2number(argv[0]), js2number(argv[1]), js2number(argv[2])));
)

JSC_CCALL(os_make_sphere,
  return parmesh2js(par_shapes_create_parametric_sphere(js2number(argv[0]), js2number(argv[1])));
)

JSC_CCALL(os_make_klein_bottle,
  return parmesh2js(par_shapes_create_klein_bottle(js2number(argv[0]), js2number(argv[1])));
)

JSC_CCALL(os_make_trefoil_knot,
  return parmesh2js(par_shapes_create_trefoil_knot(js2number(argv[0]), js2number(argv[1]), js2number(argv[2])));
)

JSC_CCALL(os_make_hemisphere,
  return parmesh2js(par_shapes_create_hemisphere(js2number(argv[0]), js2number(argv[1])));
)

JSC_CCALL(os_make_plane,
  return parmesh2js(par_shapes_create_plane(js2number(argv[0]), js2number(argv[1])));
)

JSC_SCALL(os_make_video,
  datastream *ds = ds_openvideo(str);
  ret = datastream2js(ds);
  texture *t = malloc(sizeof(texture));
  t->id = ds->img;
  js_setpropstr(ret, "texture", texture2js(t));
)

static const JSCFunctionListEntry js_os_funcs[] = {
  MIST_FUNC_DEF(os, cwd, 0),
  MIST_FUNC_DEF(os, env, 1),
  MIST_FUNC_DEF(os, sys, 0),
  MIST_FUNC_DEF(os, system, 1),
  MIST_FUNC_DEF(os, quit, 0),
  MIST_FUNC_DEF(os, exit, 1),
  MIST_FUNC_DEF(os, reindex_static, 0),
  MIST_FUNC_DEF(os, gc, 0),
  MIST_FUNC_DEF(os, eval, 2),
  MIST_FUNC_DEF(os, make_gameobject, 0),
  MIST_FUNC_DEF(os, make_circle2d, 2),
  MIST_FUNC_DEF(os, make_poly2d, 2),
  MIST_FUNC_DEF(os, make_edge2d, 2),
  MIST_FUNC_DEF(os, make_texture, 1),
  MIST_FUNC_DEF(os, make_font, 2),
  MIST_FUNC_DEF(os, make_model, 1),
  MIST_FUNC_DEF(os, make_transform, 0),
  MIST_FUNC_DEF(os, make_emitter, 0),
  MIST_FUNC_DEF(os, make_buffer, 1),
  MIST_FUNC_DEF(os, make_line_prim, 2),
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
};

#include "steam.h"

void ffi_load() {
  JSUNDEF = JS_UNDEFINED;
  globalThis = JS_GetGlobalObject(js);
  
  QJSCLASSPREP(ptr);
  
  QJSGLOBALCLASS(os);
  
  QJSCLASSPREP_FUNCS(gameobject);
  QJSCLASSPREP_FUNCS(transform);
  QJSCLASSPREP_FUNCS(dsp_node);
  QJSCLASSPREP_FUNCS(emitter);
  QJSCLASSPREP_FUNCS(warp_gravity);
  QJSCLASSPREP_FUNCS(warp_damp);
  QJSCLASSPREP_FUNCS(texture);
  QJSCLASSPREP_FUNCS(font);
  QJSCLASSPREP_FUNCS(constraint);
  QJSCLASSPREP_FUNCS(window);
  QJSCLASSPREP_FUNCS(model);
  QJSCLASSPREP_FUNCS(datastream);

  QJSGLOBALCLASS(nota);
  QJSGLOBALCLASS(input);
  QJSGLOBALCLASS(io);
  QJSGLOBALCLASS(prosperon);
  QJSGLOBALCLASS(time);
  QJSGLOBALCLASS(console);
  QJSGLOBALCLASS(audio);
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
  
  //JS_SetPropertyStr(js, globalThis, "steam", js_init_steam(js));
  
  sound_proto = JS_NewObject(js);
  JS_SetPropertyStr(js,globalThis, "sound_proto", sound_proto);
  JS_SetPropertyFunctionList(js, sound_proto, js_sound_funcs, countof(js_sound_funcs));
  JS_SetPrototype(js, sound_proto, dsp_node_proto);
  
  JS_FreeValue(js,globalThis);  
}
