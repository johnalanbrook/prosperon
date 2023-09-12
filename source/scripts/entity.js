function grab_from_points(pos, points, slop) {
  var shortest = slop;
  var idx = -1;
  points.forEach(function(x,i) {
    if (Vector.length(pos.sub(x)) < shortest) {
      shortest = Vector.length(pos.sub(x));
      idx = i;
    }
  });
  return idx;
};

var gameobject = {
  scale: 1.0,
  save: true,
  selectable: true,

  spawn(ur) {
    if (typeof ur === 'string')
      ur = prototypes.get_ur(ur);
    return ur.type.make(this);
  },

  layer: 0, /* Collision layer; should probably have been called "mask" */
  layer_nuke() {
    Nuke.label("Collision layer");
    Nuke.newline(Collision.num);
    for (var i = 0; i < Collision.num; i++)
      this.layer = Nuke.radio(i, this.layer, i);
  },

  draw_layer: 1,
  draw_layer_nuke() {
    Nuke.label("Draw layer");
    Nuke.newline(5);
    for (var i = 0; i < 5; i++)
      this.draw_layer = Nuke.radio(i, this.draw_layer, i);
  },

  in_air() {
    return q_body(7, this.body);
  },

  on_ground() { return !this.in_air(); },

  name: "gameobject",

  toString() { return this.name; },

  clone(name, ext) {
    var obj = Object.create(this);
    complete_assign(obj, ext);
    return obj;
  },

  dup(diff) {
    var dup = World.spawn(gameobjects[this.from]);
    Object.assign(dup, diff);
    return dup;
  },

  instandup() {
    var dup = World.spawn(gameobjects[this.from]);
    dup.pos = this.pos;
    dup.velocity = this.velocity;
  },
  
  ed_locked: false,
  
  _visible:  true,
  get visible(){ return this._visible; },
  set visible(x) {
    this._visible = x; 
    for (var key in this.components) {
      if ('visible' in this.components[key]) {
        this.components[key].visible = x;
      }
    }
  },

  mass: 1,
  bodytype: {
    dynamic: 0,
    kinematic: 1,
    static: 2
  },

  get moi() { return q_body(6, this.body); },
  set moi(x) { set_body(13, this.body, x); },
  
  phys: 2,
  phys_nuke() {
    Nuke.newline(1);
    Nuke.label("phys");
    Nuke.newline(3);
    this.phys = Nuke.radio("dynamic", this.phys, 0);
    this.phys = Nuke.radio("kinematic", this.phys, 1);
    this.phys = Nuke.radio("static", this.phys, 2);
  },
  friction: 0,
  elasticity: 0,
  flipx: false,
  flipy: false,
  
  body: -1,
  get controlled() {
    return Player.obj_controlled(this);
  },

  set_center(pos) {
    var change = pos.sub(this.pos);
    this.pos = pos;
    
    for (var key in this.components) {
      this.components[key].finish_center(change);
    }
  },

  varname: "",
  
  pos: [0,0],
  
  set relpos(x) {
    if (!this.level) {
      this.pos = x;
      return;
    }

    this.pos = Vector.rotate(x, Math.deg2rad(this.level.angle)).add(this.level.pos);
  },

  get relpos() {
    if (!this.level) return this.pos;

    var offset = this.pos.sub(this.level.pos);
    return Vector.rotate(offset, -Math.deg2rad(this.level.angle));
  },
  
  angle: 0,
  
  get relangle() {
    if (!this.level) return this.angle;

    return this.angle - this.level.angle;
  },
  
  get velocity() { return q_body(3, this.body); },
  set velocity(x) { set_body(9, this.body, x); },
  get angularvelocity() { return Math.rad2deg(q_body(4, this.body)); },
  set angularvelocity(x) { if (this.alive) set_body(8, this.body, Math.deg2rad(x)); },

  get alive() { return this.body >= 0; },

  disable() {
    this.components.forEach(function(x) { x.disable(); });
    
  },

  enable() {
    this.components.forEach(function(x) { x.enable(); });
  },
  
  sync() {
    if (this.body === -1) return;
    cmd(55, this.body, this.flipx);
    cmd(56, this.body, this.flipy);
    set_body(2, this.body, this.pos);
    set_body(0, this.body, Math.deg2rad(this.angle));
    cmd(36, this.body, this.scale);
    set_body(10,this.body,this.elasticity);
    set_body(11,this.body,this.friction);
    set_body(1, this.body, this.phys);
    cmd(75,this.body,this.layer);
    cmd(54, this.body);
  },

  syncall() {
    this.instances.forEach(function(x) { x.sync(); });
  },
  
  pulse(vec) { /* apply impulse */
    set_body(4, this.body, vec);
  },

  push(vec) { /* apply force */
    set_body(12,this.body,vec);
  },

  gizmo: "", /* Path to an image to draw for this gameobject */

  /* Bounding box of the object in world dimensions */
  get boundingbox() {
    var boxes = [];
    boxes.push({t:0, r:0,b:0,l:0});

    for (var key in this.components) {
      if ('boundingbox' in this.components[key])
        boxes.push(this.components[key].boundingbox);
    }
    
    if (boxes.empty) return cwh2bb([0,0], [0,0]);
    
    var bb = boxes[0];
    
    boxes.forEach(function(x) {
      bb = bb_expand(bb, x);
    });
    
    var cwh = bb2cwh(bb);
    
    if (!bb) return;
    
    if (this.flipx) cwh.c.x *= -1;
    if (this.flipy) cwh.c.y *= -1;
    
    cwh.c = cwh.c.add(this.pos);
    bb = cwh2bb(cwh.c, cwh.wh);
    
    return bb ? bb : cwh2bb([0,0], [0,0]);
  },

  set width(x) {},
  get width() {
    var bb = this.boundingbox;
    return bb.r - bb.l;
  },
  set height(x) {},
  get height() {
    var bb = this.boundingbox;
    return bb.t-bb.b;
  },

  stop() {},

  kill() {
    if (this.body === -1) {
      Log.warn(`Object is already dead!`);
      return;
    }
    Register.endofloop(() => {
      cmd(2, this.body);
      delete Game.objects[this.body];
    
      if (this.level)
        this.level.unregister(this);
      
      Player.uncontrol(this);
      this.instances.remove(this);
      Register.unregister_obj(this);
//      Signal.clear_obj(this);
    
      this.body = -1;
      for (var key in this.components) {
        Register.unregister_obj(this.components[key]);
        this.components[key].kill();
      }

      this.stop();
    });
  },

  get up() {
    return [0,1].rotate(Math.deg2rad(this.angle));
  },
  
  get down() {
    return [0,-1].rotate(Math.deg2rad(this.angle));
  },
  
  get right() {
    return [1,0].rotate(Math.deg2rad(this.angle));
  },
  
  get left() {
    return [-1,0].rotate(Math.deg2rad(this.angle));
  },

  /* Make a unique object the same as its prototype */
  revert() {
    unmerge(this, this.prop_obj());
    this.sync();
  },

  gui() {
    var go_guis = walk_up_get_prop(this, 'go_gui');
    Nuke.newline();

    go_guis.forEach(function(x) { x.call(this); }, this);

    for (var key in this) {
      if (typeof this[key] === 'object' && 'gui' in this[key]) this[key].gui();
    }
  },

  world2this(pos) { return cmd(70, this.body, pos); },
  this2world(pos) { return cmd(71, this.body,pos); },

  check_registers(obj) {
    Register.unregister_obj(this);
  
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

    obj.components.forEach(function(x) {
      if (typeof x.collide === 'function')
        register_collide(1, x.collide, x, obj.body, x.shape);
    });
  },
  instances: [],

  make(level) {
    level ??= Primum;
    var obj = Object.create(this);
    this.instances.push(obj);
    obj.toString = function() {
      var props = obj.prop_obj();
      for (var key in props)
        if (typeof props[key] === 'object' && !props[key] === null && props[key].empty)
	  delete props[key];
	  
      var edited = !props.empty;
//      return (edited ? "#" : "") + obj.name + " object " + obj.body + ", layer " + obj.draw_layer + ", phys " + obj.layer;
      return obj.ur.tag;
    };

    obj.fullpath = function() {
      return obj.ur.tag;
    };
    obj.deflock('toString');
    obj.defc('from', this.name);
    obj.defn('body', make_gameobject(this.scale,
                           this.phys,
                           this.mass,
                           this.friction,
                           this.elasticity) );
    obj.sync();
    obj.defn('components', {});
    Game.register_obj(obj);

    var objects = [];
    obj.objects = objects;

    obj.remove_child = function(child) {
      objects.remove(child);
    }

    obj.add_child = function(child) {
      child.unparent();
      objects.push(child);
      child.level = obj;
    }

    /* Reparent this object to a new one */
    obj.reparent = function(parent) {
      if (parent === obj.level)
        return;

      parent.add_child(obj);
    }

    obj.unparent = function() {
      if (!obj.level) return;
      obj.level.remove_child(obj);
    }

    cmd(113, obj.body, obj);

    /* Now that it's concrete in the engine, these functions update to return engine data */
    complete_assign(obj, {
      set scale(x) { cmd(36, obj.body, x); },
      get scale() { return cmd(103, obj.body); },
      get flipx() { return cmd(104,obj.body); },
      set flipx(x) { cmd(55, obj.body, x); },
      get flipy() { return cmd(105,obj.body); },
      set flipy(x) { cmd(56, obj.body, x); },

      get angle() { return Math.rad2deg(q_body(2,obj.body))%360; },
      set angle(x) { set_body(0,obj.body, Math.deg2rad(x)); },

      set pos(x) {
        var diff = x.sub(this.pos);
	objects.forEach(function(x) { x.pos = x.pos.add(diff); });
        set_body(2,obj.body,x); },
      get pos() { return q_body(1,obj.body); },

      get elasticity() { return cmd(107,obj.body); },
      set elasticity(x) { cmd(106,obj.body,x); },

      get friction() { return cmd(109,obj.body); },
      set friction(x) { cmd(108,obj.body,x); },

      set mass(x) { set_body(7,obj.body,x); },
      get mass() { return q_body(5, obj.body); },

      set phys(x) { set_body(1, obj.body, x); },
      get phys() { return q_body(0,obj.body); },
    });

    for (var prop in obj) {
       if (typeof obj[prop] === 'object' && 'make' in obj[prop]) {
	   if (prop === 'flipper') return;
           obj[prop] = obj[prop].make(obj.body);
	   obj[prop].defn('gameobject', obj);
	   obj.components[prop] = obj[prop];
       }
    };

    obj.check_registers(obj);

    /* Spawn subobjects defined */
    if (obj.$) {
      for (var e in obj.$)
        obj.$[e] = obj.spawn(prototypes.get_ur(obj.$[e].ur));
    }

    if (typeof obj.start === 'function') obj.start();

    return obj;
  },

  register_hit(fn, obj) {
    if (!obj)
      obj = this;

    Signal.obj_begin(fn, obj, this);
  },

  register_separate(fn, obj) {
    if (!obj)
      obj = this;

    Signal.obj_separate(fn,obj,this);
  },
}

var locks = ['height', 'width', 'visible', 'body', 'controlled', 'selectable', 'save', 'velocity', 'angularvelocity', 'alive', 'boundingbox', 'name', 'scale', 'angle', 'properties', 'moi', 'relpos', 'relangle', 'up', 'down', 'right', 'left', 'bodytype', 'gizmo', 'pos'];
locks.forEach(x => gameobject.obscure(x));


/* Default objects */
var prototypes = {};
prototypes.ur = {};
prototypes.load_all = function()
{
if (IO.exists("proto.json"))
  prototypes = JSON.parse(IO.slurp("proto.json"));

for (var key in prototypes) {
  if (key in gameobjects)
    dainty_assign(gameobjects[key], prototypes[key]);
  else {
    /* Create this gameobject fresh */
    Log.info("Making new prototype: " + key + " from " + prototypes[key].from);
    var newproto = gameobjects[prototypes[key].from].clone(key);
    gameobjects[key] = newproto;

    for (var pkey in newproto)
      if (typeof newproto[pkey] === 'object' && newproto[pkey] && 'clone' in newproto[pkey])
        newproto[pkey] = newproto[pkey].clone();

    dainty_assign(gameobjects[key], prototypes[key]);
  }
}
}

prototypes.save_gameobjects = function() { slurpwrite(JSON.stringify(gameobjects,null,2), "proto.json"); };

prototypes.from_file = function(file)
{
  if (!IO.exists(file)) {
    Log.error(`File ${file} does not exist.`);
    return;
  }

  var newobj = gameobject.clone(file, {});
  var script = IO.slurp(file);

  newobj.$ = {};
  var json = {};
  if (IO.exists(file.name() + ".json")) {
    json = JSON.parse(IO.slurp(file.name() + ".json"));
    Object.assign(newobj.$, json.$);
    delete json.$;
  }

  compile_env(`var self = this; var $ = self.$; ${script}`, newobj, file);
  dainty_assign(newobj, json);

  file = file.replaceAll('/', '.');
  var path = file.name().split('.');
  var nested_access = function(base, names) {
    for (var i = 0; i < names.length; i++)
      base = base[names[i]] = base[names[i]] || {};

    return base;
  };
  var a = nested_access(ur, path);
  
  a.tag = file.name();
  prototypes.list.push(a.tag);
  a.type = newobj;
  a.instances = [];
  newobj.ur = a;

  return a;
}
prototypes.from_file.doc = "Create a new ur-type from a given script file.";
prototypes.list = [];

prototypes.from_obj = function(name, obj)
{
  var newobj = gameobject.clone(name, obj);
  prototypes.ur[name] = {
    tag: name,
    type: newobj
  };
  return prototypes.ur[name];
}

prototypes.load_config = function(name)
{
  if (!prototypes.ur[name])
    prototypes.ur[name] = gameobject.clone(name);

  Log.warn(`Made new ur of name ${name}`);

  return prototypes.ur[name];
}


prototypes.list_ur = function()
{
  var list = [];
  function list_obj(obj, prefix)
  {
    prefix ??= "";
    var list = [];
    for (var e in obj) {
      list.push(prefix + e);
      Log.warn("Descending into " + e);
      list.concat(list_obj(obj[e], e + "."));
    }

    return list;
  }
  
  return list_obj(ur);
}

prototypes.get_ur = function(name)
{
  if (!prototypes.ur[name]) {
    if (IO.exists(name + ".js"))
      prototypes.from_file(name + ".js");

    prototypes.load_config(name);
    return prototypes.ur[name];
  } else
    return prototypes.ur[name];
}

prototypes.from_obj("polygon2d", {
  polygon2d: polygon2d.clone(),
});

prototypes.from_obj("edge2d", {
  edge2d: bucket.clone(),
});

prototypes.from_obj("sprite", {
  sprite: sprite.clone(),
});

prototypes.generate_ur = function(path)
{
  var ob = IO.glob("*.js");
  ob = ob.concat(IO.glob("**/*.js"));
  ob = ob.filter(function(str) { return !str.startsWith("scripts"); });

  ob.forEach(function(name) {
    if (name === "game.js") return;
    if (name === "play.js") return;

    prototypes.from_file(name);
  });
}

var ur = prototypes.ur;

prototypes.from_obj("camera2d", {
    phys: gameobject.bodytype.kinematic,
    speed: 300,
    
    get zoom() { return this._zoom; },
    set zoom(x) {
      if (x <= 0) return;
      this._zoom = x;
      cmd(62, this._zoom);
    },
    _zoom: 1.0,
    speedmult: 1.0,
    
    selectable: false,
    
    view2world(pos) {
      return pos.mapc(mult, [1,-1]).add([-Window.width,Window.height].scale(0.5)).scale(this.zoom).add(this.pos);
    },
    
    world2view(pos) {
      return pos.sub(this.pos).scale(1/this.zoom).add(Window.dimensions.scale(0.5));
    },
});

prototypes.from_obj("arena", {});
