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
  var amt = Object.values(this.particles).length;
  if (amt === 0) return;
  render.use_shader(this.shader);
  render.use_mat(this);
  render.make_particle_ssbo(Object.values(this.particles), this.ssbo);
  render.draw(this.shape, this.ssbo, amt);
}

var std_spawn = function(par)
{

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

var std_die = function(par)
{
}

emitter.spawn_hook = std_spawn;
emitter.step_hook = std_step;
emitter.die_hook = std_die;

emitter.spawn = function(t)
{
  t ??= this.transform;
  var par = {
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

    this.step_hook(p);
    
    if (p.time >= p.life) {
      this.die_hook(p);
      delete this.particles[p.id];
    }
  }
}

emitter.burst = function(count, t) {
  for (var i = 0; i < count; i++) this.spawn(t);
}

var make_emitter = function()
{
  var e = Object.create(emitter);
  e.ssbo = render.make_textssbo();
  e.shape = shape.quad;
  e.shader = render.make_shader("shaders/baseparticle.cg");  
  return e;
}

return {make_emitter};
