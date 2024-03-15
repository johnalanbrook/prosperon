function obj_unique_name(name, obj) {
  name = name.replaceAll('.', '_');
  if (!(name in obj)) return name;
  var t = 1;
  var n = name + t;
  while (n in obj) {
    t++;
    n = name + t;
  }
  return n;
}

var gameobject_impl = {
  get pos() {
    assert(this.master, `Entity ${this.toString()} has no master.`);
    return this.master.world2this(this.worldpos());
  },

  set pos(x) {
    assert(this.master, `Entity ${this.toString()} has no master.`);
    this.set_worldpos(this.master.this2world(x));
  },

  get angle() {
    assert(this.master, `No master set on ${this.toString()}`);
    return this.worldangle() - this.master.worldangle();
  },

  set angle(x) {
    var diff = x - this.angle;

    this.objects.forEach(function(x) {
      x.rotate(diff);
      x.pos = Vector.rotate(x.pos, diff);
    });

    this.sworldangle(x - this.master.worldangle());
  },

  get scale() {
    assert(this.master, `No master set on ${this.toString()}`);
    var pscale = [1, 1, 1];
    return this.gscale().map((x, i) => x / (this.master.gscale()[i] * pscale[i]));
  },

  set scale(x) {
    if (typeof x === 'number')
      x = [x, x];

    var pct = this.scale.map((s, i) => x[i] / s);
    this.grow(pct);

    /* TRANSLATE ALL SUB OBJECTS */
    this.objects.forEach(obj => {
      obj.grow(pct);
      obj.pos = obj.pos.map((x, i) => x * pct[i]);
    });
  },

  get draw_layer() { return this.body.drawlayer; },
  set draw_layer(x) { this.body.drawlayer = x; },
  set layer(x) { this.body.layer = x; },
  get layer() { return this.body.layer; },
  set warp_layer(x) { this.body.warp_filter = x; },
  get warp_layer() { return this.body.warp_filter; },

  set mass(x) { set_body(7, this.body, x); },
  get mass() {
    if (!(this.phys === physics.dynamic))
      return undefined;

    return q_body(5, this.body);
  },
  get elasticity() { return this.body.e; },
  set elasticity(x) { this.body.e = x; },
  get friction() { return this.body.f; },
  set friction(x) { this.body.f = x; },
  set timescale(x) { this.body.timescale = x; },
  get timescale() { return this.body.timescale; },
  set phys(x) { set_body(1, this.body, x); },
  get phys() { return q_body(0, this.body); },
  get velocity() { return q_body(3, this.body); },
  set velocity(x) { set_body(9, this.body, x); },
  get damping() { return this.body.damping; },
  set damping(x) { this.body.damping = x },
  get angularvelocity() { return Math.rad2turn(q_body(4, this.body)); },
  set angularvelocity(x) { set_body(8, this.body, Math.turn2rad(x)); },
  get max_velocity() { return this.body.maxvelocity; },
  set max_velocity(x) { this.body.maxvelocity = x; },
  get max_angularvelocity() { return this.body.maxangularvelocity; },
  set max_angularvelocity(x) { this.body.maxangularvelocity = x; },
  get_moi() { return q_body(6, this.body); },
  set_moi(x) {
    if (x <= 0) {
      console.error("Cannot set moment of inertia to 0 or less.");
      return;
    }
    set_body(13, this.body, x);
  },
};

var gameobject = {
  get_comp_by_name(name) {
    var comps = [];
    for (var c of Object.values(this.components))
      if (c.comp === name) comps.push(c);

    if (comps.length) return comps;
    return undefined;
  },
  check_dirty() {
    this._ed.urdiff = this.json_obj();
    this._ed.dirty = !Object.empty(this._ed.urdiff);
    return; // TODO: IMPLEMENT
    var lur = ur[this.master.ur];
    if (!lur) return;
    var lur = lur.objects[this.toString()];
    var d = ediff(this._ed.urdiff, lur);
    if (!d || Object.empty(d))
      this._ed.inst = true;
    else
      this._ed.inst = false;
  },
  _ed: {
    selectable: false,
    dirty: false
  },

  namestr() {
    var s = this.toString();
    if (this._ed.dirty)
      if (this._ed.inst) s += "#";
      else s += "*";
    return s;
  },

  urstr() {
    if (this._ed.dirty) return "*" + this.ur;
    return this.ur;
  },

  full_path() {
    return this.path_from(world);
  },
  /* pin this object to the to object */
  pin(to) {
    var p = cmd(222, this.body, to.body);
  },
  slide(to, a, b, min, max) {
    a ??= [0, 0];
    b ??= [0, 0];
    min ??= 0;
    max ??= 50;
    var p = cmd(229, this.body, to.body, a, b, min, max);
    p.max_force = 500;
    p.break();
  },
  pivot(to, piv) {
    piv ??= this.worldpos();
    var p = cmd(221, this.body, to.body, piv);
  },
  /* groove is on to, from local points a and b, anchored to this at local anchor */
  groove(to, a, b, anchor) {
    anchor ??= [0, 0];
    var p = cmd(228, to.body, this.body, a, b, anchor);
  },
  damped_spring(to, length, stiffness, damping) {
    length ??= Vector.length(this.worldpos(), to.worldpos());
    stiffness ??= 1;
    damping ??= 1;
    var dc = 2 * Math.sqrt(stiffness * this.mass);
    var p = cmd(227, this.body, to.body, [0, 0], [0, 0], stiffness, damping * dc);
  },
  damped_rotary_spring(to, angle, stiffness, damping) {
    angle ??= 0;
    stiffness ??= 1;
    damping ??= 1;
    /* calculate actual damping value from the damping ratio */
    /* damping = 1 is critical */
    var dc = 2 * Math.sqrt(stiffness * this.get_moi()); /* critical damping number */
    /* zeta = actual/critical */
    var p = cmd(226, this.body, to.body, angle, stiffness, damping * dc);
  },
  rotary_limit(to, min, max) {
    var p = cmd(225, this.body, to.body, Math.turn2rad(min), Math.turn2rad(max));
  },
  ratchet(to, ratch) {
    var phase = this.angle - to.angle;
    var p = cmd(230, this.body, to.body, phase, Math.turn2rad(ratch));
  },
  gear(to, ratio) {
    phase ??= 1;
    ratio ??= 1;
    var phase = this.angle - to.angle;
    var p = cmd(223, this.body, to.body, phase, ratio);
  },
  motor(to, rate) {
    var p = cmd(231, this.body, to.body, rate);
  },

  path_from(o) {
    var p = this.toString();
    var c = this.master;
    while (c && c !== o && c !== world) {
      p = c.toString() + "." + p;
      c = c.master;
    }
    if (c === world) p = "world." + p;
    return p;
  },

  clear() {
    for (var k in this.objects) {
      this.objects[k].kill();
    };
    this.objects = {};
  },

  delay(fn, seconds) {
    var timescale = this.timescale;
    var timers = this.timers;
    var thisfn = fn.bind(this);
    var rm_register;
    
    var ud = function(dt) {
      seconds -= dt*timescale;
      if (seconds <= 0) {
        thisfn();
        rm_register();
        rm_register = undefined;
        thisfn = undefined;
      }
    }
    
    rm_register = Register.update.register(ud);
    return rm_register;
  },

  tween(prop, values, def) {
    var t = Tween.make(this, prop, values, def);
    t.play();

    var k = function() { t.pause(); }
    this.timers.push(k);
    return k;
  },

  cry(file) {
    return;
    this.crying = audio.sound.play(file, audio.sound.bus.sfx);
    var killfn = () => { this.crying = undefined;
      console.warn("killed"); }
    this.crying.hook = killfn;
    return killfn;
  },

  set torque(x) { if (!(x >= 0 && x <= Infinity)) return;
    cmd(153, this.body, x); },
  gscale() { return cmd(103, this.body); },
  sgscale(x) {
    if (typeof x === 'number')
      x = [x, x];
    cmd(36, this.body, x)
  },

  phys_material() {
    var mat = {};
    mat.elasticity = this.elasticity;
    mat.friction = this.friction;
    return mat;
  },

  worldpos() { return q_body(1, this.body); },
  set_worldpos(x) {
    var poses = this.objects.map(x => x.pos);
    set_body(2, this.body, x);
    this.objects.forEach((o, i) => o.set_worldpos(this.this2world(poses[i])));
  },
  screenpos() { return Window.world2screen(this.worldpos()); },

  worldangle() { return Math.rad2turn(q_body(2, this.body)); },
  sworldangle(x) { set_body(0, this.body, Math.turn2rad(x)); },

  get_ur() {
    //     if (this.ur === 'empty') return undefined;
    return Object.access(ur, this.ur);
  },

  /* spawn an entity
         text can be:
	       the file path of a script
	       an ur object
	       nothing
      */
  spawn(text) {
    var ent = Object.create(gameobject);

    if (typeof text === 'object')
      text = text.name;

    if (typeof text === 'undefined')
      ent.ur = "empty";
    else if (typeof text !== 'string') {
      console.error(`Must pass in an ur type or a string to make an entity.`);
      return;
    } else {
      if (Object.access(ur, text))
        ent.ur = text;
      else if (io.exists(text))
        ent.ur = "script";
      else {
        console.warn(`Cannot make an entity from '${text}'. Not a valid ur.`);
        return;
      }
    }

    Object.mixin(ent, gameobject_impl);
    ent.body = make_gameobject();
    ent.warp_layer = [true];
    ent.phys = 2;
    ent.components = {};
    ent.objects = {};
    ent.timers = [];

    ent.reparent(this);

    ent._ed = {
      selectable: true,
      dirty: false,
      inst: false,
      urdiff: {},
    };

    cmd(113, ent.body, ent); // set the internal obj reference to this obj

    Object.hide(ent, 'ur', 'body', 'components', 'objects', '_ed', 'timers', 'master');
    if (ent.ur === 'empty') {
      if (!ur.empty.proto) ur.empty.proto = json.decode(json.encode(ent));
      return ent;
    }

    if (ent.ur === 'script')
      eval_env(io.slurp(text), ent, ent.ur);
    else
      apply_ur(ent.ur, ent);

    for (var [prop, p] of Object.entries(ent)) {
      if (!p) continue;
      if (typeof p !== 'object') continue;
      if (component.isComponent(p)) continue;
      if (!p.comp) continue;
      ent[prop] = component[p.comp].make(ent);
      Object.merge(ent[prop], p);
      ent.components[prop] = ent[prop];
    };

    check_registers(ent);
    ent.components.forEach(function(x) {
      if (typeof x.collide === 'function')
        register_collide(1, x.collide.bind(x), ent.body, x.shape);
    });


    if (typeof ent.load === 'function') ent.load();
    if (Game.playing())
      if (typeof ent.start === 'function') ent.start();

    var mur = ent.get_ur();
    if (mur && !mur.proto)
      mur.proto = json.decode(json.encode(ent));

    ent.sync();

    if (!Object.empty(ent.objects)) {
      var o = ent.objects;
      delete ent.objects;
      for (var i in o) {
        say(`MAKING ${i}`);
        var n = ent.spawn(ur[o[i].ur]);
        ent.rename_obj(n.toString(), i);
        delete o[i].ur;
        Object.assign(n, o[i]);
      }
    }
    
    this.body.phys = this.body.phys; // simple way to sync

    return ent;
  },

  /* Reparent 'this' to be 'parent's child */
  reparent(parent) {
    assert(parent, `Tried to reparent ${this.toString()} to nothing.`);
    if (this.master === parent) {
      console.warn("not reparenting ...");
      console.warn(`${this.master} is the same as ${parent}`);
      return;
    }

    this.master?.remove_obj(this);

    this.master = parent;

    function unique_name(list, name) {
      name ??= "new_object";
      var str = name.replaceAll('.', '_');
      var n = 1;
      var t = str;
      while (list.indexOf(t) !== -1) {
        t = str + n;
        n++;
      }
      return t;
    };

    var name = unique_name(Object.keys(parent.objects), this.ur);

    parent.objects[name] = this;
    parent[name] = this;
    this.toString = function() { return name; };
  },

  remove_obj(obj) {
    if (this[obj.toString()] === this.objects[obj.toString()])
      delete this[obj.toString()];

    delete this.objects[obj.toString()];
    delete this[obj.toString()];
  },

  components: {},
  objects: {},
  master: undefined,

  pulse(vec) { set_body(4, this.body, vec); },
  shove(vec) { set_body(12, this.body, vec); },
  shove_at(vec, at) { set_body(14, this.body, vec, at); },
  world2this(pos) { return cmd(70, this.body, pos); },
  this2world(pos) { return cmd(71, this.body, pos); },
  this2screen(pos) { return Window.world2screen(this.this2world(pos)); },
  screen2this(pos) { return this.world2this(Window.screen2world(pos)); },
  dir_world2this(dir) { return cmd(160, this.body, dir); },
  dir_this2world(dir) { return cmd(161, this.body, dir); },

  alive() { return this.body >= 0; },
  in_air() { return q_body(7, this.body); },

  hide() { this.components.forEach(x => x.hide?.());
    this.objects.forEach(x => x.hide?.()); },
  show() { this.components.forEach(function(x) { x.show?.(); });
    this.objects.forEach(function(x) { x.show?.(); }); },

  width() {
    var bb = this.boundingbox();
    return bb.r - bb.l;
  },

  height() {
    var bb = this.boundingbox();
    return bb.t - bb.b;
  },

  /* Moving, rotating, scaling functions, world relative */
  move(vec) { this.set_worldpos(this.worldpos().add(vec)); },
  rotate(x) { this.sworldangle(this.worldangle() + x); },
  grow(vec) { this.sgscale(this.gscale().map((x, i) => x * vec[i])); },

  /* Make a unique object the same as its prototype */
  revert() {
    var jobj = this.json_obj();
    var lobj = this.master.__proto__.objects[this.toString()];
    delete jobj.objects;
    Object.keys(jobj).forEach(function(x) {
      if (lobj && x in lobj)
        this[x] = lobj[x];
      else
        this[x] = this.__proto__[x];
    }, this);
    this.sync();
  },

  toString() { return "new_object"; },

  flipx() { return this.scale.x < 0; },
  flipy() { return this.scale.y < 0; },

  mirror(plane) {
    this.scale = Vector.reflect(this.scale, plane);
  },

  save: true,
  selectable: true,
  ed_locked: false,

  disable() { this.components.forEach(function(x) { x.disable(); }); },
  enable() { this.components.forEach(function(x) { x.enable(); }); },
  sync() {
    this.components.forEach(function(x) { x.sync?.(); });
    this.objects.forEach(function(x) { x.sync?.(); });
  },

  /* Bounding box of the object in world dimensions */
  boundingbox() {
    var boxes = [];
    boxes.push({
      t: 0,
      r: 0,
      b: 0,
      l: 0
    });

    for (var key in this.components) {
      if ('boundingbox' in this.components[key])
        boxes.push(this.components[key].boundingbox());
    }
    for (var key in this.objects)
      boxes.push(this.objects[key].boundingbox());

    var bb = boxes.shift();

    boxes.forEach(function(x) { bb = bbox.expand(bb, x); });

    bb = bbox.move(bb, this.pos);

    return bb ? bb : bbox.fromcwh([0, 0], [0, 0]);
  },

  /* The unique components of this object. Its diff. */
  json_obj() {
    var u = this.get_ur();
    if (!u) return {};
    var proto = u.proto;
    var thiso = json.decode(json.encode(this)); // TODO: SLOW. Used to ignore properties in toJSON of components.

    var d = ediff(thiso, proto);

    d ??= {};

    var objects = {};
    proto.objects ??= {};
    var curobjs = {};
    for (var o in this.objects)
      curobjs[o] = this.objects[o].instance_obj();

    var odiff = ediff(curobjs, proto.objects);
    if (odiff)
      d.objects = curobjs;

    delete d.pos;
    delete d.angle;
    delete d.scale;
    delete d.velocity;
    delete d.angularvelocity;
    return d;
  },

  /* The object needed to store an object as an instance of a master */
  instance_obj() {
    var t = this.transform();
    t.ur = this.ur;
    return t;
  },

  proto() {
    var u = this.get_ur();
    if (!u) return {};
    return u.proto;
  },

  transform() {
    var t = {};
    t.pos = this.pos;
    if (t.pos.every(x => x === 0)) delete t.pos;
    t.angle = Math.places(this.angle, 4);
    if (t.angle === 0) delete t.angle;
    t.scale = this.scale;
    t.scale = t.scale.map((x, i) => x / this.proto().scale[i]);
    t.scale = t.scale.map(x => Math.places(x, 3));
    if (t.scale.every(x => x === 1)) delete t.scale;
    return t;
  },

  /* Velocity and angular velocity of the object */
  phys_obj() {
    var phys = {};
    phys.velocity = this.velocity;
    phys.angularvelocity = this.angularvelocity;
    return phys;
  },

  dup(diff) {
    var n = this.master.spawn(this.__proto__);
    Object.totalmerge(n, this.instance_obj());
    return n;
  },

  kill() {
    if (this.__kill) return;
    this.__kill = true;

    this.timers.forEach(t => t());
    this.timers = [];
    Event.rm_obj(this);
    Player.do_uncontrol(this);
    register_collide(2, undefined, this.body);

    if (this.master) {
      this.master.remove_obj(this);
      this.master = undefined;
    }

    if (this.__proto__.instances)
      delete this.__proto__.instances[this.toString()];

    for (var key in this.components) {
      this.components[key].kill?.();
      this.components[key].gameobject = undefined;
      delete this.components[key];
      delete this[key];
    }

    this.clear();
    this.objects = undefined;

    if (typeof this.stop === 'function') this.stop();
    if (typeof this.die === 'function') this.die();
  },

  up() { return [0, 1].rotate(this.angle); },
  down() { return [0, -1].rotate(this.angle); },
  right() { return [1, 0].rotate(this.angle); },
  left() { return [-1, 0].rotate(this.angle); },

  make_objs(objs) {
    for (var prop in objs) {
      say(`spawning ${json.encode(objs[prop])}`);
      var newobj = this.spawn(objs[prop]);
    }
  },

  rename_obj(name, newname) {
    if (!this.objects[name]) {
      console.warn(`No object with name ${name}. Could not rename to ${newname}.`);
      return;
    }
    if (name === newname) {
      Object.hide(this, name);
      return;
    }
    if (this.objects[newname])
      return;

    this.objects[newname] = this.objects[name];
    this[newname] = this[name];
    this[newname].toString = function() { return newname; };
    Object.hide(this, newname);
    delete this.objects[name];
    delete this[name];
    return this.objects[newname];
  },

  add_component(comp, data, name) {
    data ??= undefined;
    if (typeof comp.make !== 'function') return;
    name ??= comp.toString();
    name = obj_unique_name(name, this);
    this[name] = comp.make(this);
    this[name].comp = comp.toString();
    this.components[name] = this[name];
    if (data)
      Object.assign(this[name], data);
    return this[name];
  },

  obj_descend(fn) {
    fn(this);
    for (var o in this.objects)
      this.objects[o].obj_descend(fn);
  },
}
Object.mixin(gameobject, gameobject_impl);

gameobject.spawn.doc = `Spawn an entity of type 'ur' on this entity. Returns the spawned entity.`;

gameobject.doc = {
  doc: "All objects in the game created through spawning have these attributes.",
  pos: "Position of the object, relative to its master.",
  angle: "Rotation of this object, relative to its master.",
  velocity: "Velocity of the object, relative to world.",
  angularvelocity: "Angular velocity of the object, relative to the world.",
  scale: "Scale of the object, relative to its master.",
  flipx: "Check if the object is flipped on its x axis.",
  flipy: "Check if the object is flipped on its y axis.",
  elasticity: `When two objects collide, their elasticities are multiplied together. Their velocities are then multiplied by this value to find their resultant velocities.`,
  friction: `When one object touches another, friction slows them down.`,
  mass: `The higher the mass of the object, the less forces will affect it.`,
  phys: `Set to 0, 1, or 2, representing dynamic, kinematic, and static.`,
  worldpos: `Function returns the world position of the object.`,
  set_worldpos: `Function to set the position of the object in world coordinates.`,
  worldangle: `Function to get the angle of the entity in the world.`,
  rotate: `Function to rotate this object by x degrees.`,
  move: 'Move an object by x,y,z. If the first parameter is an array, uses up to the first three array values.',
  pulse: `Apply an impulse to this body in world coordinates. Impulse is a short force.`,
  shove: `Apply a force to this body in world coordinates. Should be used over many frames.`,
  shove_at: 'Apply a force to this body, at a position relative to itself.',
  max_velocity: 'The max linear velocity this object can travel.',
  max_angularvelocity: 'The max angular velocity this object can rotate.',
  in_air: `Return true if the object is in the air.`,
  on_ground: `Return true if the object is on the ground.`,
  spawn: `Create an instance of a supplied ur-type on this object. Optionally provide a data object to modify the created entity.`,
  hide: `Make this object invisible.`,
  show: `Make this object visible.`,
  width: `The total width of the object and all its components.`,
  height: `The total height of the object.`,
  move: `Move this object the given amount.`,
  boundingbox: `The boundingbox of the object.`,
  dup: `Make an exact copy of this object.`,
  transform: `Return an object representing the transform state of this object.`,
  kill: `Remove this object from the world.`,
  master: "The entity this entity belongs to.",
  delay: 'Run the given function after the given number of seconds has elapsed.',
  cry: 'Make a sound. Can only make one at a time.',
  add_component: 'Add a component to the object by name.',
  pin: 'Pin joint to another object. Acts as if a rigid rod is between the two objects.',
  slide: 'Slide joint, similar to a pin but with min and max allowed distances.',
  pivot: 'Pivot joint to an object, with the pivot given in world coordinates.',
  groove: 'Groove joint. The groove is on to, from to local coordinates a and b, with this object anchored at anchor.',
  damped_spring: 'Damped spring to another object. Length is the distance it wants to be, stiffness is the spring constant, and damping is the damping ratio. 1 is critical, < 1 is underdamped, > 1 is overdamped.',
  damped_rotary_spring: 'Similar to damped spring but for rotation. Rest angle is the attempted angle.',
  rotary_limit: 'Limit the angle relative to the to body between min and max.',
  ratchet: 'Like a socket wrench, relative to to. ratch is the distance between clicks.',
  gear: 'Keeps the angular velocity ratio of this body and to constant. Ratio is the gear ratio.',
  motor: 'Keeps the relative angular velocity of this body to to at a constant rate. The most simple idea is for one of the bodies to be static, to the other is kept at rate.',
  layer: 'Bitmask for collision layers.',
  draw_layer: 'Layer for drawing. Higher numbers draw above lower ones.',
  warp_layer: 'Bitmask for selecting what warps should affect this entity.',
};

global.ur = {};

if (io.exists(".prosperon/ur.json"))
  ur = json.decode(io.slurp(".prosperon/ur.json"));
else {
  ur = {};
  ur._list = [];
}

/* UR OBJECT
ur {
  name: fully qualified name of ur
  text: file path to the script
  data: file path to data
  proto: resultant object of a freshly made entity
}
*/

/* Apply an ur u to an entity e */
/* u is given as */
function apply_ur(u, e) {
  console.info(`Applying ur named ${u}.`);
  if (typeof u !== 'string') {
    console.warn("Must give u as a string.");
    return;
  }

  var urs = u.split('.');
  var config = {};
  var topur = ur;
  for (var i = 0; i < urs.length; i++) {
    topur = topur[urs[i]];
    if (!topur) {
      console.warn(`Ur given by ${u} does not exist. Stopped at ${urs[i]}.`);
      return;
    }

    if (topur.text) {
      var script = Resources.replstrs(topur.text);
      eval_env(script, e, topur.text);
    }

    if (topur.data) {
      var jss = Resources.replstrs(topur.data);
      Object.merge(config, json.decode(jss));
    }
  }

  Object.merge(e, config);
}

function file2fqn(file) {
  var fqn = file.strip_ext();
  if (fqn.folder_same_name())
    fqn = fqn.up_path();

  fqn = fqn.replace('/', '.');
  var topur;
  if (topur = Object.access(ur, fqn)) return topur;

  var fqnlast = fqn.split('.').last();

  if (topur = Object.access(ur, fqn.tolast('.'))) {
    topur[fqnlast] = {
      name: fqn
    };
    ur._list.push(fqn);
    return Object.access(ur, fqn);
  }

  fqn = fqnlast;

  ur[fqn] = {
    name: fqn
  };
  ur._list.push(fqn);
  return ur[fqn];
}

Game.loadurs = function() {
  ur = {};
  ur._list = [];
  /* FIND ALL URS IN A PROJECT */
  for (var file of io.glob("**.jso")) {
    if (file[0] === '.' || file[0] === '_') continue;
    var topur = file2fqn(file);
    topur.text = file;
  }

  for (var file of io.glob("**.json")) {
    if (file[0] === '.' || file[0] === '_') continue;
    var topur = file2fqn(file);
    topur.data = file;
  }

  ur.empty = {
    name: "empty"
  };
};

return {
  gameobject
}