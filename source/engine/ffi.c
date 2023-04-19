#include "ffi.h"

#include "script.h"

#include "string.h"
#include "debug.h"
#include "window.h"
#include "editor.h"
#include "engine.h"
#include "log.h"
#include "input.h"
#include "gameobject.h"
#include "openglrender.h"
#include "2dphysics.h"
#include "sprite.h"
#include "anim.h"
#include "yugine.h"
#include "nuke.h"
#include "sound.h"
#include "font.h"
#include "sound.h"
#include "music.h"
#include "level.h"
#include "tinyspline.h"
#include "mix.h"
#include "debugdraw.h"
#include "stb_ds.h"
#include <ftw.h>

#include "miniaudio.h"

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte) \
        (byte & 0x80 ? '1' : '0'), \
        (byte & 0x40 ? '1' : '0'), \
	(byte & 0x20 ? '1' : '0'), \
        (byte & 0x10 ? '1' : '0'), \
        (byte & 0x08 ? '1' : '0'), \
        (byte & 0x04 ? '1' : '0'), \
        (byte & 0x02 ? '1' : '0'), \
        (byte & 0x01 ? '1' : '0')

int js2int(JSValue v)
{
  int32_t i;
  JS_ToInt32(js,&i,v);
  return i;
}

JSValue int2js(int i)
{
  return JS_NewInt64(js, i);
}

double js2number(JSValue v)
{
  double g;
  JS_ToFloat64(js,&g, v);
  return g;
}

JSValue float2js(double g)
{
  return JS_NewFloat64(js,g);
}

JSValue num2js(double g)
{
  return float2js(g);
}

struct gameobject *js2go(JSValue v)
{
  return id2go(js2int(v));
}

void *js2ptr(JSValue v)
{
  void *p;
  JS_ToInt64(js, &p, v);
  return p;
}

JSValue ptr2js(void *ptr)
{
  return JS_NewInt64(js,(long)ptr);
}

struct timer *js2timer(JSValue v)
{
  return id2timer(js2int(v));
}


double js_get_prop_number(JSValue v, const char *p)
{
  double num;
  JS_ToFloat64(js, &num, JS_GetPropertyStr(js, v, p));
  return num;
}

struct glrect js2glrect(JSValue v)
{
  struct glrect rect;
  rect.s0 = js_get_prop_number(v,"s0");
  rect.s1 = js_get_prop_number(v,"s1");
  rect.t0 = js_get_prop_number(v,"t0");
  rect.t1 = js_get_prop_number(v,"t1");
  return rect;
}

JSValue js_arridx(JSValue v, int idx)
{
  return JS_GetPropertyUint32(js,v,idx);
}

int js_arrlen(JSValue v)
{
  int len;
  JS_ToInt32(js, &len, JS_GetPropertyStr(js, v, "length"));
  return len;
}
struct color js2color(JSValue v)
{
    struct color color = {0,0,0};
    color.r = js2int(js_arridx(v,0));
    color.g = js2int(js_arridx(v,1));
    color.b = js2int(js_arridx(v,2));
    return color;
}

cpVect js2vec2(JSValue v)
{
  cpVect vect;
  vect.x = js2number(js_arridx(v,0));
  vect.y = js2number(js_arridx(v,1));
  return vect;
}

cpBitmask js2bitmask(JSValue v)
{
    cpBitmask mask = 0;
    int len = js_arrlen(v);

    for (int i = 0; i < len; i++) {
      int val = JS_ToBool(js, JS_GetPropertyUint32(js, v, i));
      if (!val) continue;

      mask |= 1<<i;
    }

    return mask;
}

cpVect *js2cpvec2arr(JSValue v)
{
  int n = js_arrlen(v);
  cpVect *points = NULL;

  for (int i = 0; i < n; i++)
    arrput(points, js2vec2(JS_GetPropertyUint32(js, v, i)));

  return points;
}

JSValue bitmask2js(cpBitmask mask)
{
  JSValue arr = JS_NewObject(js);
  for (int i = 0; i < 11; i++) {
    int on = mask & 1<<i;
    JS_SetPropertyUint32(js, arr, i, JS_NewBool(js, on));
  }

  return arr;
}

void vec2float(cpVect v, float *f)
{
    f[0] = v.x;
    f[1] = v.y;
}

JSValue vec2js(cpVect v)
{
  JSValue array = JS_NewArray(js);
  JS_SetPropertyInt64(js, array, 0, JS_NewFloat64(js,v.x));
  JS_SetPropertyInt64(js, array, 1, JS_NewFloat64(js,v.y));
  return array;
}

JSValue vecarr2js(cpVect *points, int n)
{
  JSValue array = JS_NewObject(js);
  for (int i = 0; i < n; i++) 
    JS_SetPropertyInt64(js, array, i, vec2js(points[i]));
  return array;
}

JSValue duk_gui_text(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
    const char *s = JS_ToCString(js, argv[0]);
    cpVect pos = js2vec2(argv[1]);

    float size = js2number(argv[2]);
    const float white[3] = {1.f, 1.f, 1.f};
    renderText(s, &pos, size, white, 500,-1);
    return JS_NULL;
}

JSValue duk_ui_text(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
    const char *s = JS_ToCString(js,argv[0]);
    cpVect pos = js2vec2(argv[1]);

    float size = js2number(argv[2]);
    struct color c = js2color(argv[3]);
    const float col[3] = {(float)c.r/255, (float)c.g/255, (float)c.b/255};
    int wrap = js2int(argv[4]);
    return JS_NewInt64(js, renderText(s, &pos, size, col, wrap,-1));
}

JSValue duk_cursor_text(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
    const char *s = JS_ToCString(js,argv[0]);
    cpVect pos = js2vec2(argv[1]);

    float size = js2number(argv[2]);
    struct color c = js2color(argv[3]);
    const float col[3] = {(float)c.r/255, (float)c.g/255, (float)c.b/255};
    int wrap = js2int(argv[5]);
    int cursor = js2int(argv[4]);
    renderText(s, &pos, size, col, wrap,cursor);
    return JS_NULL;
}

JSValue duk_gui_img(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
    const char *img = JS_ToCString(js,argv[0]);
    cpVect pos = js2vec2(argv[1]);
    gui_draw_img(img, pos.x, pos.y);
    return JS_NULL;
}

struct nk_rect js2nk_rect(JSValue v)
{
  struct nk_rect rect;
  rect.x = js2number(JS_GetPropertyStr(js, v, "x"));
  rect.y = js2number(JS_GetPropertyStr(js, v, "y"));
  rect.w = js2number(JS_GetPropertyStr(js, v, "w"));
  rect.h = js2number(JS_GetPropertyStr(js, v, "h"));
  return rect;
}

JSValue nk_rect2js(struct nk_rect rect)
{
  JSValue obj = JS_NewObject(js);
  JS_SetPropertyStr(js, obj, "x", JS_NewFloat64(js, rect.x));
  JS_SetPropertyStr(js, obj, "y", JS_NewFloat64(js,rect.y));
  JS_SetPropertyStr(js, obj, "w", JS_NewFloat64(js, rect.w));
  JS_SetPropertyStr(js, obj, "h", JS_NewFloat64(js, rect.h));
  return obj;
}

JSValue duk_nuke(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
    int cmd = js2int(argv[0]);
    float editnum;
    int editint;
    char textbox[130];
    struct nk_rect rect = nk_rect(0,0,0,0);

    switch(cmd) {
        case 0:
          nuke_begin(JS_ToCString(js, argv[1]),js2nk_rect(argv[1]), NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_TITLE);
          break;

        case 1:
          nuke_stop();
          break;

        case 2:
          editnum = js2number(argv[2]);
          nuke_property_float(JS_ToCString(js,argv[1]), js2number(argv[3]), &editnum, js2number(argv[4]), js2number(argv[5]), js2number(argv[5]));
	  return JS_NewFloat64(js, editnum);

        case 3:
          nuke_nel(js2number(argv[1]));
          return JS_NULL;

        case 4:
          editint = JS_ToBool(js, argv[2]);
          nuke_checkbox(JS_ToCString(js, argv[1]), &editint);
	  return JS_NewBool(js,editint);

        case 5:
          nuke_label(JS_ToCString(js, argv[1]));
          return JS_NULL;

	case 6:
	  return JS_NewBool(js, nuke_btn(JS_ToCString(js, argv[1])));

	case 7:
	  strncpy(textbox, JS_ToCString(js, argv[1]), 1000);
	  nuke_edit_str(textbox);
	  return JS_NewString(js,textbox);

	case 8:
	  nuke_img(JS_ToCString(js, argv[1]));
	  break;

	case 9:
	  editint = js2int(argv[2]);
	  nuke_radio_btn(JS_ToCString(js, argv[1]), &editint, js2int(argv[3]));
	  return JS_NewInt64(js, editint);
	
	case 10:
	  rect = nuke_win_get_bounds();
	  return nk_rect2js(rect);

	case 11:
	  return JS_NewBool(js,nuke_push_tree_id(JS_ToCString(js, argv[1]), js2int(argv[2])));

	case 12:
	  nuke_tree_pop();
	  return JS_NULL;

	case 13:
	  nuke_row(js2int(argv[1]));
	  break;

	case 14:
	  nuke_scrolltext(JS_ToCString(js, argv[1]));
	  break;

	case 15:
	  nuke_nel_h(js2int(argv[1]), js2int(argv[2]));
	  break;
    }

    return JS_NULL;
}

JSValue duk_win_make(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
    const char *title = JS_ToCString(js, argv[0]);
    int w = js2int(argv[1]);
    int h = js2int(argv[2]);
    struct window *win = MakeSDLWindow(title, w, h, 0);

    return JS_NewInt64(js, win->id);
}

JSValue duk_spline_cmd(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
    tsBSpline spline;

    int n = js_arrlen(argv[4]);
    int d = js2int(argv[2]);
    cpVect points[n*d]; 

    tsStatus status;
    ts_bspline_new(n, d, js2int(argv[1]), js2int(argv[3]), &spline, &status);

    if (status.code)
      YughCritical("Spline creation error %d: %s", status.code, status.message);

    for (int i = 0; i < n; i++)
      points[i] = js2vec2(JS_GetPropertyUint32(js, argv[4], i));

  ts_bspline_set_control_points(&spline, points, NULL);


  size_t nsamples = js2int(argv[5]);
  cpVect samples[nsamples];
  static_assert(sizeof(tsReal)*2 == sizeof(cpVect));
  size_t rsamples;
  ts_bspline_sample(&spline, nsamples, &samples, &rsamples, NULL);

  JSValue arr = JS_NewObject(js);

  for (int i = 0; i < nsamples; i++) {
    JSValue psample;
    JS_SetPropertyUint32(js, psample, 0, float2js(samples[i].x));
    JS_SetPropertyUint32(js, psample, 1, float2js(samples[i].y));
    JS_SetPropertyUint32(js, arr, i, psample);
  }

  ts_bspline_free(&spline);

  return arr;
}


JSValue ints2js(int *ints)
{
  JSValue arr = JS_NewObject(js);
  for (int i = 0; i < arrlen(ints); i++)
    JS_SetPropertyUint32(js, arr, i, int2js(ints[i]));

  return arr;
}

int vec_between(cpVect p, cpVect a, cpVect b)
{
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
int point2segindex(cpVect p, cpVect *segs, double slop)
{
  float shortest = slop < 0 ? INFINITY : slop;
  int best = -1;

  for (int i = 0; i < arrlen(segs)-1; i++)
  {
    float a = (segs[i+1].y - segs[i].y) / (segs[i+1].x - segs[i].x);
    float c = segs[i].y - (a * segs[i].x);
    float b = -1;

    float dist = abs(a*p.x + b*p.y + c) / sqrt(pow(a,2) + 1);

    if (dist > shortest) continue;

    int between = vec_between(p, segs[i], segs[i+1]);

    if (between) {
      shortest = dist;
      best = i+1;
    } else {
      if (i == 0 && cpvdist(p, segs[0]) < slop) {
        shortest = dist;
	best = i;
      } else if (i == arrlen(segs)-2 && cpvdist(p, arrlast(segs)) < slop) {
        shortest = dist;
	best = arrlen(segs);
      }
    }
  }

  if (best == 1) {
    cpVect n;
    n.x = segs[1].x-segs[0].x;
    n.y = segs[1].y-segs[0].y;
    n = cpvnormalize(n);
    if (cpvdot(n, cpvsub(p, segs[0])) < 0)
      if (cpvdist(p, segs[0]) >= slop)
        best = -1;
      else
        best = 0;
  }

  if (best == arrlen(segs)-1) {
    cpVect n;
    n.x = segs[best-1].x-segs[best].x;
    n.y = segs[best-1].y-segs[best-1].y;
    n = cpvnormalize(n);

    if (cpvdot(n, cpvsub(p, segs[best])) < 0)
      if (cpvdist(p, segs[best]) >= slop)
        best = -1;
      else
        best = arrlen(segs);
  }
  

  return best;
}

int file_exists(char *path)
{
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

static int duk2path(const char *path, const struct stat *sb, int typeflag)
{
  if (typeflag == FTW_F) {
    char *ext = strrchr(path, '.');
    if (ext && !strcmp(ext, dukext))
      JS_SetPropertyUint32(js, dukarr, dukidx++, JS_NewString(js, &path[2]));
  }
  
  return 0;
}

JSValue dukext2paths(char *ext)
{
  dukext = ext;
  dukarr = JS_NewObject(js);
  dukidx = 0;
  ftw(".", duk2path, 10);
  return dukarr;
}

JSValue duk_cmd(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
  int cmd = js2int(argv[0]);

  switch(cmd) {
    case 0:
      return JS_NewInt64(js, script_dofile(JS_ToCString(js, argv[1])));

      case 1:
        YughWarn("Do not set pawns here anymore; Do it entirely in script.");
        //set_pawn(js2ptrduk_get_heapptr(duk, 1));
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
         return JS_NewInt64(js, file_mod_secs(JS_ToCString(js, argv[1])));

       case 12:
         sprite_loadtex(id2sprite(js2int(argv[1])), JS_ToCString(js, argv[2]), js2glrect(argv[3]));
         break;

       case 13:
         play_song(JS_ToCString(js, argv[1]), JS_ToCString(js, argv[2]));
         break;

       case 14:
         mini_sound(JS_ToCString(js, argv[1]));
         break;

       case 15:
         music_stop();
         break;

       case 16:
         color2float(js2color(argv[1]), dbg_color);
         break;

       case 17:
         color2float(js2color(argv[1]), trigger_color);
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
         vec2float(js2vec2(argv[2]), id2sprite(js2int(argv[1]))->pos);
         break;

       case 38:
         return JS_NewString(js, slurp_text(JS_ToCString(js, argv[1])));

       case 39:
         return JS_NewInt64(js, slurp_write(JS_ToCString(js, argv[1]), JS_ToCString(js, argv[2])));

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
	  draw_grid(js2int(argv[1]), js2int(argv[2]));
	  return JS_NULL;

	case 48:
	  return JS_NewInt64(js, mainwin->width);

	case 49:
	  return JS_NewInt64(js,  mainwin->height);
	  
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
	  return vec2js(tex_get_dimensions(texture_pullfromfile(JS_ToCString(js, argv[1]))));
	  
	case 65:
	  return JS_NewBool(js, file_exists(JS_ToCString(js, argv[1])));
	  
	case 66:
	  return dukext2paths(JS_ToCString(js, argv[1]));

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
	  draw_edge(js2cpvec2arr(argv[1]), 2, js2color(argv[2]), 1);
	  return JS_NULL;

	case 84:
	  return JS_NewString(js, consolelog);

	case 85:
	  return vec2js(cpvproject(js2vec2(argv[1]), js2vec2(argv[2])));
	  
	case 86:
          return ints2js(phys2d_query_box_points(js2vec2(argv[1]), js2vec2(argv[2]), js2cpvec2arr(argv[3]), js2int(argv[4])));

        case 87:
	  mini_music_play(JS_ToCString(js, argv[1]));
	  return JS_NULL;

	case 88:
	  mini_music_pause();
	  return JS_NULL;

	case 89:
	  mini_music_stop();
	  return JS_NULL;

	case 90:
	  window_set_icon(JS_ToCString(js, argv[1]));
	  break;
    }

    return JS_NULL;
}

JSValue duk_register(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
    int cmd = js2int(argv[0]);

   struct callee c;
   c.fn = argv[1];
   c.obj = argv[2];

   switch(cmd) {
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
     }

   return JS_NULL;

}

void gameobject_add_shape_collider(int go, struct callee c, struct phys2d_shape *shape)
{
  struct shape_cb shapecb;
  shapecb.shape = shape;
  shapecb.cbs.begin = c;
  arrpush(id2go(go)->shape_cbs, shapecb);
}

JSValue duk_register_collide(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
    int cmd = js2int(argv[0]);
    int go = js2int(argv[3]);
    struct callee c = {argv[1], argv[2]};

    switch(cmd) {
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

JSValue duk_sys_cmd(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
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
            return JS_NewInt64(js,  MakeGameobject());
            

        case 8:
            return JS_NewInt64(js,  frame_fps());
            

        case 9: /* Clear the level out */
            new_level();
            break;

    }

    return JS_NULL;
}

JSValue duk_make_gameobject(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
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

    return JS_NewInt64(js,g);
}

JSValue duk_yughlog(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
    int cmd = js2int(argv[0]);
    const char *s = JS_ToCString(js, argv[1]);
    const char *f = JS_ToCString(js, argv[2]);
    int line = js2int(argv[3]);

    mYughLog(1, cmd, line, f, s);

    return JS_NULL;
}

JSValue duk_set_body(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
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
      go->e = fmax(js2number(argv[2]),0);
      break;

    case 11:
      go->f = fmax(js2number(argv[2]),0);
      break;

    case 12:
      cpBodyApplyForceAtWorldPoint(go->body, js2vec2(argv[2]), cpBodyGetPosition(go->body));
      return JS_NULL;
  }

  cpSpaceReindexShapesForBody(space, go->body);

  return JS_NULL;
}

JSValue duk_q_body(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
    int q = js2int(argv[0]);
    struct gameobject *go = get_gameobject_from_id(js2int(argv[1]));

    if (!go) return JS_NULL;

    switch(q) {
        case 0:
          return JS_NewInt64(js,  cpBodyGetType(go->body));

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

JSValue duk_make_sprite(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
  int go = js2int(argv[0]);
  const char *path = JS_ToCString(js, argv[1]);
  cpVect pos = js2vec2(argv[2]);
  int sprite = make_sprite(go);
  struct sprite *sp = id2sprite(sprite);
  sprite_loadtex(sp, path, ST_UNIT);
  sp->pos[0] = pos.x;
  sp->pos[1] = pos.y;

  return JS_NewInt64(js,  sprite);
  
}

/* Make anim from texture */
JSValue duk_make_anim2d(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
  const char *path = JS_ToCString(js, argv[0]);
  int frames = js2int(argv[1]);
  int fps = js2int(argv[2]);

  struct TexAnim *anim = anim2d_from_tex(path, frames, fps);

  return ptr2js(anim);
}

JSValue duk_make_box2d(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
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

JSValue duk_cmd_box2d(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
    int cmd = js2int(argv[0]);
    struct phys2d_box *box = js2ptr(argv[1]);
    cpVect arg;

    if (!box) return JS_NULL;

    switch(cmd) {
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

JSValue duk_make_circle2d(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
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

JSValue duk_cmd_circle2d(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
    int cmd = js2int(argv[0]);
    struct phys2d_circle *circle = js2ptr(argv[1]);

    if (!circle) return JS_NULL;

    switch(cmd) {
        case 0:
          circle->radius = js2number(argv[2]);
          break;

        case 1:
          circle->offset = js2vec2(argv[2]);
          break;
    }

    phys2d_applycircle(circle);
    return JS_NULL;
}

JSValue duk_make_poly2d(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
  int go = js2int(argv[0]);
  struct phys2d_poly *poly = Make2DPoly(go);
  phys2d_poly_setverts(poly, js2cpvec2arr(argv[1]));

  JSValue polyval = JS_NewObject(js);
  JS_SetPropertyStr(js, polyval, "id", ptr2js(poly));
  JS_SetPropertyStr(js, polyval, "shape", ptr2js(&poly->shape));
  return polyval;
}

JSValue duk_cmd_poly2d(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
  int cmd = js2int(argv[0]);
  struct phys2d_poly *poly = js2ptr(argv[1]);

  if (!poly) return JS_NULL;
  
  switch(cmd) {
    case 0:
      phys2d_poly_setverts(poly, js2cpvec2arr(argv[2]));
      break;
  }
  
  return JS_NULL;  
}

JSValue duk_make_edge2d(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
  int go = js2int(argv[0]);
  struct phys2d_edge *edge = Make2DEdge(go);

  int arridx = 1;

  int n = js_arrlen(argv[1]);
  cpVect points[n];

  for (int i = 0; i < n; i++) {
      points[i] = js2vec2(JS_GetPropertyUint32(js, argv[1], i));
      phys2d_edgeaddvert(edge);
      phys2d_edge_setvert(edge, i, points[i]);
  }

  JSValue edgeval = JS_NewObject(js);
  JS_SetPropertyStr(js, edgeval, "id", ptr2js(edge));
  JS_SetPropertyStr(js, edgeval, "shape", ptr2js(&edge->shape));
  return edgeval;
}

JSValue duk_cmd_edge2d(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
  int cmd = js2int(argv[0]);
  struct phys2d_edge *edge = js2ptr(argv[1]);

  if (!edge) return JS_NULL;

  switch(cmd) {
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

JSValue duk_inflate_cpv(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
  cpVect *points = js2cpvec2arr(argv[0]);
  int n = js2int(argv[1]);
  double d = js2number(argv[2]);

  cpVect inflate_out[n];
  cpVect inflate_in[n];
  
  inflatepoints(inflate_out, points, d, n);
  inflatepoints(inflate_in, points, -d, n);
  
  JSValue arr = JS_NewObject(js);
  JS_SetPropertyUint32(js, arr, 0, vecarr2js(inflate_out, n));
  JS_SetPropertyUint32(js, arr, 1, vecarr2js(inflate_in, n));
  return arr;
}

/* These are anims for controlling properties on an object */
JSValue duk_anim(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
    JSValue prop = argv[0];
    int keyframes = js_arrlen(argv[1]);
    YughInfo("Processing %d keyframes.", keyframes);

    struct anim a = make_anim();

    for (int i  = 0; i < keyframes; i++) {
        struct keyframe k;
	cpVect v = js2vec2(JS_GetPropertyUint32(js, argv[1], i));
        k.time = v.y;
        k.val = v.x;
        a = anim_add_keyframe(a, k);
    }

    for (double i = 0; i < 3.0; i = i + 0.1) {
        YughInfo("Val is now %f at time %f", anim_val(a, i), i);
	JSValue vv = num2js(anim_val(a,i));
	JS_Call(js, prop, JS_GetGlobalObject(js), 1, &vv);
    }

    return JS_NULL;
}

JSValue duk_make_timer(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
    double secs = js2number(argv[1]);
    struct callee *c = malloc(sizeof(*c));
    c->fn = argv[0];
    c->obj = argv[2];
    int id = timer_make(secs, call_callee, c, 1);

    return JS_NewInt64(js,  id);
}

#define DUK_FUNC(NAME, ARGS) JS_SetPropertyStr(js, JS_GetGlobalObject(js), #NAME, JS_NewCFunction(js, duk_##NAME, #NAME, ARGS));

void ffi_load()
{
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
    DUK_FUNC(cursor_text,5)
    DUK_FUNC(gui_img, 2)
    
    DUK_FUNC(inflate_cpv, 3)

    DUK_FUNC(anim, 2)
}
