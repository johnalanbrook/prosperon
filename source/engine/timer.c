#include "timer.h"
#include <stdio.h>
#include "stb_ds.h"

timer **timers;

timer *timer_make()
{
  timer *t = calloc(sizeof(*t),1);
  arrput(timers, t);
  return t;
}

void timer_free(timer *t)
{
  printf("before free arrlen if timers is now %d\n", arrlen(timers));
  for (int i = 0; i < arrlen(timers); i++) {
    if (timers[i] == t) {
      arrdelswap(timers,i);
      break;
    }
  }
      
  free(t);
  printf("arrlen if timers is now %d\n", arrlen(timers));
}

void timer_update(double dt)
{
  for (int i = 0; i < arrlen(timers); i++) {
    timers[i]->remain -= dt;
  }
}
