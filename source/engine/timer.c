#include "timer.h"

#include "stb_ds.h"

timer *timers;

timer *timer_make()
{
  return NULL;
}

void timer_free(timer *t)
{
  
}

void timer_update(double dt)
{
  for (int i = 0; i < arrlen(timers); i++) {
    timers[i].remain -= dt;
  }
}