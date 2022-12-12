#include "mrbffi.h"
#include "s7.h"

#include "font.h"

#include "script.h"
#include "string.h"
#include "window.h"
#include "editor.h"
#include "engine.h"
#include "log.h"

#include "nuke.h"

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

/*

mrb_value mrb_ui_begin(mrb_state *mrb, mrb_value self) {
    char *title;
    mrb_float w, h;

    mrb_get_args(mrb, "zff", &title, &w, &h);

    return mrb_bool_value(nk_begin(ctx, title, nk_rect(0,0,w,h), NK_WINDOW_TITLE|NK_WINDOW_BORDER|NK_WINDOW_SCALABLE|NK_WINDOW_MOVABLE|NK_WINDOW_NO_SCROLLBAR));
}

*/

/*
mrb_value mrb_ui_rendertext(mrb_state *mrb, mrb_value self) {
    char *s;
    mrb_float pos[2];
    mrb_float size, ol;
    mrb_get_args(mrb, "zffff", &s, &pos[0], &pos[1], &size, &ol);

    static float white[3] = {1.f, 1.f, 1.f};
    float fpos[2] = {(float)pos[0], (float)pos[1]};
    renderText(s, fpos, size, white, ol);
    return self;
}

mrb_value mrb_win_make(mrb_state *mrb, mrb_value self) {
    char name[50] = "New Window";
    struct window *new = MakeSDLWindow(name, 500, 500, 0);
    return mrb_float_value(mrb, new->id);
}

mrb_value mrb_win_cmd(mrb_state *mrb, mrb_value self) {
    mrb_float win, cmd;
    mrb_get_args(mrb, "ff", &win, &cmd);
    struct window *new = window_i(win);

    switch ((int)cmd)
    {
        case 0:  // toggle fullscreen
            window_togglefullscreen(new);
            break;

        case 1: // Fullscreen on
            window_makefullscreen(new);
            break;

        case 2: // Fullscreen off
            window_unfullscreen(new);
            break;
    }

    return self;
}

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

mrb_value mrb_sound_make(mrb_state *mrb, mrb_value self) {
    mrb_value vals;
    mrb_get_args(mrb, "H", &vals);
    char *name = mrb_str_to_cstr(mrb, mrb_hash_fetch(mrb, vals, mrb_symbol_value(mrb_intern_cstr(mrb, "name")), mrb_str_new_cstr(mrb, "New Window")));
    YughInfo("Window name is %s.", name);
    return self;
}

mrb_value mrb_sound_cmd(mrb_state *mrb, mrb_value self) {
    mrb_float sound, cmd;
    mrb_get_args(mrb, "ff", &sound, &cmd);


    switch ((int)cmd)
    {
        case 0: // play
            break;

        case 1: // pause
            break;

        case 2: // stop
            break;

        case 3: // play from beginning
            break;
    }

    return self;
}

mrb_value mrb_settings_cmd(mrb_state *mrb, mrb_value self) {
    mrb_float cmd, val;
    mrb_get_args(mrb, "ff", &cmd, &val);

    switch((int)cmd)
    {
        case 0: // render fps
            renderMS = (double)val;
            break;

        case 1:
            updateMS = (double)val;
            break;

        case 2:
            physMS = (double)val;
            break;
    }

    return self;
}

mrb_value mrb_editor_render(mrb_state *mrb, mrb_value self) {
    editor_render();
    return self;
}
*/

//#define MRB_FUNC(NAME, ARGS) mrb_define_method(mrb, mrb->object_class, #NAME, mrb_ ## NAME, ARGS)
#define S7_FUNC(NAME, ARGS) s7_define_function(s7, #NAME, s7_ ##NAME, ARGS, 0, 0, "")

void ffi_load() {
//s7_define_function(s7, "ui_label", s7_ui_label, 1, 0, 0, "Draw UI label with given string");
S7_FUNC(ui_label, 1);
S7_FUNC(ui_btn, 1);
S7_FUNC(ui_nel, 1);
S7_FUNC(ui_prop, 6);
S7_FUNC(ui_text, 2);

/*
    MRB_FUNC(load, MRB_ARGS_REQ(1));
    MRB_FUNC(ui_label, MRB_ARGS_REQ(1));
    MRB_FUNC(ui_btn, MRB_ARGS_REQ(1));
    MRB_FUNC(ui_nel, MRB_ARGS_REQ(2));
    MRB_FUNC(ui_begin, MRB_ARGS_REQ(3));

    MRB_FUNC(ui_prop, MRB_ARGS_REQ(6));
    MRB_FUNC(ui_text, MRB_ARGS_REQ(2));


    MRB_FUNC(ui_rendertext, MRB_ARGS_REQ(5));

    MRB_FUNC(c_reload, MRB_ARGS_REQ(1));

    MRB_FUNC(win_make, MRB_ARGS_REQ(1));
    MRB_FUNC(win_cmd, MRB_ARGS_REQ(2));

    MRB_FUNC(nuke_cb, MRB_ARGS_REQ(2));
    MRB_FUNC(gui_cb, MRB_ARGS_REQ(2));

    MRB_FUNC(sound_make, MRB_ARGS_REQ(1));
    MRB_FUNC(sound_cmd, MRB_ARGS_REQ(2));

    MRB_FUNC(editor_render, MRB_ARGS_REQ(0));

    MRB_FUNC(settings_cmd, MRB_ARGS_REQ(2));
    */
}

