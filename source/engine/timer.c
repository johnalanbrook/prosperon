#include "timer.h"
#include <GLFW/glfw3.h>
#include <stdlib.h>

static double time;

void timer_init() {
    time = glfwGetTime();
}

struct timer *timer_make(int interval, void *(*callback)(void *param), void *param) {
    struct timer *new = malloc(sizeof(*new));
    new->fire_time = time + interval;

    return new;
}

void timer_pause(struct timer *t) {

}

void timer_start(struct timer *t) {

}

void timer_remove(struct timer *t) {
    free(t);
}