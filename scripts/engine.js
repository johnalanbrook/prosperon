"use math";
var files = {};
function load(file) {
   var modtime = cmd(0, file);
  files[file] = modtime;
}

load("scripts/base.js");
load("scripts/std.js");

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

function run_env(file, env)
{
  var script = IO.slurp(file);
  return function(){return eval(script);}.call(env);
}

load("scripts/diff.js");
Log.level = 1;

load("scripts/color.js");

function bb2wh(bb) {
  return [bb.r-bb.l, bb.t-bb.b];
};

var Device = {
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

load("scripts/gui.js");

var timer = {
  update(dt) {
    this.remain -= dt;
    if (this.remain <= 0)
      this.fire();
  },

  kill() {
    Register.unregister_obj(this);
  },
  
  delay(fn, secs) {
    var t = Object.create(this);
    t.time = secs;
    t.remain = secs;
    t.fire = function() { fn(); t.kill(); };
    Register.update.register(t.update, t);
    return function() { t.kill(); };
  },
};

load("scripts/animation.js");

var Render = {
  normal() {
    cmd(67);
  },
  
  wireframe() {
    cmd(68);
  },
};

Render.doc = {
  doc: "Functions for rendering modes.",
  normal: "Final render with all lighting.",
  wireframe: "Show only wireframes of models."
};

load("scripts/physics.js");
load("scripts/input.js");
load("scripts/sound.js");
load("scripts/ai.js");

function screen2world(screenpos) {
  if (Game.camera)
    return Game.camera.view2world(screenpos);
  return screenpos;
}
function world2screen(worldpos) { return Game.camera.world2view(worldpos); }

var Register = {
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
      entries = entries.filter(function(e) { return e.fn !== fn; });
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
Register.add_cb(11, "appupdate");
Register.add_cb(1, "physupdate");
Register.add_cb(2, "gui");
Register.add_cb(6, "debug");
register(7, Register.kbm_input, Register);
Register.add_cb(8, "gamepad_input");
Register.add_cb(10, "draw");

register(9, Log.stack, this);

Register.gamepad_playermap[0] = Player.players[0];

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

  clear_obj(obj) {
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

var Event = {
  events: {},

  observe(name, obj, fn) {
    this.events[name] ??= [];
    this.events[name].push([obj, fn]);
  },

  unobserve(name, obj) {
    this.events[name] = this.events[name].filter(x => x[0] !== obj);
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

var Spline = {};
Spline.sample = function(degrees, dimensions, type, ctrl_points, nsamples)
{
  var s = spline_cmd(0, degrees,dimensions,type,ctrl_points,nsamples);
  return s;
}
Spline.type = {
  open: 0,
  clamped: 1,
  beziers: 2
};

load("scripts/components.js");

var Game = {
  init() {
    if (!Game.edit) {
      load("config.js");
      load("game.js");
    }
    else {
      load("scripts/editor.js");
      load("editorconfig.js");
      editor.enter_editor();
    }
  },

  native: Device.pc,

  edit: true,

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
  find_tag(tag) {

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
  Primum._ed = {
    selectable:false,
    check_dirty() {},
    dirty:false,
    namestr(){},
  };
  Primum.toString = function() { return "Primum"; };
  Primum.ur = undefined;
}

load("scripts/physics.js");

Game.view_camera = function(cam)
{
  Game.camera = cam;
  cmd(61, Game.camera.body);
//  cam.zoom = cam.zoom;
}

Window.name = "Primum Machinam (V0.1)";
Window.width = 1280;
Window.height = 720;

var Asset = {};
Asset.doc = {
  doc: "Functions to manage the loading and unloading of assets, like sounds and images."
};
