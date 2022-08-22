#include "timer.h"
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include "vec.h"

static double time;

struct vec timers;

void timer_init() {
    time = glfwGetTime();
    timers = vec_init(sizeof(struct timer), 10);
}

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

    vec_walk(&timers, check_timer);
}

struct timer *timer_make(double interval, void (*callback)(void *param), void *param) {
    struct timer *new = calloc(sizeof(*new),1);

    new->remain_time = interval;
    new->interval = interval;
    new->cb = callback;
    new->data = param;
    new->repeat = 1;

    timer_start(new);

    struct timer *nn = vec_add(&timers, new);
    free(new);

    nn->timerid = timers.len-1;

    return nn;
}

void timer_pause(struct timer *t) {
    t->on = 0;

    t->remain_time = t->fire_time - time;
}

void timer_stop(struct timer *t) {
    t->on = 0;
    t->remain_time = t->interval;
}

void timer_start(struct timer *t) {
    t->on = 1;
    t->fire_time = time + t->remain_time;
}

void timer_remove(struct timer *t) {
    vec_delete(&timers, t->timerid);
}

void timer_settime(struct timer *t, double interval) {
    double elapsed = time - (t->fire_time - t->interval);
    t->interval = interval;
    t->remain_time = time + t->interval - elapsed;
}

