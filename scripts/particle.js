var emitter = {};
emitter.particles = {};
emitter.life = 10;
emitter.scale = 1;
emitter.grow_for = 0;
emitter.spawn_timer = 0;
emitter.pps = 0;
emitter.color = Color.white;

var ssbo;

var drawnum = 0;
emitter.draw = function (arr) {
  var pars = Object.values(this.particles);
  if (pars.length === 0) return;
//  render.make_particle_ssbo(pars, ssbo);
  for (var i of pars)
    arr.push(i);
//  drawnum += pars.length;
};

emitter.kill = function () {
  emitters.remove(this);
};

var std_step = function (p) {
  if (p.time < this.grow_for) {
    var s = Math.lerp(0, this.scale, p.time / this.grow_for);
    p.transform.scale = [s, s, s];
  } else if (p.time > p.life - this.shrink_for) {
    var s = Math.lerp(0, this.scale, (p.life - p.time) / this.shrink_for);
    p.transform.scale = [s, s, s];
  } else p.transform.scale = [this.scale, this.scale, this.scale];
};

emitter.step_hook = std_step;

emitter.spawn = function (t) {
  t ??= this.transform;

  var par = this.dead.shift();
  if (par) {
    par.body.pos = t.pos;
    par.transform.scale = [this.scale, this.scale, this.scale];
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
    color: this.color,
  };

  par.body = os.make_body(par.transform);

  par.body.pos = t.pos;
  par.transform.scale = [this.scale, this.scale, this.scale];
  par.id = prosperon.guid();
  this.particles[par.id] = par;

  this.spawn_hook(par);
};

emitter.step = function (dt) {
  // update spawning particles
  if (this.on && this.pps > 0) {
    this.spawn_timer += dt;
    var pp = 1 / this.pps;
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
};

emitter.burst = function (count, t) {
  for (var i = 0; i < count; i++) this.spawn(t);
};

var emitters = [];

var make_emitter = function () {
  var e = Object.create(emitter);
  e.ssbo = render.make_textssbo();
  e.shape = shape.centered_quad;
  e.shader = "shaders/baseparticle.cg";
  e.dead = [];
  emitters.push(e);
  return e;
};

function update_emitters(dt) {
  for (var e of emitters) e.step(dt);
}

function draw_emitters() {
  ssbo ??= render.make_textssbo();
  render.use_shader("shaders/baseparticle.cg");
  var buckets = new Map();
  var base = 0;
  for (var e of emitters) {
    var bucket = buckets.get(e.diffuse);
    if (!bucket)
      buckets.set(e.diffuse, [e]);
    else
      bucket.push(e);
  }

  for (var bucket of buckets.values()) {
    drawnum = 0;
    var append = [];
    bucket[0].baseinstance = base;
    render.use_mat(bucket[0]);
    for (var e of bucket)
      e.draw(append);

    render.make_particle_ssbo(append, ssbo);
    render.draw(bucket[0].shape, ssbo, append.length);
    base += append.length;
  }

//  for (var e of emitters) e.draw();

//  render.draw(shape.centered_quad, ssbo, drawnum);
}

function stat_emitters()
{
  var stat = {};
  stat.emitters = emitters.length;
  var particles = 0;
  for (var e of emitters) particles += Object.values(e.particles).length;
  stat.particles = particles
  return stat;
}

return { make_emitter, update_emitters, draw_emitters, stat_emitters };
