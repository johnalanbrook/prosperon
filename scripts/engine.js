var files = {};
function load(file) {
   var modtime = cmd(0, file);
  files[file] = modtime;
}

load("scripts/base.js");
load("scripts/std.js");

function initialize()
{
  if (!Game.edit)
    load("scripts/play.js");
  else
    load("scripts/editor.js");
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

load("scripts/diff.js");
Log.level = 1;

var Color = {
  white: [255,255,255],
  black: [0,0,0],
  blue: [84,110,255],
  green: [120,255,10],
  yellow: [251,255,43],
  red: [255,36,20],
  teal: [96, 252, 237],
  gray: [181,181,181],
  cyan: [0,255,255],
  purple: [162,93,227],
};

Color.tohtml = function(v)
{
  var html = v.map(function(n) { return Number.hex(n*255); });
  return "#" + html.join('');
}

Color.toesc = function(v)
{
  return Esc.color(v);
}

var Esc = {};
Esc.reset = "\x1b[0";
Esc.color = function(v) {
  var c = v.map(function(n) { return Math.floor(n*255); });
  var truecolor = "\x1b[38;2;" + c.join(';') + ';';
  return truecolor;
}

Color.Arkanoid = {
  orange: [255,143,0],
  teal: [0,255,255],
  green: [0,255,0],
  red: [255,0,0],
  blue: [0,112,255],
  purple: [255,0,255],
  yellow: [255,255,0],
  silver: [157,157,157],
  gold: [188,174,0],
};

Color.Arkanoid.Powerups = {
  red: [174,0,0], /* laser */
  blue: [0,0,174], /* enlarge */
  green: [0,174,0], /* catch */
  orange: [224,143,0], /* slow */
  purple: [210,0,210], /* break */
  cyan: [0,174,255], /* disruption */
  gray: [143,143,143] /* 1up */
};

Color.Gameboy = {
  darkest: [229,107,26],
  dark: [229,189,26],
  light: [189,229,26],
  lightest: [107,229,26],
};

Color.Apple = {
  green: [94,189,62],
  yellow: [255,185,0],
  orange: [247,130,0],
  red: [226,56,56],
  purple: [151,57,153],
  blue: [0,156,223]
};

Color.Debug = {
  boundingbox: Color.white,
  names: [84,110,255],
};

Color.Editor = {
  grid: [99,255,128],
  select: [255,255,55],
  newgroup: [120,255,10],
};

/* Detects the format of all colors and munges them into a floating point format */
Color.normalize = function(c) {
  var add_a = function(a) {
    var n = this.slice();
    n.a = a;
    return n;
  };
  
  for (var p of Object.keys(c)) {
    var fmt = "nrm";
    if (typeof c[p] !== 'object') continue;
    if (!Array.isArray(c[p])) {
      Color.normalize(c[p]);
      continue;
    }
    
    for (var color of c[p]) {
      if (color > 1) {
        fmt = "8b";
	break;
      }
    }

    switch(fmt) {
      case "8b":
        c[p] = c[p].map(function(x) { return x/255; });
    }
    c[p].alpha = add_a;
  }
};

Color.normalize(Color);

Object.deepfreeze(Color);

var ColorMap = {};
ColorMap.makemap = function(map)
{
  var newmap = Object.create(ColorMap);
  Object.assign(newmap, map);
  return newmap;
}
ColorMap.Jet = ColorMap.makemap({
  0: [0,0,131],
  0.125: [0,60,170],
  0.375: [5,255,255],
  0.625: [255,255,0],
  0.875: [250,0,0],
  1: [128,0,0]
});

ColorMap.BlueRed = ColorMap.makemap({
  0: [0,0,255],
  1: [255,0,0]
});

ColorMap.Inferno = ColorMap.makemap({
  0:[0,0,4],
  0.13: [31,12,72],
  0.25: [85,15,109],
  0.38: [136,34,106],
  0.5: [186,54,85],
  0.63: [227,89,51],
  0.75: [249,140,10],
  0.88: [249,201,50],
  1: [252,255,164]
});

ColorMap.Bathymetry = ColorMap.makemap({
  0: [40,26,44],
  0.13: [59.49,90],
  0.25: [64,76,139],
  0.38: [63,110,151],
  0.5: [72,142,158],
  0.63: [85,174,163],
  0.75: [120,206,163],
  0.88: [187,230,172],
  1: [253,254,204]
});

ColorMap.Viridis = ColorMap.makemap({
  0: [68,1,84],
  0.13: [71,44,122],
  0.25: [59,81,139],
  0.38: [44,113,142],
  0.5: [33,144,141],
  0.63: [39,173,129],
  0.75: [92,200,99],
  0.88: [170,220,50],
  1: [253,231,37]
});

Color.normalize(ColorMap);

ColorMap.sample = function(t, map)
{
  map ??= this;
  if (t < 0) return map[0];
  if (t > 1) return map[1];

  var lastkey = 0;
  for (var key of Object.keys(map).sort()) {
    if (t < key) {
      var b = map[key];
      var a = map[lastkey];
      var tt = (key - lastkey) * t;
      return a.lerp(b, tt);
    }
    lastkey = key;
  }
  return map[1];
}

Object.freeze(ColorMap);

function bb2wh(bb) {
  return [bb.r-bb.l, bb.t-bb.b];
};

load("scripts/gui.js");

var timer = {
  make(fn, secs,obj,loop,app) {
    app ??= false;
    if (secs === 0) {
      fn.call(obj);
      return;
    }
      
    var t = Object.create(this);
    t.callback = fn;
    var guardfn = function() {
      if (typeof t.callback === 'function')
        t.callback();
      else
        Log.warn("Timer trying to execute without a function.");
    };
    t.id = make_timer(guardfn, secs, app);
    
    return t;
  },

  oneshot(fn, secs,obj, app) {
    app ??= false;
    var t = this.make(fn, secs, obj, 0, app);
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
    if (state === 'released') {
      btn = btn.split('-').last;
    }
   
    switch(mode) {
      case "emacs":
        Player.players[0].raw_input(btn, state, ...args);
        break;

      case "mouse":
        Player.players[0].mouse_input(btn, state, ...args);
	break;

      case "char":
        Player.players[0].char_input(btn);
	break;
    };
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

var game_quit = function()
{
  Primum.kill();
}

Signal.register("quit", game_quit);

var Window = {
  set width(w) { cmd(125, w); },
  set height(h) { cmd(126, h); },
  get width() { return cmd(48); },
  get height() { return cmd(49); },
  get dimensions() { return [this.width, this.height]; },
  set name(str) { cmd(134, str); },
  boundingbox() {
    return {
      t: Window.height,
      b: 0,
      r: Window.width,
      l: 0
    };
  },
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
  edit: true,
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
      newgroup.color = Color.Editor.newgroup;

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

  editor_mode(m) { sys_cmd(10, m); },

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

load("scripts/entity.js");

var preprimum = Object.create(gameobject);
preprimum.objects = {};
preprimum.worldpos = function() { return [0,0]; };
preprimum.worldangle = function() { return 0; };
preprimum.scale = 1;
preprimum.gscale = function() { return 1; };
preprimum.pos = [0,0];
preprimum.angle = 0;
preprimum.remove_obj = function() {};
var World = preprimum.make(preprimum);
var Primum = World;
Primum.level = undefined;
Primum.toString = function() { return "Primum"; };
Primum._ed.selectable = false;
World.reparent = function(parent) { Log.warn("Cannot reparent the Primum."); }

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

Game.view_camera = function(cam)
{
  Game.camera = cam;
  cmd(61, Game.camera.body);
}

Game.view_camera(Primum.spawn(ur.camera2d));

Window.name = "Primum Machinam (V0.1)";
Window.width = 1280;
Window.height = 720;
