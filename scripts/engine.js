"use math";
globalThis.global = globalThis;

function eval_env(script, env)
{
  env ??= {};
//  script = `function() { ${script} }.call();`;
//  return eval(script);
  
  return function(str) { return eval(str); }.call(env, script);
}
eval_env.dov = `Counterpart to /load_env/, but with a string.`;

function load_env(file,env)
{
  env ??= global;
  var script = io.slurp(file);
  eval_env(script, env);
//  cmd(16, file, env);
//  var script = io.slurp(file);
//  cmd(123, script, env, file);
}
load_env.doc = `Load a given file with 'env' as **this**. Does not add to the global namespace.`;

function load(file) { return load_env(file);}
load.doc = `Load a given script file into the global namespace.`;


load("scripts/base.js");
load("scripts/std.js");


load("scripts/diff.js");
console.level = 1;

load("scripts/color.js");

function bb2wh(bb) {
  return [bb.r-bb.l, bb.t-bb.b];
};


var prosperon = {};
prosperon.version = cmd(255);
prosperon.revision = cmd(256);

prosperon.semver = {};
prosperon.semver.valid = function(v, range)
{
  v = v.split('.');
  range = range.split('.');
  if (v.length !== 3) return undefined;
  if (range.length !== 3) return undefined;

  if (range[0][0] === '^') {
    range[0] = range[0].slice(1);
    if (parseInt(v[0]) >= parseInt(range[0])) return true;
    
    return false;
  }
  
  if (range[0] === '~') {
    range[0] = range[0].slice(1);
    for (var i = 0; i < 2; i++)
      if (parseInt(v[i]) < parseInt(range[i])) return false;
    return true;
  }

  return prosperon.semver.cmp(v.join('.'), range.join('.')) === 0;
}

prosperon.semver.cmp = function(v1, v2)
{
  var ver1 = v1.split('.');
  var ver2 = v2.split('.');

  for (var i = 0; i < 3; i++) {
    var n1 = parseInt(ver1[i]);
    var n2 = parseInt(ver2[i]);
    if (n1 > n2)
      return 1;
    else if (n1 < n2)
      return -1;
  }
  
  return 0;
}

prosperon.semver.doc = "Functions for semantic versioning numbers. Semantic versioning is given as a triple digit number, as MAJOR.MINOR.PATCH.";
prosperon.semver.cmp.doc = "Compare two semantic version numbers, given like X.X.X.";
prosperon.semver.valid.doc = `Test if semantic version v is valid, given a range.
Range is given by a semantic versioning number, prefixed with nothing, a ~, or a ^.
~ means that MAJOR and MINOR must match exactly, but any PATCH greater or equal is valid.
^ means that MAJOR must match exactly, but any MINOR and PATCH greater or equal is valid.`;


load("scripts/gui.js");

var timer = {
  update(dt) {
    this.remain -= dt;
    if (this.remain <= 0) {
      this.fn();
      this.kill();
    }
  },

  kill() {
    this.end();
    this.fn = undefined;
  },
  
  delay(fn, secs) {
    var t = Object.create(this);
    t.time = secs;
    t.remain = secs;
    t.fn = fn;
    t.end = Register.update.register(timer.update.bind(t));
    return function() { t.kill(); };
  },
};

load("scripts/tween.js");

var render = {
  normal() { cmd(67);},
  wireframe() { cmd(68); },
  pass() { },
};

render.doc = {
  doc: "Functions for rendering modes.",
  normal: "Final render with all lighting.",
  wireframe: "Show only wireframes of models."
};

render.device = {
  pc: [1920,1080],
  macbook_m2: [2560,1664, 13.6],
  ds_top: [400,240, 3.53],
  ds_bottom: [320,240, 3.02],
  playdate: [400,240,2.7],
  switch: [1280,720, 6.2],
  switch_lite: [1280,720,5.5],
  switch_oled: [1280,720,7],
  dsi: [256,192,3.268],
  ds: [256,192, 3],
  dsixl: [256,192,4.2],
  ipad_air_m2: [2360,1640, 11.97],
  iphone_se: [1334, 750, 4.7],
  iphone_12_pro: [2532,1170,6.06],
  iphone_15: [2556,1179,6.1],
  gba: [240,160,2.9],
  gameboy: [160,144,2.48],
  gbc: [160,144,2.28],
  steamdeck: [1280,800,7],
  vita: [960,544,5],
  psp: [480,272,4.3],
  imac_m3: [4480,2520,23.5],
  macbook_pro_m3: [3024,1964, 14.2],
  ps1: [320,240,5],
  ps2: [640,480],
  snes: [256,224],
  gamecube: [640,480],
  n64: [320,240],
  c64: [320,200],
  macintosh: [512,342,9],
  gamegear: [160,144,3.2],
};

render.device.doc = `Device resolutions given as [x,y,inches diagonal].`;

load("scripts/physics.js");
load("scripts/input.js");
load("scripts/sound.js");
load("scripts/ai.js");
load("scripts/geometry.js");

var Register = {
  kbm_input(mode, btn, state, ...args) {
    if (state === 'released') {
      btn = btn.split('-').last();
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

    input.action.actions.forEach(x => {
      if (x.inputs.includes(btn))
        player.input(`action_${x.name}_${statestr}`, ...args);
    });
  },
  
  unregister_obj(obj) { Player.uncontrol(obj); },

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
    var n = {};
    var fns = [];
    
    n.register = function(fn, obj) {
      if (typeof fn !== 'function') return;
      if (typeof obj === 'object')
        fn = fn.bind(obj);
      fns.push(fn);
      return function() { fns.remove(fn); };
    }
    n.broadcast = function(...args) { fns.forEach(x => x(...args)); }
    n.clear = function() { fns = []; }

    register(idx, n.broadcast, n);

    Register[name] = n;
    Register.registries.push(n);
    
    return n;
  },
};

Register.add_cb(0, "update").doc = "Called once per frame.";
Register.add_cb(11, "appupdate");
Register.add_cb(1, "physupdate");
Register.add_cb(2, "gui");
Register.add_cb(6, "debug");
register(7, Register.kbm_input, Register);
Register.add_cb(8, "gamepad_input");
Register.add_cb(10, "draw");

register(9, console.stack, this);

Register.gamepad_playermap[0] = Player.players[0];

var Event = {
  events: {},

  observe(name, obj, fn) {
    this.events[name] ??= [];
    this.events[name].push([obj, fn]);
  },

  unobserve(name, obj) {
    this.events[name] = this.events[name].filter(x => x[0] !== obj);
  },

  rm_obj(obj) {
    Object.keys(this.events).forEach(name => Event.unobserve(name,obj));
  },

  notify(name) {
    if (!this.events[name]) return;
    this.events[name].forEach(function(x) {
      x[1].call(x[0]);
    });
  },
};

var Window = {
  fullscreen(f) { cmd(145, f); },
  set width(w) { cmd(125, w); },
  set height(h) { cmd(126, h); },
  get width() { return cmd(48); },
  get height() { return cmd(49); },
  get dimensions() { return [this.width, this.height]; },
  title(str) { cmd(134, str); },
  boundingbox() {
    return {
      t: Window.height,
      b: 0,
      r: Window.width,
      l: 0
    };
  },
};

Window.screen2world = function(screenpos) {
  if (Game.camera)
    return Game.camera.view2world(screenpos);
  return screenpos;
}
Window.world2screen = function(worldpos) { return Game.camera.world2view(worldpos); }

Window.icon = function(path) { cmd(90, path); };
Window.icon.doc = "Set the icon of the window using the PNG image at path.";

load("scripts/debug.js");
load('scripts/spline.js');
load("scripts/components.js");

var Game = {
  engine_start(fn) {
    cmd(257, fn);
    Sound.bus.master = cmd(180);
    Sound.master = Sound.bus.master;    
  },

  native: render.device.pc,

  object_count() {
    return cmd(214);
  },

  all_objects(fn) {
    /* Wind down from Primum */
  },

  /* Returns a list of objects by name */
  find(name) {
    
  },

  /* Return a list of objects derived from a specific prototype */
  find_proto(proto) {

  },

  /* List of all objects spawned that have a specific tag */
  find_tag(tag){

  },

  quit() { sys_cmd(0); },
  pause() { sys_cmd(3); },
  stop() { Game.pause(); },
  step() { sys_cmd(4);},
  editor_mode(m) { sys_cmd(10, m); },
  playing() { return sys_cmd(5); },
  paused() { return sys_cmd(6); },
  stepping() { return cmd(79); },
  play() { sys_cmd(1); },
  
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

Game.gc = function() { cmd(259); }
Game.gc.doc = "Force the garbage collector to run.";

Game.doc = {};
Game.doc.object = "Returns the entity belonging to a given id.";
Game.doc.quit = "Immediately quit the game.";
Game.doc.pause = "Pause game simulation.";
Game.doc.stop = "Stop game simulation. This does the same thing as 'pause', and if the game is a debug build, starts its editor.";
Game.doc.play = "Resume or start game simulation.";
Game.doc.editor_mode = "Set to true for the game to only update on input; otherwise the game updates every frame.";
Game.doc.dt = "Current frame dt.";
Game.doc.view_camera = "Set the camera for the current view.";
Game.doc.camera = "Current camera.";

Window.doc = {};
Window.doc.width = "Width of the game window.";
Window.doc.height = "Height of the game window.";
Window.doc.dimensions = "Window width and height packaged in an array [width,height]";
Window.doc.name = "Name in the title bar of the window.";
Window.doc.boundingbox = "Boundingbox of the window, with top and right being its height and width.";

Register.update.register(Game.exec, Game);

load("scripts/entity.js");

function world_start() {
  globalThis.Primum = Object.create(gameobject);
  Primum.objects = {};
  Primum.check_dirty = function() {};
  Primum.namestr = function(){};
  Primum._ed = {
    selectable:false,
    dirty:false,
  };
  Primum.toString = function() { return "Primum"; };
  Primum.ur = "Primum";
  Primum.kill = function() { this.clear(); };
  Primum.phys = 2;
  
  gameobject.level = Primum;
  gameobject.body = make_gameobject();
  cmd(113,gameobject.body, gameobject);
  Object.hide(gameobject, 'timescale');
}

load("scripts/physics.js");

Game.view_camera = function(cam)
{
  Game.camera = cam;
  cmd(61, Game.camera.body);
}

Window.title(`Prosperon v${prosperon.version}`);
Window.width = 1280;
Window.height = 720;
