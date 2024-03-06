"use math";
globalThis.global = globalThis;

function use(file)
{
  if (use.files[file]) return use.files[file];
    
  var c = io.slurp(file);
  
  var script = `(function() { ${c} })();`;
  use.files[file] = cmd(123,script,global,file);

  return use.files[file];
}
use.files = {};

function include(file,that)
{
  if (!that) return;
  var c = io.slurp(file);
  eval_env(c, that, file);
}

function eval_env(script, env, file)
{
  env ??= {};
  file ??= "SCRIPT";
  script = `(function() { ${script} }).call(this);`;
  return cmd(123,script,env,file);
}

global.check_registers = function(obj)
{
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

}


eval_env.dov = `Counterpart to /load_env/, but with a string.`;

function feval_env(file, env)
{
  eval_env(io.slurp(file), env, file);
}

function load_env(file,env)
{
  env ??= global;
//  var script = io.slurp(file);
  var script = io.slurp(file);
  eval_env(script, env, file);
//  cmd(16, file, env);
//  var script = io.slurp(file);
//  cmd(123, script, env, file);
}
load_env.doc = `Load a given file with 'env' as **this**. Does not add to the global namespace.`;

var load = use;

Object.assign(global, use("scripts/base.js"));
global.obscure('global');
global.mixin("scripts/render.js");

global.Game = {
  engine_start(fn) {
    cmd(257, fn);
  },

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

  quit() {
    sys_cmd(0);
    return;
  },

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

  set width(w) { cmd(125, w); },
  set height(h) { cmd(126, h); },
  get width() { return cmd(48); },
  get height() { return cmd(49); },
  dimensions() { return [this.width,this.height]; },
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

global.mixin("scripts/input.js");
global.mixin("scripts/std.js");

global.mixin("scripts/diff.js");

console.level = 1;

global.mixin("scripts/color.js");

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


global.mixin("scripts/gui.js");

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

global.mixin("scripts/tween.js");

global.mixin("scripts/physics.js");


global.mixin("scripts/ai.js");
global.mixin("scripts/geometry.js");

var Register = {
  kbm_input(mode, btn, state, ...args) {
    if (state === 'released') {
      btn = btn.split('-').last();
    }
   
    switch(mode) {
      case "emacs":
        player[0].raw_input(btn, state, ...args);
        break;

      case "mouse":
        player[0].mouse_input(btn, state, ...args);
	break;

      case "char":
        player[0].char_input(btn);
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

// Window

var Window = {
  fullscreen(f) { cmd(145, f); },
  dimensions() { return cmd(265); },
  get width() { return this.dimensions().x; },
  get height() { return this.dimensions().y; },
  mode: {
    stretch: 0,
    keep: 1,
    width: 2,
    height: 3,
    expand: 4,
    full: 5
  },
  aspect(x) { cmd(264, x); },
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

Game.width = 1920;
Game.height = 1080;

Window.screen2world = function(screenpos) {
  if (Game.camera)
    return Game.camera.view2world(screenpos);
    
  return screenpos;
}

Window.world2screen = function(worldpos) {
  return Game.camera.world2view(worldpos);
}

Window.icon = function(path) { cmd(90, path); };
Window.icon.doc = "Set the icon of the window using the PNG image at path.";

global.mixin("scripts/debug.js");
global.mixin("scripts/spline.js");
global.mixin("scripts/components.js");

Window.doc = {};
Window.doc.width = "Width of the game window.";
Window.doc.height = "Height of the game window.";
Window.doc.dimensions = "Window width and height packaged in an array [width,height]";
Window.doc.title = "Name in the title bar of the window.";
Window.doc.boundingbox = "Boundingbox of the window, with top and right being its height and width.";

Register.update.register(Game.exec, Game);

global.mixin("scripts/actor.js");
global.mixin("scripts/entity.js");

function world_start() {
  globalThis.world = Object.create(gameobject);
  world.objects = {};
  world.check_dirty = function() {};
  world.namestr = function(){};
  world._ed = {
    selectable:false,
    dirty:false,
  };
  world.toString = function() { return "world"; };
  world.master = gameobject;
  world.ur = "world";
  world.kill = function() { this.clear(); };
  world.phys = 2;
  
  gameobject.level = world;
  gameobject.body = make_gameobject();
  cmd(113,gameobject.body, gameobject);
  Object.hide(gameobject, 'timescale');
  var cam = world.spawn("scripts/camera2d.jso");
  Game.view_camera(cam);
}

global.mixin("scripts/physics.js");

Game.view_camera = function(cam)
{
  Game.camera = cam;
  cmd(61, Game.camera.body);
}

Window.title(`Prosperon v${prosperon.version}`);
