#ifndef PARTICLE_H
#define PARTICLE_H

#include "HandmadeMath.h"
#include "warp.h"
#include "transform.h"
#include "texture.h"
#include "anim.h"

typedef struct particle {
  HMM_Vec4 pos;
  HMM_Vec4 v; /* velocity */
  float angle;
  float av; /* angular velocity */
  float scale;
  double time;
  double life;
  HMM_Vec4 color;
} particle;

typedef struct emitter {
  struct particle *particles;
  transform3d t;
  float explosiveness; /* 0 for a stream, 1 for all at once. Range of values allowed. */
  int max; /* number of particles */
  double life; /* how long a particle lasts */
  double life_var;
  /* PARTICLE GEN */
  float speed; /* initial speed of particle */
  float variation; /* variation on speed */
  float divergence; /* angular degree of variation from emitter normal, up to 1 */
  sampler color; /* color over particle lifetime */
  float scale;
  float scale_var;
  float grow_for; /* seconds to grow from small until scale */
  float shrink_for; /* seconds to shrink to small prior to its death */
  /* PARTICLE TYPE */
  texture *texture;
  /* ROTATION AND COLLISION */
  int collision_mask; /* mask for collision */
  float bounce; /* bounce back after collision */
  /* PARTICLE SPAWN */
  int die_after_collision;
  float persist; /* how long to linger after death */
  float persist_var;
  /* TRAILS */
  warpmask warp_mask;
  int on;
  double tte; /* time to emit */  
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
