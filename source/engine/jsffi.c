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
#include <chipmunk/chipmunk_unsafe.h>
#include "gui.h"

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

char *js2strdup(JSValue v) {
  char *str = JS_ToCString(js, v);
  char *ret = strdup(str);
  JS_FreeCString(js, str);
  return ret;
}

void sg_buffer_free(sg_buffer *b)
{
  sg_destroy_buffer(*b);
  free(b);
}

void cpShape_free(cpShape *s)
{
  if (cpSpaceContainsShape(space, s))
    cpSpaceRemoveShape(space, s);
  cpShapeFree(s);
}

void cpConstraint_free(cpConstraint *c)
{
  if (cpSpaceContainsConstraint(space, c))
    cpSpaceRemoveConstraint(space, c);
  cpConstraintFree(c);
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
QJSCLASS(window)
QJSCLASS(sg_buffer)
QJSCLASS(datastream)
QJSCLASS(cpShape)
QJSCLASS(cpConstraint)

static JSValue js_circle2d;
static JSValue js_poly2d;
static JSValue js_seg2d;

static JSValue js_pin;
static JSValue js_motor;
static JSValue js_ratchet;
static JSValue js_slide;
static JSValue js_pivot;
static JSValue js_gear;
static JSValue js_rotary;
static JSValue js_damped_rotary;
static JSValue js_damped_spring;
static JSValue js_groove;

#define DSP_PROTO(TYPE) \
static JSValue TYPE##_proto; \
static TYPE *js2##TYPE (JSValue v) { \
  dsp_node *node = js2dsp_node(v); \
  return node->data; \
} \

DSP_PROTO(bitcrush)
DSP_PROTO(delay)
DSP_PROTO(sound)
DSP_PROTO(compressor)
DSP_PROTO(phasor)
DSP_PROTO(adsr)

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

uint64_t js2uint64(JSValue v)
{
  uint64_t g;
  JS_ToIndex(js, &g, v);
  return g;
}

JSValue angle2js(double g) {
  return number2js(g*HMM_RadToTurn);
}
double js2angle(JSValue v) {
  double n = js2number(v);
  return n * HMM_TurnToRad;
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
  for (int i = 0; i < js_arrlen(ssbo); i++)
    bind.vs.storage_buffers[i] = *js2sg_buffer(js_getpropidx(ssbo,i));
  
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
JSC_GETSET(emitter, color, vec4)
JSC_GETSET(emitter, max, number)
JSC_GETSET(emitter, explosiveness, number)
JSC_GETSET(emitter, bounce, number)
JSC_GETSET(emitter, collision_mask, bitmask)
JSC_GETSET(emitter, die_after_collision, boolean)
JSC_GETSET(emitter, tumble, number)
JSC_GETSET(emitter, tumble_rate, number)
JSC_GETSET(emitter, persist, number)
JSC_GETSET(emitter, persist_var, number)
JSC_GETSET(emitter, warp_mask, bitmask)
JSC_CCALL(emitter_emit, emitter_emit(js2emitter(self), js2number(argv[0]), js2transform(argv[1])))
JSC_CCALL(emitter_step, emitter_step(js2emitter(self), js2number(argv[0]), js2transform(argv[1])))
JSC_CCALL(emitter_draw,
  sg_buffer *b = js2sg_buffer(js_getpropstr(self, "buffer"));
  emitter_draw(js2emitter(self), b);
  return number2js(arrlen(js2emitter(self)->verts));
)

JSC_CCALL(render_flushtext,
  sg_buffer *buf = js2sg_buffer(argv[0]);
  int amt = text_flush(buf);
  return number2js(amt);
)

JSC_CCALL(render_make_textssbo,
  sg_buffer *b = malloc(sizeof(*b));
  *b = sg_make_buffer(&(sg_buffer_desc){
    .type = SG_BUFFERTYPE_STORAGEBUFFER,
    .size = 4,
    .usage = SG_USAGE_STREAM
  });
  return sg_buffer2js(b);
)

JSC_CCALL(render_glue_pass,
  sg_begin_pass(&(sg_pass){
    .swapchain = sglue_swapchain(),
    .action = (sg_pass_action){
      .colors[0] = {
        .load_action = SG_LOADACTION_CLEAR,
        .clear_value = (sg_color){0,0,0,1}
      }
    }
  });
)

// Set the portion of the window to be rendered to
JSC_CCALL(render_viewport, sg_apply_viewportf(js2number(argv[0]), js2number(argv[1]), js2number(argv[2]), js2number(argv[3]), 0))
  
JSC_CCALL(render_commit, sg_commit())
JSC_CCALL(render_end_pass, sg_end_pass())

JSC_SCALL(render_text_size, ret = bb2js(text_bb(str, js2number(argv[1]), js2number(argv[2]), 1)))

HMM_Mat4 transform2view(transform *t)
{
  HMM_Vec3 look = HMM_AddV3(t->pos, transform_direction(t, vFWD));
  return HMM_LookAt_LH(t->pos, look, vUP);
}

HMM_Mat4 camera2projection(JSValue cam) {
  int ortho = js2boolean(js_getpropstr(cam, "ortho"));
  float near = js2number(js_getpropstr(cam, "near"));
  float far = js2number(js_getpropstr(cam, "far"));
  float fov = js2number(js_getpropstr(cam, "fov"))*HMM_DegToRad;
  HMM_Vec2 size = js2vec2(js_getpropstr(cam,"size"));

  if (ortho)
    return HMM_Orthographic_Metal(
      -size.x/2,
      size.x/2,
#ifdef SOKOL_GLCORE    //flipping orthographic Y if opengl
      size.y/2,
      -size.y/2,
#else
      -size.y/2,
      size.y/2,
#endif
      near,
      far
    );
  else
    return HMM_Perspective_Metal(fov, size.x/size.y, near, far);
}

JSC_CCALL(render_camera_screen2world,
  HMM_Mat4 view = transform2view(js2transform(js_getpropstr(argv[0], "transform")));
  view = HMM_InvGeneralM4(view);
  HMM_Vec4 p = js2vec4(argv[1]);
  return vec42js(HMM_MulM4V4(view, p));
)

JSC_CCALL(render_set_camera,
  JSValue cam = argv[0];
  globalview.p = camera2projection(argv[0]);
  globalview.v = transform2view(js2transform(js_getpropstr(cam, "transform")));
  globalview.vp = HMM_MulM4(globalview.p, globalview.v);
)

sg_shader_uniform_block_desc js2uniform_block(JSValue v)
{
  sg_shader_uniform_block_desc desc = {0};
    int slot = js2number(js_getpropstr(v, "slot"));
    desc.size = js2number(js_getpropstr(v, "size"));
    desc.layout = SG_UNIFORMLAYOUT_STD140;

    JSValue uniforms = js_getpropstr(v, "uniforms");
    for (int j = 0; j < js_arrlen(uniforms); j++) {
      JSValue uniform = js_getpropidx(uniforms, j);
      desc.uniforms[j].name = js2strdup(js_getpropstr(v, "struct_name"));
      desc.uniforms[j].array_count = js2number(js_getpropstr(uniform, "array_count"));
      desc.uniforms[j].type = SG_UNIFORMTYPE_FLOAT4;
    }

  return desc;
}

sg_shader js2shader(JSValue v)
{
  sg_shader_desc desc = {0};
  JSValue prog = v;
  desc.label = js2strdup(js_getpropstr(v, "name"));
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
  desc.vs.d3d11_target = "vs_4_0";
  desc.fs.d3d11_target = "ps_4_0";
  JSValue attrs = js_getpropstr(vs, "inputs");
  int atin = js_arrlen(attrs);
  for (int i = 0; i < atin; i++) {
    JSValue u = js_getpropidx(attrs, i);
    int slot = js2number(js_getpropstr(u, "slot"));    
    desc.attrs[slot].sem_name = js2strdup(js_getpropstr(u,"sem_name"));
    desc.attrs[slot].sem_index = js2number(js_getpropstr(u, "sem_index"));
  }
  
  JSValue vsu = js_getpropstr(vs, "uniform_blocks");
  for (int i = 0; i < js_arrlen(vsu); i++)
    desc.vs.uniform_blocks[i] = js2uniform_block(js_getpropidx(vsu, i));
  
  JSValue fsu = js_getpropstr(fs, "uniform_blocks");
  for (int i = 0; i < js_arrlen(fsu); i++)
    desc.fs.uniform_blocks[i] = js2uniform_block(js_getpropidx(fsu, i));
  
  JSValue imgs = js_getpropstr(fs, "images");
  for (int i = 0; i < js_arrlen(imgs); i++) {
    JSValue u = js_getpropidx(imgs, i);
    int slot = js2number(js_getpropstr(u, "slot"));
    desc.fs.images[i].used = true;
    desc.fs.images[i].multisampled = js2boolean(js_getpropstr(u, "multisampled"));
    desc.fs.images[i].image_type = SG_IMAGETYPE_2D;
    desc.fs.images[i].sample_type = SG_IMAGESAMPLETYPE_FLOAT;
  }
  
  JSValue samps = js_getpropstr(fs, "samplers");
  for (int i = 0; i < js_arrlen(samps); i++) {
    JSValue sampler = js_getpropidx(samps, i);
    desc.fs.samplers[0].used = true;
    desc.fs.samplers[0].sampler_type = SG_SAMPLERTYPE_FILTERING;
  }
  
  JSValue pairs = js_getpropstr(fs, "image_sampler_pairs");
  for (int i = 0; i < js_arrlen(pairs); i++) {
    JSValue pair = js_getpropidx(pairs, i);
    desc.fs.image_sampler_pairs[0].used = true;
    desc.fs.image_sampler_pairs[0].image_slot = js2number(js_getpropstr(pair, "slot"));
    desc.fs.image_sampler_pairs[0].sampler_slot = 0;
    desc.fs.image_sampler_pairs[0].glsl_name = js2strdup(js_getpropstr(pair, "name"));
  }
  
  JSValue ssbos = js_getpropstr(vs, "storage_buffers");
  for (int i = 0; i < js_arrlen(ssbos); i++) {
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
  if (js2boolean(js_getpropstr(argv[0], "indexed")))
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
  } else if (!JS_IsUndefined(argv[2]))
    m = transform2mat(*js2transform(argv[2]));

  sg_apply_uniforms(js2number(argv[0]), js2number(argv[1]), SG_RANGE_REF(m.e));
);

JSC_CCALL(render_setbind,
  sg_bindings bind = js2bind(argv[0]);
  sg_apply_bindings(&bind);
)

JSC_CCALL(render_make_t_ssbo,
  JSValue array = argv[0];
  HMM_Mat4 ms[js_arrlen(array)];
  for (int i = 0; i < js_arrlen(array); i++)
    ms[i] = transform2mat(*js2transform(js_getpropidx(array, i)));

  sg_buffer *rr = malloc(sizeof(sg_buffer));
  *rr = sg_make_buffer(&(sg_buffer_desc){
    .data = {
      .ptr = ms,
      .size = sizeof(HMM_Mat4)*js_arrlen(array),
    },
    .type = SG_BUFFERTYPE_STORAGEBUFFER,
    .usage = SG_USAGE_IMMUTABLE,
    .label = "transform buffer"
  });

  return sg_buffer2js(rr);
)

JSC_CCALL(render_spdraw,
  sg_draw(0,js2number(argv[0]),js2number(argv[1]));
)

JSC_CCALL(render_setpipeline,
  sg_pipeline p = {0};
  p.id = js2number(argv[0]);
  sg_apply_pipeline(p);
)

JSC_CCALL(render_screencolor,
  texture *t = calloc(sizeof(*t), 1);
  t->id = screencolor;
  return texture2js(&screencolor)
)

JSC_CCALL(render_imgui_new, gui_newframe(js2number(argv[0]),js2number(argv[1]),js2number(argv[2])); )
JSC_CCALL(render_gfx_gui, gfx_gui())
JSC_CCALL(render_imgui_end, gui_endframe())

JSC_CCALL(render_imgui_init, return gui_init(js))

static const JSCFunctionListEntry js_render_funcs[] = {
  MIST_FUNC_DEF(render, flushtext, 1),
  MIST_FUNC_DEF(render, camera_screen2world, 2),
  MIST_FUNC_DEF(render, make_textssbo, 0),
  MIST_FUNC_DEF(render, viewport, 4),
  MIST_FUNC_DEF(render, end_pass, 0),
  MIST_FUNC_DEF(render, commit, 0),
  MIST_FUNC_DEF(render, glue_pass, 0),
  MIST_FUNC_DEF(render, text_size, 3),
  MIST_FUNC_DEF(render, set_camera, 1),
  MIST_FUNC_DEF(render, pipeline, 1),
  MIST_FUNC_DEF(render, setuniv3, 2),
  MIST_FUNC_DEF(render, setuniv, 2),
  MIST_FUNC_DEF(render, spdraw, 2),
  MIST_FUNC_DEF(render, setbind, 1),
  MIST_FUNC_DEF(render, setuniproj, 2),
  MIST_FUNC_DEF(render, setuniview, 2),
  MIST_FUNC_DEF(render, setunivp, 2),
  MIST_FUNC_DEF(render, setunim4, 3),
  MIST_FUNC_DEF(render, setuniv2, 2),
  MIST_FUNC_DEF(render, setuniv4, 2),
  MIST_FUNC_DEF(render, setpipeline, 1),
  MIST_FUNC_DEF(render, screencolor, 0),
  MIST_FUNC_DEF(render, imgui_new, 3),
  MIST_FUNC_DEF(render, gfx_gui, 0),
  MIST_FUNC_DEF(render, imgui_end, 0),
  MIST_FUNC_DEF(render, imgui_init, 0),
  MIST_FUNC_DEF(render, make_t_ssbo, 1)
};

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

JSValue js_vector_dot(JSContext *js, JSValue self, int argc, JSValue *argv) { return number2js(HMM_DotV2(js2vec2(argv[0]), js2vec2(argv[1]))) ; };

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

JSC_CCALL(game_engine_start, engine_start(argv[0],argv[1], js2number(argv[2]), js2number(argv[3])))

static const JSCFunctionListEntry js_game_funcs[] = {
  MIST_FUNC_DEF(game, engine_start, 4),
};

JSC_CCALL(input_show_keyboard, sapp_show_keyboard(js2boolean(argv[0])))
JSValue js_input_keyboard_shown(JSContext *js, JSValue self) { return boolean2js(sapp_keyboard_shown()); }
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
JSC_CCALL(prosperon_window_render, openglRender(js2vec2(argv[0])))
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
  MIST_FUNC_DEF(prosperon, window_render, 1),
  MIST_FUNC_DEF(prosperon, guid, 0),
};

JSC_CCALL(time_now, 
  struct timeval ct;
  gettimeofday(&ct, NULL);
  return number2js((double)ct.tv_sec+(double)(ct.tv_usec/1000000.0));
)

JSValue js_time_computer_dst(JSContext *js, JSValue self) {
  time_t t = time(NULL);
  return boolean2js(localtime(&t)->tm_isdst);
}

JSValue js_time_computer_zone(JSContext *js, JSValue self) {
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

JSValue js_console_rec(JSContext *js, JSValue self, int argc, JSValue *argv)
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

JSValue js_io_slurpbytes(JSContext *js, JSValue self, int argc, JSValue *argv)
{
  char *f = js2str(argv[0]);
  size_t len;
  unsigned char *d = slurp_file(f,&len);
  if (!d) {
    JS_FreeCString(js,f);
    return JS_UNDEFINED;
  }
  JSValue ret = JS_NewArrayBufferCopy(js,d,len);
  JS_FreeCString(js,f);
  free(d);
  return ret;
}

JSValue js_io_slurp(JSContext *js, JSValue self, int argc, JSValue *argv)
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

JSValue js_io_slurpwrite(JSContext *js, JSValue self, int argc, JSValue *argv)
{
  char *f = js2str(argv[0]);
  size_t len;
  JSValue ret;
  if (JS_IsString(argv[1])) {
    char *data = JS_ToCStringLen(js, &len, argv[1]);
    ret = number2js(slurp_write(data, f, len));
    JS_FreeCString(js,data);
  } else {
    unsigned char *data = JS_GetArrayBuffer(js, &len, argv[1]);
    ret = number2js(slurp_write(data, f, len));
  }

  return ret;
}

JSValue js_io_chmod(JSContext *js, JSValue self, int argc, JSValue *argv)
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
JSC_SCALL(io_mod,
  #ifndef __EMSCRIPTEN__
  ret = number2js(file_mod_secs(str));
  #endif
)

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
    JS_DupValue(js,cpShape2js(shape)),
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
  js_setpropstr(o, "distance", number2js(info.distance));
  js_setpropstr(o, "point", vec22js((HMM_Vec2)info.point));
  js_setpropstr(o, "entity", JS_DupValue(js, shape2go(info.shape)->ref));
  return o;
}

JSC_CCALL(physics_point_query_nearest,
  cpPointQueryInfo info;
  cpShape *sh = cpSpacePointQueryNearest(space, js2vec2(argv[0]).cp, js2number(argv[1]), CP_SHAPE_FILTER_ALL, &info);
  if (!sh) return JS_UNDEFINED;
  return pointinfo2js(info);
)

void space_shape_fn(cpShape *shape, JSValue *fn) {
  JSValue v = *(JSValue*)cpShapeGetUserData(shape);
  script_call_sym(*fn, 1, &v);
}

JSC_CCALL(physics_eachshape,
  JSValue fn = argv[0];
  cpSpaceEachShape(space, space_shape_fn, &fn);
)

void space_body_fn(cpBody *body, JSValue *fn) {
  JSValue v = body2go(body)->ref;
  script_call_sym(*fn, 1, &v);
}

JSC_CCALL(physics_eachbody,
  JSValue fn = argv[0];
  cpSpaceEachBody(space, space_body_fn, &fn);
)

void space_constraint_fn(cpConstraint *c, JSValue *fn) {
  JSValue v = *(JSValue*)cpConstraintGetUserData(c);
  script_call_sym(*fn, 1, &v);
}

JSC_CCALL(physics_eachconstraint,
  JSValue fn = argv[0];
  cpSpaceEachConstraint(space, space_constraint_fn, &fn);
)

#define SPACE_GETSET(ENTRY, CPENTRY, TYPE) \
JSC_CCALL(physics_get_##ENTRY, return TYPE##2js(cpSpaceGet##CPENTRY (space))) \
JSValue js_physics_set_##ENTRY (JS_SETSIG) { \
  cpSpaceSet##CPENTRY(space, js2##TYPE(val));   return JS_UNDEFINED; \
} \

SPACE_GETSET(iterations, Iterations, number)
SPACE_GETSET(idle_speed, IdleSpeedThreshold, number)
SPACE_GETSET(collision_slop, CollisionSlop, number)
SPACE_GETSET(sleep_time, SleepTimeThreshold, number)
SPACE_GETSET(collision_bias, CollisionBias, number)
SPACE_GETSET(collision_persistence, CollisionPersistence, number)

static const JSCFunctionListEntry js_physics_funcs[] = {
  MIST_FUNC_DEF(physics, point_query, 3),
  MIST_FUNC_DEF(physics, point_query_nearest, 2),
  MIST_FUNC_DEF(physics, ray_query, 4),
  MIST_FUNC_DEF(physics, box_query, 2),
  MIST_FUNC_DEF(physics, closest_point, 3),
  MIST_FUNC_DEF(physics, make_damp, 0),
  MIST_FUNC_DEF(physics, make_gravity, 0),
  MIST_FUNC_DEF(physics, eachbody, 1),
  MIST_FUNC_DEF(physics, eachshape, 1),
  MIST_FUNC_DEF(physics, eachconstraint, 1),
  CGETSET_ADD(physics, iterations),
  CGETSET_ADD(physics, idle_speed),
  CGETSET_ADD(physics, sleep_time),
  CGETSET_ADD(physics, collision_slop),
  CGETSET_ADD(physics, collision_bias),
  CGETSET_ADD(physics, collision_persistence),
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
  CGETSET_ADD(emitter, color),
  CGETSET_ADD(emitter, tumble),
  CGETSET_ADD(emitter, tumble_rate),
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
JSC_CCALL(transform_move, transform_move(js2transform(self), js2vec3(argv[0])); )

JSC_CCALL(transform_lookat,
  HMM_Vec3 point = js2vec3(argv[0]);
  transform *go = js2transform(self);
  HMM_Mat4 m = HMM_LookAt_LH(go->pos, point, vUP);
  go->rotation = HMM_M4ToQ_LH(m);
)

JSC_CCALL(transform_rotate,
  HMM_Vec3 axis = js2vec3(argv[0]);
  transform *t = js2transform(self);
  HMM_Quat rot = HMM_QFromAxisAngle_LH(axis, js2angle(argv[1]));
  t->rotation = HMM_MulQ(t->rotation,rot);
)

JSC_CCALL(transform_angle,
  HMM_Vec3 axis = js2vec3(argv[0]);
  transform *t = js2transform(self);
  if (axis.x) return angle2js(HMM_Q_Roll(t->rotation));
  if (axis.y) return angle2js(HMM_Q_Pitch(t->rotation));
  if (axis.z) return angle2js(HMM_Q_Yaw(t->rotation));
  return angle2js(0);
)

JSC_CCALL(transform_direction,
  transform *t = js2transform(self);
  return vec32js(HMM_QVRot(js2vec3(argv[0]), t->rotation));
)

static const JSCFunctionListEntry js_transform_funcs[] = {
  CGETSET_ADD(transform, pos),
  CGETSET_ADD(transform, scale),
  CGETSET_ADD(transform, rotation),
  MIST_FUNC_DEF(transform, move, 1),
  MIST_FUNC_DEF(transform, rotate, 2),
  MIST_FUNC_DEF(transform, angle, 1),
  MIST_FUNC_DEF(transform, lookat, 1),
  MIST_FUNC_DEF(transform, direction, 1),
};

JSC_GETSET(dsp_node, pass, boolean)
JSC_GETSET(dsp_node, off, boolean)
JSC_GETSET(dsp_node, gain, number)
JSC_GETSET(dsp_node, pan, number)

JSC_CCALL(dsp_node_plugin, plugin_node(js2dsp_node(self), js2dsp_node(argv[0])))
JSC_CCALL(dsp_node_unplug, unplug_node(js2dsp_node(self)))

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
JSC_CCALL(sound_frames, return number2js(js2sound(self)->data->frames))

static const JSCFunctionListEntry js_sound_funcs[] = {
  CGETSET_ADD(sound, loop),
  CGETSET_ADD(sound, timescale),
  CGETSET_ADD(sound, frame),
  MIST_FUNC_DEF(sound, frames, 0),
};

static JSValue js_window_get_fullscreen(JSContext *js, JSValue self) { return boolean2js(js2window(self)->fullscreen); }
static JSValue js_window_set_fullscreen(JSContext *js, JSValue self, JSValue v) { window_setfullscreen(js2window(self), js2boolean(v)); return JS_UNDEFINED; }

static JSValue js_window_set_title(JSContext *js, JSValue self, JSValue v)
{
  window *w = js2window(self);
  if (w->title) JS_FreeCString(js, w->title);
  w->title = js2str(v);
  if (w->start)
    sapp_set_window_title(w->title);
  return JS_UNDEFINED;
}
JSC_CCALL(window_get_title, return str2js(js2window(self)->title))
JSC_CCALL(window_set_icon, window_seticon(&mainwin, js2texture(argv[0])))
JSC_GETSET(window, vsync, boolean)
JSC_GETSET(window, enable_clipboard, boolean)
JSC_GETSET(window, enable_dragndrop, boolean)
JSC_GETSET(window, high_dpi, boolean)
JSC_GETSET(window, sample_count, number)

static const JSCFunctionListEntry js_window_funcs[] = {
  CGETSET_ADD(window, fullscreen),
  CGETSET_ADD(window, title),
  CGETSET_ADD(window, vsync),
  CGETSET_ADD(window, enable_clipboard),
  CGETSET_ADD(window, enable_dragndrop),
  CGETSET_ADD(window, high_dpi),
  CGETSET_ADD(window, sample_count),
  MIST_FUNC_DEF(window, set_icon, 1)
};

JSValue js_gameobject_set_pos(JSContext *js, JSValue self, JSValue val) {
  cpBody *b = js2gameobject(self)->body;
  cpBodySetPosition(b, js2cvec2(val));
  if (cpBodyGetType(b) == CP_BODY_TYPE_STATIC)
    cpSpaceReindexShapesForBody(space, b);

  gameobject_apply(js2gameobject(self));
  return JS_UNDEFINED;
}
JSValue js_gameobject_get_pos(JSContext *js, JSValue self) { return cvec22js(cpBodyGetPosition(js2gameobject(self)->body)); }
JSValue js_gameobject_set_angle (JSContext *js, JSValue self, JSValue val) { cpBodySetAngle(js2gameobject(self)->body, js2angle(val)); return JS_UNDEFINED; }
JSValue js_gameobject_get_angle (JSContext *js, JSValue self) { return angle2js(cpBodyGetAngle(js2gameobject(self)->body)); }
JSC_GETSET_BODY(velocity, Velocity, cvec2)
JSValue js_gameobject_set_angularvelocity (JSContext *js, JSValue self, JSValue val) { cpBodySetAngularVelocity(js2gameobject(self)->body, js2angle(val)); return JS_UNDEFINED;}
JSValue js_gameobject_get_angularvelocity (JSContext *js, JSValue self) { return angle2js(cpBodyGetAngularVelocity(js2gameobject(self)->body)); }
JSC_GETSET_BODY(moi, Moment, number)
JSC_GETSET_BODY(torque, Torque, number)
JSC_CCALL(gameobject_impulse, cpBodyApplyImpulseAtWorldPoint(js2gameobject(self)->body, js2vec2(argv[0]).cp, cpBodyGetPosition(js2gameobject(self)->body)))
JSC_CCALL(gameobject_force, cpBodyApplyForceAtWorldPoint(js2gameobject(self)->body, js2vec2(argv[0]).cp, cpBodyGetPosition(js2gameobject(self)->body)))
JSC_CCALL(gameobject_force_local, cpBodyApplyForceAtLocalPoint(js2gameobject(self)->body, js2vec2(argv[0]).cp, js2vec2(argv[1]).cp))
JSC_GETSET_BODY(mass, Mass, number)
JSC_GETSET_BODY(phys, Type, number)
JSC_GETSET_APPLY(gameobject, layer, number)
JSC_GETSET(gameobject, damping, number)
JSC_GETSET(gameobject, timescale, number)
JSC_GETSET(gameobject, maxvelocity, number)
JSC_GETSET(gameobject, maxangularvelocity, number)
JSC_GETSET(gameobject, warp_mask, bitmask)
JSC_CCALL(gameobject_sleeping, return boolean2js(cpBodyIsSleeping(js2gameobject(self)->body)))
JSC_CCALL(gameobject_sleep, cpBodySleep(js2gameobject(self)->body))
JSC_CCALL(gameobject_wake, cpBodyActivate(js2gameobject(self)->body))

void body_shape_fn(cpBody *body, cpShape *shape, JSValue *fn) {
  JSValue v = *(JSValue*)cpShapeGetUserData(shape);
  script_call_sym(*fn, 1, &v);
}

JSC_CCALL(gameobject_eachshape,
  gameobject *g = js2gameobject(self);
  cpBodyEachShape(g->body, body_shape_fn, &argv[0]);
)

void body_constraint_fn(cpBody *body, cpConstraint *c, JSValue *fn) {
  JSValue v = *(JSValue*)cpConstraintGetUserData(c);
  script_call_sym(*fn, 1, &v);
}

JSC_CCALL(gameobject_eachconstraint,
  gameobject *g = js2gameobject(self);
  JSValue fn = argv[0];
  cpBodyEachConstraint(g->body, body_constraint_fn, &fn);
)

void body_arbiter_fn(cpBody *body, cpArbiter *arb, JSValue *fn) {
  JSValue v = arb2js(arb);
  script_call_sym(*fn, 1, &v);
}

JSC_CCALL(gameobject_eacharbiter,
  gameobject *g = js2gameobject(self);
  JSValue fn = argv[0];
  cpBodyEachArbiter(g->body, body_arbiter_fn, &fn);
)

JSC_CCALL(gameobject_transform,
  return JS_DupValue(js, transform2js(js2gameobject(self)->t));
)

static const JSCFunctionListEntry js_gameobject_funcs[] = {
  CGETSET_ADD(gameobject, pos),
  CGETSET_ADD(gameobject, angle),
  CGETSET_ADD(gameobject,mass),
  CGETSET_ADD(gameobject,damping),
  CGETSET_ADD(gameobject,timescale),
  CGETSET_ADD(gameobject,maxvelocity),
  CGETSET_ADD(gameobject,maxangularvelocity),
  CGETSET_ADD(gameobject,layer),
  CGETSET_ADD(gameobject,warp_mask),
  CGETSET_ADD(gameobject, velocity),
  CGETSET_ADD(gameobject, angularvelocity),
  CGETSET_ADD(gameobject, moi),
  CGETSET_ADD(gameobject, phys),
  CGETSET_ADD(gameobject, torque),
  MIST_FUNC_DEF(gameobject, impulse, 1),
  MIST_FUNC_DEF(gameobject, force, 1),
  MIST_FUNC_DEF(gameobject, force_local, 2),
  MIST_FUNC_DEF(gameobject, eachshape, 1),
  MIST_FUNC_DEF(gameobject, eachconstraint, 1),
  MIST_FUNC_DEF(gameobject, eacharbiter, 1),
  MIST_FUNC_DEF(gameobject, sleep, 0),
  MIST_FUNC_DEF(gameobject, sleeping, 0),
  MIST_FUNC_DEF(gameobject, wake, 0),
  MIST_FUNC_DEF(gameobject, transform, 0),
};

#define CC_GETSET(CPTYPE, CP, ENTRY, CPENTRY, TYPE) \
JSC_CCALL(CP##_get_##ENTRY, return TYPE##2js(CP##Get##CPENTRY (js2##CPTYPE(self)))) \
JSValue js_##CP##_set_##ENTRY (JSContext *js, JSValue self, JSValue val) { \
  CP##Set##CPENTRY (js2##CPTYPE(self), js2##TYPE (val)); \
  return JS_UNDEFINED; \
} \

#define CNST_GETSET(ENTRY,CPENTRY,TYPE) CC_GETSET(cpConstraint, cpConstraint, ENTRY, CPENTRY, TYPE)

CNST_GETSET(max_force, MaxForce, number)
CNST_GETSET(max_bias, MaxBias, number)
CNST_GETSET(error_bias, ErrorBias, number)
CNST_GETSET(collide_bodies, CollideBodies, boolean)

JSC_CCALL(cpConstraint_broken, return boolean2js(cpSpaceContainsConstraint(space, js2cpConstraint(self))))
JSC_CCALL(cpConstraint_break, 
  if (cpSpaceContainsConstraint(space, js2cpConstraint(self)))
    cpSpaceRemoveConstraint(space, js2cpConstraint(self));
)

JSC_CCALL(cpConstraint_bodyA,
  cpBody *b = cpConstraintGetBodyA(js2cpConstraint(self));
  gameobject *go = body2go(b);
  return JS_DupValue(js,gameobject2js(go));
)

JSC_CCALL(cpConstraint_bodyB,
  cpBody *b = cpConstraintGetBodyB(js2cpConstraint(self));
  gameobject *go = body2go(b);
  return JS_DupValue(js,gameobject2js(go));
)

static const JSCFunctionListEntry js_cpConstraint_funcs[] = {
  MIST_FUNC_DEF(cpConstraint, bodyA, 0),
  MIST_FUNC_DEF(cpConstraint, bodyB, 0),
  CGETSET_ADD(cpConstraint, max_force),
  CGETSET_ADD(cpConstraint, max_bias),
  CGETSET_ADD(cpConstraint, error_bias),
  CGETSET_ADD(cpConstraint, collide_bodies),
  MIST_FUNC_DEF(cpConstraint, broken, 0),
  MIST_FUNC_DEF(cpConstraint, break, 0)
};

JSValue prep_constraint(cpConstraint *c)
{
  JSValue ret = cpConstraint2js(c);
  JSValue *cb = malloc(sizeof(JSValue));
  *cb = ret;
  cpConstraintSetUserData(c, cb);
  return ret;
}

CC_GETSET(cpConstraint, cpPinJoint, distance, Dist, number)
CC_GETSET(cpConstraint, cpPinJoint, anchor_a, AnchorA, cvec2)
CC_GETSET(cpConstraint, cpPinJoint, anchor_b, AnchorB, cvec2)

static const JSCFunctionListEntry js_pin_funcs[] = {
  CGETSET_ADD(cpPinJoint, distance),
  CGETSET_ADD(cpPinJoint, anchor_a),
  CGETSET_ADD(cpPinJoint, anchor_b)
};

JSC_CCALL(joint_pin,
  if (JS_IsUndefined(argv[0])) return JS_DupValue(js,js_pin);
  ret = prep_constraint(cpSpaceAddConstraint(space, cpPinJointNew(js2gameobject(argv[0])->body, js2gameobject(argv[1])->body, cpvzero,cpvzero)));
  JS_SetPrototype(js, ret, js_pin);
)

CC_GETSET(cpConstraint, cpPivotJoint, anchor_a, AnchorA, cvec2)
CC_GETSET(cpConstraint, cpPivotJoint, anchor_b, AnchorB, cvec2)

static const JSCFunctionListEntry js_pivot_funcs[] = {
  CGETSET_ADD(cpPivotJoint, anchor_a),
  CGETSET_ADD(cpPivotJoint, anchor_b)
};

JSC_CCALL(joint_pivot,
  if (JS_IsUndefined(argv[0])) return JS_DupValue(js,js_pivot);
  ret = prep_constraint(cpSpaceAddConstraint(space, cpPivotJointNew(js2gameobject(argv[0])->body, js2gameobject(argv[1])->body,js2vec2(argv[2]).cp)));
  JS_SetPrototype(js, ret, js_pivot);
)

CC_GETSET(cpConstraint, cpGearJoint, phase, Phase, angle)
CC_GETSET(cpConstraint, cpGearJoint, ratio, Ratio, number)

static const JSCFunctionListEntry js_gear_funcs[] = {
  CGETSET_ADD(cpGearJoint, phase),
  CGETSET_ADD(cpGearJoint, ratio)
};
  
JSC_CCALL(joint_gear,
  if (JS_IsUndefined(argv[0])) return JS_DupValue(js,js_gear);
  ret = prep_constraint(cpSpaceAddConstraint(space, cpGearJointNew(js2gameobject(argv[0])->body, js2gameobject(argv[1])->body, js2number(argv[2]), js2number(argv[3]))));
  JS_SetPrototype(js, ret, js_gear);
)

CC_GETSET(cpConstraint, cpRotaryLimitJoint, min, Min, number)
CC_GETSET(cpConstraint, cpRotaryLimitJoint, max, Max, number)

static const JSCFunctionListEntry js_rotary_funcs[] = {
  CGETSET_ADD(cpRotaryLimitJoint, min),
  CGETSET_ADD(cpRotaryLimitJoint, max)
};

JSC_CCALL(joint_rotary,
  if (JS_IsUndefined(argv[0])) return JS_DupValue(js,js_rotary);
  ret = prep_constraint(cpSpaceAddConstraint(space, cpRotaryLimitJointNew(js2gameobject(argv[0])->body, js2gameobject(argv[1])->body, js2number(argv[2]), js2number(argv[3]))));
  JS_SetPrototype(js, ret, js_rotary);
)

CC_GETSET(cpConstraint, cpDampedRotarySpring, rest_angle, RestAngle, number)
CC_GETSET(cpConstraint, cpDampedRotarySpring, stiffness, Stiffness, number)
CC_GETSET(cpConstraint, cpDampedRotarySpring, damping, Damping, number)

static const JSCFunctionListEntry js_damped_rotary_funcs[] = {
  CGETSET_ADD(cpDampedRotarySpring, rest_angle),
  CGETSET_ADD(cpDampedRotarySpring, stiffness),
  CGETSET_ADD(cpDampedRotarySpring, damping)
};

JSC_CCALL(joint_damped_rotary,
  if (JS_IsUndefined(argv[0])) return JS_DupValue(js,js_damped_rotary);
  ret = prep_constraint(cpSpaceAddConstraint(space, cpDampedRotarySpringNew(js2gameobject(argv[0])->body, js2gameobject(argv[1])->body, js2number(argv[2]), js2number(argv[3]), js2number(argv[4]))));
  JS_SetPrototype(js, ret, js_damped_rotary);
)

CC_GETSET(cpConstraint, cpDampedSpring, anchor_a, AnchorA, cvec2)
CC_GETSET(cpConstraint, cpDampedSpring, anchor_b, AnchorB, cvec2)
CC_GETSET(cpConstraint, cpDampedSpring, rest_length, RestLength, number)
CC_GETSET(cpConstraint, cpDampedSpring, stiffness, Stiffness, number)
CC_GETSET(cpConstraint, cpDampedSpring, damping, Damping, number)

static const JSCFunctionListEntry js_damped_spring_funcs[] = {
  CGETSET_ADD(cpDampedSpring, anchor_a),
  CGETSET_ADD(cpDampedSpring, anchor_b),
  CGETSET_ADD(cpDampedSpring, rest_length),
  CGETSET_ADD(cpDampedSpring, stiffness),
  CGETSET_ADD(cpDampedSpring, damping)
};

JSC_CCALL(joint_damped_spring,
  if (JS_IsUndefined(argv[0])) return JS_DupValue(js,js_damped_spring);
  ret = prep_constraint(cpSpaceAddConstraint(space, cpDampedSpringNew(js2gameobject(argv[0])->body, js2gameobject(argv[1])->body, js2vec2(argv[2]).cp, js2vec2(argv[3]).cp, js2number(argv[4]), js2number(argv[5]), js2number(argv[6]))));
  JS_SetPrototype(js, ret, js_damped_spring);
)

CC_GETSET(cpConstraint, cpGrooveJoint, groove_a, GrooveA, cvec2)
CC_GETSET(cpConstraint, cpGrooveJoint, groove_b, GrooveB, cvec2)
CC_GETSET(cpConstraint, cpGrooveJoint, anchor_b, AnchorB, cvec2)

static const JSCFunctionListEntry js_groove_funcs[] = {
  CGETSET_ADD(cpGrooveJoint, groove_a),
  CGETSET_ADD(cpGrooveJoint, groove_b),
  CGETSET_ADD(cpGrooveJoint, anchor_b)
};

JSC_CCALL(joint_groove,
  if (JS_IsUndefined(argv[0])) return JS_DupValue(js,js_groove);
  ret = prep_constraint(cpSpaceAddConstraint(space, cpGrooveJointNew(js2gameobject(argv[0])->body, js2gameobject(argv[1])->body, js2vec2(argv[2]).cp, js2vec2(argv[3]).cp, js2vec2(argv[4]).cp)));
  JS_SetPrototype(js, ret, js_groove);
)

CC_GETSET(cpConstraint, cpSlideJoint, anchor_a, AnchorA, cvec2)
CC_GETSET(cpConstraint, cpSlideJoint, anchor_b, AnchorB, cvec2)
CC_GETSET(cpConstraint, cpSlideJoint, min, Min, number)
CC_GETSET(cpConstraint, cpSlideJoint, max, Max, number)

static const JSCFunctionListEntry js_slide_funcs[] = {
  CGETSET_ADD(cpSlideJoint, anchor_a),
  CGETSET_ADD(cpSlideJoint, anchor_b),
  CGETSET_ADD(cpSlideJoint, min),
  CGETSET_ADD(cpSlideJoint, max)
};

JSC_CCALL(joint_slide,
  if (JS_IsUndefined(argv[0])) return JS_DupValue(js,js_slide);
  ret = prep_constraint(cpSpaceAddConstraint(space, cpSlideJointNew(js2gameobject(argv[0])->body, js2gameobject(argv[1])->body, js2vec2(argv[2]).cp, js2vec2(argv[3]).cp, js2number(argv[4]), js2number(argv[5]))));
  JS_SetPrototype(js, ret, js_slide);
)

CC_GETSET(cpConstraint, cpRatchetJoint, angle, Angle, angle)
CC_GETSET(cpConstraint, cpRatchetJoint, phase, Phase, angle)
CC_GETSET(cpConstraint, cpRatchetJoint, ratchet, Ratchet, number)

static const JSCFunctionListEntry js_ratchet_funcs[] = {
  CGETSET_ADD(cpRatchetJoint, angle),
  CGETSET_ADD(cpRatchetJoint, phase),
  CGETSET_ADD(cpRatchetJoint, ratchet)
};

JSC_CCALL(joint_ratchet,
  if (JS_IsUndefined(argv[0])) return JS_DupValue(js,js_ratchet);
  ret = prep_constraint(cpSpaceAddConstraint(space, cpRatchetJointNew(js2body(argv[0]), js2body(argv[1]), js2number(argv[2]), js2number(argv[3]))));
  JS_SetPrototype(js, ret, js_ratchet);
)

CC_GETSET(cpConstraint, cpSimpleMotor, rate, Rate, angle)

static const JSCFunctionListEntry js_motor_funcs[] = {
  CGETSET_ADD(cpSimpleMotor, rate)
};

JSC_CCALL(joint_motor,
  if (JS_IsUndefined(argv[0])) return JS_DupValue(js,js_motor);
  ret = prep_constraint(cpSpaceAddConstraint(space, cpSimpleMotorNew(js2body(argv[0]), js2body(argv[1]), js2number(argv[2]))));
  JS_SetPrototype(js, ret, js_motor);
)

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

static JSValue dspnum;

JSC_CCALL(dspsound_limiter,
  ret = dsp_node2js(dsp_limiter(js2number(argv[0])));
  JS_SetPrototype(js, ret, dspnum);  
)

static const JSCFunctionListEntry js_compressor_funcs[] = {

};
JSC_CCALL(dspsound_compressor, return dsp_node2js(dsp_compressor()))

JSC_GETSET(bitcrush, sr, number)
JSC_GETSET(bitcrush, depth, number)
static const JSCFunctionListEntry js_bitcrush_funcs[] = {
  CGETSET_ADD(bitcrush, sr),
  CGETSET_ADD(bitcrush, depth)
};
JSC_CCALL(dspsound_crush,
  ret = dsp_node2js(dsp_bitcrush(js2number(argv[0]), js2number(argv[1])));
  JS_SetPrototype(js, ret, bitcrush_proto);
)

static const JSCFunctionListEntry js_adsr_funcs[] = {

};

static const JSCFunctionListEntry js_phasor_funcs[] = {

};

JSC_CCALL(dspsound_lpf, return dsp_node2js(dsp_lpf(js2number(argv[0]))))
JSC_CCALL(dspsound_hpf, return dsp_node2js(dsp_hpf(js2number(argv[0]))))

JSC_GETSET(delay, ms_delay, number)
JSC_GETSET(delay, decay, number)
static const JSCFunctionListEntry js_delay_funcs[] = {
  CGETSET_ADD(delay, ms_delay),
  CGETSET_ADD(delay, decay),
};
JSC_CCALL(dspsound_delay,
  ret = dsp_node2js(dsp_delay(js2number(argv[0]), js2number(argv[1])));
  JS_SetPrototype(js, ret, delay_proto);
)

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

JSC_CCALL(datastream_time, return number2js(plm_get_time(js2datastream(self)->plm)); )

JSC_CCALL(datastream_advance_frames,  ds_advanceframes(js2datastream(self), js2number(argv[0])))

JSC_CCALL(datastream_seek, ds_seek(js2datastream(self), js2number(argv[0])))

JSC_CCALL(datastream_advance, ds_advance(js2datastream(self), js2number(argv[0])))

JSC_CCALL(datastream_duration, return number2js(ds_length(js2datastream(self))))

JSC_CCALL(datastream_framerate, return number2js(plm_get_framerate(js2datastream(self)->plm)))

static const JSCFunctionListEntry js_datastream_funcs[] = {
  MIST_FUNC_DEF(datastream, time, 0),
  MIST_FUNC_DEF(datastream, advance_frames, 1),
  MIST_FUNC_DEF(datastream, seek, 1),
  MIST_FUNC_DEF(datastream, advance, 1),
  MIST_FUNC_DEF(datastream, duration, 0),
  MIST_FUNC_DEF(datastream, framerate, 0),
};

JSC_GET(texture, width, number)
JSC_GET(texture, height, number)
JSC_GET(texture, frames, number)
JSC_GET(texture, delays, ints)

JSC_SCALL(texture_save, texture_save(js2texture(self), str));

JSC_CCALL(texture_blit,
  texture *tex = js2texture(self);
  texture_blit(js2texture(self), js2texture(argv[0]), js2number(argv[1]), js2number(argv[2]), js2number(argv[3]), js2number(argv[4])))

static const JSCFunctionListEntry js_texture_funcs[] = {
  MIST_GET(texture, width),
  MIST_GET(texture, height),
  MIST_GET(texture, frames),
  MIST_GET(texture, delays),
  MIST_FUNC_DEF(texture, save, 1),
  MIST_FUNC_DEF(texture, blit, 5)
};

JSC_GETSET(font, linegap, number)
JSC_GET(font, height, number)

static const JSCFunctionListEntry js_font_funcs[] = {
  CGETSET_ADD(font, linegap),
  MIST_GET(font, height),
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

JSValue js_nota_encode(JSContext *js, JSValue self, int argc, JSValue *argv)
{
  if (argc < 1) return JS_UNDEFINED;
  
  JSValue obj = argv[0];
  char nota[1024*1024]; // 1MB
  char *e = js_do_nota_encode(obj, nota);

  return JS_NewArrayBufferCopy(js, (unsigned char*)nota, e-nota);
}

JSValue js_nota_decode(JSContext *js, JSValue self, int argc, JSValue *argv)
{
  if (argc < 1) return JS_UNDEFINED;

  size_t len;
  unsigned char *nota = JS_GetArrayBuffer(js, &len, argv[0]);
  JSValue ret;
  js_do_nota_decode(&ret, (char*)nota);
  return ret;
}

static const JSCFunctionListEntry js_nota_funcs[] = {
  MIST_FUNC_DEF(nota, encode, 1),
  MIST_FUNC_DEF(nota, decode, 1)
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
  return str2js(cwd);
}

JSC_SCALL(os_env, ret = str2js(getenv(str)))

JSValue js_os_sys(JSContext *js, JSValue self, int argc, JSValue *argv)
{
  #ifdef __linux__
  return str2js("linux");
  #elif defined(_WIN32) || defined(_WIN64)
  return str2js("windows");
  #elif defined(IOS)
  return str2js("ios");
  #elif defined(__APPLE__)
  return str2js("macos");
  #elif defined(__EMSCRIPTEN__)
  return str2js("web");
  #endif
  return JS_UNDEFINED;
}

JSC_CCALL(os_quit, quit();)
JSC_CCALL(os_exit, exit(js2number(argv[0]));)
JSC_CCALL(os_reindex_static, cpSpaceReindexStatic(space));
JSC_CCALL(os_gc, script_gc());
JSC_SSCALL(os_eval, ret = script_eval(str, str2))

JSC_CCALL(os_make_body,
  gameobject *g = MakeGameobject();
  g->t = js2transform(argv[0]);
  ret = gameobject2js(g);
  g->ref = ret;  
  return ret;
)

#define CP_GETSET(ENTRY, CPENTRY, TYPE) \
JSC_CCALL(cpShape_get_##ENTRY, return TYPE##2js(cpShapeGet##CPENTRY (js2cpShape(self)))) \
JSValue js_cpShape_set_##ENTRY (JSContext *js, JSValue self, JSValue val) { \
  cpShapeSet##CPENTRY (js2cpShape(self), js2##TYPE (val)); return JS_UNDEFINED; \
} \

CP_GETSET(sensor, Sensor, boolean)
CP_GETSET(friction, Friction, number)
CP_GETSET(elasticity, Elasticity, number)
CP_GETSET(mass, Mass, number)

JSC_CCALL(cpShape_area, return number2js(cpShapeGetArea(js2cpShape(self))))

JSC_CCALL(cpShape_get_surface_velocity, return vec22js((HMM_Vec2)cpShapeGetSurfaceVelocity(js2cpShape(self))))
JSValue js_cpShape_set_surface_velocity(JS_SETSIG) {
  HMM_Vec2 v = js2vec2(val);
  cpShapeSetSurfaceVelocity(js2cpShape(self), v.cp);
  return JS_UNDEFINED;
}

JSC_CCALL(cpShape_get_mask, return bitmask2js(cpShapeGetFilter(js2cpShape(self)).mask));
JSValue js_cpShape_set_mask (JS_SETSIG) {
  cpShapeFilter f = cpShapeGetFilter(js2cpShape(self));
  f.mask = js2bitmask(val);
  cpShapeSetFilter(js2cpShape(self), f);
  return JS_UNDEFINED;
}

JSC_CCALL(cpShape_get_category, return bitmask2js(cpShapeGetFilter(js2cpShape(self)).categories))
JSValue js_cpShape_set_category (JS_SETSIG) {
  cpShapeFilter f = cpShapeGetFilter(js2cpShape(self));
  f.categories = js2bitmask(val);
  cpShapeSetFilter(js2cpShape(self), f);
  return JS_UNDEFINED;
}

JSC_CCALL(cpShape_body,
  cpBody *b = cpShapeGetBody(js2cpShape(self));
  gameobject *go = body2go(b);
  return JS_DupValue(js,gameobject2js(go));
)

static void shape_query_fn(cpShape *shape, cpContactPointSet *points, JSValue *cb)
{
  JSValue v = *(JSValue*)cpShapeGetUserData(shape);
  script_call_sym(*cb, 1, &v);
}

JSC_CCALL(cpShape_query,
  cpSpaceShapeQuery(space, js2cpShape(self), shape_query_fn, &argv[0]);
)

static const JSCFunctionListEntry js_cpShape_funcs[] = {
  CGETSET_ADD(cpShape, sensor),
  CGETSET_ADD(cpShape, friction),
  CGETSET_ADD(cpShape, elasticity),
  CGETSET_ADD(cpShape, surface_velocity),
  CGETSET_ADD(cpShape, mask),
  CGETSET_ADD(cpShape, category),
  CGETSET_ADD(cpShape, mass),
  MIST_FUNC_DEF(cpShape, area, 0),
  MIST_FUNC_DEF(cpShape, body, 0),
  MIST_FUNC_DEF(cpShape, query, 1),
};

JSValue js_circle2d_set_radius(JSContext *js, JSValue self, JSValue val) {
  cpCircleShapeSetRadius(js2cpShape(self), js2number(val));
  return JS_UNDEFINED;
}
JSC_CCALL(circle2d_get_radius, return number2js(cpCircleShapeGetRadius(js2cpShape(self))))
JSValue js_circle2d_set_offset(JSContext *js, JSValue self, JSValue val) {
  cpCircleShapeSetOffset(js2cpShape(self), js2vec2(val).cp);
  return JS_UNDEFINED;
}
JSC_CCALL(circle2d_get_offset, return vec22js((HMM_Vec2)cpCircleShapeGetOffset(js2cpShape(self))))

static const JSCFunctionListEntry js_circle2d_funcs[] = {
  CGETSET_ADD(circle2d, radius),
  CGETSET_ADD(circle2d, offset)
};

JSValue prep_cpshape(cpShape *shape, gameobject *go)
{
  cpSpaceAddShape(space, shape);
  cpShapeSetFilter(shape, (cpShapeFilter) {
    .group = (cpGroup)go,
    .mask = CP_ALL_CATEGORIES,
    .categories = CP_ALL_CATEGORIES
  });
  cpShapeSetCollisionType(shape, (cpCollisionType)go);

  JSValue ret = cpShape2js(shape);
  JSValue *cb = malloc(sizeof(JSValue));
  *cb = ret;
  cpShapeSetUserData(shape, cb);
  return ret;
}

JSC_CCALL(os_make_circle2d,
  if (JS_IsUndefined(argv[0])) return JS_DupValue(js,js_circle2d);
  gameobject *go = js2gameobject(argv[0]);
  cpShape *shape = cpCircleShapeNew(go->body, 10, (cpVect){0,0});
  ret = prep_cpshape(shape,go);    
  JS_SetPrototype(js, ret, js_circle2d);
  return ret;
)

JSC_CCALL(poly2d_setverts,
  cpShape *s = js2cpShape(self);
  HMM_Vec2 *v = js2cpvec2arr(argv[0]);
  gameobject *go = shape2go(s);
  cpTransform t = {0};
  t.a = go->t->scale.x;
  t.b = 0;
  t.tx = 0;
  t.c = 0;
  t.d = go->t->scale.y;
  t.ty = 0;
  cpPolyShapeSetVerts(s, arrlen(v), v, t);
  arrfree(v);  
)

JSValue js_poly2d_set_radius(JSContext *js, JSValue self, JSValue val) {
  cpPolyShapeSetRadius(js2cpShape(self), js2number(val));
  return JS_UNDEFINED;
}
JSC_CCALL(poly2d_get_radius, return number2js(cpPolyShapeGetRadius(js2cpShape(self))))

static const JSCFunctionListEntry js_poly2d_funcs[] = {
  MIST_FUNC_DEF(poly2d, setverts, 2),
  CGETSET_ADD(poly2d, radius)
};

JSC_CCALL(os_make_poly2d,
  if (JS_IsUndefined(argv[0])) return JS_DupValue(js, js_poly2d);
  gameobject *go = js2gameobject(argv[0]);
  cpShape *shape = cpPolyShapeNew(go->body, 0, NULL, (cpTransform){0}, 0);
  ret = prep_cpshape(shape,go);  
  JS_SetPrototype(js, ret, js_poly2d);
  return ret;
)

JSC_CCALL(seg2d_set_endpoints,
  cpSegmentShapeSetEndpoints(js2cpShape(self), js2vec2(argv[0]).cp, js2vec2(argv[1]).cp);
  cpSpaceReindexShape(space, js2cpShape(self));
)

JSValue js_seg2d_set_radius(JSContext *js, JSValue self, JSValue val) {
  cpSegmentShapeSetRadius(js2cpShape(self), js2number(val));
  return JS_UNDEFINED;
}
JSC_CCALL(seg2d_get_radius, return number2js(cpSegmentShapeGetRadius(js2cpShape(self))))

JSC_CCALL(seg2d_set_neighbors,
  HMM_Vec2 prev = js2vec2(argv[0]);
  HMM_Vec2 next = js2vec2(argv[1]);
  cpSegmentShapeSetNeighbors(js2cpShape(self), prev.cp, next.cp);
)

JSC_CCALL(seg2d_a, return cvec22js(cpSegmentShapeGetA(js2cpShape(self))))
JSC_CCALL(seg2d_b, return cvec22js(cpSegmentShapeGetB(js2cpShape(self))))

static const JSCFunctionListEntry js_seg2d_funcs[] = {
  MIST_FUNC_DEF(seg2d, set_endpoints, 2),
  CGETSET_ADD(seg2d, radius),
  MIST_FUNC_DEF(seg2d, set_neighbors, 2),
  MIST_FUNC_DEF(seg2d, a, 0),
  MIST_FUNC_DEF(seg2d, b, 0),
};

JSC_CCALL(os_make_seg2d,
  if (JS_IsUndefined(argv[0])) return JS_DupValue(js,js_seg2d);
  gameobject *go = js2gameobject(argv[0]);
  cpShape *shape = cpSegmentShapeNew(go->body, (cpVect){0,0}, (cpVect){0,0}, 0);
  ret = prep_cpshape(shape,go);
  JS_SetPrototype(js, ret, js_seg2d);
  return ret;
)

JSC_SCALL(os_make_texture,
  ret = texture2js(texture_from_file(str));
  JS_SetPropertyStr(js, ret, "path", JS_DupValue(js,argv[0]));
)

JSC_CCALL(os_make_tex_data,
  ret = texture2js(texture_empty(js2number(argv[0]), js2number(argv[1]), js2number(argv[2])))
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
  
  return v;
}

JSValue material2js(material *m)
{
  JSValue v = JS_NewObject(js);
  if (m->diffuse)
    js_setpropstr(v, "diffuse", texture2js(m->diffuse));
    
  return v;
}

JSC_SCALL(os_make_model,
  model *m = model_make(str);
  if (arrlen(m->meshes) != 1) return JSUNDEF;
  mesh *me = m->meshes+0;
  
  JSValue v = JS_NewObject(js);
  js_setpropstr(v, "mesh", primitive2js(me->primitives+0));
  js_setpropstr(v, "material", material2js(me->primitives[0].mat));
  return v;
)

JSC_CCALL(os_make_emitter,
  emitter *e = make_emitter();
  ret = emitter2js(e);
  sg_buffer *b = malloc(sizeof(*b));
  *b = sg_make_buffer(&(sg_buffer_desc) {
    .type = SG_BUFFERTYPE_STORAGEBUFFER,
    .size = 4,
    .usage = SG_USAGE_STREAM
  });
  js_setpropstr(ret, "buffer", sg_buffer2js(b));
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
  MIST_FUNC_DEF(os, make_body, 1),
  MIST_FUNC_DEF(os, make_circle2d, 2),
  MIST_FUNC_DEF(os, make_poly2d, 2),
  MIST_FUNC_DEF(os, make_seg2d, 1),
  MIST_FUNC_DEF(os, make_texture, 1),
  MIST_FUNC_DEF(os, make_tex_data, 3),
  MIST_FUNC_DEF(os, make_font, 2),
  MIST_FUNC_DEF(os, make_model, 1),
  MIST_FUNC_DEF(os, make_transform, 0),
  MIST_FUNC_DEF(os, make_emitter, 0),
  MIST_FUNC_DEF(os, make_buffer, 1),
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
};

#include "steam.h"

#define JSSTATIC(NAME, PARENT) \
js_##NAME = JS_NewObject(js); \
JS_SetPropertyFunctionList(js, js_##NAME, js_##NAME##_funcs, countof(js_##NAME##_funcs)); \
JS_SetPrototype(js, js_##NAME, PARENT); \

void ffi_load() {
  JSUNDEF = JS_UNDEFINED;
  globalThis = JS_GetGlobalObject(js);
  
  QJSCLASSPREP(ptr);
  QJSCLASSPREP(sg_buffer);
  QJSGLOBALCLASS(os);
  
  QJSCLASSPREP_FUNCS(gameobject);
  QJSCLASSPREP_FUNCS(transform);
  QJSCLASSPREP_FUNCS(dsp_node);
  QJSCLASSPREP_FUNCS(emitter);
  QJSCLASSPREP_FUNCS(warp_gravity);
  QJSCLASSPREP_FUNCS(warp_damp);
  QJSCLASSPREP_FUNCS(texture);
  QJSCLASSPREP_FUNCS(font);
  QJSCLASSPREP_FUNCS(cpConstraint);
  QJSCLASSPREP_FUNCS(window);
  QJSCLASSPREP_FUNCS(datastream);
  QJSCLASSPREP_FUNCS(cpShape);

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
  QJSGLOBALCLASS(render);
  QJSGLOBALCLASS(physics);
  QJSGLOBALCLASS(vector);
  QJSGLOBALCLASS(spline);
  QJSGLOBALCLASS(joint);
  QJSGLOBALCLASS(dspsound);
  QJSGLOBALCLASS(performance);

  QJSGLOBALCLASS(poly2d);
  
  JS_SetPropertyStr(js, prosperon, "version", str2js(VER));
  JS_SetPropertyStr(js, prosperon, "revision", str2js(COM));
  JS_SetPropertyStr(js, globalThis, "window", window2js(&mainwin));
  JS_SetPropertyStr(js, globalThis, "texture", JS_DupValue(js,texture_proto));

  #ifndef NSTEAM
  JS_SetPropertyStr(js, globalThis, "steam", js_init_steam(js));
  #endif
  
  PREP_PARENT(bitcrush, dsp_node);
  PREP_PARENT(delay, dsp_node);
  PREP_PARENT(sound, dsp_node);
  PREP_PARENT(compressor, dsp_node);
  PREP_PARENT(phasor, dsp_node);
  PREP_PARENT(adsr, dsp_node);
   
  JSSTATIC(circle2d, cpShape_proto)
  JSSTATIC(poly2d, cpShape_proto)
  JSSTATIC(seg2d, cpShape_proto)  
  
  JSSTATIC(pin, cpConstraint_proto)
  JSSTATIC(motor, cpConstraint_proto)
  JSSTATIC(ratchet, cpConstraint_proto)
  JSSTATIC(slide, cpConstraint_proto)
  JSSTATIC(pivot, cpConstraint_proto)
  JSSTATIC(gear, cpConstraint_proto)
  JSSTATIC(rotary, cpConstraint_proto)
  JSSTATIC(damped_rotary, cpConstraint_proto)
  JSSTATIC(damped_spring, cpConstraint_proto)
  JSSTATIC(groove, cpConstraint_proto)
  
  JS_FreeValue(js,globalThis);  
}

