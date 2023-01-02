#include "mrbffi.h"
#include "s7.h"

#include "font.h"

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

#include "s7.h"

#include "nuke.h"


cpVect s7tovec2(s7_scheme *sc, s7_pointer s7vec) {
    cpVect ret;
    ret.x = s7_real(s7_vector_ref(sc, s7vec, 0));
    ret.y = s7_real(s7_vector_ref(sc, s7vec, 1));

    return ret;
}



extern s7_scheme *s7;

/* FFI */
s7_pointer s7_ui_label(s7_scheme *sc, s7_pointer args) {
    if (s7_is_string(s7_car(args))) {
        nuke_label(s7_string(s7_car(args)));
        return s7_make_boolean(sc, 1);
    }

    return s7_wrong_type_arg_error(sc, "ui_label", 1, args, "Should be a string.");
}

s7_pointer s7_ui_btn(s7_scheme *sc, s7_pointer args) {
    return s7_make_boolean(sc, nuke_btn(s7_string(s7_car(args))));
}

s7_pointer s7_ui_nel(s7_scheme *sc, s7_pointer args) {
    nuke_nel(s7_integer(s7_cadr(args)));
    return s7_make_boolean(sc, 1);
}

s7_pointer s7_ui_prop(s7_scheme *sc, s7_pointer args) {
    float val = s7_real(s7_cadr(args));
    nuke_prop_float(s7_string(s7_car(args)), (float)s7_real(s7_caddr(args)), &val, s7_real(s7_cadddr(args)), s7_real(s7_car(s7_cddddr(args))), s7_real(s7_car(s7_cdr(s7_cddddr(args)))));
    return s7_make_real(sc, val);
}

s7_pointer s7_ui_text(s7_scheme *sc, s7_pointer args) {
    const char *s = s7_string(s7_car(args));
    int len = s7_integer(s7_cadr(args));
    char str[len+1];
    strncpy(str,s,len);
    nuke_edit_str(str);
    return s7_make_string(sc, str);
}

s7_pointer s7_gui_text(s7_scheme *sc, s7_pointer args) {
    const char *s = s7_string(s7_car(args));
    cpVect pos = s7tovec2(sc, s7_cadr(args));
    float fpos[2] = {pos.x, pos.y};

    float size = s7_real(s7_caddr(args));
    const float white[3] = {1.f, 1.f, 1.f};

    renderText(s, fpos, size, white, 1800);

    return s7_car(args);
}

s7_pointer s7_gui_img(s7_scheme *sc, s7_pointer args) {
    const char *img = s7_string(s7_car(args));
    cpVect pos = s7tovec2(sc, s7_cadr(args));

    gui_draw_img(img, pos.x, pos.y);

    return args;
}

s7_pointer s7_settings_cmd(s7_scheme *sc, s7_pointer args) {
    int cmd = s7_integer(s7_car(args));
    double val = s7_real(s7_cadr(args));

    switch(cmd) {
        case 0: // render fps
            renderMS = val;
            break;

        case 1:
            updateMS = val;
            break;

        case 2:
            physMS = val;
            break;

        case 3:
            debug_draw_phys(val);
            break;

        case 4:
            set_timescale(val);
            break;

        case 5:
            add_zoom(val);
            break;
    }

    return args;
}

s7_pointer s7_log(s7_scheme *sc, s7_pointer args) {
    int lvl = s7_integer(s7_car(args));
    const char *msg = s7_string(s7_cadr(args));
    const char *file = s7_string(s7_caddr(args));
    int line = s7_integer(s7_cadddr(args));
    mYughLog(1, lvl, line, file, msg);

    return args;
}

/* Call like (ui_rendertext "string" (xpos ypos) size) */
s7_pointer s7_ui_rendertext(s7_scheme *sc, s7_pointer args) {
    const char *s = s7_string(s7_car(args));
    s7_pointer s7pos = s7_cadr(args);
    cpVect cpos = s7tovec2(sc, s7_cadr(args));
    double pos[2] = { cpos.x, cpos.y };
    double size = s7_real(s7_caddr(args));
    double white[3] = {1.f, 1.f, 1.f};

    renderText(s, pos, size, white, 0);

    return args;
}

s7_pointer s7_win_cmd(s7_scheme *sc, s7_pointer args) {
    int win = s7_integer(s7_car(args));
    int cmd = s7_integer(s7_cadr(args));
    struct window *w = window_i(win);

    /*
        3: return win width
        4: return win height

    */

    switch (cmd) {
        case 0:  /* toggle fullscreen */
            window_togglefullscreen(w);
            break;

        case 1: /* Fullscreen on */
            window_makefullscreen(w);
            break;

        case 2: /* Fullscreen off */
            window_unfullscreen(w);
            break;

        case 3:
            return s7_make_integer(sc, w->width);
            break;

        case 4:
            return s7_make_integer(sc, w->height);
            break;
    }

    return args;
}

s7_pointer s7_win_make(s7_scheme *sc, s7_pointer args) {
    const char *title = s7_string(s7_car(args));
    int w = s7_integer(s7_cadr(args));
    int h = s7_integer(s7_caddr(args));
    struct window *win = MakeSDLWindow(title, w, h, 0);
    return s7_make_integer(sc, win->id);
}

s7_pointer s7_gen_cmd(s7_scheme *sc, s7_pointer args) {
    int cmd = s7_integer(s7_car(args));
    const char *s = s7_string(s7_cadr(args));

    /* Branch table for general commands from scheme */
    /* 0 : load level */
    /* 1: load prefab */

    int response = 0;

    switch (cmd) {
        case 0:
            load_level(s);
            break;

        case 1:
            response = gameobject_makefromprefab(s);
            break;
    }

    return s7_make_integer(sc, response);
}

s7_pointer s7_sys_cmd(s7_scheme *sc, s7_pointer args) {
    int cmd = s7_integer(s7_car(args));

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
            return s7_make_boolean(sc, sim_playing());

        case 6:
            return s7_make_boolean(sc, sim_paused());

        case 7:
            return s7_make_integer(sc, MakeGameobject());

        case 8:
            return s7_make_integer(sc, frame_fps());

    }

    return args;
}

s7_pointer s7_sound_cmd(s7_scheme *sc, s7_pointer args) {
    int sound = s7_integer(s7_car(args));
    int cmd = s7_integer(s7_cadr(args));

    switch (cmd) {
        case 0: // play
            break;

        case 1: // pause
            break;

        case 2: // stop
            break;

        case 3: // play from beginning
            break;
    }

    return args;
}

s7_pointer s7_gui_hook(s7_scheme *sc, s7_pointer args) {
    s7_pointer cb = s7_car(args);
    script_call_sym(cb);

    return cb;
}

s7_pointer s7_register(s7_scheme *sc, s7_pointer args) {
    int hook = s7_integer(s7_car(args));
    s7_pointer sym = s7_cadr(args);

    /* 0 : update */
    /* 1 : gui */
    /* 2 : physics */

    switch (hook) {
        case 0:
            register_update(sym);
            break;

        case 1:
            register_gui(sym);
            break;

        case 2:
            register_physics(sym);
            break;
    }

    return sym;
}

s7_pointer s7_set_pawn(s7_scheme *sc, s7_pointer args) {
    s7_pointer pawn = s7_car(args);
    set_pawn(pawn);
    return args;
}

s7_pointer s7_set_body(s7_scheme *sc, s7_pointer args) {
    int id = s7_integer(s7_car(args));
    int cmd = s7_integer(s7_cadr(args));
    struct gameobject *go = get_gameobject_from_id(id);

    switch (cmd) {
        case 0:
            gameobject_setangle(go, s7_real(s7_caddr(args)));
            break;

        case 1:
            cpBodySetType(go->body, s7_integer(s7_caddr(args)));
            break;

        case 2:
            cpBodySetPosition(go->body, s7tovec2(sc, s7_caddr(args)));
            break;

        case 3:
            gameobject_move(go, s7tovec2(sc, s7_caddr(args)));
            break;

        case 4:
            cpBodyApplyImpulseAtWorldPoint(go->body, s7tovec2(sc, s7_caddr(args)), cpBodyGetPosition(go->body));
            break;
    }

    return args;
}

s7_pointer s7_phys_cmd(s7_scheme *sc, s7_pointer args) {
    int go = s7_integer(s7_car(args));
    int cmd = s7_integer(s7_cadr(args));
    s7_pointer env = s7_caddr(args);

    if (go == -1) return s7_nil(sc);

    phys2d_add_handler_type(cmd, get_gameobject_from_id(go), env);
}

/* Query physics bodies */
s7_pointer s7_phys_q(s7_scheme *sc, s7_pointer args) {
    struct gameobject * go = get_gameobject_from_id(s7_integer(s7_car(args)));
    int q = s7_integer(s7_cadr(args));

    s7_pointer ret;

    /* Queries about a body
        0: body type of static, dynamic, kinematic
        1: body position
        2: body rotation
    */
    switch(q) {
        case 0:
            return s7_make_integer(sc, cpBodyGetType(go->body));

        case 1:
            ret = s7_make_vector(sc, 2);
            s7_vector_set(sc, ret, 0, s7_make_real(sc, cpBodyGetPosition(go->body).x));
            s7_vector_set(sc, ret, 1, s7_make_real(sc, cpBodyGetPosition(go->body).y));
            return ret;

        case 2:
            return s7_make_real(sc, cpBodyGetAngle(go->body));

    }
}

s7_pointer s7_phys_set(s7_scheme *sc, s7_pointer args) {
    int cmd = s7_integer(s7_car(args));
    double x = s7_real(s7_cadr(args));
    double y = s7_real(s7_caddr(args));

    phys2d_set_gravity(x, y);
}

s7_pointer s7_int_cmd(s7_scheme *sc, s7_pointer args) {
    int cmd = s7_integer(s7_car(args));
    int val = s7_integer(s7_cadr(args));

    switch (cmd) {
        case 0:
            set_cam_body(get_gameobject_from_id(val)->body);
            break;
    }
}

s7_pointer s7_yield(s7_scheme *sc, s7_pointer args) {
    /* arg 1: condition
         arg 2: function  to run
    */


    s7_pointer cond = s7_car(args);
    s7_pointer func = s7_cadr(args);


}

void timer_s7_call(s7_pointer sym) {
    s7_call(s7, sym, s7_nil(s7));
}

s7_pointer s7_timer(s7_scheme *sc, s7_pointer args) {
    double delay = s7_real(s7_car(args));
    s7_pointer sym = s7_cadr(args);

    struct timer *timer = timer_make(delay, timer_s7_call, sym);
    timer_start(timer);
    return args;
}

s7_pointer s7_timer_cmd(s7_scheme *sc, s7_pointer args) {
    int cmd = s7_integer(s7_car(args));
    int id = s7_integer(s7_cadr(args));

    struct timer *t = NULL;

    switch (cmd) {
        case 0:
            timer_pause(t);
            break;
        case 1:
             timer_start(t);
             break;
         case 2:
             timer_stop(t);
             break;
    }

    return args;
}

s7_pointer s7_anim(s7_scheme *sc, s7_pointer args) {
    s7_pointer prop = s7_car(args);
    s7_pointer keyframes = s7_cadr(args);

    YughInfo("Animating property %s.", s7_symbol_name(prop));

    struct anim a = make_anim();

    for (int i = 0; i < s7_list_length(sc, keyframes); i++) {
        struct keyframe k;
        s7_pointer kf = s7_list_ref(sc, keyframes, i);
        k.time = s7_real(s7_car(kf));
        k.val = s7_real(s7_cadr(kf));
        a = anim_add_keyframe(a, k);
    }

    for (double i = 0; i < 3.0; i = i + 0.1) {
        YughInfo("Val is now %f at time %f", anim_val(a, i), i);
        s7_symbol_set_value(sc, prop, s7_make_real(sc, anim_val(a, i)));
    }

    free_anim(a);
}

s7_pointer s7_anim_cmd(s7_scheme *sc, s7_pointer args) {
    int cmd = s7_integer(s7_car(args));
    int body = s7_integer(s7_cadr(args));
    s7_pointer sym = s7_caddr(args);

    switch (cmd) {
        case 0:
            YughInfo("Playing animation called %s.", s7_symbol_name(sym));
            break;
    }

    return args;
}

s7_pointer s7_make_gameobject(s7_scheme *sc, s7_pointer args) {
    int g = MakeGameobject();
    struct gameobject *go = get_gameobject_from_id(g);

    go->scale = s7_real(s7_car(args));
    go->bodytype = s7_integer(s7_cadr(args));
    go->mass = s7_real(s7_caddr(args));
    go->f = s7_real(s7_cadddr(args));
    go->e = s7_real(s7_list_ref(sc, args, 4));

    return s7_make_integer(sc, g);
}

s7_pointer s7_make_sprite(s7_scheme *sc, s7_pointer args) {
    int go = s7_integer(s7_car(args));
    const char *path = s7_string(s7_cadr(args));
    cpVect pos = s7tovec2(sc, s7_caddr(args));

    YughInfo("Using gameid %d.", go);

    struct sprite *sp = make_sprite(get_gameobject_from_id(go));

    sprite_loadtex(sp, path);
    sp->pos[0] = pos.x;
    sp->pos[1] = pos.y;

    return args;
}

s7_pointer s7_make_box2d(s7_scheme *sc, s7_pointer args) {
    int go = s7_integer(s7_car(args));
    cpVect size = s7tovec2(sc, s7_cadr(args));
    cpVect offset = s7tovec2(sc, s7_caddr(args));

    struct phys2d_box *box = Make2DBox(get_gameobject_from_id(go));
    box->w = size.x;
    box->h = size.y;
    box->offset[0] = offset.x;
    box->offset[1] = offset.y;

    phys2d_boxinit(box, get_gameobject_from_id(go));

    return args;
}

s7_pointer s7_make_circ2d(s7_scheme *sc, s7_pointer args) {
    int go = s7_integer(s7_car(args));
    double radius = s7_real(s7_cadr(args));
    cpVect offset = s7tovec2(sc, s7_caddr(args));

    struct phys2d_circle *circle = Make2DCircle(get_gameobject_from_id(go));
    circle->radius = radius;
    circle->offset[0] = offset.x;
    circle->offset[1] = offset.y;

    phys2d_applycircle(circle);
}

#define S7_FUNC(NAME, ARGS) s7_define_function(s7, #NAME, s7_ ##NAME, ARGS, 0, 0, "")

void ffi_load() {
    S7_FUNC(ui_label, 1);
    S7_FUNC(ui_btn, 1);
    S7_FUNC(ui_nel, 1);
    S7_FUNC(ui_prop, 6);
    S7_FUNC(ui_text, 2);
    S7_FUNC(ui_rendertext, 3);

    S7_FUNC(gui_text, 3);
    S7_FUNC(gui_img, 2);

    S7_FUNC(gen_cmd, 2);
    S7_FUNC(sys_cmd, 1);
    S7_FUNC(settings_cmd, 2);


    S7_FUNC(win_cmd, 2);
    S7_FUNC(win_make, 3);


    S7_FUNC(sound_cmd, 2);
    S7_FUNC(gui_hook, 1);
    S7_FUNC(register, 2);
    S7_FUNC(set_pawn, 1);
    S7_FUNC(set_body, 3);
    S7_FUNC(phys_cmd, 3);
    S7_FUNC(phys_q, 2);
    S7_FUNC(phys_set, 3);
    S7_FUNC(int_cmd, 2);

    S7_FUNC(log, 4);

    S7_FUNC(yield, 2);
    S7_FUNC(timer, 2);
    S7_FUNC(timer_cmd, 2);

    S7_FUNC(anim, 2);
    S7_FUNC(anim_cmd, 3);

    S7_FUNC(make_gameobject, 5);
    S7_FUNC(make_sprite, 3);
    S7_FUNC(make_box2d, 3);
    S7_FUNC(make_circ2d, 3);
}

