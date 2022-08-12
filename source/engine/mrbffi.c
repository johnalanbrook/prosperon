#include "mrbffi.h"

#include "mruby.h"
#include "mruby/compile.h"
#include "mruby/string.h"
#include "mruby/hash.h"

#include "font.h"


#include "script.h"
#include "string.h"
#include "window.h"
#include "editor.h"
#include "engine.h"

extern mrb_state *mrb;

#include "nuklear.h"
extern struct nk_context *ctx;

/* FFI */

mrb_value mrb_load(mrb_state *mrb, mrb_value self) {
    char *path;
    mrb_get_args(mrb, "z!", &path);
    script_dofile(path);
    return self;
}

mrb_value mrb_ui_label(mrb_state *mrb, mrb_value self) {
    char *str;
    mrb_get_args(mrb, "z", &str);
    nk_labelf(ctx, NK_TEXT_LEFT, "%s", str);
    return self;
}

mrb_value mrb_ui_btn(mrb_state *mrb, mrb_value self) {
    char *str;
    mrb_get_args(mrb, "z", &str);
    return mrb_bool_value(nk_button_label(ctx, str));
}

mrb_value mrb_ui_nel(mrb_state *mrb, mrb_value self) {
    int height, cols;
    mrb_get_args(mrb, "ii", &height, &cols);
    nk_layout_row_dynamic(ctx, height, cols);
    return self;
}

mrb_value mrb_ui_prop(mrb_state *mrb, mrb_value self) {
    mrb_float min, max, step1, step2, val;
    char *s;
    mrb_get_args(mrb, "zfffff", &s, &val, &min, &max, &step1, &step2);
    nk_property_float(ctx, s, min, &val, max, step1, step2);
    return mrb_float_value(mrb, val);
}

mrb_value mrb_ui_text(mrb_state *mrb, mrb_value self) {
    char *s;
    mrb_float len;

    mrb_get_args(mrb, "zf", &s, &len);

    char str[(int)len+1];
    strncpy(str, s,len);
    nk_edit_string_zero_terminated(ctx, NK_EDIT_BOX|NK_EDIT_NO_HORIZONTAL_SCROLL, str, 130, nk_filter_ascii);

    return mrb_str_new_cstr(mrb, str);
}

mrb_value mrb_ui_begin(mrb_state *mrb, mrb_value self) {
    char *title;
    mrb_float w, h;

    mrb_get_args(mrb, "zff", &title, &w, &h);

    return mrb_bool_value(nk_begin(ctx, title, nk_rect(0,0,w,h), NK_WINDOW_TITLE|NK_WINDOW_BORDER|NK_WINDOW_SCALABLE|NK_WINDOW_MOVABLE|NK_WINDOW_NO_SCROLLBAR));
}

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

mrb_value mrb_c_reload(mrb_state *mrb, mrb_value self) {
    return self;
}

mrb_value mrb_win_make(mrb_state *mrb, mrb_value self) {
    char name[50] = "New Window";
    struct mSDLWindow *new = MakeSDLWindow(name, 500, 500, 0);
    return mrb_float_value(mrb, new->id);
}

mrb_value mrb_win_cmd(mrb_state *mrb, mrb_value self) {
    mrb_float win, cmd;
    mrb_get_args(mrb, "ff", &win, &cmd);
    struct mSDLWindow *new = window_i(win);

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
    printf("Window name is %s.\n", name);
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

mrb_value mrb_sprite_make(mrb_state *mrb, mrb_value self) {
    struct mSprite *new = gui_makesprite();
}

mrb_value mrb_editor_render(mrb_state *mrb, mrb_value self) {
    editor_render();
    return self;
}

#define MRB_FUNC(NAME, ARGS) mrb_define_method(mrb, mrb->object_class, #NAME, mrb_ ## NAME, ARGS)

void ffi_load() {
    MRB_FUNC(load, MRB_ARGS_REQ(1));
    MRB_FUNC(ui_label, MRB_ARGS_REQ(1));
    MRB_FUNC(ui_btn, MRB_ARGS_REQ(1));
    MRB_FUNC(ui_nel, MRB_ARGS_REQ(2));
    MRB_FUNC(ui_begin, MRB_ARGS_REQ(3));

    MRB_FUNC(ui_prop, MRB_ARGS_REQ(6));
    MRB_FUNC(ui_text, MRB_ARGS_REQ(2));


    MRB_FUNC(ui_rendertext, MRB_ARGS_REQ(5));

    MRB_FUNC(sprite_make, MRB_ARGS_REQ(1));

    MRB_FUNC(c_reload, MRB_ARGS_REQ(1));

    MRB_FUNC(win_make, MRB_ARGS_REQ(1));
    MRB_FUNC(win_cmd, MRB_ARGS_REQ(2));

    MRB_FUNC(nuke_cb, MRB_ARGS_REQ(2));
    MRB_FUNC(gui_cb, MRB_ARGS_REQ(2));

    MRB_FUNC(sound_make, MRB_ARGS_REQ(1));
    MRB_FUNC(sound_cmd, MRB_ARGS_REQ(2));

    MRB_FUNC(editor_render, MRB_ARGS_REQ(0));

    MRB_FUNC(settings_cmd, MRB_ARGS_REQ(2));
}

