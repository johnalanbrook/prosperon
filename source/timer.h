#ifndef TIMER_H

#include "script.h"

typedef struct timer {
  double remain;
  JSValue fn;
} timer;

timer *timer_make(JSContext *js, JSValue fn);
void timer_free(JSRuntime *rt, timer *t);
void timer_update(JSContext *js, double dt);

#endif
