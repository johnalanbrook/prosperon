var files = {};
function load(file) {
  var modtime = cmd(0, file);

  if (modtime === 0) {
    Log.stack();
    return false;
  }
  files[file] = modtime;
}

load("scripts/std.js");

function initialize()
{
  if (IO.exists("config.js"))
    load("config.js");

  if (Cmdline.play)
    run("scripts/play.js");
  else
    run("scripts/editor.js");
}

function run(file)
{
  var modtime = cmd(119, file);
  if (modtime === 0) {
    Log.stack();
    return false;
  }

  files[file] = modtime;
  return cmd(117, file);
}


load("scripts/base.js");

load("scripts/diff.js");
Log.level = 1;

var Color = {
  white: [255,255,255,255],
  blue: [84,110,255,255],
  green: [120,255,10,255],
  yellow: [251,255,43,255],
  red: [255,36,20,255],
  teal: [96, 252, 237,255],
  gray: [181, 181,181,255],
  cyan: [0,255,255],
};

function bb2wh(bb) {
  return [bb.r-bb.l, bb.t-bb.b];
};

load("scripts/gui.js");

var timer = {
  guardfn(fn) {
    if (typeof fn === 'function')
      fn();
    else {
      Log.warn("TIMER TRYING TO EXECUTE WIHTOUT!!!");
      Log.warn(this);
      this.kill();
    }
  },

  make(fn, secs,obj,loop) {
    if (secs === 0) {
      fn.call(obj);
      return;
    }
      
    var t = clone(this);
    t.callback = fn;
    var guardfn = function() {
      if (typeof t.callback === 'function')
        t.callback();
      else
        Log.warn("Timer trying to execute without a function.");
    };
    t.id = make_timer(guardfn, secs, obj);
    
    return t;
  },

  oneshot(fn, secs,obj) {
    var t = this.make(() => {
      fn.call();
      t.kill();
    },secs);
    t.loop = 0;
    t.start();
  },

  get remain() { return cmd(32, this.id); },
  get on() { return cmd(33, this.id); },
  get loop() { return cmd(34, this.id); },
  set loop(x) { cmd(35, this.id, x); },
  
  start() { cmd(26, this.id); },
  stop() { cmd(25, this.id); },
  pause() { cmd(24, this.id); },
  kill() { cmd(27, this.id); },
  set time(x) { cmd(28, this.id, x); },
  get time() { return cmd(29, this.id); },
  get pct() { return this.remain / this.time; },
};

var animation = {
  time: 0,
  loop: false,
  playtime: 0,
  playing: false,
  keyframes: [],

  create() {
    var anim = Object.create(animation);
    Register.update.register(anim.update, anim);
    return anim;
  },

  start() {
    this.playing = true;
    this.time = this.keyframes.last[1];
    this.playtime = 0;
  },

  interval(a, b, t) {
    return (t - a) / (b - a);
  },

  near_val(t) {
    for (var i = 0; i < this.keyframes.length-1; i++) {
      if (t > this.keyframes[i+1][1]) continue;

      return this.interval(this.keyframes[i][1], this.keyframes[i+1][1], t) >= 0.5 ? this.keyframes[i+1][0] : this.keyframes[i][0];
    }

    return this.keyframes.last[0];
  },

  lerp_val(t) {
    for (var i = 0; i < this.keyframes.length-1; i++) {
      if (t > this.keyframes[i+1][1]) continue;

      var intv = this.interval(this.keyframes[i][1], this.keyframes[i+1][1], t);
      return ((1 - intv) * this.keyframes[i][0]) + (intv * this.keyframes[i+1][0]);
    }

    return this.keyframes.last[0];
  },

  cubic_val(t) {

  },

  mirror() {
    if (this.keyframes.length <= 1) return;
    for (var i = this.keyframes.length-1; i >= 1; i--) {
      this.keyframes.push(this.keyframes[i-1]);
      this.keyframes.last[1] = this.keyframes[i][1] + (this.keyframes[i][1] - this.keyframes[i-1][1]);
    }
  },

  update(dt) {
    if (!this.playing) return;

    this.playtime += dt;
    if (this.playtime >= this.time) {
      if (this.loop)
        this.playtime = 0;
      else {
        this.playing = false;
        return;
      }
    }

    this.fn(this.lerp_val(this.playtime));
  },
};

var Render = {
  normal() {
    cmd(67);
  },
  
  wireframe() {
    cmd(68);
  },
};

var Mouse = {
  get pos() {
    return cmd(45);
  },

  get screenpos() {
    var p = this.pos;
    p.y = Window.dimensions.y - p.y;
    return p;
  },

  get worldpos() {
    return screen2world(cmd(45));
  },
  
  disabled() {
    cmd(46, 212995);
  },
  
  hidden() {
    cmd(46, 212994);
  },
  
  normal() {
    cmd(46, 212993);
  },
};

var Keys = {
  shift() {
    return cmd(50, 340);// || cmd(50, 344);
  },
  
  ctrl() {
    return cmd(50, 341);// || cmd(50, 344);
  },
  
  alt() {
    return cmd(50, 342);// || cmd(50, 346);
  },

  super() {
    return cmd(50, 343);// || cmd(50, 347);
  },
};

load("scripts/physics.js");
load("scripts/input.js");
load("scripts/sound.js");

function screen2world(screenpos) { return Game.camera.view2world(screenpos); }
function world2screen(worldpos) { return Game.camera.world2view(worldpos); }

var Register = {
  inloop: false,
  loopcbs: [],
  finloop() {
    this.loopcbs.forEach(x => x());
    this.loopcbs = [];
  },

  wraploop(loop) {
    this.inloop = true;
    loop();
    this.inloop = false;
    this.finloop();
  },

  kbm_input(mode, btn, state, ...args) {
    switch(mode) {
      case "emacs":
        Player.players[0].raw_input(btn, state, ...args);
        break;
    };

    if (btn === 'lmouse')
      btn = 'lm';

    if (btn === 'rmouse')
      btn = 'rm';
  },

  gamepad_playermap: [],
  gamepad_input(pad, btn, state, ...args) {
    var player = this.gamepad_playermap[pad];
    if (!player) return;

    var statestr = Input.state2str(state);

    var rawfn = `gamepad_${btn}_${statestr}`;
    player.input(rawfn, ...args);

    Action.actions.forEach(x => {
      if (x.inputs.includes(btn))
        player.input(`action_${x.name}_${statestr}`, ...args);
    });
  },
  
  unregister_obj(obj) {
    Register.registries.forEach(function(x) {
      x.unregister_obj(obj);
    });
    Player.uncontrol(obj);
  },

  endofloop(fn) {
    if (!this.inloop)
      fn();
    else {
      this.loopcbs.push(fn);
    }
  },

  clear() {
    Register.registries.forEach(function(n) {
      n.entries = [];
    });
  },

  registries: [],

  add_cb(idx, name) {
    var entries = [];
    var n = {};
    n.register = function(fn, obj) {
      if (!obj) {
        Log.error("Refusing to register a function without a destroying object.");
	return;
      }
      entries.push({
        fn: fn,
	obj: obj
      });
    }

    n.unregister = function(fn) {
      entries = entries.filter(function(e) { return e.fn !== f; });
    }

    n.unregister_obj = function(obj) {
      entries = entries.filter(function(e) { return e.obj !== obj; });
    }

    n.broadcast = function(...args) {
      entries.forEach(x => x.fn.call(x.obj, ...args));
    }

    n.clear = function() {
      entries = [];
    }

    register(idx, n.broadcast, n);

    Register[name] = n;
    Register.registries.push(n);
    
    return n;
  },
};

Register.add_cb(0, "update").doc = "Called once per frame.";
Register.add_cb(1, "physupdate");
Register.add_cb(2, "gui");
Register.add_cb(3, "nk_gui");
Register.add_cb(6, "debug");
register(7, Register.kbm_input, Register);
Register.add_cb(8, "gamepad_input");
Register.add_cb(10, "draw");

register(9, Log.stack, this);

Register.gamepad_playermap[0] = Player.players[0];



Player.players[0].control(GUI);

var Signal = {
  signals: [],
  obj_begin(fn, obj, go) {
    this.signals.push([fn, obj]);
    register_collide(0, fn, obj, go.body);
  },

  obj_separate(fn, obj, go) {
    this.signals.push([fn,obj]);
    register_collide(3,fn,obj,go.body);
  },

  clera_obj(obj) {
    this.signals.filter(function(x) { return x[1] !== obj; });
  },

  c:{},
  register(name, fn) {
    if (!this.c[name])
      this.c[name] = [];

    this.c[name].push(fn);
  },

  call(name, ...args) {
    if (this.c[name])
      this.c[name].forEach(function(fn) { fn.call(this, ...args); });
  },
};

var Window = {
  set width(w) { cmd(125, w); },
  set height(h) { cmd(126, h); },
  get width() { return cmd(48); },
  get height() { return cmd(49); },
  get dimensions() { return [this.width, this.height]; },
};

Window.icon = function(path) { cmd(90, path); };
Window.icon.doc = "Set the icon of the window using the PNG image at path.";


function reloadfiles() {
  Object.keys(files).forEach(function (x) { load(x); });
}

load("scripts/debug.js");

/*
function Color(from) {
  var color = Object.create(Array);
  Object.defineProperty(color, 'r', setelem(0));
  Object.defineProperty(color, 'g', setelem(1));
  Object.defineProperty(color, 'b', setelem(2));
  Object.defineProperty(color, 'a', setelem(3));

  color.a = color.g = color.b = color.a = 1;
  Object.assign(color, from);

  return color;
};
*/

load("scripts/components.js");

function replacer_empty_nil(key, val) {
  if (typeof val === 'object' && JSON.stringify(val) === '{}')
    return undefined;

//  if (typeof val === 'number')
//    return parseFloat(val.toFixed(4));

  return val;
};

function clean_object(obj) {
  Object.keys(obj).forEach(function(x) {
    if (!(x in obj.__proto__)) return;

    switch(typeof obj[x]) {
      case 'object':
        if (Array.isArray(obj[x])) {
          if (obj[x].equal(obj.__proto__[x])) {
	    delete obj[x];
	  }
	} else
	  clean_object(obj[x]);
	  
        break;

      case 'function':
        return;

      default:
        if (obj[x] === obj.__proto__[x])
	  delete obj[x];
	break;
    }
  });
};

function find_com(objects)
{
  if (!objects || objects.length === 0)
    return [0,0];
    var com = [0,0];
    com[0] = objects.reduce(function(acc, val) {
      return acc + val.pos[0];
    }, 0);
    com[0] /= objects.length;
    
    com[1] = objects.reduce(function(acc, val) {
      return acc + val.pos[1];
    }, 0);
    com[1] /= objects.length;    
    
    return com;
};

var Game = {
  objects: [],
  resolution: [1200,720],
  name: "Untitled",
  register_obj(obj) {
    this.objects[obj.body] = obj;
  },
  
  /* Returns an object given an id */
  object(id) {
    return this.objects[id];
  },

  /* Returns a list of objects by name */
  find(name) {
    
  },

  /* Return a list of objects derived from a specific prototype */
  find_proto(proto) {

  },

  /* List of all objects spawned that have a specific tag */
  find_tag(tag) {

  },

  groupify(objects, spec) {
    var newgroup = {
      locked: true,
      breakable: true,
      objs: objects,
//      get pos() { return find_com(objects); },
//      set pos(x) { this.objs.forEach(function(obj) { obj.pos = x; }) },
    };

    Object.assign(newgroup, spec);
    objects.forEach(function(x) {
      x.defn('group', newgroup);
    });

    var bb = bb_from_objects(newgroup.objs);
    newgroup.startbb = bb2cwh(bb);
    newgroup.bboffset = newgroup.startbb.c.sub(newgroup.objs[0].pos);

    newgroup.boundingbox = function() {
      newgroup.startbb.c = newgroup.objs[0].pos.add(newgroup.bboffset);
      return cwh2bb(newgroup.startbb.c, newgroup.startbb.wh);
    };

    if (newgroup.file)
      newgroup.color = [120,255,10];

    return newgroup;
  },

  quit()
  {
    sys_cmd(0);
  },

  pause()
  {
    sys_cmd(3);
  },

  stop()
  {
    Game.pause();
    /* And return to editor .. */
    Log.warn("Stopping not implemented. Paused, and go to editor.");
  },

  step()
  {
    sys_cmd(4);
  },

  render() { sys_cmd(10); },

  playing() { return sys_cmd(5); },
  paused() { return sys_cmd(6); },
  stepping() {
  return cmd(79); },

  play()
  {
    sys_cmd(1);
  },

  get dt() {
    return cmd(63);
  },

  wait_fns: [],

  wait_exec(fn) {
    if (!phys_stepping())
      fn();
    else
      this.wait_fns.push(fn);
  },

  exec() {
    this.wait_fns.forEach(function(x) { x(); });

    this.wait_fns = [];
  },
};

Register.update.register(Game.exec, Game);

load("scripts/level.js");

var World = Level.create();
World.name = "World";
World.fullpath = function() { return World.name; };
World.load = function(lvl) {
  if (World.loaded)
    World.loaded.kill();

  World.loaded = World.spawn(lvl);
  return World.loaded;
};

var gameobjects = {};
var Prefabs = gameobjects;

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
    gameobjects[name] = obj;
    obj.defc('name', name);
    obj.from = this.name;
    obj.defn('instances', []);
    obj.obscure('from');
	
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

  make(props, level) {
    level ??= World;
    var obj = Object.create(this);
    this.instances.push(obj);
    obj.toString = function() {
      var props = obj.prop_obj();
      for (var key in props)
        if (typeof props[key] === 'object' && !props[key] === null && props[key].empty)
	  delete props[key];
	  
      var edited = !props.empty;
      return (edited ? "#" : "") + obj.name + " object " + obj.body + ", layer " + obj.draw_layer + ", phys " + obj.layer;
    };

    obj.fullpath = function() {
      return `${obj.level.fullpath()}.${obj.name}`;
    };
    obj.deflock('toString');
    obj.defc('from', this.name);
    obj.defn('body', make_gameobject(this.scale,
                           this.phys,
                           this.mass,
                           this.friction,
                           this.elasticity) );
    complete_assign(obj, props);
    obj.sync();
    obj.defn('components', {});

    cmd(113, obj.body, obj);

    complete_assign(obj, {
      set scale(x) {
        Log.warn(obj.body); cmd(36, obj.body, x); },
      get scale() { return cmd(103, obj.body); },
      get flipx() { return cmd(104,obj.body); },
      set flipx(x) { cmd(55, obj.body, x); },
      get flipy() { return cmd(105,obj.body); },
      set flipy(x) { cmd(56, obj.body, x); },

      get angle() { return Math.rad2deg(q_body(2,obj.body))%360; },
      set angle(x) { set_body(0,obj.body, Math.deg2rad(x)); },

      set pos(x) { set_body(2,obj.body,x); },
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

    if ('begin' in obj) obj.begin();

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
/* Load configs */
function load_configs(file) {
  Log.info(`Loading config file ${file}.`);
  var configs = JSON.parse(IO.slurp(file));
  for (var key in configs) {
    if (typeof globalThis[key] !== "object") continue;
    Object.assign(globalThis[key], configs[key]);
  }
  
  Collision.sync();
  Game.objects.forEach(function(x) { x.sync(); });
  
  if (!local_conf.mouse) {
    Log.info("disabling mouse features");
    Mouse.disabled = function() {};
    Mouse.hidden = function() {};
  };
};

var local_conf = {
  mouse: true,
};

if (IO.exists("game.config"))
  load_configs("game.config");

/* Save configs */
function save_configs() {
    Log.info("saving configs");
    var configs = {};
    configs.editor_config = editor_config;
    configs.Nuke = Nuke;
    configs.local_conf = local_conf;
    IO.slurpwrite(JSON.stringify(configs, null, 1), "editor.config");
    
    save_game_configs();
};

function save_game_configs() {
  var configs = {};
  configs.physics = physics;
  configs.Collision = Collision;
  Log.info(configs);
  IO.slurpwrite(JSON.stringify(configs,null,1), "game.config");

  Collision.sync();
  Game.objects.forEach(function(x) { x.sync(); });
};

load("scripts/physics.js");

var camera2d = gameobject.clone("camera2d", {
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

Game.view_camera = function(cam)
{
  Game.camera = cam;
  cmd(61, Game.camera.body);
}

Game.view_camera(World.spawn(camera2d));

win_make(Game.title, Game.resolution[0], Game.resolution[1]);

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
  compile_env(`var self = this;${script}`, newobj, file);
  prototypes.ur[file.name()] = newobj;
  return newobj;
}
prototypes.from_file.doc = "Create a new ur-type from a given script file.";

prototypes.from_obj = function(name, obj)
{
  var newobj = gameobject.clone(name, obj);
  prototypes.ur[name] = newobj;
  return newobj;
}

prototypes.load_config = function(name)
{
  if (!prototypes.config) {
    prototypes.config = {};

    if (IO.exists("proto.json"))
      prototypes.config = JSON.parse(IO.slurp("proto.json"));
  }

  Log.warn(`Loading a config for ${name}`);
    
  if (!prototypes.ur[name])
    prototypes.ur[name] = gameobject.clone(name);

  return prototypes.ur[name];
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

var Gamestate = {};

prototypes.from_obj("polygon2d", {
  polygon2d: polygon2d.clone(),
});

prototypes.from_obj("edge2d", {
  edge2d: bucket.clone(),
});

prototypes.from_obj("sprite", {
  sprite: sprite.clone(),
});
