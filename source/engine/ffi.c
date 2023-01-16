#include "ffi.h"

#include "script.h"

#include "string.h"
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

struct color duk2color(duk_context *duk, int p)
{
    struct color color;

    duk_get_prop_index(duk, p, 0);
    color.r = duk_to_number(duk, -1);
    duk_get_prop_index(duk, p, 0);
    color.b = duk_to_number(duk, -1);
    duk_get_prop_index(duk, p, 0);
    color.g = duk_to_number(duk, -1);

    return color;
}

cpVect duk2vec2(duk_context *duk, int p) {
    cpVect pos;
    duk_get_prop_index(duk, p, 0);
    pos.x = duk_to_number(duk, -1);
    duk_get_prop_index(duk, p, 1);
    pos.y = duk_to_number(duk, -1);

    return pos;
}

duk_ret_t duk_gui_text(duk_context *duk) {
    const char *s = duk_to_string(duk, 0);
    cpVect pos = duk2vec2(duk, 1);
    float fpos[2] = {pos.x, pos.y};

    float size = duk_to_number(duk, 2);
    const float white[3] = {1.f, 1.f, 1.f};
    renderText(s, fpos, size, white, 1800);
    return 0;
}

duk_ret_t duk_gui_img(duk_context *duk) {
    const char *img = duk_to_string(duk, 0);
    cpVect pos = duk2vec2(duk, 1);
    gui_draw_img(img, pos.x, pos.y);
    return 0;
}

duk_ret_t duk_win_make(duk_context *duk) {
    const char *title = duk_to_string(duk, 0);
    int w = duk_to_int(duk, 1);
    int h = duk_to_int(duk, 2);
    struct window *win = MakeSDLWindow(title, w, h, 0);

    duk_push_int(duk, win->id);
    return 1;
}

duk_ret_t duk_cmd(duk_context *duk) {
    int cmd = duk_to_int(duk, 0);

    switch(cmd) {
      case 0:
        duk_push_int(duk, script_dofile(duk_to_string(duk, 1)));
        return 1;
        break;

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
         //anim2d_delete(duk_to_int(duk, 1));
         break;


       case 13:
         play_song(duk_to_string(duk, 1), duk_to_string(duk, 2));
         break;

       case 14:
         play_oneshot(make_sound(duk_to_string(duk, 1)));
         break;

       case 15:
         music_stop();
         break;

       case 16:
         set_dbg_color(duk2color(duk, 1));
         break;

       case 17:
         set_trigger_color(duk2color(duk, 1));
         break;

    }

    return 0;
}

duk_ret_t duk_register(duk_context *duk) {
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
     }

   return 0;
}

duk_ret_t duk_register_collide(duk_context *duk) {
    int cmd = duk_to_int(duk, 0);
    void *fn = duk_get_heapptr(duk, 1);
    void *obj = duk_get_heapptr(duk, 2);
    int go = duk_get_int(duk, 3);

    struct callee c = {fn, obj};

    phys2d_add_handler_type(0, go, c);

    return 0;
}

duk_ret_t duk_sys_cmd(duk_context *duk) {
    int cmd = duk_to_int(duk, 0);

    switch (cmd) {
        case 0:
            quit();
            break;

        case 1:
            sim_start();
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

duk_ret_t duk_make_gameobject(duk_context *duk) {
    int g = MakeGameobject();
    struct gameobject *go = get_gameobject_from_id(g);

    go->scale = duk_to_number(duk, 0);
    go->bodytype = duk_to_int(duk, 1);
    go->mass = duk_to_number(duk, 2);
    go->f = duk_to_number(duk, 3);
    go->e = duk_to_number(duk, 4);
    go->flipx = duk_to_boolean(duk, 5) ? -1 : 1;
    go->flipy = duk_to_boolean(duk, 6) ? -1 : 1;

    gameobject_apply(go);

    duk_push_int(duk, g);
    return 1;
}

duk_ret_t duk_yughlog(duk_context *duk) {
    int cmd = duk_to_int(duk, 0);
    const char *s = duk_to_string(duk,1);
    const char *f = duk_to_string(duk, 2);
    int line = duk_to_int(duk, 3);

    mYughLog(1, cmd, line, f, s);

    return 0;
}



duk_ret_t duk_set_body(duk_context *duk) {
  int cmd = duk_to_int(duk, 0);
  int id = duk_to_int(duk, 1);
  struct gameobject *go = get_gameobject_from_id(id);

  /* TODO: Possible that reindexing shapes only needs done for static shapes? */
  switch (cmd) {
    case 0:
      gameobject_setangle(go, duk_to_number(duk, 2));
      cpSpaceReindexShapesForBody(space, go->body);
      break;

    case 1:
      cpBodySetType(go->body, duk_to_int(duk, 2));
      break;

    case 2:
      cpBodySetPosition(go->body, duk2vec2(duk, 2));
      cpSpaceReindexShapesForBody(space, go->body);
      break;

    case 3:
      gameobject_move(go, duk2vec2(duk, 2));
      break;

    case 4:
      cpBodyApplyImpulseAtWorldPoint(go->body, duk2vec2(duk, 2), cpBodyGetPosition(go->body));
      break;

    case 5:
      go->flipx = duk_to_boolean(duk, 2);
      break;

    case 6:
      go->flipy = duk_to_boolean(duk, 2);
      break;

    case 7:
      cpBodySetMass(go->body, duk_to_number(duk, 2));
      break;

    case 8:
      cpBodySetAngularVelocity(go->body, duk_to_number(duk, 2));
      break;

    case 9:
      cpBodySetVelocity(go->body, duk2vec2(duk, 2));
      break;

  }

  return 0;
}

duk_ret_t duk_q_body(duk_context *duk) {
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
    }

    return 0;
}

duk_ret_t duk_make_sprite(duk_context *duk) {
  int go = duk_to_int(duk, 0);
  const char *path = duk_to_string(duk, 1);
  cpVect pos = duk2vec2(duk, 2);

  int sprite = make_sprite(go);
  struct sprite *sp = id2sprite(sprite);
  sprite_loadtex(sp, path);
  sp->pos[0] = pos.x;
  sp->pos[1] = pos.y;

   duk_push_int(duk, sprite);
  return 1;
}

duk_ret_t duk_make_anim2d(duk_context *duk) {
  int go = duk_to_int(duk, 0);
  const char *path = duk_to_string(duk, 1);
  int frames = duk_to_int(duk, 2);
  int fps = duk_to_int(duk, 3);

  int sprite = make_sprite(go);
  struct sprite *sp = id2sprite(sprite);
  sp->pos[0] = -0.5f;
  sp->pos[1] = -0.5f;
  anim_load(&sp->anim, path);
  sp->tex = sp->anim.anim->tex;

  YughInfo("Made an animation.");

  duk_push_int(duk, sprite);
  return 1;
}

duk_ret_t duk_make_box2d(duk_context *duk) {
  int go = duk_to_int(duk, 0);
  cpVect size = duk2vec2(duk, 1);
  cpVect offset = duk2vec2(duk, 2);

  struct phys2d_box *box = Make2DBox(go);
  box->w = size.x;
  box->h = size.y;
  box->offset[0] = offset.x;
  box->offset[1] = offset.y;

  phys2d_applybox(box);

  return 0;
}

duk_ret_t duk_make_circle2d(duk_context *duk) {
   int go = duk_to_int(duk, 0);
  double radius = duk_to_number(duk, 1);
  cpVect offset = duk2vec2(duk, 2);

    struct phys2d_circle *circle = Make2DCircle(go);
    circle->radius = radius;
    circle->offset[0] = offset.x;
    circle->offset[1] = offset.y;

    phys2d_applycircle(circle);

  return 0;
}

duk_ret_t duk_anim(duk_context *duk) {
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



duk_ret_t duk_anim_cmd(duk_context *duk) {
    return 0;
}

duk_ret_t duk_timer(duk_context *duk) {
    return 0;
}

duk_ret_t duk_timer_cmd(duk_context *duk) {
    return 0;
}

#define DUK_FUNC(NAME, ARGS) duk_push_c_function(duk, duk_##NAME, ARGS); duk_put_global_string(duk, #NAME);

void ffi_load()
{
    DUK_FUNC(yughlog, 4);
    DUK_FUNC(make_gameobject, 7);
    DUK_FUNC(set_body, 3);
    DUK_FUNC(q_body, 2);

    DUK_FUNC(sys_cmd, 1);
    DUK_FUNC(win_make, 3);

    DUK_FUNC(make_sprite, 3);
    DUK_FUNC(make_anim2d, 4);
    DUK_FUNC(make_box2d, 3);
    DUK_FUNC(make_circle2d, 3);
    DUK_FUNC(cmd, DUK_VARARGS);
    DUK_FUNC(register, 3);
    DUK_FUNC(register_collide, 4);

    DUK_FUNC(gui_text, 3);
    DUK_FUNC(gui_img, 2);

    DUK_FUNC(timer, 2);
    DUK_FUNC(timer_cmd, 2);

    DUK_FUNC(anim, 2);
    DUK_FUNC(anim_cmd, 3);
}
