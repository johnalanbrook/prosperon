#include "particle.h"
#include "stb_ds.h"
#include "render.h"
#include "particle.sglsl.h"
#include "2dphysics.h"
#include "log.h"
#include "simplex.h"

static emitter **emitters;

static sg_shader par_shader;
static sg_pipeline par_pipe;
static sg_bindings par_bind;
static int draw_count;

#define MAX_PARTICLES 500000

struct par_vert {
  HMM_Vec2 pos;
  float angle;
  HMM_Vec2 scale;
  struct rgba color;
};

typedef struct par_vert par_vert;

void particle_init()
{
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
      .compare = SG_COMPAREFUNC_LESS_EQUAL,
      .pixel_format = SG_PIXELFORMAT_DEPTH
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
    .usage = SG_USAGE_IMMUTABLE
  });

  par_bind.fs.samplers[0] = sg_make_sampler(&(sg_sampler_desc){});
}

emitter *make_emitter() {
  emitter *e = NULL;
  e = malloc(sizeof(*e));
  e->max = 20;
  e->particles = NULL;
  arrsetcap(e->particles, e->max);
  for (int i = 0; i < arrlen(e->particles); i++)
    e->particles[i].life = 0;
    
  e->life = 10;
  e->explosiveness = 0;
  e->tte = lerp(e->explosiveness, e->life/e->max, 0);
  e->color.times = NULL;
  e->color.data = NULL;
  e->color.type = LINEAR;
  sampler_add(&e->color, 0, (HMM_Vec4){1,1,1,1});
  e->scale = 1;
  e->speed = 20;
  e->gravity = 1;
  e->on = 0;
  e->texture = texture_pullfromfile("glass_chunk2.gif");
  arrpush(emitters,e);
  return e;
}

void free_emitter(emitter *e)
{
  arrfree(e->particles);
  for (int i = arrlen(emitters)-1; i >= 0; i--)
    if (emitters[i] == e) {
      arrdelswap(emitters,i);
      break;
    }
}

void start_emitter(emitter *e) { e->on = 1; }
void stop_emitter(emitter *e) { e->on = 0; }

int emitter_spawn(emitter *e)
{
  particle p;
  p.life = e->life;
  p.pos = (HMM_Vec3){0,0,0};
  p.v = (HMM_Vec3){frand(1)-0.5,frand(1)-0.5,0};
  p.v = HMM_ScaleV3(HMM_NormV3(p.v), e->speed);
  p.angle = 0;
  p.av = 1;
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

void emitters_draw()
{
  int draw_count = 0;
  for (int i = 0; i < arrlen(emitters); i++) {
    emitter *e = emitters[i];
    par_bind.fs.images[0] = e->texture->id;
    for (int j = 0; j < arrlen(e->particles); j++) {
      particle *p = &e->particles[j];
      pv[j].pos = p->pos.xy;
      pv[j].angle = p->angle;
      pv[j].scale = HMM_ScaleV2(tex_get_dimensions(e->texture), p->scale);
      pv[j].color = vec2rgba(p->color);
    }
    sg_append_buffer(par_bind.vertex_buffers[0], &(sg_range){.ptr=&pv, .size=sizeof(struct par_vert)*arrlen(e->particles)});
    draw_count += arrlen(e->particles);
  }

  sg_apply_pipeline(par_pipe);
  sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, SG_RANGE_REF(projection));
  sg_apply_bindings(&par_bind);
  sg_draw(0, 4, draw_count);
}

void emitter_step(emitter *e, double dt) {
  for (int i = arrlen(e->particles)-1; i >= 0; i--) {
    particle p = e->particles[i];
    if (e->gravity) 
      p.v = HMM_AddV3(p.v, HMM_MulV3F((HMM_Vec3){cpSpaceGetGravity(space).x, cpSpaceGetGravity(space).y, 0}, dt));

//    float freq = 1;
//    p.v = HMM_AddV3(p.v, HMM_MulV3F((HMM_Vec3){Noise2D(p.pos.x*freq, p.pos.y*freq), Noise2D(p.pos.x*freq+5,p.pos.y*freq+5), 0}, 1000*dt));
    p.v = HMM_AddV3(p.v, HMM_MulV3F((HMM_Vec3){frand(2)-1, frand(2)-1, 0}, 1000*dt));

    p.pos = HMM_AddV3(p.pos, HMM_MulV3F(p.v, dt));
    p.angle += p.av*dt;
    p.life -= dt;
    p.color = sample_sampler(&e->color, (e->life-p.life)/e->life);
    p.scale = e->scale;
    e->particles[i] = p;    

    if (p.life <= 0)
      arrdelswap(e->particles,i);

    if (query_point(p.pos.xy))
      arrdelswap(e->particles,i);
  }

  if (!e->on) return;
  e->tte-=dt;
  if (e->tte <= 0) {
    emitter_spawn(e);
    e->tte = lerp(e->explosiveness, e->life/e->max,0);
  }
}
