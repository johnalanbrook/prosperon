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
  int i;
  JS_ToInt32(js,&i,v);
  return i;
}

struct gameobject *js2go(JSValue v)
{
  return id2go(js2int(v));
}

struct timer *js2timer(JSValue v)
{
  return id2timer(js2int(v));
}

struct color duk2color(JSValue v)
{
    struct color color;

    JS_ToInt64(js, &color.r, JS_GetPropertyUint32(js,v,0));
    JS_ToInt64(js, &color.g, JS_GetPropertyUint32(js,v,1));
    JS_ToInt64(js, &color.b, JS_GetPropertyUint32(js,v,2));

    return color;
}

float js_get_prop_number(JSValue v, const char *p)
{
  float num;
  JS_ToFloat64(js, &num, JS_GetPropertyStr(js, v, p));
  return num;
}

struct glrect js_to_glrect(JSValue v)
{
  struct glrect rect;
  rect.s0 = js_get_prop_number(v,"s0");
  rect.s1 = js_get_prop_number(v,"s1");
  rect.t0 = js_get_prop_number(v,"t0");
  rect.t1 = js_get_prop_number(v,"t1");
  return rect;
}

int js_arrlen(JSValue v)
{
  int len;
  JS_ToInt32(js, &len, JS_GetProperty(js, v, JS_PROP_LENGTH));
  return len;
}

cpVect js2vec2(JSValue v)
{
  cpVect vect;
  JS_ToInt64(js, &vect.x, JS_GetPropertyUint32(js, v, 0));
  JS_ToInt64(js, &vect.y, JS_GetPropertyUint32(js,v,1));
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
  JSValue array = JS_NewObject(js);
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

    float size = js_to_number(argv[2]);
    const float white[3] = {1.f, 1.f, 1.f};
    renderText(s, &pos, size, white, 500,-1);
    return JS_NULL;
}

JSValue duk_ui_text(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
    const char *s = JS_ToCString(js,argv[0]);
    cpVect pos = js2vec2(argv[1]);

    float size = js_to_number(argv[2]);
    struct color c = js2color(duk,3);
    const float col[3] = {(float)c.r/255, (float)c.g/255, (float)c.b/255};
    int wrap = js_to_int(argv[4]);
    return JS_NewInt64(js, renderText(s, &pos, size, col, wrap,-1));
}

JSValue duk_cursor_text(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
    const char *s = JS_ToCString(js,argv[0]);
    cpVect pos = js2vec2(argv[1]);

    float size = js_to_number(argv[2]);
    struct color c = js2color(duk,3);
    const float col[3] = {(float)c.r/255, (float)c.g/255, (float)c.b/255};
    int wrap = js_to_int(argv[5]);
    int cursor = js_to_int(argv[4]);
    renderText(s, &pos, size, col, wrap,cursor);
    return JS_NULL;
}

JSValue duk_gui_img(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
    const char *img = JS_ToCString(js,argv[0]);
    cpVect pos = js2devc2(argv[1]);
    gui_draw_img(img, pos.x, pos.y);
    return JS_NULL;
}

JSValue duk_nuke(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
    int cmd = js_to_int(argv[0]);
    float editnum;
    int editint;
    char textbox[130];
    struct nk_rect rect = nk_rect(0,0,0,0);

    switch(cmd) {
        case 0:
	  rect.x = js_to_number(JS_GetPropertyStr(js, argv[1], "x"));
	  rect.y = js_to_number(JS_GetPropertyStr(js, argv[1], "y"));
	  rect.w = js_to_number(JS_GetPropertyStr(js, argv[1], "w"));
	  rect.h = js_to_number(JS_GetPropertyStr(js, argv[1], "h"));
          nuke_begin(duk_to_string(duk, 1),rect, NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_TITLE);
          break;

        case 1:
          nuke_stop();
          break;

        case 2:
          editnum = js_to_number(argv[2]);
          nuke_property_float(JS_ToCString(js,argv[1]), js_to_number(argv[3]), &editnum, js_to_number(argv[4]), js_to_number(argv[5]), js_to_number(argv[5]));
          duk_push_number(duk, editnum);
	  return JS_NewFloat64(js, editnum);

        case 3:
          nuke_nel(js_to_number(argv[1]));
          return 0;

        case 4:
          editint = duk_to_boolean(duk, 2);
          nuke_checkbox(duk_to_string(duk, 1), &editint);
          duk_push_boolean(duk, editint);
          return 1;

        case 5:
          nuke_label(duk_to_string(duk, 1));
          return 0;

	case 6:
	  duk_push_boolean(duk, nuke_btn(duk_to_string(duk, 1)));
	  return 1;

	case 7:
	  strncpy(textbox, duk_to_string(duk, 1), 130);
	  nuke_edit_str(textbox);
	  duk_push_string(duk, textbox);
	  return 1;

	case 8:
	  nuke_img(duk_to_string(duk, 1));
	  break;

	case 9:
	  editint = duk_to_int(duk,2);
	  nuke_radio_btn(duk_to_string(duk,1), &editint, duk_to_int(duk, 3));
	  duk_push_int(duk, editint);
	  return 1;
	
	case 10:
	  rect = nuke_win_get_bounds();
	  duk_push_object(duk);
	  duk_push_number(duk, rect.x);
	  duk_put_prop_string(duk, -2, "x");
	  duk_push_number(duk,rect.y);
	  duk_put_prop_string(duk, -2, "y");
	  duk_push_number(duk,rect.w);
	  duk_put_prop_string(duk,-2,"w");
	  duk_push_number(duk,rect.h);
	  duk_put_prop_string(duk,-2,"h");
	  return 1;

	case 11:
	  duk_push_boolean(duk, nuke_push_tree_id(duk_to_string(duk, 1), duk_to_int(duk,2)));
	  return 1;

	case 12:
	  nuke_tree_pop();
	  return 0;

	case 13:
	  nuke_row(duk_to_int(duk,1));
	  break;

	case 14:
	  nuke_scrolltext(duk_to_string(duk,1));
	  break;

	case 15:
	  nuke_nel_h(duk_to_int(duk,1), duk_to_int(duk,2));
	  break;
    }

    return 0;
}

JSValue duk_win_make(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
    const char *title = duk_to_string(duk, 0);
    int w = duk_to_int(duk, 1);
    int h = duk_to_int(duk, 2);
    struct window *win = MakeSDLWindow(title, w, h, 0);

    duk_push_int(duk, win->id);
    return 1;
}

JSValue duk_spline_cmd(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
    tsBSpline spline;

    int n = duk_get_length(duk, 4);
    int d = duk_to_int(duk, 2);
    cpVect points[n*d]; 

    tsStatus status;
    ts_bspline_new(n, d, duk_to_int(duk, 1), duk_to_int(duk, 3), &spline, &status);

    if (status.code) {
      YughCritical("Spline creation error %d: %s", status.code, status.message);
    }

    for (int i = 0; i < n; i++) {
      duk_get_prop_index(duk, 4, i);
      points[i] = duk2vec2(duk, -1);
      duk_pop(duk);
    }

  ts_bspline_set_control_points(&spline, points, NULL);


  size_t nsamples = duk_to_int(duk, 5);
  cpVect samples[nsamples];
  static_assert(sizeof(tsReal)*2 == sizeof(cpVect));
  size_t rsamples;
  ts_bspline_sample(&spline, nsamples, &samples, &rsamples, NULL);

  int arridx = duk_push_array(duk);

  duk_require_stack(duk, nsamples*3);

  for (int i = 0; i < nsamples; i++) {
    int pidx = duk_push_array(duk);
    duk_push_number(duk, samples[i].x);
    duk_put_prop_index(duk, pidx, 0);
    duk_push_number(duk, samples[i].y);
    duk_put_prop_index(duk, pidx, 1);
    duk_put_prop_index(duk, arridx, i);
  }

  ts_bspline_free(&spline);

  return 1;
}


void ints2duk(int *ints)
{
  int idx = duk_push_array(duk);
  
  for (int i = 0; i < arrlen(ints); i++) {
    duk_push_int(duk, ints[i]);
    duk_put_prop_index(duk, idx, i);
  }
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
static int dukarr;
static int dukidx;

static int duk2path(const char *path, const struct stat *sb, int typeflag)
{
  if (typeflag == FTW_F) {
    char *ext = strrchr(path, '.');
    if (ext && !strcmp(ext, dukext)) {
      duk_push_string(duk, &path[2]);
      duk_put_prop_index(duk, dukarr, dukidx++);
    }
  }
  
  return 0;
}

void dukext2paths(char *ext)
{
  dukext = ext;
  dukarr = duk_push_array(duk);
  dukidx = 0;
  ftw(".", duk2path, 10);
}

JSValue duk_cmd(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
    int cmd = duk_to_int(duk, 0);

  switch(cmd) {
    case 0:
      duk_push_int(duk, script_dofile(duk_to_string(duk, 1)));
      return 1;

      case 1:
        set_pawn(duk_get_heapptr(duk, 1));
        break;

       case 2:
         gameobject_delete(duk_get_int(duk, 1));
         break;

       case 3:
         set_timescale(duk_get_number(duk,1));
         break;

       case 4:
         debug_draw_phys(duk_get_boolean(duk, 1));
         break;

       case 5:
         renderMS = duk_get_number(duk, 1);
         break;

       case 6:
         updateMS = duk_get_number(duk, 1);
         break;

       case 7:
         physMS = duk_get_number(duk, 1);
         break;

       case 8:
         phys2d_set_gravity(duk2vec2(duk, 1));
         break;

       case 9:
         sprite_delete(duk_to_int(duk, 1));
         break;

       case 10:
         remove_pawn(duk_get_heapptr(duk, 1));
         break;

       case 11:
         duk_push_int(duk, file_mod_secs(duk_to_string(duk, 1)));
         return 1;
         break;

       case 12:
         sprite_loadtex(id2sprite(duk_to_int(duk, 1)), duk_to_string(duk, 2), duk_to_glrect(duk, 3));
         break;

       case 13:
         play_song(duk_to_string(duk, 1), duk_to_string(duk, 2));
         break;

       case 14:
         mini_sound(duk_to_string(duk,1));
         break;

       case 15:
         music_stop();
         break;

       case 16:
         color2float(js2color(duk, 1), dbg_color);
         break;

       case 17:
         color2float(js2color(duk, 1), trigger_color);
         break;

       case 18:
         shape_set_sensor(duk_to_pointer(duk, 1), duk_to_boolean(duk, 2));
         break;

       case 19:
         mini_master(duk_to_number(duk,1));
         break;

       case 20:
         sprite_enabled(duk_to_int(duk, 1), duk_to_boolean(duk, 2));
         break;

       case 21:
         duk_push_boolean(duk, shape_get_sensor(duk_to_pointer(duk, 1)));
         return 1;

       case 22:
         shape_enabled(duk_to_pointer(duk, 1), duk_to_boolean(duk, 2));
         break;

       case 23:
         duk_push_boolean(duk, shape_is_enabled(duk_to_pointer(duk, 1)));
         return 1;

       case 24:
         timer_pause(js2timer(duk,1));
         break;

       case 25:
         timer_stop(js2timer(duk,1));
         break;

       case 26:
         timer_start(js2timer(duk,1));
         break;

       case 27:
         timer_remove(duk_to_int(duk,1));
         break;

       case 28:
         timerr_settime(js2timer(duk,1), js_to_number(argv[2]));
         break;

       case 29:
         duk_push_number(duk, js2timer(duk,1)->interval);
         return 1;

       case 30:
         sprite_setanim(id2sprite(duk_to_int(duk, 1)), duk_to_pointer(duk, 2), duk_to_int(duk, 3));
         return 0;

       case 31:
         free(duk_to_pointer(duk, 1));
         break;

       case 32:
         duk_push_number(duk, js2timer(duk,1)->remain_time);
         return 1;

       case 33:
         duk_push_boolean(duk, js2timer(duk, 1)->on);
         return 1;

       case 34:
         duk_push_boolean(duk, js2timer(duk,1)->repeat);
         return 1;

       case 35:
         js2timer(duk,1)->repeat = duk_to_boolean(duk, 2);
         return 0;

       case 36:
         id2go(duk_to_int(duk, 1))->scale = js_to_number(argv[2]);
         cpSpaceReindexShapesForBody(space, id2go(duk_to_int(duk, 1))->body);
         return 0;

       case 37:
         if (!id2sprite(duk_to_int(duk, 1))) return 0;
         vec2float(duk2vec2(duk, 2), id2sprite(duk_to_int(duk, 1))->pos);
         break;

       case 38:
         duk_push_string(duk, slurp_text(duk_to_string(duk, 1)));
         return 1;

       case 39:
         duk_push_int(duk, slurp_write(duk_to_string(duk, 1), duk_to_string(duk, 2)));
         return 1;

       case 40:
         id2go(duk_to_int(duk, 1))->filter.categories = duk2bitmask(duk, 2);
	 gameobject_apply(id2go(duk_to_int(duk, 1)));
	 break;

       case 41:
         id2go(duk_to_int(duk, 1))->filter.mask = duk2bitmask(duk, 2);
	 gameobject_apply(id2go(duk_to_int(duk, 1)));
	 break;

       case 42:
         bitmask2duk(duk, id2go(duk_to_int(duk, 1))->filter.categories);
	 return 1;

       case 43:
         bitmask2duk(duk, id2go(duk_to_int(duk, 1))->filter.mask);
	 return 1;

       case 44:
         duk_push_int(duk, pos2gameobject(duk2vec2(duk, 1)));
	 return 1;

       case 45:
         vect2duk(mouse_pos);
	 return 1;
	 
       case 46:
         set_mouse_mode(duk_to_int(duk, 1));
	 return 0;

	case 47:
	  draw_grid(duk_to_int(duk, 1), duk_to_int(duk, 2));
	  return 0;

	case 48:
	  duk_push_int(duk, mainwin->width);
	  return 1;

	case 49:
	  duk_push_int(duk, mainwin->height);
	  return 1;
	  
	case 50:
	  duk_push_boolean(duk, action_down(duk_to_int(duk, 1)));
	  return 1;
	  
	case 51:
	  draw_cppoint(duk2vec2(duk, 1), js_to_number(argv[2]), js2color(duk,3));
	  return 0;
	  
	case 52:
	  ints2duk(phys2d_query_box(duk2vec2(duk, 1), duk2vec2(duk, 2)));
	  return 1;
	  
	case 53:
	  draw_box(duk2vec2(duk, 1), duk2vec2(duk, 2), js2color(duk,3));
	  return 0;

	case 54:
	  gameobject_apply(id2go(duk_to_int(duk, 1)));
	  return 0;

	case 55:
	  duk2go(duk, 1)->flipx = duk_to_boolean(duk, 2) ? -1 : 1;
	  return 0;

	case 56:
	  duk2go(duk, 1)->flipy = duk_to_boolean(duk, 2) ? -1 : 1;
	  return 0;

	case 57:
	  duk_push_boolean(duk, duk2go(duk, 1)->flipx == -1 ? 1 : 0);
	  return 1;

	case 58:
	  duk_push_boolean(duk, duk2go(duk, 1)->flipy == -1 ? 1 : 0);
	  return 1;

	case 59:
	  duk_push_int(duk, point2segindex(duk2vec2(duk, 1), duk2cpvec2arr(duk, 2), duk_to_number(duk, 3)));
	  return 1;
	  
	case 60:
	  if (!id2sprite(duk_to_int(duk, 1))) return 0;
          id2sprite(duk_to_int(duk, 1))->layer = duk_to_int(duk, 2);
	  break;
	
	case 61:
	  set_cam_body(id2body(duk_to_int(duk, 1)));
	  break;
	  
	case 62:
	  add_zoom(js_to_number(argv[1]));
	  break;
	  
	case 63:
	  duk_push_number(duk, deltaT);
	  return 1;
	  
	case 64:
	  vect2duk(tex_get_dimensions(texture_pullfromfile(duk_to_string(duk, 1))));
	  return 1;
	  
	case 65:
	  duk_push_boolean(duk, file_exists(duk_to_string(duk, 1)));
	  return 1;
	  
	case 66:
	  dukext2paths(duk_to_string(duk, 1));
	  return 1;

	case 67:
	  opengl_rendermode(LIT);
	  break;

	case 68:
	  opengl_rendermode(WIREFRAME);
	  break;

	case 69:
	  gameobject_set_sensor(duk_to_int(duk, 1), duk_to_boolean(duk,2));
	  break;
	
	case 70:
	  vect2duk(world2go(id2go(duk_to_int(duk,1)), duk2vec2(duk,2)));
	  return 1;
	  
	case 71:
	  vect2duk(go2world(id2go(duk_to_int(duk,1)),duk2vec2(duk,2)));
	  return 1;

	case 72:
	  vect2duk(cpSpaceGetGravity(space));
	  return 1;

	case 73:
	  cpSpaceSetDamping(space, duk_to_number(duk,1));
	  return 0;

	case 74:
	  duk_push_number(duk, cpSpaceGetDamping(space));
	  return 1;

	case 75:
	  id2go(duk_to_int(duk,1))->layer = duk_to_int(duk,2);
	  return 0;

	case 76:
	  set_cat_mask(duk_to_int(duk,1), duk2bitmask(duk,2));
	  return 0;

	case 77:
	  input_to_game();
	  break;

	case 78:
	  input_to_nuke();
	  break;

	case 79:
	  duk_push_boolean(duk, phys_stepping());
	  return 1;
	  
	case 80:
	  ints2duk(phys2d_query_shape(duk_get_pointer(duk,1)));
	  return 1;
	  
	case 81:
	  draw_arrow(duk2vec2(duk,1), duk2vec2(duk,2), js2color(duk,3), duk_to_int(duk,4));
	  return 0;

	case 82:
	  gameobject_draw_debug(duk_to_int(duk,1));
	  return 0;

	case 83:
	  draw_edge(duk2cpvec2arr(duk, 1), 2, js2color(duk,2), 1);
	  return 0;

	case 84:
	  duk_push_string(duk, consolelog);
	  return 1;

	case 85:
	  vect2duk(cpvproject(duk2vec2(duk,1), duk2vec2(duk,2)));
	  return 1;
	  
	case 86:
          ints2duk(phys2d_query_box_points(duk2vec2(duk, 1), duk2vec2(duk, 2), duk2cpvec2arr(duk,3), duk_to_int(duk,4)));
	  return 1;

        case 87:
	  mini_music_play(duk_to_string(duk,1));
	  return 0;

	case 88:
	  mini_music_pause();
	  return 0;

	case 89:
	  mini_music_stop();
	  return 0;

	case 90:
	  window_set_icon(duk_to_string(duk,1));
	  break;
    }

    return 0;
}

JSValue duk_register(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
    int cmd = duk_to_int(duk, 0);
    void *fn = duk_get_heapptr(duk, 1);
    void *obj = duk_get_heapptr(duk, 2);

   struct callee c = {fn, obj};

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
       unregister_obj(obj);
       break;

     case 5:
       unregister_gui(c);
       break;

     case 6:
       register_debug(c);
       break;
     }

   return 0;
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
    int cmd = duk_to_int(duk, 0);
    void *fn = duk_get_heapptr(duk, 1);
    void *obj = duk_get_heapptr(duk, 2);
    int go = duk_get_int(duk, 3);
    struct callee c = {fn, obj};

    switch(cmd) {
      case 0:
        id2go(go)->cbs.begin = c;
        break;

      case 1:
        gameobject_add_shape_collider(go, c, duk_get_pointer(duk,4));
	break;

      case 2:
        phys2d_rm_go_handlers(go);
	break;

      case 3:
        id2go(go)->cbs.separate = c;
	break;
    }

    return 0;
}

JSValue duk_sys_cmd(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
    int cmd = duk_to_int(duk, 0);

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
            duk_push_boolean(duk, sim_playing());
            return 1;

        case 6:
            duk_push_boolean(duk, sim_paused());
            return 1;

        case 7:
            duk_push_int(duk, MakeGameobject());
            return 1;

        case 8:
            duk_push_int(duk, frame_fps());
            return 1;

        case 9: /* Clear the level out */
            new_level();
            break;

    }

    return 0;
}

JSValue duk_make_gameobject(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
    int g = MakeGameobject();
    struct gameobject *go = get_gameobject_from_id(g);

    go->scale = duk_to_number(duk, 0);
    go->bodytype = duk_to_int(duk, 1);
    go->mass = js_to_number(argv[2]);
    go->f = duk_to_number(duk, 3);
    go->e = duk_to_number(duk, 4);
    go->flipx = 1.f;
    go->flipy = 1.f;

    gameobject_apply(go);

    duk_push_int(duk, g);
    return 1;
}

JSValue duk_yughlog(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
    int cmd = duk_to_int(duk, 0);
    const char *s = duk_to_string(duk,1);
    const char *f = duk_to_string(duk, 2);
    int line = duk_to_int(duk, 3);

    mYughLog(1, cmd, line, f, s);

    return 0;
}

JSValue duk_set_body(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
  int cmd = duk_to_int(duk, 0);
  int id = duk_to_int(duk, 1);
  struct gameobject *go = get_gameobject_from_id(id);
  if (!go) return 0;

  /* TODO: Possible that reindexing shapes only needs done for static shapes? */
  switch (cmd) {
    case 0:
      gameobject_setangle(go, js_to_number(argv[2]));
      break;

    case 1:
      go->bodytype = duk_to_int(duk,2);
      break;

    case 2:
      cpBodySetPosition(go->body, duk2vec2(duk, 2));
      break;

    case 3:
      gameobject_move(go, duk2vec2(duk, 2));
      break;

    case 4:
      cpBodyApplyImpulseAtWorldPoint(go->body, duk2vec2(duk, 2), cpBodyGetPosition(go->body));
      return 0;

    case 5:
      go->flipx = duk_to_boolean(duk, 2);
      break;

    case 6:
      go->flipy = duk_to_boolean(duk, 2);
      break;

    case 7:
      cpBodySetMass(go->body, js_to_number(argv[2]));
      break;

    case 8:
      cpBodySetAngularVelocity(go->body, js_to_number(argv[2]));
      return 0;

    case 9:
      cpBodySetVelocity(go->body, duk2vec2(duk, 2));
      return 0;

    case 10:
      go->e = fmax(duk_to_number(duk,2), 0);
      break;

    case 11:
      go->f = fmax(duk_to_number(duk,2),0);
      break;

    case 12:
      cpBodyApplyForceAtWorldPoint(go->body, duk2vec2(duk, 2), cpBodyGetPosition(go->body));
      return 0;
  }

  cpSpaceReindexShapesForBody(space, go->body);

  return 0;
}

JSValue duk_q_body(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
    int q = duk_to_int(duk, 0);
    struct gameobject *go = get_gameobject_from_id(duk_to_int(duk, 1));

    if (!go) return 0;

    switch(q) {
        case 0:
          duk_push_int(duk, cpBodyGetType(go->body));
          return 1;

        case 1:
          vect2duk(cpBodyGetPosition(go->body));
          return 1;

        case 2:
          duk_push_number(duk, cpBodyGetAngle(go->body));
          return 1;

        case 3:
          vect2duk(cpBodyGetVelocity(go->body));
          return 1;

        case 4:
          duk_push_number(duk, cpBodyGetAngularVelocity(go->body));
          return 1;

        case 5:
          duk_push_number(duk, cpBodyGetMass(go->body));
          return 1;

	case 6:
	  duk_push_number(duk, cpBodyGetMoment(go->body));
	  return 1;

	case 7:
	  duk_push_boolean(duk, phys2d_in_air(go->body));
	  return 1;
    }

    return 0;
}

JSValue duk_make_sprite(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
  int go = duk_to_int(duk, 0);
  const char *path = duk_to_string(duk, 1);
  cpVect pos = duk2vec2(duk, 2);
  int sprite = make_sprite(go);
  struct sprite *sp = id2sprite(sprite);
  sprite_loadtex(sp, path, ST_UNIT);
  sp->pos[0] = pos.x;
  sp->pos[1] = pos.y;

  duk_push_int(duk, sprite);
  return 1;
}

/* Make anim from texture */
JSValue duk_make_anim2d(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
  const char *path = duk_to_string(duk, 0);
  int frames = duk_to_int(duk, 1);
  int fps = duk_to_int(duk, 2);

  struct TexAnim *anim = anim2d_from_tex(path, frames, fps);

  duk_push_pointer(duk, anim);
  return 1;
}

JSValue duk_make_box2d(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
  int go = duk_to_int(duk, 0);
  cpVect size = duk2vec2(duk, 1);
  cpVect offset = duk2vec2(duk, 2);

  struct phys2d_box *box = Make2DBox(go);
  box->w = size.x;
  box->h = size.y;
  box->offset[0] = offset.x;
  box->offset[1] = offset.y;

  phys2d_applybox(box);

      int idx = duk_push_object(duk);
    duk_push_pointer(duk, box);
    duk_put_prop_string(duk, idx, "id");
    duk_push_pointer(duk, &box->shape);
    duk_put_prop_string(duk, idx, "shape");

  return 1;
}

JSValue duk_cmd_box2d(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
    int cmd = duk_to_int(duk, 0);
    struct phys2d_box *box = duk_to_pointer(duk, 1);
    cpVect arg;

    if (!box) return 0;

    switch(cmd) {
        case 0:
          arg = duk2vec2(duk, 2);
          box->w = arg.x;
          box->h = arg.y;
          break;

        case 1:
          arg = duk2vec2(duk, 2);
          box->offset[0] = arg.x;
          box->offset[1] = arg.y;
          break;

        case 2:
          box->rotation = js_to_number(argv[2]);
          break;
    }

    phys2d_applybox(box);
    return 0;
}

JSValue duk_make_circle2d(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
   int go = duk_to_int(duk, 0);
  double radius = js_to_number(argv[1]);

    struct phys2d_circle *circle = Make2DCircle(go);
    circle->radius = radius;
    circle->offset = duk2vec2(duk, 2);

    phys2d_applycircle(circle);

    int idx = duk_push_object(duk);
    duk_push_pointer(duk, circle);
    duk_put_prop_string(duk, idx, "id");
    duk_push_pointer(duk, &circle->shape);
    duk_put_prop_string(duk, idx, "shape");

  return 1;
}

JSValue duk_cmd_circle2d(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
    int cmd = duk_to_int(duk, 0);
    struct phys2d_circle *circle = duk_to_pointer(duk, 1);

    if (!circle) return 0;

    switch(cmd) {
        case 0:
          circle->radius = js_to_number(argv[2]);
          break;

        case 1:
          circle->offset = duk2vec2(duk, 2);
          break;
    }

    phys2d_applycircle(circle);
    return 0;
}

JSValue duk_make_poly2d(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
  int go = duk_to_int(duk, 0);
  struct phys2d_poly *poly = Make2DPoly(go);
  phys2d_poly_setverts(poly, duk2cpvec2arr(duk,1));

  int idx = duk_push_object(duk);
  duk_push_pointer(duk, poly);
  duk_put_prop_string(duk, idx, "id");
  duk_push_pointer(duk, &poly->shape);
  duk_put_prop_string(duk, idx, "shape");

  return 1;
}

JSValue duk_cmd_poly2d(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
  int cmd = duk_to_int(duk,0);
  struct phys2d_poly *poly = duk_to_pointer(duk,1);

  if (!poly) return 0;
  
  switch(cmd) {
    case 0:
      phys2d_poly_setverts(poly, duk2cpvec2arr(duk,2));
      break;
  }
  
  return 0;  
}

JSValue duk_make_edge2d(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
  int go = duk_to_int(duk, 0);
  struct phys2d_edge *edge = Make2DEdge(go);

  int arridx = 1;

  int n = duk_get_length(duk, arridx);
  cpVect points[n];

  for (int i = 0; i < n; i++) {
      duk_get_prop_index(duk, arridx, i);

      points[i] = duk2vec2(duk, -1);

      phys2d_edgeaddvert(edge);
      phys2d_edge_setvert(edge, i, points[i]);
  }

    int idx = duk_push_object(duk);
    duk_push_pointer(duk, edge);
    duk_put_prop_string(duk, idx, "id");
    duk_push_pointer(duk, &edge->shape);
    duk_put_prop_string(duk, idx, "shape");

  return 1;
}

JSValue duk_cmd_edge2d(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
  int cmd = duk_to_int(duk, 0);
  struct phys2d_edge *edge = duk_to_pointer(duk, 1);

  if (!edge) return 0;

  switch(cmd) {
    case 0:
      phys2d_edge_clearverts(edge);
      phys2d_edge_addverts(edge, duk2cpvec2arr(duk, 2));
      break;
      
    case 1:
      edge->thickness = js_to_number(argv[2]);
      break;
  }
  
  return 0;
}

JSValue duk_inflate_cpv(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
  cpVect *points = duk2cpvec2arr(duk,0);
  int n = duk_to_int(duk,1);
  double d = duk_to_number(duk,2);

  cpVect inflate_out[n];
  cpVect inflate_in[n];
  
  inflatepoints(inflate_out, points, d, n);
  inflatepoints(inflate_in, points, -d, n);
  
  duk_idx_t arr = duk_push_array(duk);
  duk_idx_t out = vecarr2duk(duk, inflate_out, n);
  duk_put_prop_index(duk,arr, 0);
  duk_idx_t in = vecarr2duk(duk, inflate_in, n);
  duk_put_prop_index(duk,arr,1);
  
  return 1;
}

/* These are anims for controlling properties on an object */
JSValue duk_anim(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
    void *prop = duk_get_heapptr(duk, 0);
    int keyframes = duk_get_length(duk, 1);
    YughInfo("Processing %d keyframes.", keyframes);

    struct anim a = make_anim();

    for (int i  = 0; i < keyframes; i++) {
        struct keyframe k;
        duk_get_prop_index(duk, 1, i); /* End of stack is now the keyframe */
        cpVect v = duk2vec2(duk, duk_get_top_index(duk));
        k.time = v.y;
        k.val = v.x;
        a = anim_add_keyframe(a, k);
    }

    for (double i = 0; i < 3.0; i = i + 0.1) {
        YughInfo("Val is now %f at time %f", anim_val(a, i), i);
        duk_push_heapptr(duk, prop);
        duk_push_number(duk, anim_val(a, i));
        duk_call(duk, 1);
        duk_pop(duk);
    }

    return 0;
}

JSValue duk_make_timer(JSContext *js, JSValueConst this, int argc, JSValueConst *argv)
{
    void *sym = duk_get_heapptr(duk, 0);
    double secs = js_to_number(argv[1]);
    void *obj = duk_get_heapptr(duk, 2);
    struct callee *c = malloc(sizeof(*c));
    c->fn = sym;
    c->obj = obj;
    int id = timer_make(secs, call_callee, c, 1);

    duk_push_int(duk, id);
    return 1;
}

#define DUK_FUNC(NAME, ARGS) JS_CFUNC_DEF(NAME, ARGS, duk_##NAME);

void ffi_load()
{
    DUK_FUNC(yughlog, 4);
    DUK_FUNC(nuke, DUK_VARARGS);
    DUK_FUNC(make_gameobject, 7);
    DUK_FUNC(set_body, 3);
    DUK_FUNC(q_body, 2);

    DUK_FUNC(sys_cmd, 1);
    DUK_FUNC(win_make, 3);

    DUK_FUNC(make_sprite, 3);
    DUK_FUNC(make_anim2d, 3);
    DUK_FUNC(spline_cmd, 6);
    
    DUK_FUNC(make_box2d, 3);
    DUK_FUNC(cmd_box2d, DUK_VARARGS);
    DUK_FUNC(make_circle2d, 3);
    DUK_FUNC(cmd_circle2d, DUK_VARARGS);
    DUK_FUNC(make_poly2d, 2);
    DUK_FUNC(cmd_poly2d, DUK_VARARGS);
    DUK_FUNC(make_edge2d, 3);
    DUK_FUNC(cmd_edge2d, DUK_VARARGS);
    DUK_FUNC(make_timer, 3);

    DUK_FUNC(cmd, DUK_VARARGS);
    DUK_FUNC(register, 3);
    DUK_FUNC(register_collide, DUK_VARARGS);

    DUK_FUNC(gui_text, DUK_VARARGS);
    DUK_FUNC(ui_text, 5);
    DUK_FUNC(cursor_text,5);
    DUK_FUNC(gui_img, 2);
    
    DUK_FUNC(inflate_cpv, 3);

    DUK_FUNC(anim, 2);
}
