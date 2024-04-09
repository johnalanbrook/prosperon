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
    var lur = this.master.ur;
    if (!lur) return;
    var lur = lur.objects[this.toString()];
    var d = ediff(this._ed.urdiff, lur);
    if (!d || Object.empty(d))
      this._ed.inst = true;
    else
      this._ed.inst = false;
  },

  namestr() {
    var s = this.toString();
    if (this._ed?.dirty)
      if (this._ed.inst) s += "#";
      else s += "*";
    return s;
  },

  urstr() {
    var str = this.ur.name;
    if (this._ed.dirty) str = "*" + str;
    return str;
  },

  full_path() {
    return this.path_from(world);
  },
  /* pin this object to the to object */
  pin(to) {
    var p = joint.pin(this,to);
  },
  slide(to, a = [0,0], b = [0,0], min = 0, max = 50) {
    var p = joint.slide(this, to, a, b, min, max);
    p.max_force = 500;
    p.break();
  },
  pivot(to, piv = this.pos) {
    var p = joint.pivot(this, to, piv);
  },
  /* groove is on to, from local points a and b, anchored to this at local anchor */
  groove(to, a, b, anchor = [0,0]) {
    var p = joint.groove(to, this, a, b, anchor);
  },
  damped_spring(to, length = Vector.length(this.pos,to.pos), stiffness = 1, damping = 1) {
    var dc = 2 * Math.sqrt(stiffness * this.mass);
    var p = joint.damped_spring(this, to, [0, 0], [0, 0], stiffness, damping * dc);
  },
  damped_rotary_spring(to, angle = 0, stiffness = 1, damping = 1) {
    /* calculate actual damping value from the damping ratio */
    /* damping = 1 is critical */
    var dc = 2 * Math.sqrt(stiffness * this.get_moi()); /* critical damping number */
    /* zeta = actual/critical */
    var p = joint.damped_rotary(this, to, angle, stiffness, damping * dc);
  },
  rotary_limit(to, min, max) {
    var p = joint.rotary(this, to, Math.turn2rad(min), Math.turn2rad(max));
  },
  ratchet(to, ratch) {
    var phase = this.angle - to.angle;
    var p = joint.ratchet(this, to, phase, Math.turn2rad(ratch));
  },
  gear(to, ratio = 1, phase = 0) {
    var phase = this.angle - to.angle;
    var p = joint.gear(this, to, phase, ratio);
  },
  motor(to, rate) {
    var p = joint.motor(this, to, rate);
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
    var timers = this.timers;
    var stop = function() { 
      timers.remove(stop);
      execute = undefined;
      stop = undefined;
      rm?.();
      rm = undefined;
      update = undefined;
    }
    
    function execute() {
      fn();
      stop?.();
    }
    
    stop.remain = seconds;
    stop.seconds = seconds;
    
    function update(dt) {
      stop.remain -= dt;
      if (stop.remain <= 0) execute();
    }
    
    var rm = Register.update.register(update);
    timers.push(stop);
    return stop;
  },

  cry(file) {
    return audio.cry(file);
  },
  
  set pos(x) { this.set_pos(x); },
  get pos() { return this.rpos; },
  set angle(x) { this.set_angle(x); },
  get angle() { return this.rangle; },
  set scale(x) { this.set_scale(x); },
  get scale() { return this.rscale; },

  set_pos(x, relative = world) {
    var newpos = relative.this2world(x);
    var move = newpos.sub(this.pos);
    this.rpos = newpos;
    this.objects.forEach(x => x.move(move));
  },
  
  set_angle(x, relative = world) {
    var newangle = relative.angle + x;
    var diff = newangle - this.angle;
    this.rangle = newangle;
    this.objects.forEach(obj => {
      obj.rotate(diff);
      obj.set_pos(Vector.rotate(obj.get_pos(obj.master), diff), obj.master);
    });
  },
  
  set_scale(x, relative = world) {
    if (typeof x === 'number') x = [x,x,x];
    var newscale = relative.scale.map((s,i) => x[i]*s);
    var pct = this.scale.map((s,i) => newscale[i]/s);
    this.rscale = newscale;
    this.objects.forEach(obj => {
      obj.grow(pct);
      obj.set_pos(obj.get_pos(obj.master).map((x,i) => x*pct[i]), obj.master);
    });
  },
  
  get_pos(relative = world) {
    if (relative === world) return this.pos;
    return relative.world2this(this.pos);
    //return this.pos.sub(relative.pos);
  },
  
  get_angle(relative = world) {
    if (relative === world) return this.angle;
    return this.angle - relative.angle;
  },
  
  get_scale(relative = world) {
    if (relative === world) return this.scale;
    var masterscale = relative.scale;
    return this.scale.map((x,i) => x/masterscale[i]);
  },
  
  /* Moving, rotating, scaling functions, world relative */
  move(vec) { this.set_pos(this.pos.add(vec)); },
  rotate(x) { this.set_angle(this.angle + x); },
  grow(vec) { 
    if (typeof vec === 'number') vec = [vec,vec,vec];
    this.set_scale(this.scale.map((x, i) => x * vec[i]));
  },
  
  screenpos() { return game.camera.world2view(this.pos); },

  get_ur() { return this.ur; },

  /* spawn an entity
      text can be:
      the file path of a script
      an ur object
      nothing
  */
  spawn(text, config, callback) {
    var ent = os.make_gameobject();
    ent.guid = prosperon.guid();
    ent.components = {};
    ent.objects = {};
    ent.timers = [];
    
    if (typeof text === 'object' && text) // assume it's an ur
      ent.ur = text;
    else 
      ent.ur = getur(text, config);

    text = ent.ur.text;
    config = [ent.ur.data, config];

    if (typeof text === 'string')
      use(text, ent);
    else if (Array.isArray(text))
      text.forEach(path => use(path,ent));

    if (typeof config === 'string')
      Object.merge(ent, json.decode(Resources.replstrs(config)));
    else if (Array.isArray(config))
      config.forEach(function(path) {
        if (typeof path === 'string')
          Object.merge(ent, json.decode(Resources.replstrs(path)));
        else if (typeof path === 'object')
          Object.merge(ent,path);
      });

    ent.reparent(this);

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

    if (typeof ent.load === 'function') ent.load();
    if (sim.playing())
      if (typeof ent.start === 'function') ent.start();

    Object.hide(ent, 'ur', 'components', 'objects', 'timers', 'guid', 'master');
    
    ent._ed = {
      selectable: true,
      dirty: false,
      inst: false,
      urdiff: {}
    };

    Object.hide(ent, '_ed');

    ent.sync();
    
    if (!Object.empty(ent.objects)) {
      var o = ent.objects;
      delete ent.objects;
      for (var i in o) {
        var newur = o[i].ur;
        delete o[i].ur;
        var n = ent.spawn(ur[newur], o[i]);
        ent.rename_obj(n.toString(), i);
      }
    }
    
    if (ent.tag) game.tag_add(ent.tag, ent);
    
    if (callback) callback(ent);

    ent.ur.fresh ??= json.decode(json.encode(ent));
    ent.ur.fresh.objects = {};
    for (var i in ent.objects)
      ent.ur.fresh.objects[i] = ent.objects[i].instance_obj();

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

    function unique_name(list, name = "new_object") {
      var str = name.replaceAll('.', '_');
      var n = 1;
      var t = str;
      while (list.indexOf(t) !== -1) {
        t = str + n;
        n++;
      }
      return t;
    };

    var name = unique_name(Object.keys(parent.objects), this.ur.name);

    parent.objects[name] = this;
    parent[name] = this;
    Object.hide(parent, name);
    this.toString = function() { return name; };
  },

  remove_obj(obj) {
    delete this.objects[obj.toString()];
    delete this[obj.toString()];
    Object.unhide(this, obj.toString());
  },

  components: {},
  objects: {},
  master: undefined,

  this2screen(pos) { return game.camera.world2view(this.this2world(pos)); },
  screen2this(pos) { return this.world2this(game.camera.view2world(pos)); },
  
  in_air() { return this.in_air(); },

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

  mirror(plane) { this.scale = Vector.reflect(this.scale, plane); },

  disable() { this.components.forEach(function(x) { x.disable(); }); },
  enable() { this.components.forEach(function(x) { x.enable(); }); },
  
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
    var fresh = this.ur.fresh;
    var thiso = json.decode(json.encode(this)); // TODO: SLOW. Used to ignore properties in toJSON of components.
    var d = ediff(thiso, fresh);

    d ??= {};

    fresh.objects ??= {};
    var curobjs = {};
    for (var o in this.objects)
      curobjs[o] = this.objects[o].instance_obj();

    var odiff = ediff(curobjs, fresh.objects);
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
    t.ur = this.ur.name;
    return t;
  },

  transform() {
    var t = {};
    t.pos = this.get_pos(this.master).map(x => Math.places(x, 0));
    t.angle = Math.places(this.get_angle(this.master), 4);
    t.scale = this.get_scale(this.master).map(x => Math.places(x, 2));;
    return t;
  },

  /* Velocity and angular velocity of the object */
  phys_obj() {
    var phys = {};
    phys.velocity = this.velocity;
    phys.angularvelocity = this.angularvelocity;
    return phys;
  },

  phys_mat() { 
   return {
      friction: this.friction,
      elasticity: this.elasticity
    }
  },

  dup(diff) {
    var n = this.master.spawn(this.ur);
    Object.totalmerge(n, this.transform());
    return n;
  },

  kill() {
    if (this.__kill) return;
    this.__kill = true;
    console.spam(`Killing entity of type ${this.ur}`);

    this.timers.forEach(t => t());
    this.timers = [];
    Event.rm_obj(this);
    input.do_uncontrol(this);

    if (this.master) {
      this.master.remove_obj(this);
      this.master = undefined;
    }

    if (this.__proto__.instances)
      delete this.__proto__.instances[this.toString()];

    for (var key in this.components) {
      this.components[key].kill?.();
      this.components[key].gameobject = undefined;
      this[key].enabled = false;
      delete this.components[key];
      delete this[key];
    }
    delete this.components;

    this.clear();
    if (typeof this.stop === 'function') this.stop();
    
    game.tag_clear_guid(this.guid);
    
    for (var i in this) {
      if (typeof this[i] === 'object') delete this[i];
      if (typeof this[i] === 'function') delete this[i];
    }
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

  add_component(comp, data, name = comp.toString()) {
    if (typeof comp.make !== 'function') return;
    name = obj_unique_name(name, this);
    this[name] = comp.make(this);
    this[name].comp = comp.toString();
    this.components[name] = this[name];
    if (data)
      Object.assign(this[name], data);
    return this[name];
  },
}

function go_init() {
  var gop = os.make_gameobject().__proto__;
  Object.mixin(gop, gameobject);
  gop.sync = function() {
    this.selfsync();
    this.components.forEach(function(x) { x.sync?.(); });
    this.objects.forEach(function(x) { x.sync?.(); });
  }
}

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
  elasticity: `When two objects collide, their elasticities are multiplied together. Their velocities are then multiplied by this value to find √their resultant velocities.`,
  friction: `When one object touches another, friction slows them down.`,
  mass: `The higher the mass of the object, the less forces will affect it.`,
  phys: `Set to 0, 1, or 2, representing dynamic, kinematic, and static.`,
  worldpos: `Function returns the world position of the object.`,
  set_pos: `Function to set the position of the object in world coordinates.`,
  worldangle: `Function to get the angle of the entity in the world.`,
  rotate: `Function to rotate this object by x degrees.`,
  move: 'Move an object by x,y,z. If the first parameter is an array, uses up to the first three array values.',
  pulse: `Apply an impulse to this body in world coordinates. Impulse is a short force.`,
  shove: `Apply a force to this body in world coordinates. Should be used over many frames.`,
  shove_at: 'Apply a force to this body, at a position relative to itself.',
  max_velocity: 'The max linear velocity this object can travel.',
  max_angularvelocity: 'The max angular velocity this object can rotate.',
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
  drawlayer: 'Layer for drawing. Higher numbers draw above lower ones.',
  warp_filter: 'Bitmask for selecting what warps should affect this entity.',
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
      use(topur.text, e, script);
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

var getur = function(text, data)
{
  var urstr = text + "+" + data;
  if (!ur[urstr]) {
    ur[urstr] = {
      name: urstr,
      text: text,
      data: data
    }
  }
  return ur[urstr];
}

game.loadurs = function() {
  ur = {};
  ur._list = [];
  /* FIND ALL URS IN A PROJECT */
  for (var file of io.glob("**.ur")) {
    var urname = file.name();
    if (ur[urname]) {
      console.warn(`Tried to make another ur with the name ${urname} from ${file}, but it already exists.`);
      continue;
    }
    var urjson = json.decode(io.slurp(file));
    urjson.name = urname;
    ur[urname] = urjson;
  }
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
};

return { go_init }