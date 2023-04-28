#include "timer.h"
#include <stdlib.h>
#include "log.h"

#include <stb_ds.h>

struct timer *timers;
static int first = -1;

void check_timer(struct timer *t, double dt)
{
    if (!t->on)
        return;

    t->remain_time -= dt;

    if (t->remain_time <= 0) {
        t->cb(t->data);
        if (t->repeat) {
            t->remain_time = t->interval;
            return;
        }

	timer_pause(t);
        return;
    }
}

void timer_update(double dt) {
    for (int i = 0; i < arrlen(timers); i++)
        check_timer(&timers[i], dt);
}

int timer_make(double interval, void (*callback)(void *param), void *param, int own) {
    struct timer new;
    new.remain_time = interval;
    new.interval = interval;
    new.cb = callback;
    new.data = param;
    new.repeat = 1;
    new.owndata = own;
    new.next = -1;
    
    if (first < 0) {
      timer_start(&new);
      arrput(timers, new);
      return arrlen(timers)-1;
    } else {
      int retid = first;
      first = id2timer(first)->next;
      *id2timer(retid) = new;
      timer_start(id2timer(retid));
      return retid;
    }
}

void timer_pause(struct timer *t) {
    if (!t->on) return;
    t->on = 0;
}

void timer_stop(struct timer *t) {
    if (!t->on) return;
    t->on = 0;
    t->remain_time = t->interval;
}

void timer_start(struct timer *t) {
    if (t->on) return;
    t->on = 1;
}

void timer_remove(int id) {
    struct timer *t = id2timer(id);
    if (t->owndata) free(t->data);
    t->next = first;
    first = id;
}

void timerr_settime(struct timer *t, double interval) {
    t->remain_time += (interval - t->interval);
    t->interval = interval;
}

struct timer *id2timer(int id)
{
    return &timers[id];
}
