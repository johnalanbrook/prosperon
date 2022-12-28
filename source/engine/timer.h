#ifndef TIMER
#define TIMER

struct timer {
    int timerid;
    int on;
    double interval;   // Time of timer
    int repeat;
    double remain_time;   // How much time until the timer executes
    void (*cb)(void *data);
    void *data;
};

struct timer *timer_make(double interval, void (*callback)(void *param), void *param);
void timer_remove(struct timer *t);
void timer_start(struct timer *t);
void timer_pause(struct timer *t);
void timer_stop(struct timer *t);
void timer_update(double dt);
void timerr_settime(struct timer *t, double interval);

#endif
