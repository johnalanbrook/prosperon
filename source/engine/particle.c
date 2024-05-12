#include "particle.h"
#include "stb_ds.h"
#include "render.h"
#include "2dphysics.h"
#include "math.h"
#include "log.h"

emitter *make_emitter() {
  emitter *e = calloc(sizeof(*e),1);
  
  e->max = 20;
  arrsetcap(e->particles, 10);
  for (int i = 0; i < arrlen(e->particles); i++)
    e->particles[i].life = 0;
    
  e->life = 10;
  e->tte = lerp(e->explosiveness, e->life/e->max, 0);
  e->scale = 1;
  e->speed = 20;
  e->buffer = sg_make_buffer(&(sg_buffer_desc){
    .size = sizeof(struct par_vert),
    .type = SG_BUFFERTYPE_STORAGEBUFFER,
    .usage = SG_USAGE_STREAM
  });
  return e;
}

void emitter_free(emitter *e)
{
  arrfree(e->particles);
  arrfree(e->verts);
  free(e);
}

/* Variate a value around variance. Variance between 0 and 1. */

float variate(float val, float variance)
{
  return val + val*(frand(variance)-(variance/2));
}

int emitter_spawn(emitter *e, transform *t)
{
  if (arrlen(e->particles) == e->max) return 0;
  particle p = {0};
  p.life = e->life;
  p.pos = (HMM_Vec4){t->pos.x,t->pos.y,t->pos.z,0};
  HMM_Vec3 up = trans_forward(t);
  float newan = (frand(e->divergence)-(e->divergence/2))*HMM_TurnToRad;
  HMM_Vec2 v2n = HMM_V2Rotate((HMM_Vec2){0,1}, newan);
  HMM_Vec3 norm = (HMM_Vec3){v2n.x, v2n.y,0};
  p.v = HMM_MulV4F((HMM_Vec4){norm.x,norm.y,norm.z,0}, variate(e->speed, e->variation));
  p.angle = 0.25;
  p.scale = variate(e->scale*t->scale.x, e->scale_var);
  arrput(e->particles,p);
  return 1;
}

void emitter_emit(emitter *e, int count, transform *t)
{
  for (int i = 0; i < count; i++)
    emitter_spawn(e, t);
}

void emitter_draw(emitter *e)
{
  if (arrlen(e->particles) == 0) return;
  arrsetlen(e->verts, arrlen(e->particles));
  for (int i = 0; i < arrlen(e->particles); i++) {
    if (e->particles[i].time >= e->particles[i].life) continue;
    particle *p = e->particles+i;
    e->verts[i].pos = p->pos.xy;
    e->verts[i].angle = p->angle;
    e->verts[i].scale = p->scale;
/*    if (p->time < e->grow_for)
      e->verts[i].scale = lerp(p->time/e->grow_for, 0, p->scale);
    else if (p->time > (p->life - e->shrink_for))
      e->verts[i].scale = lerp((p->time-(p->life-e->shrink_for))/e->shrink_for, p->scale, 0);*/
    e->verts[i].color = p->color;
  }

  sg_range verts;
  verts.ptr = e->verts;
  verts.size = sizeof(*e->verts)*arrlen(e->verts);
  if (sg_query_buffer_will_overflow(e->buffer, verts.size)) {
    sg_destroy_buffer(e->buffer);
    e->buffer = sg_make_buffer(&(sg_buffer_desc){
      .size = verts.size,
      .type = SG_BUFFERTYPE_STORAGEBUFFER,
      .usage = SG_USAGE_STREAM
    });
  }
    
  sg_append_buffer(e->buffer, &verts);
}

void emitter_step(emitter *e, double dt, transform *t) {
  
  HMM_Vec4 g_accel = HMM_MulV4F((HMM_Vec4){cpSpaceGetGravity(space).x, cpSpaceGetGravity(space).y, 0, 0}, dt);
  
  for (int i = 0; i < arrlen(e->particles); i++) {
    if (e->particles[i].time >= e->particles[i].life) continue;
  
    //if (e->warp_mask & gravmask)
//      e->particles[i].v = HMM_AddV4(e->particles[i].v, g_accel);
      
    e->particles[i].pos = HMM_AddV4(e->particles[i].pos, HMM_MulV4F(e->particles[i].v, dt));
    e->particles[i].angle += e->particles[i].av*dt;
    e->particles[i].time += dt;
    e->particles[i].color = sample_sampler(&e->color, e->particles[i].time/e->particles[i].life);
    e->particles[i].scale = e->scale;
  
   if (e->particles[i].time >= e->particles[i].life)
     arrdelswap(e->particles, i);
//   else if (query_point(e->particles[i].pos.xy))
//     arrdelswap(e->particles,i);
  }

  e->tte-=dt;
  float step = lerp(e->explosiveness, e->life/e->max,0);
  while (e->tte <= 0) {
    e->tte += step;
    if (!emitter_spawn(e, t)) break;
  }
}
