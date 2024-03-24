"use math";

globalThis.Resources = {};
Resources.scripts = ["jsoc", "jsc", "jso", "js"];
Resources.images = ["png", "gif", "jpg", "jpeg"];
Resources.sounds =  ["wav", 'flac', 'mp3', "qoa"];
Resources.is_image = function(path) {
  var ext = path.ext();
  return Resources.images.any(x => x === ext);
}

function find_ext(file, ext)
{
  if (io.exists(file)) return file;
  for (var e of ext) {
    var nf = `${file}.${e}`;
    if (io.exists(nf)) return nf;
  }
  return;
}

Resources.find_image = function(file) { return find_ext(file,Resources.images); }
Resources.find_sound = function(file) { return find_ext(file,Resources.sounds); }
Resources.find_script = function(file) { return find_ext(file,Resources.scripts); }

profile.best_t = function(t) {
  var qq = 'ns';
  if (t > 1000) {
    t /= 1000;
    qq = 'us';
    if (t > 1000) {
      t /= 1000;
qq = 'ms';
    }
  }
  return `${t.toPrecision(4)} ${qq}`;
}

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

console.stdout_lvl = 1;
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

function use(file, env, script)
{
  file = Resources.find_script(file);
  var st = profile.now();
  env ??= {};  
  
  if (use.cache[file]) {
    var ret = use.cache[file].call(env);
    console.info(`CACHE eval ${file} in ${profile.best_t(profile.now()-st)}`);
    return;
  }
  
  script ??= io.slurp(file);
  script = `(function() { ${script}; })`;
  var fn = os.eval(file,script);
  use.cache[file] = fn;
  var ret = fn.call(env);
  console.info(`eval ${file} in ${profile.best_t(profile.now()-st)}`);  
  return ret;
}

use.cache = {};

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

Object.assign(global, use("scripts/base"));
global.obscure('global');
global.mixin("scripts/render");
global.mixin("scripts/debug");

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
  gggstart(function() {
      world_start();
      go_init();
      window.set_icon(os.make_texture("icons/moon.gif"))
      s();
  }, process);  
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

  if (!game.camera)  
    prosperon.window_render(world, 1);
  else
    prosperon.window_render(game.camera, game.camera.zoom);
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
game.doc.play = "Resume or start game simulation.";
game.doc.dt = "Current frame dt.";
game.doc.camera = "Current camera.";

game.texture = function(path)
{
  game.texture.cache[path] ??= os.make_texture(path);  
  return game.texture.cache[path];
}
game.texture.cache = {};


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

global.mixin("scripts/input");
global.mixin("scripts/std");
global.mixin("scripts/diff");
global.mixin("scripts/color");
global.mixin("scripts/gui");

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

global.mixin("scripts/physics");
global.mixin("scripts/geometry");

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

global.mixin("scripts/spline");
global.mixin("scripts/components");

window.doc = {};
window.doc.width = "Width of the game window.";
window.doc.height = "Height of the game window.";
window.doc.dimensions = "Window width and height packaged in an array [width,height]";
window.doc.title = "Name in the title bar of the window.";
window.doc.boundingbox = "Boundingbox of the window, with top and right being its height and width.";

global.mixin("scripts/actor");
global.mixin("scripts/entity");

function world_start() {
  globalThis.world = os.make_gameobject();
  world.setref(world);
  world.objects = {};
  world.toString = function() { return "world"; };
  world.ur = "world";
  world.kill = function() { this.clear(); };
  world.phys = 2;
  world.zoom = 1;
  game.cam = world;
}

global.mixin("scripts/physics");

window.title = `Prosperon v${prosperon.version}`;
window.size = [500,500];
window.boundingbox = function() {
  var pos = game.camera.pos;
  var wh = window.rendersize.scale(game.camera.zoom);
  return bbox.fromcwh(pos,wh);
}
