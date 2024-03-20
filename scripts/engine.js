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
      var md = caller.match(/\((.*)\:/);
      var m = md ? md[1] : "SCRIPT";
      if (m) file = m;
      md = caller.match(/\:(\d*)\)/);
      m = md ? md[1] : 0;
      if (m) line = m;
    }
    
    console.rec(lvl, msg, file, line);
  },

  spam(msg) { console.pprint (msg,0); },
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
  use.files[file] = os.eval_env(file,script,global);

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
  console.spam(`eval ${file}`);  
  script = `(function() { ${script}; }).call(this);\n`;
  return os.eval_env(file,script,env);
}

global.check_registers = function(obj)
{
    if (typeof obj.update === 'function')
      obj.timers.push(Register.update.register(obj.update.bind(obj)));

    if (typeof obj.physupdate === 'function')
      obj.timers.push(Register.physupdate.register(obj.physupdate.bind(obj)));

    if (typeof obj.collide === 'function')
      physics.collide_begin(obj.collide.bind(obj), obj);

    if (typeof obj.separate === 'function')
      physics.collide_separate(obj.separate.bind(obj), obj);

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
  var script = io.slurp(file);
  eval_env(script, env, file);
}

load_env.doc = `Load a given file with 'env' as **this**. Does not add to the global namespace.`;

var load = use;

Object.assign(global, use("scripts/base.js"));
global.obscure('global');
global.mixin("scripts/render.js");
global.mixin("scripts/debug.js");

var frame_t = profile.secs(profile.now());
var phys_step = 1/60;

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

var gggstart = game.engine_start;
game.engine_start = function(s) {
  gggstart(function() { world_start(); s(); }, process);  
}

function process()
{
  var dt = profile.secs(profile.now()) - frame_t;
  frame_t = profile.secs(profile.now());
  
  prosperon.appupdate(dt);
  prosperon.emitters_step(dt);
  input.procdown();
  
  if (sim.mode === "play" || sim.mode === "step") {
    prosperon.update(dt*game.timescale);
    if (sim.mode === "step")
      sim.pause();
  }
  
  physlag += dt;
  
  while (physlag > phys_step) {
    physlag -= phys_step;
    prosperon.phys2d_step(phys_step*timescale);
    prosperon.physupdate(phys_step*timescale);
  }

  prosperon.window_render();
  render.sprites();
  render.models();
  render.emitters();
  prosperon.draw();
  debug.draw();
  render.flush();
  render.pass();
  prosperon.gui();
  render.flush_hud();

  render.end_pass();
  render.commit();
}

game.timescale = 1;

var eachobj = function(obj,fn)
{
  fn(obj);
  for (var o in obj.objects)
    eachobj(obj.objects[o],fn);
}

game.all_objects = function(fn) { eachobj(world,fn); };

game.doc = {};
game.doc.object = "Returns the entity belonging to a given id.";
game.doc.pause = "Pause game simulation.";
game.doc.stop = "Stop game simulation. This does the same thing as 'pause', and if the game is a debug build, starts its editor.";
game.doc.play = "Resume or start game simulation.";
game.doc.editor_mode = "Set to true for the game to only update on input; otherwise the game updates every frame.";
game.doc.dt = "Current frame dt.";
game.doc.view_camera = "Set the camera for the current view.";
game.doc.camera = "Current camera.";

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
prosperon.quit = function(){
  for (var i in debug.log.time)
    console.warn(debug.log.time[i].map(x=>profile.ms(x)));
};

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
      return function() {
        console.spam(`removed from ${name}.`);
        fns.remove(fn);
      };
    }
    prosperon[name] = function(...args) { fns.forEach(x => x(...args)); }
    prosperon[name].fns = fns;
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

window.modetypes = { 
  stretch: 0, // stretch to fill window
  keep: 1, // keep exact dimensions
  width: 2, // keep width
  height: 3, // keep height
  expand: 4, // expand width or height
  full: 5 // expand out beyond window
};

window.size = [640, 480];

window.screen2world = function(screenpos) {
  if (game.camera)
    return game.camera.view2world(screenpos);
    
  return screenpos;
}

window.world2screen = function(worldpos) {
  return game.camera.world2view(worldpos);
}

window.set_icon.doc = "Set the icon of the window using the PNG image at path.";

global.mixin("scripts/spline.js");
global.mixin("scripts/components.js");

window.doc = {};
window.doc.width = "Width of the game window.";
window.doc.height = "Height of the game window.";
window.doc.dimensions = "Window width and height packaged in an array [width,height]";
window.doc.title = "Name in the title bar of the window.";
window.doc.boundingbox = "Boundingbox of the window, with top and right being its height and width.";

global.mixin("scripts/actor.js");
global.mixin("scripts/entity.js");

function world_start() {
  globalThis.world = os.make_gameobject();
  world.setref(world);
  world.objects = {};
  world.toString = function() { return "world"; };
  world.ur = "world";
  world.kill = function() { this.clear(); };
  world.phys = 2;
  var cam = world.spawn("scripts/camera2d.jso");
  game.view_camera(cam);
}

global.mixin("scripts/physics.js");

game.view_camera = function(cam)
{
  game.camera = cam;
  render.cam_body(game.camera);
}

window.title = `Prosperon v${prosperon.version}`;
window.size = [500,500];
