var emitter = {};
emitter.particles = {};
emitter.life = 10;
emitter.scale = 1;
emitter.grow_for = 0;
emitter.spawn_timer = 0;
emitter.pps = 0;
emitter.color = Color.white;

emitter.draw = function()
{
  var pars = Object.values(this.particles);
  if (pars.length === 0) return;
  render.use_shader(this.shader);
  render.use_mat(this);
  render.make_particle_ssbo(pars, this.ssbo);
  render.draw(this.shape, this.ssbo, pars.length);
}

emitter.kill = function()
{
  emitters.remove(this);
}

var std_step = function(p)
{
  if (p.time < this.grow_for) {
    var s = Math.lerp(0, this.scale, p.time/this.grow_for);
    p.transform.scale = [s,s,s];
  }
  else if (p.time > (p.life - this.shrink_for)) {
    var s = Math.lerp(0,this.scale,(p.life-p.time)/this.shrink_for);
    p.transform.scale=[s,s,s];
  } else
    p.transform.scale = [this.scale,this.scale,this.scale];
}

emitter.step_hook = std_step;

emitter.spawn = function(t)
{
  t ??= this.transform;
  
  var par = this.dead.shift();
  if (par) {
    par.body.pos = t.pos;
    par.transform.scale = [this.scale,this.scale,this.scale];
    this.particles[par.id] = par;
    par.time = 0;
    this.spawn_hook?.(par);
    par.life = this.life;
    return;
  }

  par = {
    transform: os.make_transform(),
    life: this.life,
    time: 0,
    color: this.color
  };

  par.body = os.make_body(par.transform);

  par.body.pos = t.pos;
  par.transform.scale = [this.scale,this.scale,this.scale];
  par.id = prosperon.guid();
  this.particles[par.id] = par;

  this.spawn_hook(par);
}

emitter.step = function(dt)
{
  // update spawning particles
  if (this.on && this.pps > 0) {
    this.spawn_timer += dt;
    var pp = 1/this.pps;
    while (this.spawn_timer > pp) {
      this.spawn_timer -= pp;
      this.spawn();
    }
  }

  // update all particles
  for (var p of Object.values(this.particles)) {
    p.time += dt;
    this.step_hook?.(p);

    if (this.kill_hook?.(p) || p.time >= p.life) {
      this.die_hook?.(p);
      this.dead.push(this.particles[p.id]);
      delete this.particles[p.id];
    }
  }
}

emitter.burst = function(count, t) {
  for (var i = 0; i < count; i++) this.spawn(t);
}

var emitters = [];

var make_emitter = function()
{
  var e = Object.create(emitter);
  e.ssbo = render.make_textssbo();
  e.shape = shape.centered_quad;
  e.shader = "shaders/baseparticle.cg";
  e.dead = [];
  emitters.push(e);
  return e;
}

function update_emitters(dt)
{
  for (var e of emitters)
    e.step(dt);
}

function draw_emitters()
{
  for (var e of emitters) e.draw();
}

return {make_emitter, update_emitters, draw_emitters};
