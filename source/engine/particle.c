#include "particle.h"
#include "stb_ds.h"
#include "render.h"
#include "particle.sglsl.h"
#include "2dphysics.h"
#include "log.h"
#include "simplex.h"
#include "pthread.h"
#include "math.h"

#define SCHED_IMPLEMENTATION
#include "sched.h"

static emitter **emitters;

static sg_shader par_shader;
static sg_pipeline par_pipe;
static sg_bindings par_bind;
static int draw_count;

#define MAX_PARTICLES 1000000

struct scheduler sched;
void *mem;

struct par_vert {
  HMM_Vec2 pos;
  float angle;
  HMM_Vec2 scale;
  struct rgba color;
};

typedef struct par_vert par_vert;

void particle_init()
{
  sched_size needed;
  scheduler_init(&sched, &needed, 1, NULL);
  mem = calloc(needed, 1);
  scheduler_start(&sched,mem);
  
  par_shader = sg_make_shader(particle_shader_desc(sg_query_backend()));
  
  par_pipe = sg_make_pipeline(&(sg_pipeline_desc){
    .shader = par_shader,
    .layout = {
      .attrs = {
        [1].format = SG_VERTEXFORMAT_FLOAT2,
	[2].format = SG_VERTEXFORMAT_FLOAT,
	[3].format = SG_VERTEXFORMAT_FLOAT2,
	[4].format = SG_VERTEXFORMAT_UBYTE4N,
	[0].format = SG_VERTEXFORMAT_FLOAT2,
	[0].buffer_index = 1
      },
    .buffers[0].step_func = SG_VERTEXSTEP_PER_INSTANCE,
    },
    .primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP,
    .label = "particle pipeline",
    .cull_mode = SG_CULLMODE_BACK,
    .colors[0].blend = blend_trans,
    .depth = {
      .write_enabled = true,
      .compare = SG_COMPAREFUNC_LESS_EQUAL
    }
  });

  par_bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
    .size = sizeof(par_vert)*MAX_PARTICLES,
    .type = SG_BUFFERTYPE_VERTEXBUFFER,
    .usage = SG_USAGE_STREAM,
    .label = "particle buffer"
  });
  
  float circleverts[8] = {
    0,0,
    0,1,
    1,0,
    1,1,
  };

  par_bind.vertex_buffers[1] = sg_make_buffer(&(sg_buffer_desc){
    .data = (sg_range){.ptr = circleverts, .size = sizeof(float)*8},
    .usage = SG_USAGE_IMMUTABLE,
    .label = "particle quater buffer"
  });

  par_bind.fs.samplers[0] = sg_make_sampler(&(sg_sampler_desc){});
}

emitter *make_emitter() {
  emitter *e = calloc(sizeof(*e),1);
  
  e->max = 20;
  arrsetcap(e->particles, e->max);
  for (int i = 0; i < arrlen(e->particles); i++)
    e->particles[i].life = 0;
    
  e->life = 10;
  e->tte = lerp(e->explosiveness, e->life/e->max, 0);
  sampler_add(&e->color, 0, (HMM_Vec4){1,1,1,1});
  e->scale = 1;
  e->speed = 20;
  e->texture = NULL;
  arrpush(emitters,e);
  return e;
}

void emitter_free(emitter *e)
{
  YughWarn("kill emitter");
  arrfree(e->particles);
  for (int i = arrlen(emitters)-1; i >= 0; i--)
    if (emitters[i] == e) {
      arrdelswap(emitters,i);
      break;
    }
}

void start_emitter(emitter *e) { e->on = 1; }
void stop_emitter(emitter *e) { e->on = 0; }

/* Variate a value around variance. Variance between 0 and 1. */

float variate(float val, float variance)
{
  return val + val*(frand(variance)-(variance/2));
}

int emitter_spawn(emitter *e)
{
  particle p;
  p.life = e->life;
  p.pos = (HMM_Vec4){e->t.pos.x,e->t.pos.y,0,0};
  float newan = e->t.rotation.Elements[0]+(2*HMM_PI*(frand(e->divergence)-(e->divergence/2)));
  HMM_Vec2 norm = HMM_V2Rotate((HMM_Vec2){0,1}, newan);
  p.v = HMM_MulV4F((HMM_Vec4){norm.x,norm.y,0,0}, variate(e->speed, e->variation));
  p.angle = 0;
  p.scale = variate(e->scale, e->scale_var);
//  p.av = 1;
  arrput(e->particles,p);
  return 1;
}

void emitter_emit(emitter *e, int count)
{
  for (int i = 0; i < count; i++)
    emitter_spawn(e);
}

void emitters_step(double dt)
{
  for (int i = 0; i < arrlen(emitters); i++)
    emitter_step(emitters[i], dt);
}

static struct par_vert pv[MAX_PARTICLES];

void parallel_pv(emitter *e, struct scheduler *sched, struct sched_task_partition t, sched_uint thread_num)
{
  for (int i=t.start; i < t.end; i++) {
    if (e->particles[i].time >= e->particles[i].life) continue;
    particle *p = &e->particles[i];
    pv[i].pos = p->pos.xy;
    pv[i].angle = p->angle;
    float s = p->scale;
    if (p->time < e->grow_for)
      s = lerp(p->time/e->grow_for, 0, p->scale);
    else if (p->time > (p->life - e->shrink_for))
      s = lerp((p->time-(p->life-e->shrink_for))/e->shrink_for, p->scale, 0);
    pv[i].scale = HMM_ScaleV2((HMM_Vec2){e->texture->width,e->texture->height}, s);
    pv[i].color = vec2rgba(p->color);
  }
}

void emitters_draw(HMM_Mat4 *proj)
{
  if (arrlen(emitters) == 0) return;
  int draw_count = 0;
  for (int i = 0; i < arrlen(emitters); i++) {
    emitter *e = emitters[i];
    par_bind.fs.images[0] = e->texture->id;

    struct sched_task task;
    scheduler_add(&sched, &task, parallel_pv, e, arrlen(e->particles), arrlen(e->particles)/sched.threads_num);
    scheduler_join(&sched, &task);
    
    sg_append_buffer(par_bind.vertex_buffers[0], &(sg_range){.ptr=&pv, .size=sizeof(struct par_vert)*arrlen(e->particles)});
    draw_count += arrlen(e->particles);
  }

  sg_apply_pipeline(par_pipe);
  sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, SG_RANGE_REF(*proj));
  sg_apply_bindings(&par_bind);
  sg_draw(0, 4, draw_count);
}

static double dt;
static HMM_Vec4 g_accel;

void parallel_step(emitter *e, struct scheduler *shed, struct sched_task_partition t, sched_uint thread_num)
{
  for (int i = t.end-1; i >=0; i--) {
    if (e->particles[i].time >= e->particles[i].life) continue;

    if (e->warp_mask & gravmask)
      e->particles[i].v = HMM_AddV4(e->particles[i].v, g_accel);
      
    e->particles[i].pos = HMM_AddV4(e->particles[i].pos, HMM_MulV4F(e->particles[i].v, dt));
    e->particles[i].angle += e->particles[i].av*dt;
    e->particles[i].time += dt;
    e->particles[i].color = sample_sampler(&e->color, e->particles[i].time/e->particles[i].life);
    e->particles[i].scale = e->scale;

   if (e->particles[i].time >= e->particles[i].life)
     arrdelswap(e->particles, i);
   else if (query_point(e->particles[i].pos.xy))
     arrdelswap(e->particles,i);
  }
}

void emitter_step(emitter *e, double mdt) {
  dt = mdt;
  g_accel = HMM_MulV4F((HMM_Vec4){cpSpaceGetGravity(space).x, cpSpaceGetGravity(space).y, 0, 0}, dt);
  if (arrlen(e->particles) == 0) return;
  struct sched_task task;
  scheduler_add(&sched, &task, parallel_step, e, arrlen(e->particles), arrlen(e->particles)/sched.threads_num);
  scheduler_join(&sched, &task);

  if (!e->on) return;
  e->tte-=dt;
  if (e->tte <= 0) {
    emitter_spawn(e);
    e->tte = lerp(e->explosiveness, e->life/e->max,0);
  }
}
