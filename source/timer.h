#ifndef TIMER_H

#include "script.h"

typedef struct timer {
  double remain;
  JSValue fn;
} timer;

timer *timer_make(JSValue fn);
void timer_free(timer *t);
void timer_update(double dt);

#endif
