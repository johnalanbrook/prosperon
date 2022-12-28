#include "particle.h"

struct emitter make_emitter()
{
    struct emitter e = { 0 };
    return e;
}

struct emitter set_emitter(struct emitter e)
{
    arrsetlen(e.particles, e.max);

    e.first = &e.particles[0];

    for (int i = 0; i < arrlen(e.particles)-1; i++) {
        e.particles[i].next = &e.particles[i+1];
    }
}

void free_emitter(struct emitter e)
{
    arrfree(e.particles);
}

void start_emitter(struct emitter e)
{

}

void pause_emitter(struct emitter e)
{

}

void stop_emitter(struct emitter e)
{

}

void emitter_step(struct emitter e, double dt)
{
    for (int i = 0; i < arrlen(e.particles); i++) {
        if (e.particles[i].life <= 0)
            continue;

        e.particles[i].pos = cpvadd(e.particles[i].pos, cpvmult(e.particles[i].v, dt));
        e.particles[i].angle += e.particles[i].av * dt;
        e.particles[i].life -= dt;

        if (e.particles[i].life <= 0) {
            e.particles[i].next = e.first;
            e.first = &e.particles[i];
        }
    }
}