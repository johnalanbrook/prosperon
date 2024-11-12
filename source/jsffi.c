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
#include <sokol/sokol_time.h>
#include <sokol/sokol_app.h>
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
#include "sokol_glue.h"
#include "sokol_gfx.h"
#include "sokol_log.h"
#include <stdint.h>
#include "timer.h"
#include <signal.h>
#include <dirent.h>

#include <fts.h>

#include "cute_aseprite.h"

JSValue number2js(JSContext *js, double g) { return JS_NewFloat64(js,g); }
double js2number(JSContext *js, JSValue v) {
  double g;
  JS_ToFloat64(js, &g, v);
  if (isnan(g)) g = 0;
  return g;
}

static inline JSValue boolean2js(JSContext *js, int b) { return JS_NewBool(js,b); }
static inline int js2boolean(JSContext *js, JSValue v) { return JS_ToBool(js,v); }

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
  ret.l = js2number(js,JS_GetPropertyStr(js,v,"l"));
  ret.b = js2number(js,JS_GetPropertyStr(js,v,"b"));
  ret.t = js2number(js,JS_GetPropertyStr(js,v,"t"));
  ret.r = js2number(js,JS_GetPropertyStr(js,v,"r"));
  return ret;
}

JSValue vec22js(JSContext *js,HMM_Vec2 v)
{
  JSValue array = JS_NewArray(js);
  JS_SetPropertyUint32(js, array,0,number2js(js,v.x));
  JS_SetPropertyUint32(js, array,1,number2js(js,v.y));
  return array;
}

const char *sapp_str[] = {
  "invalid",
  "key_down",
  "key_up",
  "char",
  "mouse_down",
  "mouse_up",
  "mouse_scroll",
  "mouse_move",
  "mouse_enter",
  "mouse_leave",
  "touches_began",
  "touches_moved",
  "touches_ended",
  "touches_cancelled",
  "resized",
  "iconified",
  "restored",
  "focused",
  "unfocused",
  "suspended",
  "resumed",
  "quit_requested",
  "clipboard_pasted",
  "files_dropped"
};
JSValue sapp_event2js(JSContext *js, sapp_event *e)
{
  JSValue event = JS_NewObject(js);
  JS_SetPropertyStr(js, event, "frame_count", JS_NewInt64(js, e->frame_count));
  JS_SetPropertyStr(js, event, "type", JS_NewString(js, sapp_str[e->type]));
  JS_SetPropertyStr(js,event,"mouse", vec22js(js, (HMM_Vec2){e->mouse_x,e->mouse_y}));
  JS_SetPropertyStr(js,event,"mouse_d", vec22js(js, (HMM_Vec2){e->mouse_dx,e->mouse_dy}));  
  JS_SetPropertyStr(js, event, "window_width", JS_NewFloat64(js, e->window_width));
  JS_SetPropertyStr(js, event, "window_height", JS_NewFloat64(js, e->window_height));
  JS_SetPropertyStr(js, event, "framebuffer_height", JS_NewFloat64(js, e->framebuffer_height));
  JS_SetPropertyStr(js, event, "framebuffer_height", JS_NewFloat64(js, e->framebuffer_height));    

  switch(e->type) {
    case SAPP_EVENTTYPE_MOUSE_SCROLL:
      JS_SetPropertyStr(js, event, "scroll", vec22js(js, (HMM_Vec2){e->scroll_x,e->scroll_y}));
      JS_SetPropertyStr(js, event, "modifiers", JS_NewBool(js, e->modifiers));                  
      break;
    case SAPP_EVENTTYPE_KEY_UP:
    case SAPP_EVENTTYPE_KEY_DOWN:
      JS_SetPropertyStr(js, event, "key_code", JS_NewInt64(js, e->key_code));
      JS_SetPropertyStr(js, event, "char_code", JS_NewUint32(js, e->char_code));
      JS_SetPropertyStr(js, event, "key_repeat", JS_NewBool(js, e->key_repeat));
      JS_SetPropertyStr(js, event, "modifiers", JS_NewBool(js, e->modifiers));                  
      break;
    case SAPP_EVENTTYPE_CHAR:
      JS_SetPropertyStr(js, event, "char_code", JS_NewUint32(js, e->char_code));
      JS_SetPropertyStr(js, event, "key_repeat", JS_NewBool(js, e->key_repeat));
      JS_SetPropertyStr(js, event, "modifiers", JS_NewBool(js, e->modifiers));            
      break;
    case SAPP_EVENTTYPE_TOUCHES_BEGAN:
    case SAPP_EVENTTYPE_TOUCHES_MOVED:
    case SAPP_EVENTTYPE_TOUCHES_ENDED:
      JS_SetPropertyStr(js, event, "num_touches", JS_NewUint32(js, e->num_touches));
      JSValue touches = JS_NewObject(js);
      for (int i = 0; i < e->num_touches; i++) {
        JSValue touch = JS_NewObject(js);
        JS_SetPropertyStr(js, touch,"id", JS_NewInt64(js,e->touches[i].identifier));
        JS_SetPropertyStr(js, touch, "pos", vec22js(js, (HMM_Vec2){e->touches[i].pos_x, e->touches[i].pos_y}));
        JS_SetPropertyStr(js,touch,"changed", JS_NewBool(js,e->touches[i].changed));
        JS_SetPropertyUint32(js,touches,i, touch);
      }
      JS_SetPropertyStr(js,event,"touches",touches);
      break;
    case SAPP_EVENTTYPE_MOUSE_UP:
    case SAPP_EVENTTYPE_MOUSE_DOWN:
      JS_SetPropertyStr(js, event, "mouse_button", JS_NewUint32(js, e->mouse_button));
      JS_SetPropertyStr(js, event, "modifiers", JS_NewBool(js, e->modifiers));                  
      break;
    case SAPP_EVENTTYPE_FILES_DROPPED:
      JSValue files = JS_NewObject(js);
      for (int i = 0; i < sapp_get_num_dropped_files(); i++)
        JS_SetPropertyUint32(js,files,i,JS_NewString(js,sapp_get_dropped_file_path(0)));
      JS_SetPropertyStr(js,event,"files",files);
      break;
    case SAPP_EVENTTYPE_CLIPBOARD_PASTED:
      JS_SetPropertyStr(js,event,"clipboard",JS_NewString(js,sapp_get_clipboard_string()));
      break;
  }
  return event;
}

char *js2strdup(JSContext *js, JSValue v) {
  const char *str = JS_ToCString(js, v);
  char *ret = strdup(str);
  JS_FreeCString(js, str);
  return ret;
}

void sg_buffer_free(JSRuntime *rt,sg_buffer *b)
{
  sg_destroy_buffer(*b);
  free(b);
}

void sg_pipeline_free(JSRuntime *rt,sg_pipeline *p)
{
  sg_destroy_pipeline(*p);
  free(p);
}

void skin_free(JSRuntime *rt,skin *sk) {
  arrfree(sk->invbind);
  free(sk);
}

#include "qjs_macros.h"

QJSCLASS(transform)
QJSCLASS(texture)
QJSCLASS(font)
//QJSCLASS(warp_gravity)
//QJSCLASS(warp_damp)
QJSCLASS(sg_buffer)
QJSCLASS(sg_pipeline)
QJSCLASS(datastream)
QJSCLASS(timer)
QJSCLASS(skin)

static inline HMM_Mat4 js2transform_mat(JSContext *js, JSValue v)
{
  transform *T = js2transform(js, v);
  transform *P = js2transform(js, JS_GetPropertyStr(js,v, "parent"));
  if (P) {
    HMM_Mat4 pm = transform2mat(P);
    HMM_Mat4 tm = transform2mat(T);
    
    return HMM_MulM4(pm, tm);
  }

  return transform2mat(T);
}

int js_arrlen(JSContext *js,JSValue v) {
  if (JS_IsUndefined(v)) return 0;
  int len;
  JS_ToInt32(js, &len, JS_GetPropertyStr(js, v, "length"));
  return len;
}

uint64_t js2uint64(JSContext *js,JSValue v)
{
  uint64_t g;
  JS_ToIndex(js, &g, v);
  return g;
}

JSValue angle2js(JSContext *js,double g) {
  return number2js(js,g*HMM_RadToTurn);
}

double js2angle(JSContext *js,JSValue v) {
  double n = js2number(js,v);
  return n * HMM_TurnToRad;
}

struct rgba js2color(JSContext *js,JSValue v) {
  JSValue c[4];
  for (int i = 0; i < 4; i++)
    c[i] = JS_GetPropertyUint32(js, v,i);
  float a = JS_IsUndefined(c[3]) ? 1.0 : js2number(js,c[3]);
  struct rgba color = {
    .r = js2number(js,c[0])*RGBA_MAX,
    .g = js2number(js,c[1])*RGBA_MAX,
    .b = js2number(js,c[2])*RGBA_MAX,
    .a = a*RGBA_MAX,
  };

  for (int i = 0; i < 4; i++)
    JS_FreeValue(js,c[i]);

  return color;
}

JSValue color2js(JSContext *js,struct rgba color)
{
  JSValue arr = JS_NewArray(js);
  JS_SetPropertyUint32(js, arr,0,number2js(js,(double)color.r/255));
  JS_SetPropertyUint32(js, arr,1,number2js(js,(double)color.g/255));  
  JS_SetPropertyUint32(js, arr,2,number2js(js,(double)color.b/255));
  JS_SetPropertyUint32(js, arr,3,number2js(js,(double)color.a/255));
  return arr;
}

HMM_Vec2 js2vec2(JSContext *js,JSValue v)
{
  HMM_Vec2 v2;
  v2.X = js2number(js,JS_GetPropertyUint32(js, v,0));
  v2.Y = js2number(js,JS_GetPropertyUint32(js, v,1));
  return v2;
}

HMM_Vec3 js2vec3(JSContext *js,JSValue v)
{
  HMM_Vec3 v3;
  v3.x = js2number(js,JS_GetPropertyUint32(js, v,0));
  v3.y = js2number(js,JS_GetPropertyUint32(js, v,1));
  v3.z = js2number(js,JS_GetPropertyUint32(js, v,2));
  return v3;
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
    v4.e[i] = js2number(js,JS_GetPropertyUint32(js, v,i));
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
    sum += pow(js2number(js,JS_GetPropertyUint32(js, v, i)), 2);

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
    arr[i] = js2vec2(js,JS_GetPropertyUint32(js,  v, i));
    
  return arr;
}

HMM_Vec3 *js2cpvec3arr(JSContext *js,JSValue v)
{
  HMM_Vec3 *arr = NULL;
  int n = js_arrlen(js,v);
  arrsetlen(arr,n);
  for (int i = 0; i < n; i++)
    arr[i] = js2vec3(js,JS_GetPropertyUint32(js, v,i));
  return arr;
}

HMM_Vec2 *jsfloat2vec(JSContext *js,JSValue v)
{
  size_t s;
  void *buf = JS_GetArrayBuffer(js, &s, v);
  HMM_Vec2 *arr = NULL;
  int n = s/2;
  n /= sizeof(float);
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
    
    const char *name = JS_ToCString(js, JS_GetPropertyStr(js,ex, "name"));
    const char *msg = JS_ToCString(js, JS_GetPropertyStr(js,ex, "message"));
    const char *stack = JS_ToCString(js, JS_GetPropertyStr(js,ex, "stack"));
    int line = 0;
    printf("%s :: %s\n%s", name, msg, stack);
//    mYughLog(LOG_SCRIPT, LOG_ERROR, line, "script", "%s :: %s\n%s", name, msg, stack);

    JS_FreeCString(js, name);
    JS_FreeCString(js, msg);
    JS_FreeCString(js, stack);
    JS_FreeValue(js,ex);
    
    return 1;
#endif
  return 0;
}

struct rect js2rect(JSContext *js,JSValue v) {
  struct rect rect;
  rect.x = js2number(js,JS_GetPropertyStr(js,v, "x"));
  rect.y = js2number(js,JS_GetPropertyStr(js,v, "y"));
  rect.w = js2number(js,JS_GetPropertyStr(js,v, "width"));
  rect.h = js2number(js,JS_GetPropertyStr(js,v, "height"));
  float anchor_x = js2number(js,JS_GetPropertyStr(js,v, "anchor_x"));
  float anchor_y = js2number(js,JS_GetPropertyStr(js,v, "anchor_y"));
  
  rect.y -= anchor_y*rect.h;
  rect.x -= anchor_x*rect.w;

  return rect;
}

JSValue rect2js(JSContext *js,struct rect rect) {
  JSValue obj = JS_NewObject(js);
  JS_SetPropertyStr(js, obj, "x", number2js(js,rect.x));
  JS_SetPropertyStr(js, obj, "y", number2js(js,rect.y));
  JS_SetPropertyStr(js, obj, "width", number2js(js,rect.w));
  JS_SetPropertyStr(js, obj, "height", number2js(js,rect.h));
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
*/
sg_bindings js2bind(JSContext *js,JSValue v)
{
  sg_bindings bind = {0};
  JSValue attrib = JS_GetPropertyStr(js,v, "attrib");

  for (int i = 0; i < js_arrlen(js,attrib); i++)
    bind.vertex_buffers[i] = *js2sg_buffer(js, JS_GetPropertyUint32(js, attrib,i));
    
  JSValue index = JS_GetPropertyStr(js,v, "index");
  if (!JS_IsUndefined(index))
    bind.index_buffer = *js2sg_buffer(js, index);
  
  JSValue imgs = JS_GetPropertyStr(js,v, "images");
  JSValue samplers = JS_GetPropertyStr(js,v, "samplers");
  for (int i = 0; i < js_arrlen(js,imgs); i++) {
    bind.fs.images[i] = js2texture(js, JS_GetPropertyUint32(js, imgs, i))->id;
    int use_std = JS_ToBool(js,JS_GetPropertyUint32(js, samplers, i));
    bind.fs.samplers[i] = use_std ? std_sampler : tex_sampler;
  }
  
  JSValue ssbo = JS_GetPropertyStr(js,v, "ssbo");
  for (int i = 0; i < js_arrlen(js,ssbo); i++)
    bind.vs.storage_buffers[i] = *js2sg_buffer(js, JS_GetPropertyUint32(js, ssbo,i));
  
  return bind;
}

JSC_CCALL(render_flushtext,
  sg_buffer *buf = js2sg_buffer(js, argv[0]);
  int amt = text_flush(buf);
  return number2js(js,amt);
)

JSC_CCALL(render_make_textssbo,
  sg_buffer *b = malloc(sizeof(*b));
  *b = sg_make_buffer(&(sg_buffer_desc){
    .type = SG_BUFFERTYPE_STORAGEBUFFER,
    .size = 4,
    .usage = SG_USAGE_STREAM
  });
  return sg_buffer2js(js, b);
)

JSC_CCALL(render_glue_pass,
  sg_begin_pass(&(sg_pass){
    .swapchain = sglue_swapchain(),
    .action = (sg_pass_action){
      .colors[0] = {
        .load_action = SG_LOADACTION_CLEAR,
        .clear_value = (sg_color){0,0,0,1}
      },
      .stencil = { .clear_value = 1
      }
    }
  });
)

// Set the portion of the window to be rendered to
JSC_CCALL(render_viewport,
  rect view = js2rect(js,argv[0]);
  sg_apply_viewportf(view.x, view.y,view.w,view.h, JS_ToBool(js,argv[1]));
)
  
JSC_CCALL(render_commit, sg_commit())
JSC_CCALL(render_end_pass, sg_end_pass())

HMM_Mat4 transform2view(transform *t)
{
  HMM_Vec3 look = HMM_AddV3(t->pos, transform_direction(t, vFWD));
  HMM_Mat4 ret = HMM_LookAt_RH(t->pos, look, vUP);
  ret = HMM_MulM4(ret, HMM_Scale(t->scale));
  return ret;
}

JSC_CCALL(render_camera_screen2world,
  HMM_Mat4 view = transform2view(js2transform(js, JS_GetPropertyStr(js,argv[0], "transform")));
  view = HMM_InvGeneralM4(view);
  HMM_Vec4 p = js2vec4(js, argv[1]);
  return vec42js(js, HMM_MulM4V4(view, p));
)

JSC_CCALL(render_set_projection_ortho,
  lrtb extents = js2lrtb(js, argv[0]);
  float near = js2number(js,argv[1]);
  float far = js2number(js,argv[2]);
  globalview.p = HMM_Orthographic_RH_ZO(
    extents.l,
    extents.r,
    extents.b,
    extents.t,
    near,
    far
  );
  globalview.vp = HMM_MulM4(globalview.p, globalview.v);
)

JSC_CCALL(render_set_projection_perspective,
  float fov = js2number(js,argv[0]);
  float aspect = js2number(js,argv[1]);
  float near = js2number(js,argv[2]);
  float far = js2number(js,argv[3]);
  globalview.p = HMM_Perspective_RH_NO(fov, aspect, near, far);
  globalview.vp = HMM_MulM4(globalview.p, globalview.v);
)

JSC_CCALL(render_set_view,
  globalview.v = transform2view(js2transform(js,argv[0]));
  globalview.vp = HMM_MulM4(globalview.p, globalview.v);
)

sg_shader_uniform_block_desc js2uniform_block(JSContext *js,JSValue v)
{
  sg_shader_uniform_block_desc desc = {0};
    int slot = js2number(js,JS_GetPropertyStr(js,v, "slot"));
    desc.size = js2number(js,JS_GetPropertyStr(js,v, "size"));
    desc.layout = SG_UNIFORMLAYOUT_STD140;

    JSValue uniforms = JS_GetPropertyStr(js,v, "uniforms");
    for (int j = 0; j < js_arrlen(js,uniforms); j++) {
      JSValue uniform = JS_GetPropertyUint32(js, uniforms, j);
      desc.uniforms[j].name = js2strdup(js, JS_GetPropertyStr(js,v, "struct_name"));
      desc.uniforms[j].array_count = js2number(js,JS_GetPropertyStr(js,uniform, "array_count"));
      desc.uniforms[j].type = SG_UNIFORMTYPE_FLOAT4;
    }

  return desc;
}

sg_shader js2shader(JSContext *js,JSValue v)
{
  sg_shader_desc desc = {0};
  JSValue prog = v;
  desc.label = js2strdup(js, JS_GetPropertyStr(js,v, "name"));
  JSValue vs = JS_GetPropertyStr(js,prog, "vs");
  JSValue fs = JS_GetPropertyStr(js,prog, "fs");
  const char *vsf = JS_ToCString(js, JS_GetPropertyStr(js,vs, "code"));
  const char *fsf = JS_ToCString(js, JS_GetPropertyStr(js,fs, "code"));
  desc.vs.source = vsf;
  desc.fs.source = fsf;
  const char *vsmain = JS_ToCString(js, JS_GetPropertyStr(js,vs, "entry_point"));
  const char *fsmain = JS_ToCString(js, JS_GetPropertyStr(js,fs, "entry_point"));
  desc.vs.entry = vsmain;
  desc.fs.entry = fsmain;
  desc.vs.d3d11_target = "vs_4_0";
  desc.fs.d3d11_target = "ps_4_0";
  JSValue attrs = JS_GetPropertyStr(js,vs, "inputs");
  int atin = js_arrlen(js,attrs);
  for (int i = 0; i < atin; i++) {
    JSValue u = JS_GetPropertyUint32(js, attrs, i);
    int slot = js2number(js,JS_GetPropertyStr(js,u, "slot"));    
    desc.attrs[slot].sem_name = js2strdup(js,JS_GetPropertyStr(js,u,"sem_name"));
    desc.attrs[slot].sem_index = js2number(js,JS_GetPropertyStr(js,u, "sem_index"));
  }
  
  JSValue vsu = JS_GetPropertyStr(js,vs, "uniform_blocks");
  for (int i = 0; i < js_arrlen(js,vsu); i++)
    desc.vs.uniform_blocks[i] = js2uniform_block(js,JS_GetPropertyUint32(js, vsu, i));
  
  JSValue fsu = JS_GetPropertyStr(js,fs, "uniform_blocks");
  for (int i = 0; i < js_arrlen(js,fsu); i++)
    desc.fs.uniform_blocks[i] = js2uniform_block(js,JS_GetPropertyUint32(js, fsu, i));
  
  JSValue imgs = JS_GetPropertyStr(js,fs, "images");
  for (int i = 0; i < js_arrlen(js,imgs); i++) {
    JSValue u = JS_GetPropertyUint32(js, imgs, i);
    int slot = js2number(js,JS_GetPropertyStr(js,u, "slot"));
    desc.fs.images[i].used = true;
    desc.fs.images[i].multisampled = JS_ToBool(js,JS_GetPropertyStr(js,u, "multisampled"));
    desc.fs.images[i].image_type = SG_IMAGETYPE_2D;
    desc.fs.images[i].sample_type = SG_IMAGESAMPLETYPE_FLOAT;
  }
  
  JSValue samps = JS_GetPropertyStr(js,fs, "samplers");
  for (int i = 0; i < js_arrlen(js,samps); i++) {
    JSValue sampler = JS_GetPropertyUint32(js, samps, i);
    desc.fs.samplers[0].used = true;
    desc.fs.samplers[0].sampler_type = SG_SAMPLERTYPE_FILTERING;
  }
  
  JSValue pairs = JS_GetPropertyStr(js,fs, "image_sampler_pairs");
  for (int i = 0; i < js_arrlen(js,pairs); i++) {
    JSValue pair = JS_GetPropertyUint32(js, pairs, i);
    desc.fs.image_sampler_pairs[0].used = true;
    desc.fs.image_sampler_pairs[0].image_slot = js2number(js,JS_GetPropertyStr(js,pair, "slot"));
    desc.fs.image_sampler_pairs[0].sampler_slot = 0;
    desc.fs.image_sampler_pairs[0].glsl_name = js2strdup(js,JS_GetPropertyStr(js,pair, "name"));
  }
  
  JSValue ssbos = JS_GetPropertyStr(js,vs, "storage_buffers");
  for (int i = 0; i < js_arrlen(js,ssbos); i++) {
    desc.vs.storage_buffers[i].used = true;
    desc.vs.storage_buffers[i].readonly = true;
  }
  
  sg_shader sh = sg_make_shader(&desc);
  
  JS_FreeCString(js,vsf);
  JS_FreeCString(js,fsf);
  JS_FreeCString(js,vsmain);
  JS_FreeCString(js,fsmain);
  
  return sh;
}

static int mat2type(int mat)
{
  switch(mat) {
    case MAT_POS:
    case MAT_NORM:
      return SG_VERTEXFORMAT_FLOAT3;
    case MAT_PPOS:
    case MAT_WH:
    case MAT_ST:
      return SG_VERTEXFORMAT_FLOAT2;
    case MAT_UV:
      return SG_VERTEXFORMAT_USHORT2N;
    case MAT_TAN:
      return SG_VERTEXFORMAT_UINT10_N2;
    case MAT_BONE:
      return SG_VERTEXFORMAT_UBYTE4;
    case MAT_WEIGHT:
    case MAT_COLOR:
      return SG_VERTEXFORMAT_UBYTE4N;
    case MAT_ANGLE:
    case MAT_SCALE:
      return SG_VERTEXFORMAT_FLOAT;
  };
  
  return -1;
}

sg_vertex_layout_state js2vertex_layout(JSContext *js,JSValue v)
{
  sg_vertex_layout_state st = {0};
  JSValue inputs = JS_GetPropertyStr(js,JS_GetPropertyStr(js,v, "vs"), "inputs");
  for (int i = 0; i < js_arrlen(js,inputs); i++) {
    JSValue attr = JS_GetPropertyUint32(js, inputs, i);
    int slot = js2number(js,JS_GetPropertyStr(js,attr, "slot"));
    int mat = js2number(js,JS_GetPropertyStr(js,attr, "mat"));
    st.attrs[slot].format = mat2type(mat);
    st.attrs[slot].buffer_index = slot;
  }
  
  return st;
}

sg_depth_state js2depth(JSContext *js,JSValue v)
{
  sg_depth_state depth = {0};
  depth.compare = js2number(js,JS_GetPropertyStr(js,v, "compare"));
  depth.write_enabled = JS_ToBool(js,JS_GetPropertyStr(js,v, "write"));
  depth.bias = js2number(js,JS_GetPropertyStr(js,v, "bias"));
  depth.bias_slope_scale = js2number(js,JS_GetPropertyStr(js,v, "bias_slope_scale"));
  depth.bias_clamp = js2number(js,JS_GetPropertyStr(js,v, "bias_clamp"));
  return depth;
}

sg_stencil_face_state js2face_state(JSContext *js,JSValue v)
{
  sg_stencil_face_state face = {0};
  face.compare = js2number(js,JS_GetPropertyStr(js,v, "compare"));
  face.fail_op = js2number(js,JS_GetPropertyStr(js,v, "fail"));
  face.depth_fail_op = js2number(js,JS_GetPropertyStr(js,v, "depth_fail"));
  face.pass_op = js2number(js,JS_GetPropertyStr(js,v, "pass_op"));
  return face;
}

sg_stencil_state js2stencil(JSContext *js,JSValue v)
{
  sg_stencil_state stencil = {0};
  stencil.enabled = JS_ToBool(js,JS_GetPropertyStr(js,v, "enabled"));
  stencil.read_mask = JS_ToBool(js,JS_GetPropertyStr(js,v, "read")) ? 0xFF : 0x00;
  stencil.write_mask = JS_ToBool(js,JS_GetPropertyStr(js,v, "write")) ? 0xFF : 0x00;
  stencil.front = js2face_state(js,JS_GetPropertyStr(js,v, "front"));
  stencil.back = js2face_state(js,JS_GetPropertyStr(js,v, "back"));
  stencil.ref = js2number(js,JS_GetPropertyStr(js,v, "ref"));
  return stencil;
}

#define GETNUMVALUE(STRUCT, NAME) STRUCT.NAME = js2number(js,JS_GetPropertyStr(js,v, #NAME));
sg_blend_state js2blend(JSContext *js,JSValue v)
{
  sg_blend_state blend = {0};
  blend.enabled = JS_ToBool(js,JS_GetPropertyStr(js,v, "enabled"));
  GETNUMVALUE(blend, src_factor_rgb);
  GETNUMVALUE(blend, dst_factor_rgb);
  GETNUMVALUE(blend, op_rgb);
  GETNUMVALUE(blend, src_factor_alpha);
  GETNUMVALUE(blend, dst_factor_alpha);
  GETNUMVALUE(blend, op_alpha);
  return blend;
}

JSC_CCALL(render_make_pipeline,
  sg_pipeline_desc p = {0};
  p.shader = js2shader(js,argv[0]);
  p.layout = js2vertex_layout(js,argv[0]);
  p.primitive_type = js2number(js,JS_GetPropertyStr(js,argv[0], "primitive"));
  if (JS_ToBool(js,JS_GetPropertyStr(js,argv[0], "indexed")))
    p.index_type = SG_INDEXTYPE_UINT16;
  
  JSValue pipe = argv[1];
  p.primitive_type = js2number(js,JS_GetPropertyStr(js,pipe, "primitive"));
  p.cull_mode = js2number(js,JS_GetPropertyStr(js,pipe, "cull"));
  p.face_winding = js2number(js,JS_GetPropertyStr(js,pipe, "face"));
  p.colors[0].blend = js2blend(js,JS_GetPropertyStr(js,pipe, "blend"));
  p.colors[0].write_mask = js2number(js,JS_GetPropertyStr(js,pipe, "write_mask"));
  p.alpha_to_coverage_enabled = JS_ToBool(js,JS_GetPropertyStr(js,pipe, "alpha_to_coverage"));
  p.depth = js2depth(js,JS_GetPropertyStr(js,pipe, "depth"));
  p.stencil = js2stencil(js,JS_GetPropertyStr(js,pipe, "stencil"));

  sg_pipeline *g = malloc(sizeof(*g));
  *g = sg_make_pipeline(&p);
  return sg_pipeline2js(js,g);
)

JSC_CCALL(render_setuniv,
  HMM_Vec4 f = {0};
  f.x = js2number(js,argv[2]);
  sg_apply_uniforms(js2number(js,argv[0]), js2number(js,argv[1]), SG_RANGE_REF(f));
)

JSC_CCALL(render_setuniv2,
  HMM_Vec4 v = {0};
  v.xy = js2vec2(js,argv[2]);
  sg_apply_uniforms(js2number(js,argv[0]), js2number(js,argv[1]), SG_RANGE_REF(v.e));
)

JSC_CCALL(render_setuniv3,
  HMM_Vec4 f = {0};
  f.xyz = js2vec3(js,argv[2]);
  sg_apply_uniforms(js2number(js,argv[0]), js2number(js,argv[1]), SG_RANGE_REF(f.e));
)

JSC_CCALL(render_setuniv4,
  HMM_Vec4 v = {0};
  if (JS_IsArray(js, argv[2])) {
    for (int i = 0; i < js_arrlen(js,argv[2]); i++)
     v.e[i] = js2number(js,JS_GetPropertyUint32(js, argv[2], i));
  } else
    v.x = js2number(js,argv[2]);

  sg_apply_uniforms(js2number(js,argv[0]), js2number(js,argv[1]), SG_RANGE_REF(v.e));
)

JSC_CCALL(render_setuniproj,
  sg_apply_uniforms(js2number(js,argv[0]), js2number(js,argv[1]), SG_RANGE_REF(globalview.p.e));
)

JSC_CCALL(render_setuniview,
  sg_apply_uniforms(js2number(js,argv[0]), js2number(js,argv[1]), SG_RANGE_REF(globalview.v.e));
)

JSC_CCALL(render_setunivp,
  sg_apply_uniforms(js2number(js,argv[0]), js2number(js,argv[1]), SG_RANGE_REF(globalview.vp.e));
)

JSC_CCALL(render_setunibones,
  skin *sk = js2skin(js, argv[2]);
  sg_apply_uniforms(js2number(js,argv[0]), js2number(js,argv[1]), SG_RANGE_REF(sk->binds));
)

JSC_CCALL(render_setunim4,
  HMM_Mat4 m = MAT1;
  if (JS_IsArray(js, argv[2])) {
    JSValue arr = argv[2];
    int n = js_arrlen(js,arr);
    if (n == 1)
      m = transform2mat(js2transform(js,JS_GetPropertyUint32(js, arr,0)));
    else {
      for (int i = 0; i < n; i++) {
        HMM_Mat4 p = transform2mat(js2transform(js,JS_GetPropertyUint32(js, arr, i)));
        m = HMM_MulM4(p,m);
      }
    }
  } else if (!JS_IsUndefined(argv[2]))
    m = transform2mat(js2transform(js,argv[2]));

  sg_apply_uniforms(js2number(js,argv[0]), js2number(js,argv[1]), SG_RANGE_REF(m.e));
);

JSC_CCALL(render_setbind,
  sg_bindings bind = js2bind(js,argv[0]);
  sg_apply_bindings(&bind);
)

typedef struct particle_ss {
  HMM_Mat4 model;
  HMM_Vec4 color;
} particle_ss;

JSC_CCALL(render_make_particle_ssbo,
  JSValue array = argv[0];
  size_t size = js_arrlen(js,array)*(sizeof(particle_ss));
  sg_buffer *b = js2sg_buffer(js, argv[1]);
  if (!b) return JS_UNDEFINED;
  particle_ss ms[js_arrlen(js,array)];

  if (sg_query_buffer_will_overflow(*b, size)) {
    sg_buffer_desc desc = sg_query_buffer_desc(*b);
    sg_destroy_buffer(*b);

    *b = sg_make_buffer(&(sg_buffer_desc){
      .type = SG_BUFFERTYPE_STORAGEBUFFER,
      .size = size+desc.size,
      .usage = SG_USAGE_STREAM,
      .label = "particle ssbo buffer"
    });
  }

  for (int i = 0; i < js_arrlen(js,array); i++) {
    JSValue sub = JS_GetPropertyUint32(js, array,i);
    ms[i].model = transform2mat(js2transform(js,JS_GetPropertyStr(js,sub, "transform")));
    ms[i].color = js2vec4(js,JS_GetPropertyStr(js,sub,"color"));
  }

  int offset = sg_append_buffer(*b, (&(sg_range){
    .ptr = ms,
    .size = size
  }));

  ret = number2js(js,offset/sizeof(particle_ss));
)

typedef struct sprite_ss {
  HMM_Mat4 model;
  rect rect;
  HMM_Vec4 shade;
} sprite_ss;

JSC_CCALL(render_make_sprite_ssbo,
  JSValue array = argv[0];
  size_t size = js_arrlen(js,array)*(sizeof(sprite_ss));
  sg_buffer *b = js2sg_buffer(js, argv[1]);
  if (!b) return JS_UNDEFINED;
  
  sprite_ss ms[js_arrlen(js,array)];
  
  if (sg_query_buffer_will_overflow(*b, size)) {
    sg_buffer_desc desc = sg_query_buffer_desc(*b);
    sg_destroy_buffer(*b);
    *b = sg_make_buffer(&(sg_buffer_desc){
      .type = SG_BUFFERTYPE_STORAGEBUFFER,
      .size = size+desc.size,
      .usage = SG_USAGE_STREAM,
      .label = "sprite ssbo buffer"
    });
  }

  for (int i = 0; i < js_arrlen(js,array); i++) {
    JSValue sub = JS_GetPropertyUint32(js, array,i);
    
//    transform *tr = js2transform(js,JS_GetPropertyStr(js,sub, "transform"));
    JSValue image = JS_GetPropertyStr(js,sub, "image");
    
    ms[i].model = js2transform_mat(js, JS_GetPropertyStr(js,sub,"transform"));// transform2mat(tr);
    ms[i].rect = js2rect(js,JS_GetPropertyStr(js,image,"rect"));
    ms[i].shade = js2vec4(js,JS_GetPropertyStr(js,sub,"shade"));
  }

  int offset = sg_append_buffer(*b, (&(sg_range){
    .ptr = ms,
    .size = size
  }));
  
  ret = number2js(js,offset/sizeof(sprite_ss)); // 96 size of a sprite struct
)

JSC_CCALL(render_make_t_ssbo,
  JSValue array = argv[0];
  size_t size = js_arrlen(js,array)*sizeof(HMM_Mat4);
  sg_buffer *b = js2sg_buffer(js, argv[1]);
  if (!b) return JS_UNDEFINED;
  
  HMM_Mat4 ms[js_arrlen(js,array)];

  if (sg_query_buffer_will_overflow(*b, size)) {
    sg_destroy_buffer(*b);
    *b = sg_make_buffer(&(sg_buffer_desc){
      .type = SG_BUFFERTYPE_STORAGEBUFFER,
      .size = size,
      .usage = SG_USAGE_STREAM,
      .label = "transform buffer"
    });
  }

  for (int i = 0; i < js_arrlen(js,array); i++)
    ms[i] = transform2mat(js2transform(js,JS_GetPropertyUint32(js, array, i)));

  sg_append_buffer(*b, (&(sg_range){
    .ptr = ms,
    .size = size
  }));
)

JSC_CCALL(render_spdraw,
  sg_draw(js2number(js,argv[0]),js2number(js,argv[1]),js2number(js,argv[2]));
)

JSC_CCALL(render_setpipeline, sg_apply_pipeline(*js2sg_pipeline(js,argv[0]));)

JSC_SCALL(render_text_size,
  font *f = js2font(js,argv[1]);
  float size = js2number(js,argv[2]);
  if (!size) size = f->height;
  float letterSpacing = js2number(js,argv[3]);
  float wrap = js2number(js,argv[4]);
  ret = vec22js(js,measure_text(str, f, size, letterSpacing, wrap));
)

static const JSCFunctionListEntry js_render_funcs[] = {
  MIST_FUNC_DEF(render, flushtext, 1),
  MIST_FUNC_DEF(render, camera_screen2world, 2),
  MIST_FUNC_DEF(render, make_textssbo, 0),
  MIST_FUNC_DEF(render, viewport, 4),
  MIST_FUNC_DEF(render, end_pass, 0),
  MIST_FUNC_DEF(render, commit, 0),
  MIST_FUNC_DEF(render, glue_pass, 0),
  MIST_FUNC_DEF(render, text_size, 5),
  MIST_FUNC_DEF(render, set_projection_ortho, 3),
  MIST_FUNC_DEF(render, set_projection_perspective, 4),  
  MIST_FUNC_DEF(render, set_view, 1),
  MIST_FUNC_DEF(render, make_pipeline, 1),
  MIST_FUNC_DEF(render, setuniv3, 2),
  MIST_FUNC_DEF(render, setuniv, 2),
  MIST_FUNC_DEF(render, spdraw, 3),
  MIST_FUNC_DEF(render, setunibones, 3),
  MIST_FUNC_DEF(render, setbind, 1),
  MIST_FUNC_DEF(render, setuniproj, 2),
  MIST_FUNC_DEF(render, setuniview, 2),
  MIST_FUNC_DEF(render, setunivp, 2),
  MIST_FUNC_DEF(render, setunim4, 3),
  MIST_FUNC_DEF(render, setuniv2, 2),
  MIST_FUNC_DEF(render, setuniv4, 2),
  MIST_FUNC_DEF(render, setpipeline, 1),
  MIST_FUNC_DEF(render, make_t_ssbo, 2),
  MIST_FUNC_DEF(render, make_particle_ssbo, 2),
  MIST_FUNC_DEF(render, make_sprite_ssbo, 2)
};

JSC_CCALL(gui_scissor,
  sg_apply_scissor_rect(js2number(js,argv[0]), js2number(js,argv[1]), js2number(js,argv[2]), js2number(js,argv[3]), 0);
)

JSC_CCALL(gui_text,
  const char *s = JS_ToCString(js, argv[0]);
  HMM_Vec2 pos = js2vec2(js,argv[1]);
  float size = js2number(js,argv[2]);
  font *f = js2font(js,argv[5]);
  if (!size) size = f->height;
  struct rgba c = js2color(js,argv[3]);
  int wrap = js2number(js,argv[4]);
  renderText(s, pos, f, size, c, wrap);
  JS_FreeCString(js, s);
  return ret;
)

static const JSCFunctionListEntry js_gui_funcs[] = {
  MIST_FUNC_DEF(gui, scissor, 4),
  MIST_FUNC_DEF(gui, text, 6),
};

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

JSValue js_vector_dot(JSContext *js, JSValue self, int argc, JSValue *argv) { return number2js(js,HMM_DotV2(js2vec2(js,argv[0]), js2vec2(js,argv[1]))) ; };

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
  double angle = js2angle(js,argv[1]);
  HMM_Vec2 pivot = JS_IsUndefined(argv[2]) ? v2zero : js2vec2(js,argv[2]);
  vec = HMM_SubV2(vec,pivot);
  float r = HMM_LenV2(vec);  
  angle += atan2(vec.y,vec.x);
  vec.x = r*cos(angle);
  vec.y = r*sin(angle);
  return vec22js(js,HMM_AddV2(vec,pivot));
)

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
    JS_SetPropertyUint32(js, newarr, i, number2js(js,js2number(js,JS_GetPropertyUint32(js, argv[0],i))/length));

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
  int len = js2number(js,JS_GetPropertyStr(js,argv[0], "length"));
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
  double sum;
  for (int i = 0; i < len; i++)
    sum += js2number(js,JS_GetPropertyUint32(js, argv[0], i));
    
  return number2js(js,sum/len);
)

JSC_CCALL(vector_sum,
  double sum = 0.0;
  int len = js_arrlen(js,argv[0]);
  for (int i = 0; i < len; i++)
    sum += js2number(js,JS_GetPropertyUint32(js, argv[0], i));
  
  return number2js(js,sum);
)

JSC_CCALL(vector_sigma,
  int len = js_arrlen(js,argv[0]);
  double sum;
  for (int i = 0; i < len; i++)
    sum += js2number(js,JS_GetPropertyUint32(js, argv[0], i));
  
  double mean = sum/(double)len;
  
  double sq_diff = 0;

  for (int i = 0; i < len; i++) {
    double x = js2number(js,JS_GetPropertyUint32(js, argv[0],i));
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
    arr[i] = js2number(js,JS_GetPropertyUint32(js, argv[0], i));
    
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

static const JSCFunctionListEntry js_vector_funcs[] = {
  MIST_FUNC_DEF(vector, dot,2),
  MIST_FUNC_DEF(vector, project,2),
  MIST_FUNC_DEF(vector, inflate, 2),
  MIST_FUNC_DEF(vector, rotate, 2),
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
  MIST_FUNC_DEF(vector, sigma, 1),
  MIST_FUNC_DEF(vector, median, 1),
  MIST_FUNC_DEF(vector, length, 1)
};

#define JS_HMM_FN(OP, HMM, SIGN) \
JSC_CCALL(array_##OP, \
  int len = js_arrlen(js,self); \
  if (!JS_IsArray(js, argv[0])) { \
    double n = js2number(js,argv[0]); \
    JSValue arr = JS_NewArray(js); \
    for (int i = 0; i < len; i++) \
      JS_SetPropertyUint32(js, arr, i, number2js(js,js2number(js,JS_GetPropertyUint32(js, self,i)) SIGN n)); \
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
    double a = js2number(js,JS_GetPropertyUint32(js, self,i)); \
    double b = js2number(js,JS_GetPropertyUint32(js, argv[0],i)); \
    JS_SetPropertyUint32(js, arr, i, number2js(js,a SIGN b)); \
  } \
  return arr; \
) \

JS_HMM_FN(add, Add, +)
JS_HMM_FN(sub, Sub, -)
JS_HMM_FN(div, Div, /)
JS_HMM_FN(scale, Mul, *)

JSC_CCALL(array_lerp,
  double t = js2number(js,argv[1]);
  int len = js_arrlen(js,self);
  JSValue arr =  JS_NewArray(js);
  
  for (int i = 0; i < len; i++) {
    double from = js2number(js,JS_GetPropertyUint32(js, self, i));
    double to = js2number(js,JS_GetPropertyUint32(js, argv[0], i));
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

static JSValue js_sg_init;
static JSValue js_sg_frame;
static JSValue js_sg_event;
static JSValue js_sg_cleanup;

static void c_init(JSContext *js)
{
  render_init();
  script_call_sym(js_sg_init, 0,NULL);
}

static void c_frame(JSContext *js)
{
  script_call_sym(js_sg_frame,0,NULL);
}

static void c_cleanup(JSContext *js)
{
  script_call_sym(js_sg_cleanup,0,NULL);
  sg_shutdown();
  sapp_quit();  
}

void gui_input(sapp_event *e);

static void c_event(const sapp_event *e, JSContext *js)
{
#ifndef NEDITOR
  gui_input(e);
#endif

  JSValue event = sapp_event2js(js, e);
  script_call_sym(js_sg_event, 1, &event);
  JS_FreeValue(js,event);
}

JSC_CCALL(game_engine_start,
  sapp_desc start_desc = {0};

  start_desc.user_data = js;
  start_desc.init_userdata_cb = c_init;  
  start_desc.frame_userdata_cb = c_frame;
  start_desc.event_userdata_cb = c_event;
  start_desc.cleanup_userdata_cb = c_cleanup;
  js_sg_init = JS_DupValue(js, JS_GetPropertyStr(js,argv[0], "init"));
  js_sg_frame = JS_DupValue(js, JS_GetPropertyStr(js,argv[0], "frame"));
  js_sg_cleanup = JS_DupValue(js, JS_GetPropertyStr(js,argv[0], "cleanup"));
  js_sg_event = JS_DupValue(js, JS_GetPropertyStr(js,argv[0], "event"));
  start_desc.width = js2number(js, JS_GetPropertyStr(js, argv[0], "width"));
  start_desc.height = js2number(js, JS_GetPropertyStr(js, argv[0], "height"));
  start_desc.sample_count = js2number(js, JS_GetPropertyStr(js, argv[0], "sample_count"));
  start_desc.high_dpi = js2number(js, JS_GetPropertyStr(js, argv[0], "high_dpi"));
  start_desc.alpha = js2number(js, JS_GetPropertyStr(js, argv[0], "alpha"));  
  start_desc.fullscreen = js2number(js, JS_GetPropertyStr(js, argv[0], "fullscreen"));
  start_desc.swap_interval = js2number(js, JS_GetPropertyStr(js, argv[0], "swap_interval"));  
  start_desc.window_title = "PROSPERON WINDOW";//js2number(js, JS_GetPropertyStr(js, argv[0], "width"));
  start_desc.enable_clipboard = js2number(js, JS_GetPropertyStr(js, argv[0], "enable_clipboard"));
  start_desc.enable_dragndrop = js2number(js, JS_GetPropertyStr(js, argv[0], "enable_dragndrop"));
  start_desc.max_dropped_files = js2number(js, JS_GetPropertyStr(js, argv[0], "max_dropped_files"));
  start_desc.logger.func = slog_func;
  texture *tex = js2texture(js, JS_GetPropertyStr(js, argv[0], "icon"));
  if (tex) start_desc.icon = texture2icon(tex);
  sapp_run(&start_desc);
)

static const JSCFunctionListEntry js_game_funcs[] = {
  MIST_FUNC_DEF(game, engine_start, 1),
};

JSC_CCALL(input_show_keyboard, sapp_show_keyboard(JS_ToBool(js,argv[0])))
JSValue js_input_keyboard_shown(JSContext *js, JSValue self) { return JS_NewBool(js,sapp_keyboard_shown()); }
JSC_CCALL(input_mouse_lock, sapp_lock_mouse(js2number(js,argv[0])))
JSC_CCALL(input_mouse_cursor, sapp_set_mouse_cursor(js2number(js,argv[0])))
JSC_CCALL(input_mouse_show, sapp_show_mouse(JS_ToBool(js,argv[0])))

static const JSCFunctionListEntry js_input_funcs[] = {
  MIST_FUNC_DEF(input, show_keyboard, 1),
  MIST_FUNC_DEF(input, keyboard_shown, 0),
  MIST_FUNC_DEF(input, mouse_cursor, 1),
  MIST_FUNC_DEF(input, mouse_show, 1),
  MIST_FUNC_DEF(input, mouse_lock, 1),
};

JSC_CCALL(prosperon_guid,
  int bits = 32;
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

static const JSCFunctionListEntry js_sg_buffer_funcs[] = {
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
  printf("%s\n",str);
  fflush(stdout);
)

static const JSCFunctionListEntry js_console_funcs[] = {
  MIST_FUNC_DEF(console,log,1),
};

JSC_CCALL(profile_now, return number2js(js,stm_now()))

static JSValue instr_v = JS_UNDEFINED;
int iiihandle(JSRuntime *rt, void *data)
{
  script_call_sym(instr_v, 0, NULL);
  return 0;
}

JSC_CCALL(profile_gather,
  int count = js2number(js,argv[0]);
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
  double seconds = stm_sec(js2number(js,argv[0]));
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

JSC_CCALL(profile_secs, return number2js(js,stm_sec(js2number(js,argv[0]))); )

static const JSCFunctionListEntry js_profile_funcs[] = {
  MIST_FUNC_DEF(profile,now,0),
  MIST_FUNC_DEF(profile,best_t, 1),
  MIST_FUNC_DEF(profile,gather,2),
  MIST_FUNC_DEF(profile,gather_rate,1),
  MIST_FUNC_DEF(profile,gather_stop,0),
  MIST_FUNC_DEF(profile,secs,1),
};

static void list_files(const char *path, JSContext *js, JSValue v, int *n)
{
  DIR *dir = opendir(path);
  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".")  == 0 || strcmp(entry->d_name, "..") == 0)
      continue;

    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
    struct stat statbuf;
    if (stat(full_path, &statbuf) == 0) {
      if (S_ISDIR(statbuf.st_mode)) {
        list_files(full_path, js, v, n);
      } else if (S_ISREG(statbuf.st_mode)) {
        JS_SetPropertyUint32(js, v, *n, JS_NewString(js, full_path+2));
        *n = *n + 1;
      }
    }
  }
  closedir(dir);
}

JSC_CCALL(io_ls, 
  JSValue strarr = JS_NewArray(js);
  int i = 0;
  list_files(".", js, strarr, &i);
  return strarr;
)
JSC_SSCALL(io_mv, ret = number2js(js,rename(str,str2)))
JSC_SCALL(io_chdir, ret = number2js(js,chdir(str)))
JSC_SCALL(io_rm, ret = number2js(js,remove(str)))
JSC_SCALL(io_mkdir, ret = number2js(js,mkdir(str,0777)))

JSValue js_io_slurp(JSContext *js, JSValue self, int argc, JSValue *argv)
{
  const char *file = JS_ToCString(js, argv[0]);
  
  FILE *f = fopen(file, "rb");
  JS_FreeCString(js,file);  
  if (!f)
    return JS_UNDEFINED;

  fseek(f,0,SEEK_END);
  size_t fsize = ftell(f);
  rewind(f);
  void *slurp = malloc(fsize);
  fread(slurp,fsize,1,f);
  fclose(f);

  JSValue ret;
  if (JS_ToBool(js,argv[1]))
    ret = JS_NewStringLen(js, slurp, fsize);
  else
    ret = JS_NewArrayBufferCopy(js, slurp, fsize);

  free(slurp);
  
  return ret;
}

JSValue js_io_slurpwrite(JSContext *js, JSValue self, int argc, JSValue *argv)
{
  const char *f = JS_ToCString(js, argv[0]);
  FILE *file = fopen(f,"wb");
  if (!f) return JS_ThrowReferenceError(js, "could not open file for writing at %s", f);
  size_t len;
  JSValue ret;
  if (JS_IsString(argv[1])) {
    const char *data = JS_ToCStringLen(js, &len, argv[1]);
    ret = number2js(js, fwrite(data, len, 1, file));
    JS_FreeCString(js,data);
    fclose(file);
  } else {
    unsigned char *data = JS_GetArrayBuffer(js, &len, argv[1]);
    ret = number2js(js,fwrite(data,len,1,file));
    fclose(file);
  }

  return ret;
}

JSValue js_io_chmod(JSContext *js, JSValue self, int argc, JSValue *argv)
{
  const char *f = JS_ToCString(js, argv[0]);
  int mod = js2number(js,argv[1]);
  chmod(f, mod);
  return JS_UNDEFINED;
}

JSC_SCALL(io_mod,
  #ifndef __EMSCRIPTEN__
  struct stat attr;
  if (!stat(str,&attr))
    ret = number2js(js,attr.st_mtime);
  else
    return JS_UNDEFINED;//JS_ThrowReferenceError(js, "could not get file attributes of %s", str);
  #endif
)

static const JSCFunctionListEntry js_io_funcs[] = {
  MIST_FUNC_DEF(io, mv, 2),
  MIST_FUNC_DEF(io, ls, 0),
  MIST_FUNC_DEF(io, rm, 1),
  MIST_FUNC_DEF(io, chdir, 1),
  MIST_FUNC_DEF(io, mkdir, 1),
  MIST_FUNC_DEF(io, chmod, 2),
  MIST_FUNC_DEF(io, slurp, 2),
  MIST_FUNC_DEF(io, slurpwrite, 2),
  MIST_FUNC_DEF(io, mod,1)
};

JSC_GETSET(transform, pos, vec3)
JSC_GETSET(transform, scale, vec3f)
JSC_GETSET(transform, rotation, quat)
JSC_CCALL(transform_move, transform_move(js2transform(js,self), js2vec3(js,argv[0])); )

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
};


JSC_CCALL(datastream_time, return number2js(js,plm_get_time(js2datastream(js,self)->plm)); )

JSC_CCALL(datastream_advance_frames,  ds_advanceframes(js2datastream(js,self), js2number(js,argv[0])))

JSC_CCALL(datastream_seek, ds_seek(js2datastream(js,self), js2number(js,argv[0])))

JSC_CCALL(datastream_advance, ds_advance(js2datastream(js,self), js2number(js,argv[0])))

JSC_CCALL(datastream_duration, return number2js(js,ds_length(js2datastream(js,self))))

JSC_CCALL(datastream_framerate, return number2js(js,plm_get_framerate(js2datastream(js,self)->plm)))

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
JSC_GET(texture, vram, number)

JSC_SCALL(texture_save, texture_save(js2texture(js,self), str));

JSC_CCALL(texture_blit,
  texture_blit(js2texture(js,self), js2texture(js,argv[0]), js2rect(js,argv[1]), js2rect(js,argv[2]), JS_ToBool(js,argv[3]));
)

JSC_CCALL(texture_getid,
  texture *tex = js2texture(js,self);
  return number2js(js,tex->id.id);
)

JSC_CCALL(texture_inram, return JS_NewBool(js,(int)js2texture(js,self)->data));

JSC_CCALL(texture_fill,
//  texture_fill(js2texture(js,self), js2color(js,argv[0]));
)

JSC_CCALL(texture_fill_rect,
//  texture_fill_rect(js2texture(js,self), js2rect(js,argv[0]), js2color(js,argv[1]));
)

JSC_CCALL(texture_flip,
//  texture_flip(js2texture(js,self), JS_ToBool(js,argv[0]));
)

JSC_CCALL(texture_write_pixel,
  HMM_Vec2 c = js2vec2(js,argv[0]);
//  texture_write_pixel(js2texture(js,self), c.x, c.y, js2color(js,argv[1]));
)

JSC_CCALL(texture_dup,
  ret = texture2js(js,texture_dup(js2texture(js,self)));
)

JSC_CCALL(texture_scale,
  ret = texture2js(js,texture_scale(js2texture(js,self), js2number(js,argv[0]), js2number(js,argv[1])));
)

JSC_CCALL(texture_load_gpu,
  texture_load_gpu(js2texture(js,self));
)

JSC_CCALL(texture_offload,
  texture_offload(js2texture(js,self));
)

static const JSCFunctionListEntry js_texture_funcs[] = {
  MIST_GET(texture, width),
  MIST_GET(texture, height),
  MIST_GET(texture, vram),
  MIST_FUNC_DEF(texture, save, 1),
  MIST_FUNC_DEF(texture, write_pixel, 2),  
  MIST_FUNC_DEF(texture, fill, 1),
  MIST_FUNC_DEF(texture, fill_rect, 2),
  MIST_FUNC_DEF(texture, dup, 0),
  MIST_FUNC_DEF(texture, scale, 2),
  MIST_FUNC_DEF(texture, flip, 1),
  MIST_FUNC_DEF(texture, blit, 4),
  MIST_FUNC_DEF(texture, getid, 0),
  MIST_FUNC_DEF(texture, inram, 0),
  MIST_FUNC_DEF(texture, load_gpu, 0),
  MIST_FUNC_DEF(texture, offload, 0),
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
JSC_CCALL(performance_unpack_num, int i = js2number(js,argv[0]))
JSC_CCALL(performance_unpack_array,
  void *v = js2cpvec2arr(js,argv[0]);
  arrfree(v);
)
JSC_CCALL(performance_pack_num, return number2js(js,1.0))
JSC_CCALL(performance_pack_string, return JS_NewStringLen(js, STRTEST, sizeof(*STRTEST)))
JSC_CCALL(performance_unpack_string, JS_ToCString(js, argv[0]))
JSC_CCALL(performance_unpack_32farr, jsfloat2vec(js,argv[0]))
JSC_CCALL(performance_call_fn_n, 
  for (int i = 0; i < js2number(js,argv[1]); i++)
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

JSC_CCALL(geometry_rect_intersect,
  rect a = js2rect(js,argv[0]);
  rect b = js2rect(js,argv[1]);
  return JS_NewBool(js,a.x < (b.x+b.w) &&
         (a.x + a.w) > b.x &&
         a.y < (b.y + b.h) &&
         (a.y + a.h) > b.y );
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

static const JSCFunctionListEntry js_geometry_funcs[] = {
  MIST_FUNC_DEF(geometry, rect_intersect, 2),
  MIST_FUNC_DEF(geometry, rect_inside, 2),
  MIST_FUNC_DEF(geometry, rect_random, 1),
  MIST_FUNC_DEF(geometry, cwh2rect, 2),
  MIST_FUNC_DEF(geometry, rect_point_inside, 2),
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

JSC_SCALL(os_env, ret = JS_NewString(js,getenv(str)))

JSValue js_os_sys(JSContext *js, JSValue self, int argc, JSValue *argv)
{
  #ifdef __linux__
  return JS_NewString(js,"linux");
  #elif defined(_WIN32) || defined(_WIN64)
  return JS_NewString(js,"windows");
  #elif defined(IOS)
  return JS_NewString(js,"ios");
  #elif defined(__APPLE__)
  return JS_NewString(js,"macos");
  #elif defined(__EMSCRIPTEN__)
  return JS_NewString(js,"web");
  #endif
  return JS_UNDEFINED;
}

JSC_CCALL(os_backend,
  #ifdef SOKOL_GLCORE
  return JS_NewString(js,"opengl");
  #elifdef SOKOL_WGPU
  return JS_NewString(js,"wgpu");
  #elifdef SOKOL_D3D11
  return JS_NewString(js,"directx");
  #elifdef SOKOL_METAL
  return JS_NewString(js,"metal");
  #endif
)

JSC_CCALL(os_exit, exit(js2number(js,argv[0]));)
JSC_CCALL(os_gc, JS_RunGC(JS_GetRuntime(js)))
JSC_CCALL(os_mem_limit, JS_SetMemoryLimit(JS_GetRuntime(js), js2number(js,argv[0])))
JSC_CCALL(os_gc_threshold, JS_SetGCThreshold(JS_GetRuntime(js), js2number(js,argv[0])))
JSC_CCALL(os_max_stacksize, JS_SetMaxStackSize(JS_GetRuntime(js), js2number(js,argv[0])))

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

JSC_CCALL(os_dump_value,
  
  FILE *tmp = tmpfile();
  quickjs_set_dumpout(tmp);
  JS_DumpMyValue(JS_GetRuntime(js), argv[0]);
  ret = tmp2js(js,tmp);
)

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

  texture *tex = texture_fromdata(raw, len);
  if (!tex) return JS_ThrowReferenceError(js, "unable to make texture from the given array buffer");

  ret = texture2js(js,tex);
  JS_SetPropertyStr(js, ret, "path", JS_DupValue(js,argv[0]));
)

JSC_CCALL(os_make_gif,
  size_t rawlen;
  void *raw = JS_GetArrayBuffer(js, &rawlen, argv[0]);
  if (!raw) return JS_ThrowReferenceError(js, "could not load gif from supplied array buffer");

  int n;
  texture *tex = calloc(1,sizeof(*tex));
  int frames;
  int *delays;
  tex->data = stbi_load_gif_from_memory(raw, rawlen, &delays, &tex->width, &tex->height, &frames, &n, 4);
  tex->height *= frames;

  JSValue gif = JS_NewObject(js);
  JSValue delay_arr = JS_NewArray(js);
  JSValue jstex = texture2js(js,tex);
  JS_SetPropertyStr(js, jstex, "path", JS_DupValue(js, argv[0]));

  float yslice = 1.0/frames;
  for (int i = 0; i < frames; i++) {
    JSValue frame = JS_NewObject(js);
    JS_SetPropertyStr(js, frame, "time", number2js(js,(float)delays[i]/1000.0));
    JS_SetPropertyStr(js, frame, "rect", rect2js(js,(rect){
      .x = 0,
      .y = yslice*i,
      .w = 1,
      .h = yslice
    }));
    JS_SetPropertyStr(js, frame, "texture", JS_DupValue(js,jstex));
    JS_SetPropertyUint32(js, delay_arr, i, frame);
  }

  JS_SetPropertyStr(js, gif, "frames", delay_arr);
  JS_FreeValue(js,jstex);

  free(delays);
  
  ret = gif;
)

JSValue aseframe2js(JSContext *js, ase_frame_t aframe)
{
  JSValue frame = JS_NewObject(js);
  texture *tex = calloc(1,sizeof(*tex));
  tex->width = aframe.ase->w;
  tex->height = aframe.ase->h;
  tex->data = malloc(tex->width*tex->height*4);
  memcpy(tex->data, aframe.pixels, tex->width*tex->height*4);
  JS_SetPropertyStr(js, frame, "texture", texture2js(js,tex));
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

JSC_CCALL(os_make_tex_data,
  ret = texture2js(js,texture_empty(js2number(js,argv[0]), js2number(js,argv[1])))
)

JSC_CCALL(os_make_font,
  size_t len;
  void *data = JS_GetArrayBuffer(js,&len,argv[0]);
  if (!data) return JS_ThrowReferenceError(js, "could not get array buffer data");
  font *f = MakeFont(data, len, js2number(js,argv[1]));
  if (!f) return JS_ThrowReferenceError(js, "could not create font");
  ret = font2js(js,f);
  JS_SetPropertyStr(js, ret, "texture", texture2js(js,f->texture));
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

  sg_buffer *b = malloc(sizeof(*b));
  *b = accessor2buffer(&data->accessors[buffer_idx], type);
  cgltf_free(data);

  ret = sg_buffer2js(js,b);
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

JSC_CCALL(os_make_buffer,
  int type = js2number(js,argv[1]);
  float *b = malloc(sizeof(float)*js_arrlen(js,argv[0]));
  for (int i = 0; i < js_arrlen(js,argv[0]); i++)
    b[i] = js2number(js,JS_GetPropertyUint32(js, argv[0],i));
    
  sg_buffer *p = malloc(sizeof(sg_buffer));
  
  switch(type) {
    case 0:
      *p = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(float)*js_arrlen(js,argv[0]),
        .data = b
      });
      break;
    case 1:
      *p = index_buffer(b, js_arrlen(js,argv[0]));
      break;
    case 2:
      *p = texcoord_floats(b, js_arrlen(js,argv[0]));
      break;
  }

  free(b);
  return sg_buffer2js(js,p);
)

JSC_CCALL(os_make_line_prim,
  JSValue prim = JS_NewObject(js);
  HMM_Vec2 *v = js2cpvec2arr(js,argv[0]);
  parsl_position par_v[arrlen(v)];
  for (int i = 0; i < arrlen(v); i++) {
    par_v[i].x = v[i].x;
    par_v[i].y = v[i].y;
  }
  
  parsl_context *par_ctx = parsl_create_context((parsl_config){
    .thickness = js2number(js,argv[1]),
    .flags= PARSL_FLAG_ANNOTATIONS,
    .u_mode = js2number(js,argv[2])
  });
  
  uint16_t spine_lens[] = {arrlen(v)};
  
  parsl_mesh *m = parsl_mesh_from_lines(par_ctx, (parsl_spine_list){
    .num_vertices = arrlen(v),
    .num_spines = 1,
    .vertices = par_v,
    .spine_lengths = spine_lens,
    .closed = JS_ToBool(js,argv[3])
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
  JS_SetPropertyStr(js, prim, "pos", sg_buffer2js(js,pos));
  JS_SetPropertyStr(js, prim, "count", number2js(js,m->num_triangles*3));
  sg_buffer *idx = malloc(sizeof(*idx));
  *idx = par_idx_buffer(m->triangle_indices, m->num_triangles*3);
  JS_SetPropertyStr(js, prim, "index", sg_buffer2js(js,idx));
  
  float uv[m->num_vertices*2];
  for (int i = 0; i < m->num_vertices; i++) {
    uv[i*2] = m->annotations[i].u_along_curve;
    uv[i*2+1] = m->annotations[i].v_across_curve;
  }
  sg_buffer *buv = malloc(sizeof(*buv));
  *buv = texcoord_floats(uv, m->num_vertices*2);
  JS_SetPropertyStr(js, prim, "uv", sg_buffer2js(js,buv));
  
  return prim;
)

JSValue parmesh2js(JSContext *js,par_shapes_mesh *m)
{
  JSValue obj = JS_NewObject(js);
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

JSC_CCALL(os_make_video,
  size_t len;
  void *data = JS_GetArrayBuffer(js,&len,argv[0]);
  datastream *ds = ds_openvideo(data, len);
  ret = datastream2js(js,ds);
  texture *t = malloc(sizeof(texture));
  t->id = ds->img;
  JS_SetPropertyStr(js, ret, "texture", texture2js(js,t));
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
    HMM_Vec2 wh = js2vec2(js,JS_GetPropertyUint32(js, argv[2], i));
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

static const JSCFunctionListEntry js_os_funcs[] = {
  MIST_FUNC_DEF(os, turbulence, 4),
  MIST_FUNC_DEF(os, fbm, 4),
  MIST_FUNC_DEF(os, backend, 0),
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
  MIST_FUNC_DEF(os, make_tex_data, 3),
  MIST_FUNC_DEF(os, make_font, 2),
  MIST_FUNC_DEF(os, make_transform, 0),
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
  MIST_FUNC_DEF(os, make_timer, 1),
  MIST_FUNC_DEF(os, update_timers, 1),
  MIST_FUNC_DEF(os, mem, 1),
  MIST_FUNC_DEF(os, mem_limit, 1),
  MIST_FUNC_DEF(os, gc_threshold, 1),
  MIST_FUNC_DEF(os, max_stacksize, 1),
  MIST_FUNC_DEF(os, dump_value, 1),
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
};

#define JSSTATIC(NAME, PARENT) \
js_##NAME = JS_NewObject(js); \
JS_SetPropertyFunctionList(js, js_##NAME, js_##NAME##_funcs, countof(js_##NAME##_funcs)); \
JS_SetPrototype(js, js_##NAME, PARENT); \

JSValue js_layout_use(JSContext *js);
JSValue js_miniz_use(JSContext *js);
JSValue js_soloud_use(JSContext *js);
JSValue js_chipmunk2d_use(JSContext *js);
JSValue js_dmon_use(JSContext *js);

#ifndef NEDITOR
JSValue js_imgui(JSContext *js);
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
  stm_setup(); /* time */
  cycles = tmpfile();
  quickjs_set_cycleout(cycles);
  
  JSValue globalThis = JS_GetGlobalObject(js);

  QJSGLOBALCLASS(os);
  
  QJSCLASSPREP_FUNCS(sg_buffer);  
  QJSCLASSPREP_FUNCS(transform);
//  QJSCLASSPREP_FUNCS(warp_gravity);
//  QJSCLASSPREP_FUNCS(warp_damp);
  QJSCLASSPREP_FUNCS(texture);
  QJSCLASSPREP_FUNCS(font);
  QJSCLASSPREP_FUNCS(datastream);
  QJSCLASSPREP_FUNCS(timer);

  QJSGLOBALCLASS(input);
  QJSGLOBALCLASS(io);
  QJSGLOBALCLASS(prosperon);
  QJSGLOBALCLASS(time);
  QJSGLOBALCLASS(console);
  QJSGLOBALCLASS(profile);
  QJSGLOBALCLASS(game);
  QJSGLOBALCLASS(gui);
  QJSGLOBALCLASS(render);
  QJSGLOBALCLASS(vector);
  QJSGLOBALCLASS(spline);
  QJSGLOBALCLASS(performance);
  QJSGLOBALCLASS(geometry);

  JS_SetPropertyStr(js, prosperon, "version", JS_NewString(js,"ver"));
  JS_SetPropertyStr(js, prosperon, "revision", JS_NewString(js,"com"));
  JS_SetPropertyStr(js, prosperon, "date", JS_NewString(js,"date"));
  JS_SetPropertyStr(js, globalThis, "texture", JS_DupValue(js,texture_proto));

  JSValue array_proto = JS_GetPropertyStr(js,globalThis, "Array");
  array_proto = JS_GetPropertyStr(js,array_proto, "prototype");
  JS_SetPropertyFunctionList(js, array_proto, js_array_funcs, countof(js_array_funcs));

  srand(stm_now());

  JS_SetPropertyStr(js, globalThis, "layout", js_layout_use(js));
  JS_SetPropertyStr(js, globalThis, "miniz", js_miniz_use(js));
  JS_SetPropertyStr(js, globalThis, "soloud", js_soloud_use(js));
  JS_SetPropertyStr(js, globalThis, "chipmunk2d", js_chipmunk2d_use(js));
  JS_SetPropertyStr(js, globalThis, "dmon", js_dmon_use(js));

  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);
  signal(SIGHUP, signal_handler);
  signal(SIGSEGV, signal_handler);
  signal(SIGQUIT, signal_handler);
  signal(SIGABRT, signal_handler);
  atexit(exit_handler);

#ifndef NEDITOR
  JS_SetPropertyStr(js, globalThis, "imgui", js_imgui(js));
#endif
  
  JS_FreeValue(js,globalThis);  
}

