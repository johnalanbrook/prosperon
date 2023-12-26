function obj_unique_name(name, obj)
{
  name = name.replaceAll('.', '_');
  if (!(name in obj)) return name;
  var t = 1;
  var n = name + t;
  while (n in obj) {
    t++;
    n = name+t;
  }
  return n;
}

var actor = {};
actor.spawn = function(script, config){
  if (typeof script !== 'string') return;
  var padawan = Object.create(actor);
  compile_env(script, padawan, "script");

  if (typeof config === 'object')
    Object.merge(padawan, config);

  padawan.padawans = [];
  padawan.timers = [];
  padawan.master = this;
  Object.hide(padawan, "master","timers");
  this.padawans.push(padawan);
  return padawan;
};
actor.die = function(actor){
  
};
actor.timers = [];
actor.kill = function(){
  this.timers.forEach(t => t.kill());
  this.master.remove(this);
  this.padawans.forEach(p => p.kill());
  this.__dead__ = true;
};

actor.delay = function(fn, seconds) {
  var t = Object.create(timer);
  t.remain = seconds;
  t.kill = () => {
    timer.kill.call(t);
    this.timers.remove(t);
  }
  t.fire = () => {
    if (this.__dead__) return;
    fn();
    t.kill();
  };
  Register.appupdate.register(t.update, t);
  this.timers.push(t);
  return function() { t.kill(); };
};
actor.clock = function(fn){};
actor.master = undefined;

actor.padawans = [];

actor.remaster = function(to){
  this.master.padawans.remove(this);
  this.master = to;
  to.padawans.push(this);
};

var gameobject_impl = {
  get pos() {
    Debug.assert(this.level, `Entity ${this.toString()} has no level.`);
    return this.level.world2this(this.worldpos());
  },

  set pos(x) {
    Debug.assert(this.level, `Entity ${this.toString()} has no level.`);
    this.set_worldpos(this.level.this2world(x));
  },

  get angle() {
    Debug.assert(this.level, `No level set on ${this.toString()}`);
    return this.worldangle() - this.level.worldangle();
  },
  set angle(x) {
    var diff = x - this.angle;

    this.objects.forEach(function(x) {
      x.rotate(diff);
      x.pos = Vector.rotate(x.pos, diff);
    });

    this.sworldangle(x-this.level.worldangle());
  },

  get scale() {
    Debug.assert(this.level, `No level set on ${this.toString()}`);
    var pscale;
    if (typeof this.__proto__.scale === 'object')
      pscale = this.__proto__.scale;
    else
      pscale = [1,1,1];

    return this.gscale().map((x,i) => x/(this.level.gscale()[i]*pscale[i]));
  },

  set scale(x) {
    if (typeof x === 'number')
      x = [x,x];

    var pct = this.scale.map((s,i) => x[i]/s);
    this.spread(pct);
    
    /* TRANSLATE ALL SUB OBJECTS */
    this.objects.forEach(obj => {
      obj.spread(pct);
      obj.pos = obj.pos.map((x,i)=>x*pct[i]);
    });
  },

  get draw_layer() { return cmd(171, this.body); },
  set draw_layer(x) { cmd(172, this.body, x); },
  set layer(x) { cmd(75,this.body,x); },
  get layer() { cmd(77,this.body); },

  set mass(x) { set_body(7,this.body,x); },
  get mass() {
    if (!(this.phys === Physics.dynamic))
      return this.__proto__.mass;

    return q_body(5, this.body);
  },
  get elasticity() { return cmd(107,this.body); },
  set elasticity(x) { cmd(106,this.body,x); },
  get friction() { return cmd(109,this.body); },
  set friction(x) { cmd(108,this.body,x); },
  set gravity(x) { cmd(167,this.body, x); },
  get gravity() { return cmd(159,this.body); },
  set timescale(x) { cmd(168,this.body,x); },
  get timescale() { return cmd(169,this.body); },
  set phys(x) { set_body(1, this.body, x); },
  get phys() { return q_body(0,this.body); },
  get velocity() { return q_body(3, this.body); },
  set velocity(x) { set_body(9, this.body, x); },
  get damping() { return cmd(157,this.body); },
  set damping(x) { cmd(156, this.body, x); },
  get angularvelocity() { return Math.rad2turn(q_body(4,this.body)); },
  set angularvelocity(x) { set_body(8, this.body, Math.turn2rad(x)); },
  get max_velocity() { return cmd(152, this.body); },
  set max_velocity(x) { cmd(151, this.body, x); },
  get max_angularvelocity() { return cmd(155,this.body); },
  set max_angularvelocity(x) { cmd(154,this.body,x); },
  get_moi() { return q_body(6, this.body); },
  set_moi(x) {
    if(x <= 0) {
      Log.error("Cannot set moment of inertia to 0 or less.");
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
    this._ed.dirty = !this._ed.urdiff.empty;
    var lur = ur[this.level.ur];
    if (!lur) return;
    var lur = lur.objects[this.toString()];
    var d = ediff(this._ed.urdiff,lur);
    if (!d || d.empty)
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
  full_path() {
    return this.path_from(Primum);
  },

    path_from(o) {
      var p = this.toString();
      var c = this.level;
      while (c && c !== o && c !== Primum) {
        p = c.toString() + "." + p;
	c = c.level;
      }
      if (c === Primum) p = "Primum." + p;
      return p;
    },
  
    clear() {
      for (var k in this.objects) {
	this.objects[k].kill();
      };
      this.objects = {};
    },
    
    delay(fn, seconds) {
      var t = timer.delay(fn.bind(this), seconds);
      this.timers.push(t);
      return t;
    },

    tween(prop, values, def){
      var t = Tween.make(this, prop, values, def);
      t.play();
      
      var k = function() { t.pause(); }
      this.timers.push(k);
      return k;
    },

    cry(file) {
      var p = Sound.play(file, Sound.bus.sfx);
      var killfn = p.kill.bind(p);
      p.end = killfn;
      this.timers.push(killfn);
      return killfn;
    },

      set torque(x) { if (!(x >= 0 && x <= Infinity)) return; cmd(153, this.body, x); },
      gscale() { return cmd(103,this.body); },
      sgscale(x) {
        if (typeof x === 'number')
	  x = [x,x];
        cmd(36,this.body,x)
      },
      
      phys_material() {
        var mat = {};
	mat.elasticity = this.elasticity;
	mat.friction = this.friction;
	return mat;
      },

      worldpos() { return q_body(1,this.body); },
      set_worldpos(x) {
        var poses = this.objects.map(x => x.pos);
	set_body(2,this.body,x);
	this.objects.forEach((o,i) => o.set_worldpos(this.this2world(poses[i])));
      },
      screenpos() { return world2screen(this.worldpos()); },

      worldangle() { return Math.rad2turn(q_body(2,this.body)); },
      sworldangle(x) { set_body(0,this.body,Math.turn2rad(x)); },

      spawn_from_instance(inst) {
        return this.spawn(inst.ur, inst);
      },
      
      spawn(ur, data) {
	if (typeof ur === 'string')
	  ur = prototypes.get_ur(ur);
        if (!ur) {
	  Log.error("Failed to make UR from " + ur);
	  return undefined;
	}
	
	var go = ur.make(this, data);
	Object.hide(this, go.toString());
        return go;
      },

  /* Reparent 'this' to be 'parent's child */
  reparent(parent) {
    Debug.assert(parent, `Tried to reparent ${this.toString()} to nothing.`);
    if (this.level === parent) {
      console.warn("not reparenting ...");
      console.warn(`${this.level} is the same as ${parent}`);
      return;
    }

    this.level?.remove_obj(this);
    
    this.level = parent;
      
    function unique_name(list, obj) {
      var str = obj.toString().replaceAll('.', '_');
      var n = 1;
      var t = str;
      while (t in list) {
        t = str + n;
	n++;
      }
      return t;
    };
    
    var name = unique_name(parent, this.ur);

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
  level: undefined,
  
    pulse(vec) { set_body(4, this.body, vec);},
    shove(vec) { set_body(12,this.body,vec);},
    shove_at(vec, at) { set_body(14,this.body,vec,at); },
    world2this(pos) { return cmd(70, this.body, pos); },
    this2world(pos) { return cmd(71, this.body, pos); },
    this2screen(pos) { return world2screen(this.this2world(pos)); },
    screen2this(pos) { return this.world2this(screen2world(pos)); },
    dir_world2this(dir) { return cmd(160, this.body, dir); },
    dir_this2world(dir) { return cmd(161, this.body, dir); },
      
    alive() { return this.body >= 0; },
    in_air() { return q_body(7, this.body);},

  hide() { this.components.forEach(x=>x.hide()); this.objects.forEach(x=>x.hide());},
  show() { this.components.forEach(function(x) { x.show(); }); this.objects.forEach(function(x) { x.show(); }); },

  width() {
    var bb = this.boundingbox();
    return bb.r - bb.l;
  },

  height() {
    var bb = this.boundingbox();
    return bb.t-bb.b;
  },

  /* Moving, rotating, scaling functions, world relative */
  move(vec) { this.set_worldpos(this.worldpos().add(vec)); },
  rotate(x) { this.sworldangle(this.worldangle()+x); },
  spread(vec) { this.sgscale(this.gscale().map((x,i)=>x*vec[i])); },

  /* Make a unique object the same as its prototype */
  revert() {
    var jobj = this.json_obj();
    var lobj = this.level.__proto__.objects[this.toString()];
    delete jobj.objects;
    Object.keys(jobj).forEach(function(x) {
      if (lobj && x in lobj)
        this[x] = lobj[x];
      else
        this[x] = this.__proto__[x];
    }, this);
    this.sync();
  },

  check_registers(obj) {
    Register.unregister_obj(obj);
  
    if (typeof obj.update === 'function')
      Register.update.register(obj.update, obj);

    if (typeof obj.physupdate === 'function')
      Register.physupdate.register(obj.physupdate, obj);

    if (typeof obj.collide === 'function')
      obj.register_hit(obj.collide, obj);

    if (typeof obj.separate === 'function')
      obj.register_separate(obj.separate, obj);

    if (typeof obj.draw === 'function')
      Register.draw.register(obj.draw,obj);

    if (typeof obj.debug === 'function')
      Register.debug.register(obj.debug, obj);

    if (typeof obj.gui === 'function')
      Register.gui.register(obj.gui,obj);

    for (var k in obj) {
      if (!k.startswith("on_")) continue;
      var signal = k.fromfirst("on_");
      Event.observe(signal, obj, obj[k]);
    };

    obj.components.forEach(function(x) {
      if (typeof x.collide === 'function')
        register_collide(1, x.collide, x, obj.body, x.shape);
    });
  },
  
    flipx() { return this.scale.x < 0; },
    flipy() { return this.scale.y < 0; },
    mirror(plane) {
      this.scale = Vector.reflect(this.scale, plane);
    },
    save:true,
    selectable:true,
    ed_locked:false,
    
    disable() { this.components.forEach(function(x) { x.disable(); });},
    enable() { this.components.forEach(function(x) { x.enable(); });},
    sync() {
     this.components.forEach(function(x) { x.sync(); });
     this.objects.forEach(function(x) { x.sync(); });
    },

      /* Bounding box of the object in world dimensions */
      boundingbox() {
	var boxes = [];
	boxes.push({
	  t:0,
	  r:0,
	  b:0,
	  l:0
	});

	for (var key in this.components) {
	  if ('boundingbox' in this.components[key])
	    boxes.push(this.components[key].boundingbox());
	}
	for (var key in this.objects)
	  boxes.push(this.objects[key].boundingbox());

	var bb = boxes.shift();

	boxes.forEach(function(x) { bb = bb_expand(bb, x); });
	
	bb = movebb(bb, this.pos);

	return bb ? bb : cwh2bb([0,0], [0,0]);
      },

      /* The unique components of this object. Its diff. */
      json_obj() {
        var d = ediff(this,this.__proto__);
	
	d ??= {};
 
	var objects = {};
	this.__proto__.objects ??= {};
	var curobjs = {};
	for (var o in this.objects)
	  curobjs[o] = this.objects[o].instance_obj();

        var odiff = ediff(curobjs, this.__proto__.objects);
	if (odiff)
	  d.objects = curobjs;

	delete d.pos;
	delete d.angle;
	delete d.scale;
	delete d.velocity;
	delete d.angularvelocity;
        return d;
      },

      /* The object needed to store an object as an instance of a level */
      instance_obj() {
        var t = this.transform();
	var j = this.json_obj();
	Object.assign(t,j);
	t.ur = this.ur;
	return t;
      },

      transform() {
        var t = {};
	t.pos = this.pos;
	t.angle = this.angle;
	t.scale = this.scale;
	t.scale = t.scale.map((x,i) => x/this.__proto__.scale[i]);
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
        var n = this.level.spawn(this.__proto__);
	Object.totalmerge(n, this.instance_obj());
	return n;
      },

  kill() {
    this.timers.forEach(t => t());
    this.timers = undefined;
    
    if (this.level) {
      this.level.remove_obj(this);
      this.level = undefined;
    }

    Player.do_uncontrol(this);
    Register.unregister_obj(this);

    this.__proto__.instances.remove(this);

    for (var key in this.components) {
      Register.unregister_obj(this.components[key]);
      this.components[key].kill();
      this.components[key].gameobject = undefined;
      delete this.components[key];
    }

    this.clear();
    this.objects = undefined;
    Event.rm_obj(this);

    if (typeof this.stop === 'function')
      this.stop();
  },

  up() { return [0,1].rotate(this.angle);},
  down() { return [0,-1].rotate(this.angle);},
  right() { return [1,0].rotate(this.angle);},
  left() { return [-1,0].rotate(this.angle); },

  make(level, data) {
    var obj = Object.create(this);
    
    obj.make = undefined;
    Object.mixin(obj,gameobject_impl);

    if (this.instances)
      this.instances.push(obj);
      
    obj.body = make_gameobject();

    obj.components = {};
    obj.objects = {};
    obj.timers = [];

    obj._ed = {
      selectable: true,
      dirty: false,
      inst: false,
      urdiff: {},
    };

    obj.ur = this.toString();
    obj.level = undefined;

    obj.reparent(level);

    cmd(113, obj.body, obj); // set the internal obj reference to this obj
    
    for (var [prop,p] of Object.entries(this)) {
      if (!p) continue;
      if (component.isComponent(p)) {
        obj[prop] = p.make(obj);
        obj.components[prop] = obj[prop];
      }
    };

    Object.hide(obj, 'ur','body', 'components', 'objects', '_ed', 'level', 'timers');        

    if (this.objects)
      obj.make_objs(this.objects);

    Object.dainty_assign(obj, this);
    obj.sync();
    gameobject.check_registers(obj);

    if (data)
      Object.dainty_assign(obj,data);

    if (typeof obj.load === 'function') obj.load();
    if (Game.playing() && typeof obj.start === 'function') obj.start();

    return obj;
  },

  make_objs(objs) {
    for (var prop in objs) {
      var newobj = this.spawn_from_instance(objs[prop]);
      if (!newobj) continue;
      this.rename_obj(newobj.toString(), prop);
    }
  },

  rename_obj(name, newname) {
    if (!this.objects[name]) {
      Log.warn(`No object with name ${name}. Could not rename to ${newname}.`);
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

  add_component(comp, data) {
    data ??= undefined;
    if (typeof comp.make !== 'function') return;
    var name = obj_unique_name(comp.toString(), this);
    this[name] = comp.make(this.body);
    Object.assign(this[name], data);
    return this[name];
  },

  register_hit(fn, obj) {
    obj ??= this;
    Signal.obj_begin(fn, obj, this);
  },

  register_separate(fn, obj) {
    obj ??= this;
    Signal.obj_separate(fn,obj,this);
  },
}
Object.mixin(gameobject,gameobject_impl);

gameobject.body = make_gameobject();
cmd(113,gameobject.body, gameobject);
Object.hide(gameobject, 'timescale');

gameobject.spawn.doc = `Spawn an entity of type 'ur' on this entity. Returns the spawned entity.`;

gameobject.doc = {
  doc: "All objects in the game created through spawning have these attributes.",
  pos: "Position of the object, relative to its level.",
  angle: "Rotation of this object, relative to its level.",
  velocity: "Velocity of the object, relative to world.",
  angularvelocity: "Angular velocity of the object, relative to the world.",
  scale: "Scale of the object, relative to its level.",
  flipx: "Check if the object is flipped on its x axis.",
  flipy: "Check if the object is flipped on its y axis.",
  elasticity: `When two objects collide, their elasticities are multiplied together. Their velocities are then multiplied by this value to find their resultant velocities.`,
  friction: `When one object touches another, friction slows them down.`,
  gravity: 'True if this object should be affected by gravity.',
  mass: `The higher the mass of the object, the less forces will affect it.`,
  phys: `Set to 0, 1, or 2, representing static, kinematic, and dynamic.`,
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
  level: "The entity this entity belongs to.",
  delay: 'Run the given function after the given number of seconds has elapsed.',
  cry: 'Make a sound. Can only make one at a time.',
  add_component: 'Add a component to the object by name.',
};

/* Default objects */
var prototypes = {};
prototypes.ur_ext = ".jso";
prototypes.ur = {};
prototypes.save_gameobjects = function() { slurpwrite(JSON.stringify(gameobjects,null,2), "proto.json"); };

/* Makes a new ur-type from disk. If the ur doesn't exist, it searches on the disk to create it. */
prototypes.from_file = function(file)
{
  var urpath = file;
  var path = urpath.split('.');
  if (path.length > 1 && (path.at(-1) === path.at(-2))) {
    urpath = path.slice(0,-1).join('.');
    return prototypes.get_ur(urpath);
  }
    
  var upperur = gameobject;
  
  if (path.length > 1) {
    var upur = undefined;
    var upperpath = path.slice(0,-1);
    while (!upur && upperpath) {
      upur = prototypes.get_ur(upperpath.join('/'));
      upperpath = upperpath.slice(0,-1);
    }
    if (upur) upperur = upur;
  }

  var newur = {};
  
  file = file.replaceAll('.','/');

  var jsfile = prototypes.get_ur_file(urpath, prototypes.ur_ext);
  var jsonfile = prototypes.get_ur_file(urpath, ".json");

  var script = undefined;
  var json = undefined;

  if (jsfile) script = IO.slurp(jsfile);
  try {
    if (jsonfile) json = JSON.parse(IO.slurp(jsonfile));
  } catch(e) {
    Log.warn(`Unable to create json from ${jsonfile}`);
  }

  if (!json && !script) {
    Log.warn(`Could not make ur from ${file}`);
    return undefined;
  }

  if (script)
    compile_env(script, newur, file);
  
  json ??= {};
  Object.merge(newur,json);

  Object.entries(newur).forEach(function([k,v]) {
    if (Object.isObject(v) && Object.isObject(upperur[k]))
      v.__proto__ = upperur[k];
  });

  Object.values(newur).forEach(function(v) {
    if (typeof v !== 'object') return;
    if (!v.comp) return;
    v.__proto__ = component[v.comp];
  });
      
  newur.__proto__ = upperur;
  newur.instances = [];
  Object.hide(newur, 'instances');

  prototypes.list.push(urpath);
  newur.toString = function() { return urpath; };
  ur[urpath] = newur;

  return newur;
}
prototypes.from_file.doc = "Create a new ur-type from a given script file.";
prototypes.list = [];

prototypes.list_ur = function()
{
  var list = [];
  function list_obj(obj, prefix)
  {
    prefix ??= "";
    var list = [];
    for (var e in obj) {
      list.push(prefix + e);
      list.concat(list_obj(obj[e], e + "."));
    }

    return list;
  }
  
  return list_obj(ur);
}

prototypes.ur2file = function(urpath)
{
  return urpath.replaceAll('.', '/');
}

prototypes.file2ur = function(file)
{
  file = file.strip_ext();
  file = file.replaceAll('/','.');
  return file;
}

/* Returns an ur, or makes it, for any given type of path
   could be a file on a disk like ball/big.js
   could be an ur path like ball.big
*/
prototypes.get_ur = function(name)
{
  if (!name) {
    Log.warn(`Can't get ur from an undefined.`);
    return;
  }
  var urpath = name;
  if (urpath.includes('/'))
    urpath = prototypes.file2ur(name);

  if (!prototypes.ur[urpath]) {
    var ur = prototypes.from_file(urpath);
    if (ur)
      return ur;
    else {
      Log.warn(`Could not find prototype using name ${name}.`);
      return undefined;
    }
  } else
    return prototypes.ur[urpath];
}

prototypes.get_ur_file = function(path, ext)
{
  var urpath = prototypes.ur2file(path);
  var file = urpath + ext;
  if (IO.exists(file)) return file;
  file = urpath + "/" + path.split('.').at(-1) + ext;
  if (IO.exists(file)) return file;
  return undefined;
}

prototypes.generate_ur = function(path)
{
  var ob = IO.glob("**" + prototypes.ur_ext);
  ob = ob.concat(IO.glob("**.json"));
  ob = ob.map(function(path) { return path.set_ext(""); });
  ob.forEach(function(name) { prototypes.get_ur(name); });
}

var ur = prototypes.ur;

prototypes.resavi = function(ur, path)
{
  if (!ur) return path;
  if (path[0] === '/') return path;

  var res = ur.replaceAll('.', '/');
  var dir = path.dir();
  if (res.startsWith(dir))
    return path.base();

  return path;
}

prototypes.resani = function(ur, path)
{
  if (!path) return "";
  if (!ur) return path;
  if (path[0] === '/') return path.slice(1);

  var res = ur.replaceAll('.', '/');
  var restry = res + "/" + path;
  while (!IO.exists(restry)) {
    res = res.updir() + "/";
    if (res === "/")
      return path;

    restry = res + path;
  }
  return restry;
}

prototypes.ur_dir = function(ur)
{
  var path = ur.replaceAll('.', '/');
  Log.warn(path);
  Log.warn(IO.exists(path));
  Log.warn(`${path} does not exist; sending ${path.dir()}`);
}

prototypes.ur_json = function(ur)
{
  var path = ur.replaceAll('.', '/');
  if (IO.exists(path))
    path = path + "/" + path.name() + ".json";
  else
    path = path + ".json";

  return path;
}

prototypes.ur_stem =  function(ur)
{
  var path = ur.replaceAll('.', '/');
  if (IO.exists(path))
    return path + "/" + path.name();
  else
    return path;
}

prototypes.ur_file_exts = ['.jso', '.json'];

prototypes.ur_folder = function(ur)
{
  var path = ur.replaceAll('.', '/');
  return IO.exists(path);
}

prototypes.ur_pullout_folder = function(ur)
{
  if (!prototypes.ur_folder(ur)) return;

  var stem = prototypes.ur_stem(ur);
  
/*  prototypes.ur_file_exts.forEach(function(e) {
    var p = stem + e;
    if (IO.exists(p))
  */    
}

prototypes.ur_pushin_folder = function(ur)
{

}
