#include "timer.h"
#include <stdlib.h>

#include <stb_ds.h>;

static double time;

struct timer *timers;

void check_timer(struct timer *t)
{
    if (!t->on)
        return;

    if (time >= t->fire_time) {
        t->cb(t->data);

        if (t->repeat) {
            t->fire_time = time + t->interval;
            return;
        }

        t->on = 0;
        return;
    }
}

void timer_update(double s) {
    time = s;


    for (int i = 0; i < arrlen(timers); i++)
        check_timer(&timers[i]);

}

struct timer *timer_make(double interval, void (*callback)(void *param), void *param) {
    struct timer new;
    new.remain_time = interval;
    new.interval = interval;
    new.cb = callback;
    new.data = param;
    new.repeat = 1;
    new.timerid = arrlen(timers);

    timer_start(&new);
    arrput(timers, new);

    return &arrlast(timers);
}

void timer_pause(struct timer *t) {
    if (!t->on) return;

    t->on = 0;

    t->remain_time = t->fire_time - time;
}

void timer_stop(struct timer *t) {
    if (!t->on) return;

    t->on = 0;
    t->remain_time = t->interval;
}

void timer_start(struct timer *t) {
    if (t->on) return;

    t->on = 1;
    t->fire_time = time + t->remain_time;
}

void timer_remove(struct timer *t) {
    int i = t->timerid;
    arrdelswap(timers, i);
    timers[i].timerid = i;
}

void timer_settime(struct timer *t, double interval) {
    //double elapsed = time - (t->fire_time - t->interval);
    t->interval = interval;
    t->remain_time = t->interval;

    // TODO: timer_settime reacts to elapsed time
}

void *arrfind(void *arr, int (*valid)(void *arr, void *cmp), void *cmp)
{
    for (int i = 0; i < arrlen(arr); i++) {
        if (valid(&arr[i], cmp))
            return &arr[i];
    }

    return NULL;
}

void arrwalk(void *arr, void (*fn)(void *data))
{
    for (int i = 0; i < arrlen(arr); i++)
        fn(&arr[i]);
}
