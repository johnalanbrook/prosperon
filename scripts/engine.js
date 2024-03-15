"use math";

Object.assign(console, {
  say(msg) { console.print(msg + "\n"); },
  
  pprint(msg, lvl = 0) {
    if (typeof msg === 'object')
      msg = JSON.stringify(msg, null, 2);

    var file = "nofile";
    var line = 0;
    
    var caller = (new Error()).stack.split('\n')[2];
    if (caller) {
      var m = caller.match(/\((.*)\:/)[1];
      if (m) file = m;
      m = caller.match(/\:(\d*)\)/)[1];
      if (m) line = m;
    }
    
    console.rec(lvl, msg, file, line);
  },

  spam(msg) { console.pprint(msg,0); },
  debug(msg) { console.pprint(msg,1); },
  info(msg) { console.pprint(msg, 2); },
  warn(msg) { console.pprint(msg, 3); },
  error(msg) { console.pprint(msg + "\n" + console.stackstr(2), 4);},
  panic(msg) { console.pprint(msg + "\n" + console.stackstr(2), 5); },

  stackstr(skip=0) {
    var err = new Error();
    var stack = err.stack.split('\n');
    return stack.slice(skip,stack.length).join('\n');
  },

  stack(skip = 0) {
    console.log(console.stackstr(skip+1));
  },
});

console.log = console.say;
var say = console.say;
var print = console.print;

console.doc = {
  level: "Set level to output logging to console.",
  info: "Output info level message.",
  warn: "Output warn level message.",
  error: "Output error level message, and print stacktrace.",
  critical: "Output critical level message, and exit game immediately.",
  write: "Write raw text to console.",
  say: "Write raw text to console, plus a newline.",
  stack: "Output a stacktrace to console.",
  console: "Output directly to in game console.",
  clear: "Clear console."
};

globalThis.global = globalThis;

function use(file)
{
  if (use.files[file]) return use.files[file];
  console.info(`running ${file}`);
    
  var c = io.slurp(file);
  
  var script = `(function() { ${c} })();`;
  use.files[file] = cmd(123,file,global,script);

  return use.files[file];
}
use.files = {};

function include(file,that)
{
  if (!that) return;
  console.info(`running ${file}`);
  var c = io.slurp(file);
  eval_env(c, that, file);
}

function eval_env(script, env, file)
{
  env ??= {};
  file ??= "SCRIPT";
  console.info(`eval ${file}`);  
  script = `(function() { ${script}; }).call(this);\n`;
  return cmd(123,file,env,script);
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
global.mixin("scripts/debug.js");

var frame_t = profile.secs();
var updateMS = 1/60;
var physMS = 1/60;

var sim = {
  mode: "play",
  play() { this.mode = "play"; os.reindex_static(); },
  playing() { return this.mode === 'play'; },
  pause() { this.mode = "pause"; console.stack(); },
  paused() { return this.mode === 'pause'; },
  step() { this.mode = 'step'; },
  stepping() { return this.mode === 'step'; }
}

var physlag = 0;
var timescale = 1;

function process()
{
  var dt = profile.secs() - frame_t;
  frame_t = profile.secs();
  
  prosperon.appupdate(dt);
  prosperon.emitters_step(dt);
  
  if (sim.mode === "play" || sim.mode === "step") {
    prosperon.update(dt*game.timescale);
    if (sim.mode === "step")
      sim.pause();
  }
  
  physlag += dt;
  
  while (physlag > physMS) {
    physlag -= physMS;
    prosperon.phys2d_step(physMS*timescale);
    prosperon.physupdate(physMS*timescale);
  }
  prosperon.window_render();
}

global.Game = {
  engine_start(fn) {
    console.info("Starting rendering and sound ...");
    cmd(257, fn, process);
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

prosperon.iconified = function(icon) {};
prosperon.focus = function(focus) {};
prosperon.resize = function(dimensions) {};
prosperon.suspended = function(sus) {};
prosperon.mouseenter = function(){};
prosperon.mouseleave = function(){};
prosperon.touchpress = function(touches){};
prosperon.touchrelease = function(touches){};
prosperon.touchmove = function(touches){};
prosperon.clipboardpaste = function(str){};
prosperon.quit = function(){};

global.mixin("scripts/input.js");
global.mixin("scripts/std.js");
global.mixin("scripts/diff.js");
global.mixin("scripts/color.js");
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
    delete this.fn;
  },
  
  delay(fn, secs) {
    var t = Object.create(this);
    t.time = secs;
    t.remain = secs;
    t.fn = fn;
    t.end = Register.update.register(timer.update.bind(t));
    var returnfn = timer.kill.bind(t);
    returnfn.remain = secs;
    return returnfn;
  },
};

global.mixin("scripts/tween.js");
global.mixin("scripts/physics.js");
global.mixin("scripts/ai.js");
global.mixin("scripts/geometry.js");

/*
Factory for creating registries. Register one with 'X.register',
which returns a function that, when invoked, cancels the registry.
*/
var Register = {
  registries: [],

  add_cb(name) {
    var n = {};
    var fns = [];
    
    n.register = function(fn, obj) {
      if (typeof fn !== 'function') return;
      if (typeof obj === 'object')
        fn = fn.bind(obj);
      fns.push(fn);
      return function() { fns.remove(fn); };
    }
    prosperon[name] = function(...args) { fns.forEach(x => x(...args)); }
    n.clear = function() { fns = []; }

    Register[name] = n;
    Register.registries.push(n);
    
    return n;
  },
};

Register.add_cb("appupdate");
Register.add_cb("update").doc = "Called once per frame.";
Register.add_cb("physupdate");
Register.add_cb("gui");
Register.add_cb("debug");
Register.add_cb("draw");

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

Window.modetypes = { 
  stretch: 0, // stretch to fill window
  keep: 1, // keep exact dimensions
  width: 2, // keep width
  height: 3, // keep height
  expand: 4, // expand width or height
  full: 5 // expand out beyond window
};

Window.size = [640, 480];

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

Window.title = `Prosperon v${prosperon.version}`;
Window.size = [500,500];
