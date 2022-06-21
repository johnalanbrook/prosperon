#ifndef TIMER
#define TIMER

struct timer {
    int timerid;
    double fire_time;
};

void timer_init();
struct timer *timer_make(int interval, void *(*callback)(void *param), void *param);
void timer_remove(struct timer *t);


#endif