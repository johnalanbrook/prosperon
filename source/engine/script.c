#include "script.h"

#include "gameobject.h"
#include <s7.h>

s7_scheme *s7 = NULL;

s7_pointer s7square(s7_scheme * sc, s7_pointer args)
{
    if (s7_is_integer(s7_car(args)))
	return (s7_make_integer
		(sc, s7_integer(s7_car(args)) * s7_integer(s7_car(args))));

    return (s7_wrong_type_arg_error
	    (sc, "square", 1, s7_car(args), "an integer"));
}

s7_pointer s7move(s7_scheme * sc, s7_pointer args)
{
    if (s7_is_number(s7_car(args)) && s7_is_number(s7_cadr(args))) {
	gameobject_move(updateGO, s7_real(s7_car(args)),
			s7_real(s7_cadr(args)));
	return args;
    }

    return args;
}

s7_pointer s7rotate(s7_scheme * sc, s7_pointer args)
{
    if (s7_is_number(s7_car(args))) {
	gameobject_rotate(updateGO, s7_real(s7_car(args)));
	return (s7_make_real(sc, cpBodyGetAngle(updateGO->body)));
    }

    return (s7_wrong_type_arg_error
	    (sc, "rotate", 1, s7_car(args), "a number"));
}

void script_init()
{
    s7 = s7_init();
    s7_define_function(s7, "square", s7square, 1, 0, 0,
		       "(square int) squares int");
    s7_define_function(s7, "move", s7move, 1, 0, 0,
		       "(move (xs ys)) moves at xs and ys pixels per second");
    s7_define_function(s7, "rotate", s7rotate, 1, 0, 0,
		       "(rotate ms) rotates at ms meters per second");
}

void script_run(const char *script)
{
    s7_eval_c_string(s7, script);
}
