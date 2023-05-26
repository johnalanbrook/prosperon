#include "ffi.h"

#include "script.h"

#include "anim.h"
#include "debug.h"
#include "debugdraw.h"
#include "editor.h"
#include "engine.h"
#include "font.h"
#include "gameobject.h"
#include "input.h"
#include "level.h"
#include "log.h"
#include "mix.h"
#include "music.h"
#include "nuke.h"
#include "openglrender.h"
#include "sound.h"
#include "sprite.h"
#include "stb_ds.h"
#include "string.h"
#include "tinyspline.h"
#include "window.h"
#include "yugine.h"
#include <assert.h>
#include <ftw.h>

#include "render.h"

#include "model.h"

#include "HandmadeMath.h"

#include "miniaudio.h"

static JSValue globalThis;

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

int js2int(JSValue v) {
  int32_t i;
  JS_ToInt32(js, &i, v);
  return i;
}

JSValue int2js(int i) {
  return JS_NewInt64(js, i);
}

JSValue str2js(const char *c) {
  return JS_NewString(js, c);
}

double js2number(JSValue v) {
  double g;
  JS_ToFloat64(js, &g, v);
  return g;
}

int js2bool(JSValue v) {
  return JS_ToBool(js, v);
}

JSValue float2js(double g) {
  return JS_NewFloat64(js, g);
}

JSValue num2js(double g) {
  return float2js(g);
}

struct gameobject *js2go(JSValue v) {
  return id2go(js2int(v));
}

void *js2ptr(JSValue v) {
  void *p;
  JS_ToInt64(js, &p, v);
  return p;
}

JSValue ptr2js(void *ptr) {
  return JS_NewInt64(js, (long)ptr);
}

struct timer *js2timer(JSValue v) {
  return id2timer(js2int(v));
}

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

JSValue js_arridx(JSValue v, int idx) {
  return js_getpropidx( v, idx);
}

int js_arrlen(JSValue v) {
  int len;
  JS_ToInt32(js, &len, js_getpropstr( v, "length"));
  return len;
}

struct rgba js2color(JSValue v) {
  JSValue c[4];
  for (int i = 0; i < 4; i++)
    c[i] = js_arridx(v,i);
  unsigned char a = JS_IsUndefined(c[3]) ? 255 : js2int(c[3]);
  struct rgba color = {
    .r = js2int(c[0]),
    .g = js2int(c[1]),
    .b = js2int(c[2]),
    .a = a,
  };

  return color;
}

HMM_Vec2 js2hmmv2(JSValue v)
{
  HMM_Vec2 v2;
  v2.X = js2number(js_getpropidx(v,0));
  v2.Y = js2number(js_getpropidx(v,1));
  return v2;
}

cpVect js2vec2(JSValue v) {
  cpVect vect;
  vect.x = js2number(js_getpropidx(v,0));
  vect.y = js2number(js_getpropidx(v,1));
  return vect;
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

cpVect *js2cpvec2arr(JSValue v) {
  int n = js_arrlen(v);
  cpVect *points = NULL;

  for (int i = 0; i < n; i++)
    arrput(points, js2vec2(js_getpropidx( v, i)));

  return points;
}

JSValue bitmask2js(cpBitmask mask) {
  JSValue arr = JS_NewArray(js);
  for (int i = 0; i < 11; i++) {
    int on = mask & 1 << i;
    JS_SetPropertyUint32(js, arr, i, JS_NewBool(js, on));
  }

  return arr;
}

void vec2float(cpVect v, float *f) {
  f[0] = v.x;
  f[1] = v.y;
}

JSValue vec2js(cpVect v) {
  JSValue array = JS_NewArray(js);
  JS_SetPropertyInt64(js, array, 0, JS_NewFloat64(js, v.x));
  JS_SetPropertyInt64(js, array, 1, JS_NewFloat64(js, v.y));
  return array;
}

JSValue vecarr2js(cpVect *points, int n) {
  JSValue array = JS_NewArray(js);
  for (int i = 0; i < n; i++)
    JS_SetPropertyInt64(js, array, i, vec2js(points[i]));
  return array;
}

JSValue duk_gui_text(JSContext *js, JSValueConst this, int argc, JSValueConst *argv) {
  const char *s = JS_ToCString(js, argv[0]);
  HMM_Vec2 pos = js2hmmv2(argv[1]);

  float size = js2number(argv[2]);
  renderText(s, pos, size, color_white, 500, -1);
  JS_FreeCString(js, s);
  return JS_NULL;
}

JSValue duk_ui_text(JSContext *js, JSValueConst this, int argc, JSValueConst *argv) {
  const char *s = JS_ToCString(js, argv[0]);
  HMM_Vec2 pos = js2hmmv2(argv[1]);

  float size = js2number(argv[2]);
  struct rgba c = js2color(argv[3]);
  int wrap = js2int(argv[4]);
  JSValue ret = JS_NewInt64(js, renderText(s, pos, size, c, wrap, -1));
  JS_FreeCString(js, s);
  return ret;
}

JSValue duk_cursor_text(JSContext *js, JSValueConst this, int argc, JSValueConst *argv) {
  const char *s = JS_ToCString(js, argv[0]);
  HMM_Vec2 pos = js2hmmv2(argv[1]);

  float size = js2number(argv[2]);
  struct rgba c = js2color(argv[3]);
  int wrap = js2int(argv[5]);
  int cursor = js2int(argv[4]);
  renderText(s, pos, size, c, wrap, cursor);
  JS_FreeCString(js, s);
  return JS_NULL;
}

JSValue duk_gui_img(JSContext *js, JSValueConst this, int argc, JSValueConst *argv) {
  const char *img = JS_ToCString(js, argv[0]);
  cpVect pos = js2vec2(argv[1]);
  gui_draw_img(img, pos.x, pos.y);
  JS_FreeCString(js, img);
  return JS_NULL;
}


struct nk_rect js2nk_rect(JSValue v) {
  struct nk_rect rect;
  rect.x = js2number(js_getpropstr(v, "x"));
  rect.y = js2number(js_getpropstr(v, "y"));
  rect.w = js2number(js_getpropstr(v, "w"));
  rect.h = js2number(js_getpropstr(v, "h"));
  return rect;
}

JSValue nk_rect2js(struct nk_rect rect) {
  JSValue obj = JS_NewObject(js);
  JS_SetPropertyStr(js, obj, "x", JS_NewFloat64(js, rect.x));
  JS_SetPropertyStr(js, obj, "y", JS_NewFloat64(js, rect.y));
  JS_SetPropertyStr(js, obj, "w", JS_NewFloat64(js, rect.w));
  JS_SetPropertyStr(js, obj, "h", JS_NewFloat64(js, rect.h));
  return obj;
}

JSValue duk_nuke(JSContext *js, JSValueConst this, int argc, JSValueConst *argv) {
  int cmd = js2int(argv[0]);
  float editnum;
  int editint;
  char textbox[130];
  const char *str = NULL;

  if (JS_IsString(argv[1]))
    str = JS_ToCString(js, argv[1]);
  else {
    JSValue tostr = JS_ToString(js, argv[1]);
    str = JS_ToCString(js, argv[1]);
    JS_FreeValue(js, tostr);
  }

  struct nk_rect rect = nk_rect(0, 0, 0, 0);
  JSValue ret = JS_NULL;

  switch (cmd) {
  case 0:
    rect = js2nk_rect(argv[2]);
    nuke_begin(str, rect, NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_TITLE);
    break;

  case 1:
    nuke_stop();
    break;

  case 2:
    editnum = js2number(argv[2]);
    nuke_property_float(str, js2number(argv[3]), &editnum, js2number(argv[4]), js2number(argv[5]), js2number(argv[5]));
    ret = JS_NewFloat64(js, editnum);
    break;

  case 3:
    nuke_nel(js2number(argv[1]));
    break;

  case 4:
    editint = JS_ToBool(js, argv[2]);
    nuke_checkbox(str, &editint);
    ret = JS_NewBool(js, editint);
    break;

  case 5:
    nuke_label(str);
    break;

  case 6:
    ret = JS_NewBool(js, nuke_btn(str));
    break;

  case 7:
    strncpy(textbox, str, 130);
    nuke_edit_str(textbox);
    ret = JS_NewString(js, textbox);
    break;

  case 8:
    nuke_img(str);
    break;

  case 9:
    editint = js2int(argv[2]);
    nuke_radio_btn(str, &editint, js2int(argv[3]));
    ret = JS_NewInt64(js, editint);
    break;

  case 10:
    rect = nuke_win_get_bounds();
    ret = nk_rect2js(rect);
    break;

  case 11:
    ret = JS_NewBool(js, nuke_push_tree_id(str, js2int(argv[2])));
    break;

  case 12:
    nuke_tree_pop();
    return JS_NULL;

  case 13:
    nuke_row(js2int(argv[1]));
    break;

  case 14:
    nuke_scrolltext(str);
    break;

  case 15:
    nuke_nel_h(js2int(argv[1]), js2int(argv[2]));
    break;
  }

  if (str)
    JS_FreeCString(js, str);

  return ret;
}

JSValue duk_win_make(JSContext *js, JSValueConst this, int argc, JSValueConst *argv) {
  const char *title = JS_ToCString(js, argv[0]);
  int w = js2int(argv[1]);
  int h = js2int(argv[2]);
  struct window *win = MakeSDLWindow(title, w, h, 0);
  JS_FreeCString(js, title);

  return JS_NewInt64(js, win->id);
}

JSValue duk_spline_cmd(JSContext *js, JSValueConst this, int argc, JSValueConst *argv) {
  static_assert(sizeof(tsReal) * 2 == sizeof(cpVect));

  tsBSpline spline;

  int d = js2int(argv[2]); /* dimensions */
  int degrees = js2int(argv[1]);
  int type = js2int(argv[3]);
  JSValue ctrl_pts = argv[4];
  int n = js_arrlen(ctrl_pts);
  size_t nsamples = js2int(argv[5]);

  cpVect points[n];

  tsStatus status;
  ts_bspline_new(n, d, degrees, type, &spline, &status);

  if (status.code)
    YughCritical("Spline creation error %d: %s", status.code, status.message);

  for (int i = 0; i < n; i++)
    points[i] = js2vec2(js_getpropidx( ctrl_pts, i));

  ts_bspline_set_control_points(&spline, (tsReal *)points, &status);

  if (status.code)
    YughCritical("Spline creation error %d: %s", status.code, status.message);

  cpVect samples[nsamples];

  size_t rsamples;
  /* TODO: This does not work with Clang/GCC due to UB */
  ts_bspline_sample(&spline, nsamples, (tsReal **)&samples, &rsamples, &status);

  if (status.code)
    YughCritical("Spline creation error %d: %s", status.code, status.message);

  JSValue arr = JS_NewArray(js);

  for (int i = 0; i < nsamples; i++) {
    JSValue psample = JS_NewArray(js);
    JS_SetPropertyUint32(js, psample, 0, float2js(samples[i].x));
    JS_SetPropertyUint32(js, psample, 1, float2js(samples[i].y));
    JS_SetPropertyUint32(js, arr, i, psample);
  }

  ts_bspline_free(&spline);

  return arr;
}

JSValue ints2js(int *ints) {
  JSValue arr = JS_NewArray(js);
  for (int i = 0; i < arrlen(ints); i++)
    JS_SetPropertyUint32(js, arr, i, int2js(ints[i]));

  return arr;
}

int vec_between(cpVect p, cpVect a, cpVect b) {
  cpVect n;
  n.x = b.x - a.x;
  n.y = b.y - a.y;
  n = cpvnormalize(n);

  return (cpvdot(n, cpvsub(p, a)) > 0 && cpvdot(cpvneg(n), cpvsub(p, b)) > 0);
}

/* Determines between which two points in 'segs' point 'p' falls.
   0 indicates 'p' comes before the first point.
   arrlen(segs) indicates it comes after the last point.
*/
int point2segindex(cpVect p, cpVect *segs, double slop) {
  float shortest = slop < 0 ? INFINITY : slop;
  int best = -1;

  for (int i = 0; i < arrlen(segs) - 1; i++) {
    float a = (segs[i + 1].y - segs[i].y) / (segs[i + 1].x - segs[i].x);
    float c = segs[i].y - (a * segs[i].x);
    float b = -1;

    float dist = abs(a * p.x + b * p.y + c) / sqrt(pow(a, 2) + 1);

    if (dist > shortest) continue;

    int between = vec_between(p, segs[i], segs[i + 1]);

    if (between) {
      shortest = dist;
      best = i + 1;
    } else {
      if (i == 0 && cpvdist(p, segs[0]) < slop) {
        shortest = dist;
        best = i;
      } else if (i == arrlen(segs) - 2 && cpvdist(p, arrlast(segs)) < slop) {
        shortest = dist;
        best = arrlen(segs);
      }
    }
  }

  if (best == 1) {
    cpVect n;
    n.x = segs[1].x - segs[0].x;
    n.y = segs[1].y - segs[0].y;
    n = cpvnormalize(n);
    if (cpvdot(n, cpvsub(p, segs[0])) < 0)
      if (cpvdist(p, segs[0]) >= slop)
        best = -1;
      else
        best = 0;
  }

  if (best == arrlen(segs) - 1) {
    cpVect n;
    n.x = segs[best - 1].x - segs[best].x;
    n.y = segs[best - 1].y - segs[best - 1].y;
    n = cpvnormalize(n);

    if (cpvdot(n, cpvsub(p, segs[best])) < 0)
      if (cpvdist(p, segs[best]) >= slop)
        best = -1;
      else
        best = arrlen(segs);
  }

  return best;
}

int file_exists(char *path) {
  FILE *o = fopen(path, "r");
  if (o) {
    fclose(o);
    return 1;
  }

  return 0;
}

static char *dukext;
static JSValue dukarr;
static int dukidx;

static int duk2path(const char *path, const struct stat *sb, int typeflag) {
  if (typeflag == FTW_F) {
    char *ext = strrchr(path, '.');
    if (ext && !strcmp(ext, dukext))
      JS_SetPropertyUint32(js, dukarr, dukidx++, JS_NewString(js, &path[2]));
  }

  return 0;
}

JSValue dukext2paths(char *ext) {
  dukext = ext;
  dukarr = JS_NewArray(js);
  dukidx = 0;
  ftw(".", duk2path, 10);
  return dukarr;
}

JSValue duk_cmd(JSContext *js, JSValueConst this, int argc, JSValueConst *argv) {
  int cmd = js2int(argv[0]);
  const char *str = NULL;
  const char *str2 = NULL;
  JSValue ret = JS_NULL;

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
    gameobject_delete(js2int(argv[1]));
    break;

  case 3:
    set_timescale(js2number(argv[1]));
    break;

  case 4:
    debug_draw_phys(JS_ToBool(js, argv[1]));
    break;

  case 5:
    renderMS = js2number(argv[1]);
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

  case 14:
    str = JS_ToCString(js, argv[1]);
    mini_sound(str);
    break;

  case 15:
    music_stop();
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
    mini_master(js2number(argv[1]));
    break;

  case 20:
    sprite_enabled(js2int(argv[1]), JS_ToBool(js, argv[2]));
    break;

  case 21:
    return JS_NewBool(js, shape_get_sensor(js2ptr(argv[1])));

  case 22:
    shape_enabled(js2ptr(argv[1]), JS_ToBool(js, argv[2]));
    break;

  case 23:
    return JS_NewBool(js, shape_is_enabled(js2ptr(argv[1])));

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
    return JS_NewFloat64(js, js2timer(argv[1])->interval);

  case 30:
    sprite_setanim(id2sprite(js2int(argv[1])), js2ptr(argv[2]), js2int(argv[3]));
    return JS_NULL;

  case 31:
    free(js2ptr(argv[1]));
    break;

  case 32:
    return JS_NewFloat64(js, js2timer(argv[1])->remain_time);

  case 33:
    return JS_NewBool(js, js2timer(argv[1])->on);

  case 34:
    return JS_NewBool(js, js2timer(argv[1])->repeat);

  case 35:
    js2timer(argv[1])->repeat = JS_ToBool(js, argv[2]);
    return JS_NULL;

  case 36:
    id2go(js2int(argv[1]))->scale = js2number(argv[2]);
    cpSpaceReindexShapesForBody(space, id2go(js2int(argv[1]))->body);
    return JS_NULL;

  case 37:
    if (!id2sprite(js2int(argv[1]))) return JS_NULL;
    id2sprite(js2int(argv[1]))->pos = js2hmmv2(argv[2]);
    break;

  case 38:
    str = JS_ToCString(js, argv[1]);
    ret = JS_NewString(js, slurp_text(str));
    break;

  case 39:
    str = JS_ToCString(js, argv[1]);
    str2 = JS_ToCString(js, argv[2]);
    ret = JS_NewInt64(js, slurp_write(str, str2));
    break;

  case 40:
    id2go(js2int(argv[1]))->filter.categories = js2bitmask(argv[2]);
    gameobject_apply(id2go(js2int(argv[1])));
    break;

  case 41:
    id2go(js2int(argv[1]))->filter.mask = js2bitmask(argv[2]);
    gameobject_apply(id2go(js2int(argv[1])));
    break;

  case 42:
    return bitmask2js(id2go(js2int(argv[1]))->filter.categories);

  case 43:
    return bitmask2js(id2go(js2int(argv[1]))->filter.mask);

  case 44:
    return JS_NewInt64(js, pos2gameobject(js2vec2(argv[1])));

  case 45:
    return vec2js(mouse_pos);

  case 46:
    set_mouse_mode(js2int(argv[1]));
    return JS_NULL;

  case 47:
    draw_grid(js2int(argv[1]), js2int(argv[2]), js2color(argv[3]));
    return JS_NULL;

  case 48:
    return JS_NewInt64(js, mainwin->width);

  case 49:
    return JS_NewInt64(js, mainwin->height);

  case 50:
    return JS_NewBool(js, action_down(js2int(argv[1])));

  case 51:
    draw_cppoint(js2vec2(argv[1]), js2number(argv[2]), js2color(argv[3]));
    return JS_NULL;

  case 52:
    return ints2js(phys2d_query_box(js2vec2(argv[1]), js2vec2(argv[2])));

  case 53:
    draw_box(js2vec2(argv[1]), js2vec2(argv[2]), js2color(argv[3]));
    return JS_NULL;

  case 54:
    gameobject_apply(js2go(argv[1]));
    return JS_NULL;

  case 55:
    js2go(argv[1])->flipx = JS_ToBool(js, argv[2]) ? -1 : 1;
    return JS_NULL;

  case 56:
    js2go(argv[1])->flipy = JS_ToBool(js, argv[2]) ? -1 : 1;
    return JS_NULL;

  case 57:
    return JS_NewBool(js, js2go(argv[1])->flipx == -1 ? 1 : 0);

  case 58:
    return JS_NewBool(js, js2go(argv[1])->flipy == -1 ? 1 : 0);

  case 59:
    return JS_NewInt64(js, point2segindex(js2vec2(argv[1]), js2cpvec2arr(argv[2]), js2number(argv[3])));

  case 60:
    if (!id2sprite(js2int(argv[1]))) return JS_NULL;
    id2sprite(js2int(argv[1]))->layer = js2int(argv[2]);
    break;

  case 61:
    set_cam_body(id2body(js2int(argv[1])));
    break;

  case 62:
    add_zoom(js2number(argv[1]));
    break;

  case 63:
    return JS_NewFloat64(js, deltaT);

  case 64:
    str = JS_ToCString(js, argv[1]);
    ret = vec2js(tex_get_dimensions(texture_pullfromfile(str)));
    break;

  case 65:
    str = JS_ToCString(js, argv[1]);
    ret = JS_NewBool(js, file_exists(str));
    break;

  case 66:
    str = JS_ToCString(js, argv[1]);
    ret = dukext2paths(str);
    break;

  case 67:
    opengl_rendermode(LIT);
    break;

  case 68:
    opengl_rendermode(WIREFRAME);
    break;

  case 69:
    gameobject_set_sensor(js2int(argv[1]), JS_ToBool(js, argv[2]));
    break;

  case 70:
    return vec2js(world2go(js2go(argv[1]), js2vec2(argv[2])));

  case 71:
    return vec2js(go2world(js2go(argv[1]), js2vec2(argv[2])));

  case 72:
    return vec2js(cpSpaceGetGravity(space));

  case 73:
    cpSpaceSetDamping(space, js2number(argv[1]));
    return JS_NULL;

  case 74:
    return JS_NewFloat64(js, cpSpaceGetDamping(space));

  case 75:
    js2go(argv[1])->layer = js2int(argv[2]);
    return JS_NULL;

  case 76:
    set_cat_mask(js2int(argv[1]), js2bitmask(argv[2]));
    return JS_NULL;

  case 77:
    input_to_game();
    break;

  case 78:
    input_to_nuke();
    break;

  case 79:
    return JS_NewBool(js, phys_stepping());

  case 80:
    return ints2js(phys2d_query_shape(js2ptr(argv[1])));

  case 81:
    draw_arrow(js2vec2(argv[1]), js2vec2(argv[2]), js2color(argv[3]), js2int(argv[4]));
    return JS_NULL;

  case 82:
    gameobject_draw_debug(js2int(argv[1]));
    return JS_NULL;

  case 83:
    draw_edge(js2cpvec2arr(argv[1]), 2, js2color(argv[2]), 1, 0, 0, js2color(argv[2]), 10);
    return JS_NULL;

  case 84:
    return JS_NewString(js, consolelog);

  case 85:
    return vec2js(cpvproject(js2vec2(argv[1]), js2vec2(argv[2])));

  case 86:
    return ints2js(phys2d_query_box_points(js2vec2(argv[1]), js2vec2(argv[2]), js2cpvec2arr(argv[3]), js2int(argv[4])));

  case 87:
    str = JS_ToCString(js, argv[1]);
    mini_music_play(str);
    break;

  case 88:
    mini_music_pause();
    return JS_NULL;

  case 89:
    mini_music_stop();
    return JS_NULL;

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
    return num2js(js2go(argv[1])->scale);

  case 104:
    return bool2js(js2go(argv[1])->flipx);

  case 105:
    return bool2js(js2go(argv[1])->flipy);

  case 106:
    js2go(argv[1])->e = js2num(argv[2]);
    break;

  case 107:
    return num2js(js2go(argv[1])->e);

  case 108:
    js2go(argv[1])->f = js2num(argv[2]);
    break;
  case 109:
    return num2js(js2go(argv[1])->f);
  case 110:
    return num2js(js2go(argv[1])
  }

  if (str)
    JS_FreeCString(js, str);

  if (str2)
    JS_FreeCString(js, str2);

  if (!JS_IsNull(ret))
    return ret;

  return JS_NULL;
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

  return JS_NULL;
}

void gameobject_add_shape_collider(int go, struct callee c, struct phys2d_shape *shape) {
  struct shape_cb shapecb;
  shapecb.shape = shape;
  shapecb.cbs.begin = c;
  arrpush(id2go(go)->shape_cbs, shapecb);
}

JSValue duk_register_collide(JSContext *js, JSValueConst this, int argc, JSValueConst *argv) {
  int cmd = js2int(argv[0]);
  int go = js2int(argv[3]);
  struct callee c;
  c.fn = argv[1];
  c.obj = argv[2];

  switch (cmd) {
  case 0:
    id2go(go)->cbs.begin = c;
    break;

  case 1:
    gameobject_add_shape_collider(go, c, js2ptr(argv[4]));
    break;

  case 2:
    phys2d_rm_go_handlers(go);
    break;

  case 3:
    id2go(go)->cbs.separate = c;
    break;
  }

  return JS_NULL;
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
    sim_stop();
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
    return JS_NewInt64(js, MakeGameobject());

  case 8:
    return JS_NewInt64(js, frame_fps());

  case 9: /* Clear the level out */
    new_level();
    break;
  }

  return JS_NULL;
}

JSValue duk_make_gameobject(JSContext *js, JSValueConst this, int argc, JSValueConst *argv) {
  int g = MakeGameobject();
  struct gameobject *go = get_gameobject_from_id(g);

  go->scale = js2number(argv[0]);
  go->bodytype = js2int(argv[1]);
  go->mass = js2number(argv[2]);
  go->f = js2number(argv[3]);
  go->e = js2number(argv[4]);
  go->flipx = 1.f;
  go->flipy = 1.f;

  gameobject_apply(go);

  return JS_NewInt64(js, g);
}

JSValue duk_yughlog(JSContext *js, JSValueConst this, int argc, JSValueConst *argv) {
  int cmd = js2int(argv[0]);
  const char *s = JS_ToCString(js, argv[1]);
  const char *f = JS_ToCString(js, argv[2]);
  int line = js2int(argv[3]);

  mYughLog(1, cmd, line, f, s);

  JS_FreeCString(js, s);
  JS_FreeCString(js, f);

  return JS_NULL;
}

JSValue duk_set_body(JSContext *js, JSValueConst this, int argc, JSValueConst *argv) {
  int cmd = js2int(argv[0]);
  int id = js2int(argv[1]);
  struct gameobject *go = get_gameobject_from_id(id);

  if (!go) return JS_NULL;

  /* TODO: Possible that reindexing shapes only needs done for static shapes? */
  switch (cmd) {
  case 0:
    gameobject_setangle(go, js2number(argv[2]));
    break;

  case 1:
    go->bodytype = js2int(argv[2]);
    break;

  case 2:
    cpBodySetPosition(go->body, js2vec2(argv[2]));
    break;

  case 3:
    gameobject_move(go, js2vec2(argv[2]));
    break;

  case 4:
    cpBodyApplyImpulseAtWorldPoint(go->body, js2vec2(argv[2]), cpBodyGetPosition(go->body));
    return JS_NULL;

  case 5:
    go->flipx = JS_ToBool(js, argv[2]);
    break;

  case 6:
    go->flipy = JS_ToBool(js, argv[2]);
    break;

  case 7:
    cpBodySetMass(go->body, js2number(argv[2]));
    break;

  case 8:
    cpBodySetAngularVelocity(go->body, js2number(argv[2]));
    return JS_NULL;

  case 9:
    cpBodySetVelocity(go->body, js2vec2(argv[2]));
    return JS_NULL;

  case 10:
    go->e = fmax(js2number(argv[2]), 0);
    break;

  case 11:
    go->f = fmax(js2number(argv[2]), 0);
    break;

  case 12:
    cpBodyApplyForceAtWorldPoint(go->body, js2vec2(argv[2]), cpBodyGetPosition(go->body));
    return JS_NULL;
  }

  cpSpaceReindexShapesForBody(space, go->body);

  return JS_NULL;
}

JSValue duk_q_body(JSContext *js, JSValueConst this, int argc, JSValueConst *argv) {
  int q = js2int(argv[0]);
  struct gameobject *go = get_gameobject_from_id(js2int(argv[1]));

  if (!go) return JS_NULL;

  switch (q) {
  case 0:
    return JS_NewInt64(js, cpBodyGetType(go->body));

  case 1:
    return vec2js(cpBodyGetPosition(go->body));

  case 2:
    return JS_NewFloat64(js, cpBodyGetAngle(go->body));

  case 3:
    return vec2js(cpBodyGetVelocity(go->body));

  case 4:
    return JS_NewFloat64(js, cpBodyGetAngularVelocity(go->body));

  case 5:
    return JS_NewFloat64(js, cpBodyGetMass(go->body));

  case 6:
    return JS_NewFloat64(js, cpBodyGetMoment(go->body));

  case 7:
    return JS_NewBool(js, phys2d_in_air(go->body));
  }

  return JS_NULL;
}

JSValue duk_make_sprite(JSContext *js, JSValueConst this, int argc, JSValueConst *argv) {
  int go = js2int(argv[0]);
  const char *path = JS_ToCString(js, argv[1]);
  HMM_Vec2 pos = js2hmmv2(argv[2]);
  int sprite = make_sprite(go);
  struct sprite *sp = id2sprite(sprite);
  sprite_loadtex(sp, path, ST_UNIT);
  sp->pos = pos;

  JS_FreeCString(js, path);

  return JS_NewInt64(js, sprite);
}

/* Make anim from texture */
JSValue duk_make_anim2d(JSContext *js, JSValueConst this, int argc, JSValueConst *argv) {
  const char *path = JS_ToCString(js, argv[0]);
  int frames = js2int(argv[1]);
  int fps = js2int(argv[2]);

  struct TexAnim *anim = anim2d_from_tex(path, frames, fps);

  JS_FreeCString(js, path);

  return ptr2js(anim);
}

JSValue duk_make_box2d(JSContext *js, JSValueConst this, int argc, JSValueConst *argv) {
  int go = js2int(argv[0]);
  cpVect size = js2vec2(argv[1]);
  cpVect offset = js2vec2(argv[2]);

  struct phys2d_box *box = Make2DBox(go);
  box->w = size.x;
  box->h = size.y;
  box->offset[0] = offset.x;
  box->offset[1] = offset.y;

  phys2d_applybox(box);

  JSValue boxval = JS_NewObject(js);
  JS_SetPropertyStr(js, boxval, "id", ptr2js(box));
  JS_SetPropertyStr(js, boxval, "shape", ptr2js(&box->shape));
  return boxval;
}

JSValue duk_cmd_box2d(JSContext *js, JSValueConst this, int argc, JSValueConst *argv) {
  int cmd = js2int(argv[0]);
  struct phys2d_box *box = js2ptr(argv[1]);
  cpVect arg;

  if (!box) return JS_NULL;

  switch (cmd) {
  case 0:
    arg = js2vec2(argv[2]);
    box->w = arg.x;
    box->h = arg.y;
    break;

  case 1:
    arg = js2vec2(argv[2]);
    box->offset[0] = arg.x;
    box->offset[1] = arg.y;
    break;

  case 2:
    box->rotation = js2number(argv[2]);
    break;
  }

  phys2d_applybox(box);
  return JS_NULL;
}

JSValue duk_make_circle2d(JSContext *js, JSValueConst this, int argc, JSValueConst *argv) {
  int go = js2int(argv[0]);
  double radius = js2number(argv[1]);

  struct phys2d_circle *circle = Make2DCircle(go);
  circle->radius = radius;
  circle->offset = js2vec2(argv[2]);

  phys2d_applycircle(circle);

  JSValue circleval = JS_NewObject(js);
  JS_SetPropertyStr(js, circleval, "id", ptr2js(circle));
  JS_SetPropertyStr(js, circleval, "shape", ptr2js(&circle->shape));
  return circleval;
}

JSValue duk_cmd_circle2d(JSContext *js, JSValueConst this, int argc, JSValueConst *argv) {
  int cmd = js2int(argv[0]);
  struct phys2d_circle *circle = js2ptr(argv[1]);

  if (!circle) return JS_NULL;

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
  return JS_NULL;
}

JSValue duk_make_poly2d(JSContext *js, JSValueConst this, int argc, JSValueConst *argv) {
  int go = js2int(argv[0]);
  struct phys2d_poly *poly = Make2DPoly(go);
  phys2d_poly_setverts(poly, js2cpvec2arr(argv[1]));

  JSValue polyval = JS_NewObject(js);
  JS_SetPropertyStr(js, polyval, "id", ptr2js(poly));
  JS_SetPropertyStr(js, polyval, "shape", ptr2js(&poly->shape));
  return polyval;
}

JSValue duk_cmd_poly2d(JSContext *js, JSValueConst this, int argc, JSValueConst *argv) {
  int cmd = js2int(argv[0]);
  struct phys2d_poly *poly = js2ptr(argv[1]);

  if (!poly) return JS_NULL;

  switch (cmd) {
  case 0:
    phys2d_poly_setverts(poly, js2cpvec2arr(argv[2]));
    break;
  }

  return JS_NULL;
}

JSValue duk_make_edge2d(JSContext *js, JSValueConst this, int argc, JSValueConst *argv) {
  int go = js2int(argv[0]);
  struct phys2d_edge *edge = Make2DEdge(go);

  int n = js_arrlen(argv[1]);
  cpVect points[n];

  for (int i = 0; i < n; i++) {
    points[i] = js2vec2(js_getpropidx(argv[1],i));
    phys2d_edgeaddvert(edge);
    phys2d_edge_setvert(edge, i, points[i]);
  }

  JSValue edgeval = JS_NewObject(js);
  JS_SetPropertyStr(js, edgeval, "id", ptr2js(edge));
  JS_SetPropertyStr(js, edgeval, "shape", ptr2js(&edge->shape));
  return edgeval;
}

JSValue duk_cmd_edge2d(JSContext *js, JSValueConst this, int argc, JSValueConst *argv) {
  int cmd = js2int(argv[0]);
  struct phys2d_edge *edge = js2ptr(argv[1]);

  if (!edge) return JS_NULL;

  switch (cmd) {
  case 0:
    phys2d_edge_clearverts(edge);
    phys2d_edge_addverts(edge, js2cpvec2arr(argv[2]));
    break;

  case 1:
    edge->thickness = js2number(argv[2]);
    break;
  }

  return JS_NULL;
}

JSValue duk_inflate_cpv(JSContext *js, JSValueConst this, int argc, JSValueConst *argv) {
  cpVect *points = js2cpvec2arr(argv[0]);
  int n = js2int(argv[1]);
  double d = js2number(argv[2]);

  cpVect inflate_out[n];
  cpVect inflate_in[n];

  inflatepoints(inflate_out, points, d, n);
  inflatepoints(inflate_in, points, -d, n);

  JSValue arr = JS_NewArray(js);
  JS_SetPropertyUint32(js, arr, 0, vecarr2js(inflate_out, n));
  JS_SetPropertyUint32(js, arr, 1, vecarr2js(inflate_in, n));
  return arr;
}

/* These are anims for controlling properties on an object */
JSValue duk_anim(JSContext *js, JSValueConst this, int argc, JSValueConst *argv) {
  JSValue prop = argv[0];
  int keyframes = js_arrlen(argv[1]);
  YughInfo("Processing %d keyframes.", keyframes);

  struct anim a = make_anim();

  for (int i = 0; i < keyframes; i++) {
    struct keyframe k;
    cpVect v = js2vec2(js_getpropidx( argv[1], i));
    k.time = v.y;
    k.val = v.x;
    a = anim_add_keyframe(a, k);
  }

  for (double i = 0; i < 3.0; i = i + 0.1) {
    YughInfo("Val is now %f at time %f", anim_val(a, i), i);
    JSValue vv = num2js(anim_val(a, i));
    JS_Call(js, prop, globalThis, 1, &vv);
  }

  return JS_NULL;
}

JSValue duk_make_timer(JSContext *js, JSValueConst this, int argc, JSValueConst *argv) {
  double secs = js2number(argv[1]);
  struct callee *c = malloc(sizeof(*c));
  c->fn = JS_DupValue(js, argv[0]);
  c->obj = JS_DupValue(js, globalThis);
  int id = timer_make(secs, call_callee, c, 1);

  return JS_NewInt64(js, id);
}

#define DUK_FUNC(NAME, ARGS) JS_SetPropertyStr(js, globalThis, #NAME, JS_NewCFunction(js, duk_##NAME, #NAME, ARGS));

void ffi_load() {
  globalThis = JS_GetGlobalObject(js);

  DUK_FUNC(yughlog, 4)
  DUK_FUNC(nuke, 6)
  DUK_FUNC(make_gameobject, 7)
  DUK_FUNC(set_body, 3)
  DUK_FUNC(q_body, 2)

  DUK_FUNC(sys_cmd, 1)
  DUK_FUNC(win_make, 3)

  DUK_FUNC(make_sprite, 3)
  DUK_FUNC(make_anim2d, 3)
  DUK_FUNC(spline_cmd, 6)

  DUK_FUNC(make_box2d, 3)
  DUK_FUNC(cmd_box2d, 6)
  DUK_FUNC(make_circle2d, 3)
  DUK_FUNC(cmd_circle2d, 6)
  DUK_FUNC(make_poly2d, 2)
  DUK_FUNC(cmd_poly2d, 6)
  DUK_FUNC(make_edge2d, 3)
  DUK_FUNC(cmd_edge2d, 6)
  DUK_FUNC(make_timer, 3)

  DUK_FUNC(cmd, 6)
  DUK_FUNC(register, 3)
  DUK_FUNC(register_collide, 6)

  DUK_FUNC(gui_text, 6)
  DUK_FUNC(ui_text, 5)
  DUK_FUNC(cursor_text, 5)
  DUK_FUNC(gui_img, 2)

  DUK_FUNC(inflate_cpv, 3)

  DUK_FUNC(anim, 2)
}
