#include "timer.h"
#include <stdio.h>
#include "stb_ds.h"

timer **timers;

timer *timer_make(JSContext *js, JSValue fn)
{
  timer *t = calloc(sizeof(*t),1);
  t->fn = JS_DupValue(js,fn);
  arrput(timers, t);
  return t;
}

void timer_free(JSRuntime *rt, timer *t)
{
  for (int i = 0; i < arrlen(timers); i++) {
    if (timers[i] == t) {
      arrdelswap(timers,i);
      break;
    }
  }
  
  JS_FreeValueRT(rt, t->fn);
      
  free(t);
}

void timer_update(JSContext *js, double dt)
{
  for (int i = 0; i < arrlen(timers); i++) {
    if (timers[i]->remain <= -10000) continue;
    timers[i]->remain -= dt;
    if (timers[i]->remain <= 0) {
      timers[i]->remain = -10000;
      JSValue fn = JS_DupValue(js, timers[i]->fn);
      script_call_sym(timers[i]->fn, 0, NULL);
      JS_FreeValue(js, fn);
    }
  }
}
