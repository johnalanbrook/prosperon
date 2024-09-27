#ifndef TIMER_H

typedef struct timer {
  double start;
  double remain;
} timer;

timer *timer_make();
void timer_free(timer *t);
void timer_update(double dt);
void timer_stop(timer *t);

#endif
