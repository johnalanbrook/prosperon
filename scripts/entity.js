globalThis.entityreport = {};

var timer_update = function (dt) {
  this.fn();
};

function obj_unique_name(name, obj) {
  name = name.replaceAll(".", "_");
  if (!(name in obj)) return name;
  var t = 1;
  var n = name + t;
  while (n in obj) {
    t++;
    n = name + t;
  }
  return n;
}

function unique_name(list, name = "new_object") {
  var str = name.replaceAll(".", "_");
  var n = 1;
  var t = str;
  while (list.indexOf(t) !== -1) {
    t = str + n;
    n++;
  }
  return t;
}

var entity = {
  drawlayer: -1,
  get_comp_by_name(name) {
    var comps = [];
    for (var c of Object.values(this.components)) if (c.comp === name) comps.push(c);

    if (comps.length) return comps;
    return undefined;
  },

  rigidify() {
    this.body = os.make_body(this.transform);
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

  drawlayer: 0,

  full_path() {
    return this.path_from(world);
  },

  clear() {
    for (var k in this.objects) {
      this.objects[k].kill();
    }
    this.objects = {};
  },

  sync() {
    for (var c of Object.values(this.components)) c.sync?.();
    for (var o of Object.values(this.objects)) o.sync();
  },

  delay(fn, seconds) {
    var timers = this.timers;
    var stop = function () {
      timers.remove(stop);
      execute = undefined;
      stop = undefined;
      rm?.();
      rm = undefined;
      update = undefined;
    };

    function execute() {
      fn();
      stop?.();
    }

    stop.remain = seconds;
    stop.seconds = seconds;
    stop.pct = function () {
      return 1 - stop.remain / stop.seconds;
    };

    function update(dt) {
      profile.frame("timer");
      if (stop) {
        // TODO: This seems broken
        stop.remain -= dt;
        if (stop.remain <= 0) execute();
      }
      profile.endframe();
    }

    var rm = Register.update.register(update);
    timers.push(stop);
    return stop;
  },

  cry(file) {
    return audio.cry(file);
  },

  get pos() {
    return this.transform.pos;
  },
  set pos(x) {
    this.transform.pos = x;
  },
  get angle() {
    return this.transform.angle;
  },
  set angle(x) {
    this.transform.angle = x;
  },
  get scale() {
    return this.transform.scale;
  },
  set scale(x) {
    this.transform.scale = x;
  },

  move(vec) {
    this.pos = this.pos.add(vec);
  },
  rotate(x) {
    this.transform.rotate(x, [0, 0, -1]);
  },
  grow(vec) {
    if (typeof vec === "number") vec = [vec, vec];
    this.scale = this.scale.map((x, i) => x * vec[i]);
  },

  /* Reparent 'this' to be 'parent's child */
  reparent(parent) {
    assert(parent, `Tried to reparent ${this.toString()} to nothing.`);
    if (this.master === parent) {
      console.warn(`not reparenting ... ${this.master} is the same as ${parent}`);
      return;
    }

    var name = unique_name(Object.keys(parent), this.name);
    this.name = name;

    this.master?.remove_obj(this);
    this.master = parent;
    parent.objects[this.guid] = this;
    parent[name] = this;
    Object.hide(parent, name);
  },

  remove_obj(obj) {
    if (this.objects) delete this.objects[obj.guid];
    else console.warn(`Object ${this.guid} has no objects file.`);
    delete this[obj.name];
    Object.unhide(this, obj.name);
  },

  spawn(text, config, callback) {
    var ent = class_use(text, config, entity, function (ent) {
      ent.transform = os.make_transform();
      ent.guid = prosperon.guid();
      ent.components = {};
      ent.objects = {};
      ent.timers = [];
      ent.ur = {};
      ent.urname = text;
    });
    /*    
    if (!text)
      ent.ur = emptyur;
    else if (text instanceof Object) {// assume it's an ur
      ent.ur = text;
      text = ent.ur.text;
      config = [ent.ur.data, config].filter(x => x).flat();
    }
    else {
      ent.ur = getur(text, config);
      text = ent.ur.text;
      config = [ent.ur.data, config];
    }

    if (typeof config === 'string')
      Object.merge(ent, json.decode(Resources.replstrs(config)));
    else if (Array.isArray(config))
      for (var path of config) {
        if (typeof path === 'string') {
          console.info(`ingesting ${path} ...`);
          Object.merge(ent, json.decode(Resources.replstrs(path)));
        }
        else if (path instanceof Object)
          Object.merge(ent,path);
      };
    
    if (typeof text === 'string') {
      class_use(
      use(text, ent);
    }
    else if (Array.isArray(text))
      for (var path of text) use(path,ent);
    profile.cache("ENTITY TIME", ent.ur.name);
*/
    ent.reparent(this);

    for (var [prop, p] of Object.entries(ent)) {
      if (!p) continue;
      if (typeof p !== "object") continue;
      if (!p.comp) continue;
      ent[prop] = component[p.comp](ent);
      Object.merge(ent[prop], p);
      ent.components[prop] = ent[prop];
    }

    check_registers(ent);

    if (ent.awake instanceof Function) ent.awake();
    if (sim.playing()) {
      ent._started = true;
      if (ent.start instanceof Function) ent.start();
    }

    Object.hide(ent, "ur", "components", "objects", "timers", "guid", "master", "guid");

    ent._ed = {
      selectable: true,
      dirty: false,
      inst: false,
      urdiff: {},
    };

    Object.hide(ent, "_ed");

    ent.sync();

    if (!Object.empty(ent.objects)) {
      var o = ent.objects;
      delete ent.objects;
      ent.objects = {};
      for (var i in o) {
        console.info(`creating ${i} on ${ent.toString()}`);
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
    for (var i in ent.objects) ent.ur.fresh.objects[i] = ent.objects[i].instance_obj();

    profile.endcache();

    return ent;
  },

  disable() {
    for (var x of this.components) x.disable();
  },
  enable() {
    for (var x of this.components) x.enable();
  },

  this2screen(pos) {
    return game.camera.world2view(this.this2world(pos));
  },
  screen2this(pos) {
    return this.world2this(game.camera.view2world(pos));
  },

  /* Make a unique object the same as its prototype */
  revert() {
    Object.merge(this, this.ur.fresh);
  },

  name: "new_object",
  toString() {
    return this.name;
  },
  width() {
    var bb = this.boundingbox();
    return bb.r - bb.l;
  },

  height() {
    var bb = this.boundingbox();
    return bb.t - bb.b;
  },

  flipx() {
    return this.scale.x < 0;
  },
  flipy() {
    return this.scale.y < 0;
  },

  mirror(plane) {
    this.scale = Vector.reflect(this.scale, plane);
  },

  /* Bounding box of the object in world dimensions */
  boundingbox() {
    var boxes = [];
    boxes.push({
      t: 0,
      r: 0,
      b: 0,
      l: 0,
    });

    for (var key in this.components) {
      if ("boundingbox" in this.components[key]) boxes.push(this.components[key].boundingbox());
    }
    for (var key in this.objects) boxes.push(this.objects[key].boundingbox());

    var bb = boxes.shift();

    for (var x of boxes) bb = bbox.expand(bb, x);

    bb = bbox.move(bb, this.pos);

    return bb ? bb : bbox.fromcwh([0, 0], [0, 0]);
  },

  toJSON() {
    return { guid: this.guid };
  },

  /* The unique components of this object. Its diff. */
  json_obj(depth = 0) {
    var fresh = this.ur.fresh;
    var thiso = json.decode(json.encode(this)); // TODO: SLOW. Used to ignore properties in toJSON of components.
    var d = ediff(thiso, fresh);

    d ??= {};

    fresh.objects ??= {};
    var curobjs = {};
    for (var o in this.objects) curobjs[o] = this.objects[o].instance_obj();

    var odiff = ediff(curobjs, fresh.objects);
    if (odiff) d.objects = curobjs;

    delete d.pos;
    delete d.angle;
    delete d.scale;
    delete d.velocity;
    delete d.angularvelocity;
    return d;
  },

  /* The object needed to store an object as an instance of a master */
  instance_obj() {
    var t = os.make_transform();
    t = this.transform;
    t.ur = this.ur.name;
    return t;
  },

  transform() {
    var t = {};
    t.pos = this.get_pos(this.master).map(x => Math.places(x, 0));
    t.angle = Math.places(this.get_angle(this.master), 4);
    t.scale = this.get_scale(this.master).map(x => Math.places(x, 2));
    return t;
  },

  dup(diff) {
    var n = this.master.spawn(this.ur);
    Object.totalmerge(n, this.transform());
    return n;
  },

  kill() {
    if (this.__kill) return;
    this.__kill = true;

    this.timers.forEach(x => x());
    delete this.timers;
    Event.rm_obj(this);
    input.do_uncontrol(this);

    if (this.master) {
      this.master.remove_obj(this);
      this.master = undefined;
    }

    for (var key in this.components) {
      this.components[key].kill?.();
      this.components[key].gameobject = undefined;
      this.components[key].enabled = false;
      delete this.components[key];
    }

    delete this.components;

    this.clear();
    if (this.stop instanceof Function) this.stop();
    if (typeof this.garbage === "function") this.garbage();
    if (typeof this.then === "function") this.then();

    game.tag_clear_guid(this.guid);

    rmactor(this);

    for (var i in this) {
      if (this[i] instanceof Object || this[i] instanceof Function) delete this[i];
    }
  },

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
    if (this.objects[newname]) return;

    this.objects[newname] = this.objects[name];
    this[newname] = this[name];
    this[newname].toString = function () {
      return newname;
    };
    Object.hide(this, newname);
    delete this.objects[name];
    delete this[name];
    return this.objects[newname];
  },

  add_component(comp, data) {
    var name = prosperon.guid();
    this.components[name] = comp(this);
    if (data) {
      Object.assign(this.components[name], data);
      this.components[name].sync?.();
    }
    return this.components[name];
  },
};

var gameobject = {
  check_dirty() {
    this._ed.urdiff = this.json_obj();
    this._ed.dirty = !Object.empty(this._ed.urdiff);
    return; // TODO: IMPLEMENT
    var lur = this.master.ur;
    if (!lur) return;
    var lur = lur.objects[this.toString()];
    var d = ediff(this._ed.urdiff, lur);
    if (!d || Object.empty(d)) this._ed.inst = true;
    else this._ed.inst = false;
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

  /* pin this object to the to object */
  pin(to) {
    var p = joint.pin(this, to);
  },
  slide(to, a = [0, 0], b = [0, 0], min = 0, max = 50) {
    var p = joint.slide(this, to, a, b, min, max);
    p.max_force = 500;
    p.break();
  },
  pivot(to, piv = this.pos) {
    var p = joint.pivot(this, to, piv);
  },
  /* groove is on to, from local points a and b, anchored to this at local anchor */
  groove(to, a, b, anchor = [0, 0]) {
    var p = joint.groove(to, this, a, b, anchor);
  },
  damped_spring(to, length = Vector.length(this.pos, to.pos), stiffness = 1, damping = 1) {
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

  set_pos(x, relative = world) {
    var newpos = relative.this2world(x);
    var move = newpos.sub(this.pos);
    this.rpos = newpos;
    for (var o of this.objects) o.move(move);
  },

  set_angle(x, relative = world) {
    var newangle = relative.angle + x;
    var diff = newangle - this.angle;
    this.rangle = newangle;
    for (var obj of this.objects) {
      obj.rotate(diff);
      obj.set_pos(Vector.rotate(obj.get_pos(obj.master), diff), obj.master);
    }
  },

  set_scale(x, relative = world) {
    if (typeof x === "number") x = [x, x, x];
    var newscale = relative.scale.map((s, i) => x[i] * s);
    var pct = this.scale.map((s, i) => newscale[i] / s);
    this.rscale = newscale;
    for (var obj of this.objects) {
      obj.grow(pct);
      obj.set_pos(
        obj.get_pos(obj.master).map((x, i) => x * pct[i]),
        obj.master,
      );
    }
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
    return this.scale.map((x, i) => x / masterscale[i]);
  },

  in_air() {
    return this.in_air();
  },

  /* Velocity and angular velocity of the object */
  phys_obj() {
    var phys = {};
    phys.velocity = this.velocity;
    phys.angularvelocity = this.angularvelocity;
    return phys;
  },

  set category(n) {
    if (n === 0) {
      this.categories = n;
      return;
    }
    var cat = 1 << (n - 1);
    this.categories = cat;
  },
  get category() {
    if (this.categories === 0) return 0;
    var pos = 0;
    var num = this.categories;
    while (num > 0) {
      if (num & 1) {
        break;
      }
      pos++;
      num >>>= 1;
    }

    return pos + 1;
  },
};

entity.spawn.doc = `Spawn an entity of type 'ur' on this entity. Returns the spawned entity.`;

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
  move: "Move an object by x,y,z. If the first parameter is an array, uses up to the first three array values.",
  pulse: `Apply an impulse to this body in world coordinates. Impulse is a short force.`,
  shove: `Apply a force to this body in world coordinates. Should be used over many frames.`,
  shove_at: "Apply a force to this body, at a position relative to itself.",
  max_velocity: "The max linear velocity this object can travel.",
  max_angularvelocity: "The max angular velocity this object can rotate.",
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
  delay: "Run the given function after the given number of seconds has elapsed.",
  cry: "Make a sound. Can only make one at a time.",
  add_component: "Add a component to the object by name.",
  pin: "Pin joint to another object. Acts as if a rigid rod is between the two objects.",
  slide: "Slide joint, similar to a pin but with min and max allowed distances.",
  pivot: "Pivot joint to an object, with the pivot given in world coordinates.",
  groove: "Groove joint. The groove is on to, from to local coordinates a and b, with this object anchored at anchor.",
  damped_spring: "Damped spring to another object. Length is the distance it wants to be, stiffness is the spring constant, and damping is the damping ratio. 1 is critical, < 1 is underdamped, > 1 is overdamped.",
  damped_rotary_spring: "Similar to damped spring but for rotation. Rest angle is the attempted angle.",
  rotary_limit: "Limit the angle relative to the to body between min and max.",
  ratchet: "Like a socket wrench, relative to to. ratch is the distance between clicks.",
  gear: "Keeps the angular velocity ratio of this body and to constant. Ratio is the gear ratio.",
  motor: "Keeps the relative angular velocity of this body to to at a constant rate. The most simple idea is for one of the bodies to be static, to the other is kept at rate.",
  layer: "Bitmask for collision layers.",
  drawlayer: "Layer for drawing. Higher numbers draw above lower ones.",
  warp_filter: "Bitmask for selecting what warps should affect this entity.",
};

global.ur = {};

if (io.exists(`${io.dumpfolder}/ur.json`)) ur = json.decode(io.slurp(`${io.dumpfolder}/ur.json`));
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
function apply_ur(u, ent) {
  if (typeof u !== "string") {
    console.warn("Must give u as a string.");
    return;
  }

  var urs = u.split(".");
  if (!urs.every(u => ur[u])) {
    console.error(`Attempted to make ur combo ${u} but not every ur in the chain exists.`);
    return;
  }

  for (var u of urs) {
    var text = u.text;
    var data = u.data;
    if (typeof text === "string") use(text, ent);
    else if (Array.isArray(text)) for (var path of text) use(path, ent);

    if (typeof data === "string") Object.merge(ent, json.decode(Resources.replstrs(data)));
    else if (Array.isArray(data)) {
      for (var path of data) {
        if (typeof path === "string") Object.merge(ent, json.decode(Resources.replstrs(data)));
        else if (path instanceof Object) Object.merge(ent, path);
      }
    }
  }
}

var emptyur = {
  name: "empty",
};

var getur = function (text, data) {
  if (!text && !data) {
    console.info("empty ur");
    return {
      name: "empty",
    };
  }
  var urstr = text;
  if (data) urstr += "+" + data;

  if (!ur[urstr]) {
    ur[urstr] = {
      name: urstr,
      text: text,
      data: data,
    };
  }
  return ur[urstr];
};

var ur_from_file = function (file) {
  var urname = file.name();
  if (ur[urname]) {
    console.warn(`Tried to make another ur with the name ${urname} from ${file}, but it already exists.`);
    return undefined;
  }
  var newur = {
    name: urname,
  };
  ur[urname] = newur;
  ur._list.push(urname);
  return newur;
};

game.loadurs = function () {
  return;
  ur = {};
  ur._list = [];
  /* FIND ALL URS IN A PROJECT */
  for (var file of io.glob("**.ur")) {
    var newur = ur_from_file(file);
    if (!newur) continue;
    var uur = Resources.replstrs(file);
    var urjson = json.decode(uur);
    Object.assign(newur, urjson);
  }

  for (var file of io.glob("**.jso").filter(f => !ur[f.name()])) {
    if (file[0] === "." || file[0] === "_") continue;
    var newur = ur_from_file(file);
    if (!newur) continue;
    newur.text = file;

    var data = file.set_ext(".json");
    if (io.exists(data)) {
      console.info(`Found matching json ${data} for ${file}`);
      newur.data = data;
    }
  }
};

game.ur = {};
game.ur.load = function (str) {};
game.ur.add_data = function (str, data) {
  var nur = ur[str];
  if (!nur) {
    console.warn(`Cannot add data to the ur ${str}.`);
    return;
  }
  if (!Array.isArray(ur.data)) {
    var arr = [];
    if (ur.data) arr.push(ur.data);
    ur.data = arr;
  }

  ur.data.push(data);
};

game.ur.save = function (str) {
  var nur = ur[str];
  if (!nur) {
    console.warn(`Cannot save ur ${str}.`);
    return;
  }
};

return { entity };
