#ifndef TIMER
#define TIMER

struct timer {
    int timerid;
    int on;
    double fire_time;
    double interval;
    int repeat;
    double remain_time;
    void (*cb)(void *data);
    void *data;
};

void timer_init();
struct timer *timer_make(double interval, void (*callback)(void *param), void *param);
void timer_remove(struct timer *t);
void timer_start(struct timer *t);
void timer_pause(struct timer *t);
void timer_stop(struct timer *t);
void timer_update(double s);
void timer_settime(struct timer *t, double interval);

#endif