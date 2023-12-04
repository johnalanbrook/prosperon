#ifndef PARTICLE_H
#define PARTICLE_H

#include "HandmadeMath.h"
#include "transform.h"

struct particle {
  HMM_Vec3 pos;
  HMM_Vec3 v; /* velocity */
  HMM_Quat angle;
  HMM_Quat av; /* angular velocity */

  union {
    double life;
    unsigned int next;
  };
};

typedef struct emitter {
  struct particle *particles;
  transform3d t;
  int max;
  double life;
  void (*seeder)(struct particle *p); /* Called to initialize each particle */
} emitter;

struct emitter make_emitter();
void free_emitter(struct emitter e);

void start_emitter(struct emitter e);
void pause_emitter(struct emitter e);
void stop_emitter(struct emitter e);

void emitter_step(struct emitter e, double dt);

#endif
