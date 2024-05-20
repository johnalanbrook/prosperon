#ifndef TWODPHYSICS_H
#define TWODPHYSICS_H

#include <chipmunk/chipmunk.h>
#include "gameobject.h"
#include "script.h"
extern cpSpace *space;

void phys2d_init();
void phys2d_update(float deltaT);
void phys2d_setup_handlers(gameobject *go);

JSValue arb2js(cpArbiter *arb);

#endif
