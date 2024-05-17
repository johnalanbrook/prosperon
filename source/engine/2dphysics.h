#ifndef TWODPHYSICS_H
#define TWODPHYSICS_H

#include "script.h"
#include <chipmunk/chipmunk.h>
#include "gameobject.h"
#include "render.h"
#include "transform.h"

extern cpSpace *space;

typedef struct constraint {
  cpConstraint *c;
  JSValue break_cb; /* function called when it is forcibly broken */
  JSValue remove_cb; /* called when it is removed at all */
} constraint;

constraint *constraint_make(cpConstraint *c);
void constraint_break(constraint *constraint);
void constraint_free(constraint *constraint);

void phys2d_init();
void phys2d_update(float deltaT);
void phys2d_setup_handlers(gameobject *go);

#endif
