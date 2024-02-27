prosperon.obj_unique_name = function(name, obj)
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
var a_db = {};

actor.spawn = function(script, config){
  if (typeof script !== 'string') return undefined;
  if (!a_db[script]) a_db[script] = io.slurp(script);
  var padawan = Object.create(actor);
  eval_env(a_db[script], padawan);

  if (typeof config === 'object')
    Object.merge(padawan,config);

  padawan.padawans = [];
  padawan.timers = [];
  padawan.master = this;
  Object.hide(padawan, "master","timers", "padawans");
  this.padawans.push(padawan);
  return padawan;
};

actor.spawn.doc = `Create a new actor, using this actor as the master, initializing it with 'script' and with data (as a JSON or Nota file) from 'config'.`;

actor.timers = [];
actor.kill = function(){
  if (this.__dead__) return;
  this.timers.forEach(t => t.kill());
  if (this.master)
    delete this.master[this.toString()];
  this.padawans.forEach(p => p.kill());
  this.padawans = [];
  this.__dead__ = true;
  if (typeof this.die === 'function') this.die();
};

actor.kill.doc = `Remove this actor and all its padawans from existence.`;

actor.delay = function(fn, seconds) {
  var t = Object.create(timer);
  t.remain = seconds;
  t.kill = () => {
    timer.kill.call(t);
    delete this.timers[t.toString()];
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

actor.delay.doc = `Call 'fn' after 'seconds' with 'this' set to the actor.`;

actor.master = undefined;

actor.padawans = [];

actor.remaster = function(to){
  delete this.master.padawans[this.toString()];
  this.master = to;
  to.padawans.push(this);
};

global.app = Object.create(actor);

app.die = function()
{
  Game.quit();
}

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
  get layer() { return cmd(77,this.body); },
  set warp_layer(x) { cmd(251, this.body,x); },
  get warp_layer() { return cmd(252, this.body); },

  set mass(x) { set_body(7,this.body,x); },
  get mass() {
    if (!(this.phys === physics.dynamic))
      return undefined;

    return q_body(5, this.body);
  },
  get elasticity() { return cmd(107,this.body); },
  set elasticity(x) { cmd(106,this.body,x); },
  get friction() { return cmd(109,this.body); },
  set friction(x) { cmd(108,this.body,x); },
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
    var lur = ur[this.level.ur];
    if (!lur) return;
    var lur = lur.objects[this.toString()];
    var d = ediff(this._ed.urdiff,lur);
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
  full_path() {
    return this.path_from(Primum);
  },
  /* pin this object to the to object */
  pin(to) {
    var p = cmd(222,this.body, to.body);
  },
  slide(to, a, b, min, max) {
    a ??= [0,0];
    b ??= [0,0];
    min ??= 0;
    max ??= 50;
    var p = cmd(229,this.body,to.body,a,b,min,max);
    p.max_force = 500;
    p.break();
  },
  pivot(to, piv) {
    piv ??= this.worldpos();
    var p = cmd(221,this.body,to.body,piv);
  },
  /* groove is on to, from local points a and b, anchored to this at local anchor */
  groove(to, a, b, anchor) {
    anchor ??= [0,0];
    var p = cmd(228, to.body, this.body, a, b, anchor);
  },
  damped_spring(to, length, stiffness, damping) {
    length ??= Vector.length(this.worldpos(), to.worldpos());
    stiffness ??= 1;
    damping ??= 1;
    var dc = 2*Math.sqrt(stiffness*this.mass);
    var p = cmd(227, this.body, to.body, [0,0], [0,0], stiffness, damping*dc);
  },
  damped_rotary_spring(to, angle, stiffness, damping) {
    angle ??= 0;
    stiffness ??= 1;
    damping ??= 1;
    /* calculate actual damping value from the damping ratio */
    /* damping = 1 is critical */
    var dc = 2*Math.sqrt(stiffness*this.get_moi()); /* critical damping number */
    /* zeta = actual/critical */
    var p = cmd(226,this.body,to.body,angle,stiffness,damping*dc);
  },
  rotary_limit(to, min, max) {
    var p = cmd(225,this.body,to.body, Math.turn2rad(min),Math.turn2rad(max));
  },
  ratchet(to,ratch) {
    var phase = this.angle - to.angle;
    var p = cmd(230, this.body, to.body, phase, Math.turn2rad(ratch));
  },
  gear(to, ratio) {
    phase ??= 1;
    ratio ??= 1;
    var phase = this.angle - to.angle;
    var p = cmd(223,this.body,to.body,phase,ratio);
  },
  motor(to, rate) {
    var p = cmd(231, this.body, to.body, rate);
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
      this.crying =  audio.sound.play(file, audio.sound.bus.sfx);
      var killfn = () => {this.crying = undefined; console.warn("killed"); }
      this.crying.hook = killfn;
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
      screenpos() { return Window.world2screen(this.worldpos()); },

      worldangle() { return Math.rad2turn(q_body(2,this.body)); },
      sworldangle(x) { set_body(0,this.body,Math.turn2rad(x)); },

      spawn_from_instance(inst) {
        return this.spawn(inst.ur, inst);
      },
      
      spawn(ur, data) {
        ur ??= gameobject;
	if (typeof ur === 'string') {
	  //ur = prototypes.get_ur(ur);
	  
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
    this2screen(pos) { return Window.world2screen(this.this2world(pos)); },
    screen2this(pos) { return this.world2this(Window.screen2world(pos)); },
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

  unregister() {
    this.timers.forEach(t=>t());
    this.timers = [];
  },

  check_registers(obj) {
    obj.unregister();
  
    if (typeof obj.update === 'function')
      obj.timers.push(Register.update.register(obj.update.bind(obj)));

    if (typeof obj.physupdate === 'function')
      obj.timers.push(Register.physupdate.register(obj.physupdate.bind(obj)));

    if (typeof obj.collide === 'function')
      register_collide(0, obj.collide.bind(obj), obj.body);

    if (typeof obj.separate === 'function')
      register_collide(3,obj.separate.bind(obj), obj.body);

    if (typeof obj.draw === 'function')
      obj.timers.push(Register.draw.register(obj.draw.bind(obj), obj));

    if (typeof obj.debug === 'function')
      obj.timers.push(Register.debug.register(obj.debug.bind(obj)));

    if (typeof obj.gui === 'function')
      obj.timers.push(Register.gui.register(obj.gui.bind(obj)));

    for (var k in obj) {
      if (!k.startswith("on_")) continue;
      var signal = k.fromfirst("on_");
      Event.observe(signal, obj, obj[k]);
    };

    obj.components.forEach(function(x) {
      if (typeof x.collide === 'function')
        register_collide(1, x.collide.bind(x), obj.body, x.shape);
    });
  },
  toString() { return "new_object"; },
  
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

	boxes.forEach(function(x) { bb = bbox.expand(bb, x); });
	
	bb = bbox.move(bb, this.pos);

	return bb ? bb : bbox.fromcwh([0,0], [0,0]);
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
//	var j = this.json_obj();
//	Object.assign(t,j);
	t.ur = this.ur;
	return t;
      },

      transform() {
        var t = {};
	t.pos = this.pos;
	if (t.pos.every(x=>x===0)) delete t.pos;
	t.angle = Math.places(this.angle,4);
	if (t.angle === 0) delete t.angle;
	t.scale = this.scale;
	t.scale = t.scale.map((x,i) => x/this.__proto__.scale[i]);
	t.scale = t.scale.map(x => Math.places(x,3));
	if (t.scale.every(x=>x===1)) delete t.scale;
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
    if (this.__kill) return;
    this.__kill = true;
    
    this.timers.forEach(t => t());
    this.timers = [];
    Event.rm_obj(this);
    Player.do_uncontrol(this);
    register_collide(2, undefined, this.body);
    
    if (this.level) {
      this.level.remove_obj(this);
      this.level = undefined;
    }
    
    if (this.__proto__.instances)
      delete this.__proto__.instances[this.toString()];

    for (var key in this.components) {
      this.components[key].kill();
      this.components[key].gameobject = undefined;
      delete this.components[key];
    }

    this.clear();
    this.objects = undefined;


    if (typeof this.stop === 'function')
      this.stop();
  },

  up() { return [0,1].rotate(this.angle);},
  down() { return [0,-1].rotate(this.angle);},
  right() { return [1,0].rotate(this.angle);},
  left() { return [-1,0].rotate(this.angle); },

  make() {
    var obj = Object.create(this);
    
    obj.make = undefined;
    Object.mixin(obj,gameobject_impl);

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
      obj.make_objs(this.objects)

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

  add_component(comp, data) {
    data ??= undefined;
    if (typeof comp.make !== 'function') return;
    var name = prosperon.obj_unique_name(comp.toString(), this);
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
Object.mixin(gameobject,gameobject_impl);

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
  pin: 'Pin joint to another object. Acts as if a rigid rod is between the two objects.',
  slide: 'Slide joint, similar to a pin but with min and max allowed distances.',
  pivot: 'Pivot joint to an object, with the pivot given in world coordinates.',
  groove: 'Groove joint. The groove is on to, from to local coordinates a and b, with this object anchored at anchor.',
  damped_spring: 'Damped spring to another object. Length is the distance it wants to be, stiffness is the spring constant, and damping is the damping ratio. 1 is critical, < 1 is underdamped, > 1 is overdamped.',
  damped_rotary_spring: 'Similar to damped spring but for rotation. Rest angle is the attempted angle.',
  rotary_limit: 'Limit the angle relative to the to body between min and max.',
  ratchet: 'Like a socket wrench, relative to to. ratch is the distance between clicks.',
  gear: 'Keeps the angular velocity ratio of this body and to constant. Ratio is the gear ratio.',
  motor: 'Keeps the relative angular velocity of this body to to at a constant rate. The most simple idea is for one of the bodies to be static, to the other is kept at rate.'
};

/* Default objects */
var prototypes = {};
prototypes.ur_ext = ".jso";
prototypes.ur = {};

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

  if (jsfile) script = io.slurp(jsfile);
  try {
    if (jsonfile) json = JSON.parse(io.slurp(jsonfile));
  } catch(e) {
    console.warn(`Unable to create json from ${jsonfile}. ${e}`);
  }

  if (!json && !jsfile) {
    console.warn(`Could not make ur from ${file}`);
    return undefined;
  }

  if (script)
    load_env(jsfile, newur);
  
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

prototypes.get_ur = function(name)
{
  if (!name) return;
  if (!name) {
    console.error(`Can't get ur from ${name}.`);
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
      console.warn(`Could not find prototype using name ${name}.`);
      return undefined;
    }
  } else
    return prototypes.ur[urpath];
}

prototypes.get_ur.doc = `Returns an ur, or makes it, for any given type of path
   could be a file on a disk like ball/big.js
   could be an ur path like ball.big`;

prototypes.get_ur_file = function(path, ext)
{
  var urpath = prototypes.ur2file(path);
  var file = urpath + ext;
  if (io.exists(file)) return file;
  file = urpath + "/" + path.split('.').at(-1) + ext;
  if (io.exists(file)) return file;
  return undefined;
}

prototypes.generate_ur = function(path)
{
  var ob = io.glob("**" + prototypes.ur_ext);
  ob = ob.concat(io.glob("**.json"));

  ob = ob.map(function(path) { return path.set_ext(""); });
  ob = ob.map(function(path) { return path[0] !== '.' ? path : undefined; });
  ob = ob.map(function(path) { return path[0] !== '_' ? path : undefined; });
  ob = ob.filter(x => x !== undefined);
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
  while (!io.exists(restry)) {
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
  console.warn(path);
  console.warn(io.exists(path));
  console.warn(`${path} does not exist; sending ${path.dir()}`);
}

prototypes.ur_json = function(ur)
{
  var path = ur.replaceAll('.', '/');
  if (io.exists(path))
    path = path + "/" + path.name() + ".json";
  else
    path = path + ".json";

  return path;
}

prototypes.ur_stem =  function(ur)
{
  var path = ur.replaceAll('.', '/');
  if (io.exists(path))
    return path + "/" + path.name();
  else
    return path;
}

prototypes.ur_file_exts = ['.jso', '.json'];

prototypes.ur_folder = function(ur)
{
  var path = ur.replaceAll('.', '/');
  return io.exists(path);
}

prototypes.ur_pullout_folder = function(ur)
{
  if (!prototypes.ur_folder(ur)) return;

  var stem = prototypes.ur_stem(ur);
  
/*  prototypes.ur_file_exts.forEach(function(e) {
    var p = stem + e;
    if (io.exists(p))
  */    
}

return {
  gameobject,
  actor,
  prototypes,
  ur
}
