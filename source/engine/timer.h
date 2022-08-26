#ifndef TIMER
#define TIMER

struct timer {
    int timerid;
    int on;
    double fire_time;    // Time the timer will fire
    double interval;   // Time of timer
    double start_time; // Time the timer started this loop
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
void timer_update(double s);
void timer_settime(struct timer *t, double interval);




void *arrfind(void *arr, int (*valid)(void *arr, void *cmp), void *cmp);
void arrwalk(void *arr, void (*fn)(void *data));

#endif