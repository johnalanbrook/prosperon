#ifndef PARTICLE_H
#define PARTICLE_H

#include <chipmunk/chipmunk.h>

struct particle {
    cpVect pos;
    cpVect v; /* velocity */
    double angle;
    double av; /* angular velocity */


    union {
        double life;
        struct particle *next;
    };
};

struct emitter {
    struct particle *particles;
    struct particle *first;
    int max;
    double life;
    void (*seeder)(struct particle *p); /* Called to initialize each particle */
};

struct emitter make_emitter();
void free_emitter(struct emitter e);

void start_emitter(struct emitter e);
void pause_emitter(struct emitter e);
void stop_emitter(struct emitter e);

void emitter_step(struct emitter e, double dt);

#endif