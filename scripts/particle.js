var emitter = {};
emitter.life = 10;
emitter.scale = 1;
emitter.grow_for = 0;
emitter.spawn_timer = 0;
emitter.pps = 0;
emitter.color = Color.white;

var ssbo;

emitter.kill = function () {
  emitters.remove(this);
};

var std_step = function (p) {
  if (p.time < this.grow_for) {
    var s = Math.lerp(0, this.scale, p.time / this.grow_for);
    p.transform.scale = s;
  } else if (p.time > p.life - this.shrink_for) {
    var s = Math.lerp(0, this.scale, (p.life - p.time) / this.shrink_for);
    p.transform.scale = s;
  } else p.transform.scale = [this.scale, this.scale, this.scale];
};

emitter.step_hook = std_step;

emitter.spawn = function (t) {
  t ??= this.transform;

  var par = this.dead.shift();
  if (par) {
    par.transform.unit();
    par.transform.pos = t.pos;
    par.transform.scale = this.scale;
    this.particles.push(par);
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
    body:{},
  };

  par.transform.scale = this.scale;
  this.particles.push(par);

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
  for (var p of this.particles) {
    p.time += dt;
    p.transform.move(p.body.velocity?.scale(dt));
    this.step_hook?.(p);

    if (this.kill_hook?.(p) || p.time >= p.life) {
      this.die_hook?.(p);
      this.dead.push(p);
      this.particles.remove(p);
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
  e.particles = [];
  e.dead = [];
  emitters.push(e);
  return e;
};

function update_emitters(dt) {
  for (var e of emitters) e.step(dt);
}

var arr = [];
function draw_emitters() {
  return;
  ssbo ??= render.make_textssbo();
  render.use_shader("shaders/baseparticle.cg");
  var buckets = {};
  var base = 0;
  for (var e of emitters) {
    var bucket = buckets[e.diffuse.path];
    if (!bucket)
      buckets[e.diffuse.path] = [e];
    else
      bucket.push(e);
  }

  for (var path in buckets) {
    arr.length = 0;
    var bucket = buckets[path];
    bucket[0].baseinstance = base;
    render.use_mat({diffuse:bucket[0].diffuse.texture});
    for (var e of bucket) {
      if (e.particles.length === 0) continue;
      for (var p of e.particles) arr.push(p);
   }
    render.make_particle_ssbo(arr, ssbo);
    render.draw(bucket[0].shape, ssbo, arr.length);
    base += arr.length;
  }
}

function stat_emitters()
{
  var stat = {};
  stat.emitters = emitters.length;
  var particles = 0;
  for (var e of emitters) particles += e.particles.length;
  stat.particles = particles;
  return stat;
}

return { make_emitter, update_emitters, draw_emitters, stat_emitters };
