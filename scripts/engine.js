"use math";

Object.defineProperty(String.prototype, 'tolast', {
  value: function(val) {
    var idx = this.lastIndexOf(val);
    if (idx === -1) return this.slice();
    return this.slice(0,idx);
  }
});

Object.defineProperty(String.prototype, 'dir', {
  value: function() {
    if (!this.includes('/')) return "";
    return this.tolast('/');
  }
});

Object.defineProperty(String.prototype, 'folder', {
  value: function() {
    var dir = this.dir();
    if (!dir) return "";
    else return dir + "/";
  }
});

globalThis.Resources = {};

Resources.replpath = function(str, path)
{
  if (!str) return str;
  if (str[0] === "/")
    return str.rm(0);

  if (str[0] === "@")
    return os.prefpath() + "/" + str.rm(0);

  if (!path) return str;
  
  var stem = path.dir();
  while (stem) {
    var tr = stem + "/" +str;
    if (io.exists(tr)) return tr;
    stem = stem.updir();
  }
  
  return str;
}

Resources.replstrs = function(path)
{
  if (!path) return;
  var script = io.slurp(path);
  var regexp = /"[^"\s]*?\.[^"\s]+?"/g;
  var stem = path.dir();

  script = script.replace(regexp,function(str) {
    var newstr = Resources.replpath(str.trimchr('"'), path);
    return `"${newstr}"`;
  });

  return script;
}

globalThis.json = {};
json.encode = function(value, replacer, space = 1)
{
  return JSON.stringify(value, replacer, space);
}

json.decode = function(text, reviver)
{
  if (!text) return undefined;
  return JSON.parse(text,reviver);
}

json.readout = function(obj)
{
  var j = {};
  for (var k in obj)
    if (typeof obj[k] === 'function')
      j[k] = 'function ' + obj[k].toString();
    else
      j[k] = obj[k];

  return json.encode(j);
}

json.doc = {
  doc: "json implementation.",
  encode: "Encode a value to json.",
  decode: "Decode a json string to a value.",
  readout: "Encode an object fully, including function definitions."
};

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

profile.report = function(start, msg = "[undefined report]")
{
  console.info(`${msg} in ${profile.best_t(profile.now()-start)}`);
}

profile.addreport = function(cache, line, start)
{
  cache[line] ??= [];
  cache[line].push(profile.now()-start);
}

profile.printreport = function(cache, name)
{
  var report = name + "\n";
  for (var i in cache) {
    report += `${i}    ${profile.best_t(profcache[i].reduce((a,b) => a+b)/profcache[i].length)}\n`;
  }
  
  return report;
}

console.transcript = "";
console.say = function(msg) {
  msg += "\n";
  console.print(msg);
  console.transcript += msg;
};
console.log = console.say;
globalThis.say = console.say;
globalThis.print = console.print;

console.pprint = function(msg,lvl = 0) {  

  if (typeof msg === 'object')
    msg = JSON.stringify(msg, null, 2);

  var file = "nofile";
  var line = 0;
  console.rec(0,msg,file,line);
  
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
};

console.spam = function(msg) { console.pprint (msg,0); };
console.debug = function(msg) { console.pprint(msg,1); };
console.info = function(msg) { console.pprint(msg, 2); };
console.warn = function(msg) { console.pprint(msg, 3); };
console.error = function(msg) { console.pprint(msg + "\n" + console.stackstr(2), 4);};
console.panic = function(msg) { console.pprint(msg + "\n" + console.stackstr(2), 5); };
console.stackstr = function(skip=0) {
  var err = new Error();
  var stack = err.stack.split('\n');
  return stack.slice(skip,stack.length).join('\n');
};

console.stack = function(skip = 0) { console.log(console.stackstr(skip+1)); };

console.stdout_lvl = 1;
console.trace = console.stack;

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

var profcache = {};

function use(file, env = {}, script)
{
  file = Resources.find_script(file);
  var st = profile.now();
  
  profcache[file] ??= [];
  
  if (use.cache[file]) {
    var ret = use.cache[file].call(env);
    profile.addreport(profcache, file, st);
    return;
  }
  script ??= Resources.replstrs(file);
  script = `(function() { var self = this; ${script}; })`;
  var fn = os.eval(file,script);
  use.cache[file] = fn;
  var ret = fn.call(env);
  profile.addreport(profcache, file, st);
  return ret;
}

use.cache = {};

global.check_registers = function(obj)
{
    if (typeof obj.update === 'function')
      obj.timers.push(Register.update.register(obj.update.bind(obj)));

    if (typeof obj.physupdate === 'function')
      obj.timers.push(Register.physupdate.register(obj.physupdate.bind(obj)));

    if (typeof obj.draw === 'function')
      obj.timers.push(Register.draw.register(obj.draw.bind(obj), obj));

    if (typeof obj.debug === 'function')
      obj.timers.push(Register.debug.register(obj.debug.bind(obj)));

    if (typeof obj.gui === 'function')
      obj.timers.push(Register.gui.register(obj.gui.bind(obj)));
    
    if (typeof obj.screengui === 'function')
      obj.timers.push(Register.screengui.register(obj.screengui.bind(obj)));

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
var phys_step = 1/240;

var sim = {};
sim.mode = "play";
sim.play = function() { this.mode = "play"; os.reindex_static(); };
sim.playing = function() { return this.mode === 'play'; };
sim.pause = function() { this.mode = "pause"; };
sim.paused = function() { return this.mode === 'pause'; };
sim.step = function() { this.mode = 'step'; };
sim.stepping = function() { return this.mode === 'step'; }

var physlag = 0;

var gggstart = game.engine_start;
game.engine_start = function(s) {
  game.startengine = 1;
  gggstart(function() {
    global.mixin("scripts/sound.js");
    world_start();
    window.set_icon(os.make_texture("icons/moon.gif"))
    Object.readonly(window.__proto__, 'vsync');
    Object.readonly(window.__proto__, 'enable_dragndrop');
    Object.readonly(window.__proto__, 'enable_clipboard');
    Object.readonly(window.__proto__, 'high_dpi');
    Object.readonly(window.__proto__, 'sample_count');
    s();
  }, process);  
}

game.startengine = 0;
var frames = [];

function process()
{
  var startframe = profile.now();
  var dt = profile.secs(profile.now()) - frame_t;
  frame_t = profile.secs(profile.now());
  
  prosperon.appupdate(dt);
  prosperon.emitters_step(dt);
  input.procdown();
  
  if (sim.mode === "play" || sim.mode === "step") {
    prosperon.update(dt*game.timescale);
    if (sim.mode === "step")
      sim.pause();

    physlag += dt;

    while (physlag > phys_step) {
      physlag -= phys_step;
      var st = profile.now();
      prosperon.phys2d_step(phys_step*game.timescale);
      prosperon.physupdate(phys_step*game.timescale);
      profile.addreport(profcache, "physics step", st);
    }
  }
  var st = profile.now();
  if (!game.camera)  
    prosperon.window_render(world.transform, 1);
  else
    prosperon.window_render(game.camera.transform, game.camera.zoom);
    
  render.set_camera();
  
  /*os.sprite_pipe();
  allsprites.forEach(function(x) {
    render.set_sprite_tex(x.texture);    
    x.draw(x.go);
    render.sprite_flush();                
  });
  render.sprite_flush();*/
  render.emitters(); // blits emitters
  prosperon.draw(); // draw calls
  debug.draw(); // calls needed debugs
  render.flush();
  
  prosperon.hook3d?.();
  
  render.hud_res(window.rendersize);
  prosperon.gui();
  render.flush();
  render.hud_res(window.size);
  prosperon.screengui();
  render.flush();
  
  render.end_pass();
  profile.addreport(profcache, "render frame", st);
  frames.push(profile.secs(profile.now()-startframe));
  if (frames.length > 20) frames.shift();
}

globalThis.fps = function()
{
  var sum = 0;
  for (var i = 0; i < frames.length; i++)
    sum += frames[i];
  return frames.length/sum; 
}

game.timescale = 1;

var eachobj = function(obj,fn)
{
  var val = fn(obj);
  if (val) return val;
  for (var o in obj.objects) {
    if (obj.objects[o] === obj)
      console.error(`Object ${obj.toString()} is referenced by itself.`);
    val = eachobj(obj.objects[o],fn);
    if (val) return val;
  }
}

game.all_objects = function(fn, startobj = world) { return eachobj(startobj,fn); };
game.find_object = function(fn, startobj = world) {

}

game.tags = {};
game.tag_add = function(tag, obj) {
  game.tags[tag] ??= {};
  game.tags[tag][obj.guid] = obj;
}

game.tag_rm = function(tag, obj) {
  delete game.tags[tag][obj.guid];
}

game.tag_clear_guid = function(guid)
{
  for (var tag in game.tags)
    delete game.tags[tag][guid];
}

game.objects_with_tag = function(tag)
{
  if (!game.tags[tag]) return [];
  return Object.values(game.tags[tag]);
}

game.doc = {};
game.doc.object = "Returns the entity belonging to a given id.";
game.doc.pause = "Pause game simulation.";
game.doc.play = "Resume or start game simulation.";
game.doc.camera = "Current camera.";

game.texture = function(path)
{
  if (game.texture.cache[path]) return game.texture.cache[path];
  
  if (!io.exists(path)) {
    console.warn(`Missing texture: ${path}`);
    game.texture.cache[path] = game.texture("icons/no_tex.gif");
  } else
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
  say(profile.printreport(profcache, "USE REPORT"));
  say(profile.printreport(entityreport, "ENTITY REPORT"));
  
  console.info("QUITTING");
  for (var i in debug.log.time)
    say(debug.log.time[i].map(x=>profile.ms(x)));
};

global.mixin("scripts/input");
global.mixin("scripts/std");
global.mixin("scripts/diff");
global.mixin("scripts/color");
global.mixin("scripts/gui");
global.mixin("scripts/tween");
global.mixin("scripts/ai");

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
Register.add_cb("screengui");

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

  notify(name, ...args) {
    if (!this.events[name]) return;
    this.events[name].forEach(function(x) {
      x[1].call(x[0], ...args);
    });
  },
};

// window.rendersize is the resolution the game renders at
// window.size is the physical size of the window on the desktop
window.modetypes = { 
  stretch: 0, // stretch render to fill window
  keep: 1, // keep render exact dimensions, with no stretching
  width: 2, // keep render at width
  height: 3, // keep render at height
  expand: 4, // expand width or height
  full: 5 // expand out beyond window
};

window.size = [640, 480];

window.set_icon.doc = "Set the icon of the window using the PNG image at path.";

global.mixin("scripts/spline");
global.mixin("scripts/components");

window.doc = {};
window.doc.dimensions = "Window width and height packaged in an array [width,height]";
window.doc.title = "Name in the title bar of the window.";
window.doc.boundingbox = "Boundingbox of the window, with top and right being its height and width.";

global.mixin("scripts/actor");
global.mixin("scripts/entity");

function world_start() {
  globalThis.world = Object.create(entity);
  world.transform = os.make_transform2d();
  world.objects = {};
  world.toString = function() { return "world"; };
  world.ur = "world";
  world.kill = function() { this.clear(); };
  world.phys = 2;
  world.zoom = 1;
  world._ed = { selectable: false };
  world.ur = {};
  world.ur.fresh = {};
  game.cam = world;
}

global.mixin("scripts/physics");
global.mixin("scripts/widget");

globalThis.mum = app.spawn("scripts/mum");

window.title = `Prosperon v${prosperon.version}`;
window.size = [500,500];
window.boundingbox = function() {
  var pos = game.camera.pos;
  var wh = window.rendersize.scale(game.camera.zoom);
  return bbox.fromcwh(pos,wh);
}
