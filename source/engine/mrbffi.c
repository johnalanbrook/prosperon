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

#include "s7.h"

#include "nuke.h"

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

s7_pointer s7_settings_cmd(s7_scheme *sc, s7_pointer args) {
    int cmd = s7_integer(s7_car(args));
    double val = s7_real(s7_cadr(args));
    YughInfo("Changing a setting.");
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
    }

    return args;
}

s7_pointer s7_log(s7_scheme *sc, s7_pointer args) {
    int lvl = s7_integer(s7_car(args));
    const char *msg = s7_string(s7_object_to_string(sc, s7_cadr(args), 0));
    const char *file = s7_string(s7_caddr(args));
    int line = s7_integer(s7_cadddr(args));
    mYughLog(1, lvl, line, file, msg);
    //YughInfo(s7_string(s7_object_to_string(sc, s7_car(args), 0)));

    return args;
}

/* Call like (ui_rendertext "string" (xpos ypos) size) */
s7_pointer s7_ui_rendertext(s7_scheme *sc, s7_pointer args) {
    const char *s = s7_string(s7_car(args));
    double pos[2];
    pos[0] = s7_real(s7_car(s7_cadr(args)));
    pos[1] = s7_real(s7_cadr(s7_cadr(args)));
    double size = s7_real(s7_caddr(args));
    double white[3] = {1.f, 1.f, 1.f};

    renderText(s, pos, size, white, 0);

    return args;
}

s7_pointer s7_win_cmd(s7_scheme *sc, s7_pointer args) {
    int win = s7_integer(s7_car(args));
    int cmd = s7_integer(s7_cadr(args));
    struct window *w = window_i(win);

    switch (cmd) {
        case 0:  // toggle fullscreen
            window_togglefullscreen(w);
            break;

        case 1: // Fullscreen on
            window_makefullscreen(w);
            break;

        case 2: // Fullscreen off
            window_unfullscreen(w);
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

    switch (cmd) {
        case 0:
            load_level(s);
            break;

        case 1:
            gameobject_makefromprefab(s);
            break;
    }

    return args;
}

s7_pointer s7_sys_cmd(s7_scheme *sc, s7_pointer args) {
    int cmd = s7_integer(s7_car(args));

    switch (cmd) {
        case 0:
            quit();
            break;
    }
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

/*
mrb_value mrb_nuke_cb(mrb_state *mrb, mrb_value self) {
    mrb_float win;
    mrb_sym cb;
    mrb_get_args(mrb, "fn", &win, &cb);
    window_i((int)win)->nuke_cb = cb;
    return self;
}

mrb_value mrb_gui_cb(mrb_state *mrb, mrb_value self) {
    mrb_float win;
    mrb_sym cb;
    mrb_get_args(mrb, "fn", &win, &cb);
    window_i((int)win)->gui_cb = cb;
    return self;
}

*/

#define S7_FUNC(NAME, ARGS) s7_define_function(s7, #NAME, s7_ ##NAME, ARGS, 0, 0, "")

void ffi_load() {
    S7_FUNC(ui_label, 1);
    S7_FUNC(ui_btn, 1);
    S7_FUNC(ui_nel, 1);
    S7_FUNC(ui_prop, 6);
    S7_FUNC(ui_text, 2);
    S7_FUNC(settings_cmd, 2);
    S7_FUNC(win_cmd, 2);
    S7_FUNC(ui_rendertext, 3);
    S7_FUNC(log, 4);
    S7_FUNC(win_make, 3);
    S7_FUNC(gen_cmd, 2);
    S7_FUNC(sys_cmd, 1);
    S7_FUNC(sound_cmd, 2);
}

