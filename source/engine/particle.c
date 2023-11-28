#include "particle.h"
#include "stb_ds.h"
#include "freelist.h"

struct emitter make_emitter() {
  struct emitter e = {0};
  arrsetcap(e.particles, 200);
  return e;
}

void free_emitter(struct emitter e) {
  arrfree(e.particles);
}

void start_emitter(struct emitter e) {
 
}

void pause_emitter(struct emitter e) {

}

void stop_emitter(struct emitter e) {

}

void emitter_step(struct emitter e, double dt) {
  for (int i = 0; i < arrlen(e.particles); i++) {
  e.particles[i].pos = HMM_AddV3(e.particles[i].pos, HMM_MulV3F(e.particles[i].v, dt));
  e.particles[i].angle = HMM_MulQ(e.particles[i].angle, HMM_MulQF(e.particles[i].angle, dt));
  e.particles[i].life -= dt;

  if (e.particles[i].life <= 0)
    arrdelswap(e.particles, i);
  }
}
