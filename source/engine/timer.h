#ifndef TIMER
#define TIMER

struct timer {
    int timerid;
    int on;
    double interval; // Time of timer
    int repeat;
    double remain_time;   // How much time until the timer executes
    void (*cb)(void *data);
    void *data;
    int owndata;
    int next;
  int app;
};

int timer_make(double interval, void (*callback)(void *param), void *param, int own, int app);
struct timer *id2timer(int id);
void timer_remove(int id);
void timer_start(struct timer *t);
void timer_pause(struct timer *t);
void timer_stop(struct timer *t);
void timer_update(double dt, double scale);
void timerr_settime(struct timer *t, double interval);

#endif
