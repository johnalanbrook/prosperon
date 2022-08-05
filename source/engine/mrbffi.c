#include "mrbffi.h"

#include "mruby.h"
#include "mruby/compile.h"
#include "mruby/string.h"

#include "font.h"


#include "script.h"
#include "string.h"

extern mrb_state *mrb;

#include "nuklear.h"
extern struct nk_context *ctx;

int fib(int n) {
    if (n < 2) return n;

    return fib(n-1) + fib(n-2);
}

/* FFI */

mrb_value mrb_fib(mrb_state *mrb, mrb_value self) {
    int n;
    mrb_get_args(mrb, "i", &n);
    return mrb_fixnum_value(fib(n));
}

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
    mrb_float pos[2], size, ol;
    mrb_get_args(mrb, "zffff", &s, &pos[0], &pos[1], &size, &ol);

    static float white[3] = {1.f, 1.f, 1.f};
    renderText(s, pos, size, white, ol);
    return self;
}

mrb_value mrb_c_reload(mrb_state *mrb, mrb_value self) {

}

void ffi_load() {
    mrb_define_method(mrb, mrb->object_class, "fib", mrb_fib, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, mrb->object_class, "load", mrb_load, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, mrb->object_class, "ui_label", mrb_ui_label, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, mrb->object_class, "ui_btn", mrb_ui_btn, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, mrb->object_class, "ui_nel", mrb_ui_nel, MRB_ARGS_REQ(2));
    mrb_define_method(mrb, mrb->object_class, "ui_begin", mrb_ui_begin, MRB_ARGS_REQ(3));

    mrb_define_method(mrb, mrb->object_class, "ui_prop", mrb_ui_prop, MRB_ARGS_REQ(6));
    mrb_define_method(mrb, mrb->object_class, "ui_text", mrb_ui_text, MRB_ARGS_REQ(2));


    mrb_define_method(mrb, mrb->object_class, "ui_rendertext", mrb_ui_rendertext, MRB_ARGS_REQ(5));

    mrb_define_method(mrb, mrb->object_class, "c_reload", mrb_c_reload, MRB_ARGS_REQ(1));
}

