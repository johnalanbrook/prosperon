#ifndef PARTICLE_H
#define PARTICLE_H

#include "HandmadeMath.h"
#include "transform.h"
#include "texture.h"
#include "anim.h"

typedef struct particle {
  HMM_Vec3 pos;
  HMM_Vec3 v; /* velocity */
  float angle;
  float av; /* angular velocity */
  float scale;
  double life;
  HMM_Vec4 color;
} particle;

typedef struct emitter {
  struct particle *particles;
  transform3d t;
  float explosiveness; /* 0 for a stream, 1 for all at once. Range of values allowed. */
  int max; /* number of particles */
  double life; /* how long a particle lasts */
  double tte; /* time to emit */
  sampler color; /* color over particle lifetime */
  float scale;
  float speed;
  int gravity; /* true if affected by gravity */
  texture *texture;
  int on;
} emitter;

void particle_init();

emitter *make_emitter();
void free_emitter(emitter *e);

void start_emitter(emitter *e);
void pause_emitter(emitter *e);
void stop_emitter(emitter *e);

void emitter_emit(emitter *e, int count);
void emitters_step(double dt);
void emitters_draw();
void emitter_step(emitter *e, double dt);

#endif
