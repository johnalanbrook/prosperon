"use math";

Object.defineProperty(String.prototype, "tolast", {
  value: function (val) {
    var idx = this.lastIndexOf(val);
    if (idx === -1) return this.slice();
    return this.slice(0, idx);
  },
});

Object.defineProperty(String.prototype, "dir", {
  value: function () {
    if (!this.includes("/")) return "";
    return this.tolast("/");
  },
});

Object.defineProperty(String.prototype, "folder", {
  value: function () {
    var dir = this.dir();
    if (!dir) return "";
    else return dir + "/";
  },
});

globalThis.Resources = {};

Resources.rm_fn = function(fnstr, text)
{
  while (text.match(fnstr)) {
  }  
}

Resources.rm_fn.doc = "Remove calls to a given function from a given text script.";

Resources.replpath = function (str, path) {
  if (!str) return str;
  if (str[0] === "/") return str.rm(0);

  if (str[0] === "@") return os.prefpath() + "/" + str.rm(0);

  if (!path) return str;

  var stem = path.dir();
  while (stem) {
    var tr = stem + "/" + str;
    if (io.exists(tr)) return tr;
    stem = stem.updir();
  }

  return str;
};

Resources.replstrs = function (path) {
  if (!path) return;
  var script = io.slurp(path);
  var regexp = /"[^"\s]*?\.[^"\s]+?"/g;
  var stem = path.dir();
  
  // remove console statements
  //script = script.replace(/console\.(.*?)\(.*?\)/g, '');
  //script = script.replace(/assert\(.*?\)/g, '');

  script = script.replace(regexp, function (str) {
    var newstr = Resources.replpath(str.trimchr('"'), path);
    return `"${newstr}"`;
  });

  return script;
};

globalThis.json = {};
json.encode = function (value, replacer, space = 1) {
  return JSON.stringify(value, replacer, space);
};

json.decode = function (text, reviver) {
  if (!text) return undefined;
  return JSON.parse(text, reviver);
};

json.readout = function (obj) {
  var j = {};
  for (var k in obj)
    if (typeof obj[k] === "function") j[k] = "function " + obj[k].toString();
    else j[k] = obj[k];

  return json.encode(j);
};

json.doc = {
  doc: "json implementation.",
  encode: "Encode a value to json.",
  decode: "Decode a json string to a value.",
  readout: "Encode an object fully, including function definitions.",
};

Resources.scripts = ["jsoc", "jsc", "jso", "js"];
Resources.images = ["png", "gif", "jpg", "jpeg"];
Resources.sounds = ["wav", "flac", "mp3", "qoa"];
Resources.is_image = function (path) {
  var ext = path.ext();
  return Resources.images.any((x) => x === ext);
};

function find_ext(file, ext) {
  if (io.exists(file)) return file;
  for (var e of ext) {
    var nf = `${file}.${e}`;
    if (io.exists(nf)) return nf;
  }
  return;
}

Resources.find_image = function (file) {
  return find_ext(file, Resources.images);
};
Resources.find_sound = function (file) {
  return find_ext(file, Resources.sounds);
};
Resources.find_script = function (file) {
  return find_ext(file, Resources.scripts);
};

var t_units = ["ns", "us", "ms", "s", "m", "h"];

console.transcript = "";
console.say = function (msg) {
  msg += "\n";
  console.print(msg);
  console.transcript += msg;
};
console.log = console.say;
globalThis.say = console.say;
globalThis.print = console.print;

console.pprint = function (msg, lvl = 0) {
  if (typeof msg === "object") msg = JSON.stringify(msg, null, 2);

  var file = "nofile";
  var line = 0;
  console.rec(0, msg, file, line);

  var caller = new Error().stack.split("\n")[2];
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

console.spam = function (msg) {
  console.pprint(msg, 0);
};
console.debug = function (msg) {
  console.pprint(msg, 1);
};
console.info = function (msg) {
  console.pprint(msg, 2);
};
console.warn = function (msg) {
  console.pprint(msg, 3);
};
console.error = function (msg) {
  console.pprint(msg + "\n" + console.stackstr(2), 4);
};
console.panic = function (msg) {
  console.pprint(msg + "\n" + console.stackstr(2), 5);
};
console.stackstr = function (skip = 0) {
  var err = new Error();
  var stack = err.stack.split("\n");
  return stack.slice(skip, stack.length).join("\n");
};

console.stack = function (skip = 0) {
  console.log(console.stackstr(skip + 1));
};

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
  clear: "Clear console.",
};

globalThis.global = globalThis;

var use_prof = "USE";

profile.addreport = function(){};

function use(file, env = {}, script) {
  file = Resources.find_script(file);
  var st = profile.now();

  if (use.cache[file]) {
    var ret = use.cache[file].call(env);
    profile.addreport(use_prof, file, st);
    return;
  }
  script ??= Resources.replstrs(file);
  script = `(function() { var self = this; ${script}; })`;
  var fn = os.eval(file, script);
  use.cache[file] = fn;
  var ret = fn.call(env);
  profile.addreport(use_prof, file, st);
  return ret;
}

use.cache = {};

global.check_registers = function (obj) {
  for (var reg in Register.registries) {
    if (typeof obj[reg] === 'function') {
      var fn = obj[reg].bind(obj);
      var name = obj.ur ? obj.ur.name : obj.toString();
      obj.timers.push(Register.registries[reg].register(fn, name));
    }
  }
      
  for (var k in obj) {
    if (!k.startsWith("on_")) continue;
    var signal = k.fromfirst("on_");
    Event.observe(signal, obj, obj[k]);
  }
};

Object.assign(global, use("scripts/base"));
global.obscure("global");
global.mixin("scripts/profile");
global.mixin("scripts/render");
global.mixin("scripts/debug");

var frame_t = profile.secs(profile.now());

var sim = {};
sim.mode = "play";
sim.play = function () {
  this.mode = "play";
  os.reindex_static();
};
sim.playing = function () {
  return this.mode === "play";
};
sim.pause = function () {
  this.mode = "pause";
};
sim.paused = function () {
  return this.mode === "pause";
};
sim.step = function () {
  this.mode = "step";
};
sim.stepping = function () {
  return this.mode === "step";
};

var physlag = 0;

var gggstart = game.engine_start;
game.engine_start = function (s) {
  game.startengine = 1;
  gggstart(
    function () {
      global.mixin("scripts/sound.js");
      world_start();
      window.set_icon(os.make_texture("icons/moon.gif"));
      Object.readonly(window.__proto__, "vsync");
      Object.readonly(window.__proto__, "enable_dragndrop");
      Object.readonly(window.__proto__, "enable_clipboard");
      Object.readonly(window.__proto__, "high_dpi");
      Object.readonly(window.__proto__, "sample_count");
      s();

      shape.quad = {
        pos: os.make_buffer([0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 0], 0),
        verts: 4,
        uv: os.make_buffer([0, 1, 1, 1, 0, 0, 1, 0], 2),
        index: os.make_buffer([0, 1, 2, 2, 1, 3], 1),
        count: 6,
      };

      shape.triangle = {
        pos: os.make_buffer([0, 0, 0, 0.5, 1, 0, 1, 0, 0], 0),
        uv: os.make_buffer([0, 0, 0.5, 1, 1, 0], 2),
        verts: 3,
        count: 3,
        index: os.make_buffer([0, 2, 1], 1),
      };
      
      shape.centered_quad = {
        pos: os.make_buffer([-0.5, -0.5, -0.5, 0.5, -0.5, -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5], 0),
        verts: 4,
        uv: os.make_buffer([0,1,1,1,0,0,1,0],2),
        index: os.make_buffer([0,1,2,2,1,3],1),
        count: 6
      };

      render.init();

      camera = make_camera();
      camera.transform.pos = [0,0,-100];
      camera.mode = "keep";
      camera.break = "fit";
      camera.size = game.size;
      gamestate.camera = camera;

      hudcam = make_camera();
      hudcam.near = 0;
      hudcam.size = camera.size;
      hudcam.mode = "keep";
      hudcam.break = "fit";

      appcam = make_camera();
      appcam.near = 0;
      appcam.size = window.size;
      appcam.transform.pos = [window.size.x,window.size.y,-100];
      screencolor = render.screencolor();
    },
    process,
    window.size.x,
    window.size.y,
  );
};

game.startengine = 0;
var frames = [];

prosperon.release_mode = function()
{
  prosperon.debug = false;
  mum.debug = false;
  debug.kill();
}
prosperon.debug = true;

// Returns an array in the form of [left, bottom, right, top] in pixels of the camera to render to
// Camera viewport is [left,bottom,right,top] in relative values
function camviewport()
{
  var aspect = (this.viewport[2]-this.viewport[0])/(this.viewport[3]-this.viewport[1])*window.size.x/window.size.y;
  var raspect = this.size.x/this.size.y;

  var left = this.viewport[0]*window.size.x;
  var bottom = this.viewport[1]*window.size.y;

  var usemode = this.mode;

  if (this.break && this.size.x > window.size.x && this.size.y > window.size.y)
    usemode = this.break;

  if (usemode === "fit")
    if (raspect < aspect) usemode = "height";
    else usemode = "width";

  switch(usemode) {
    case "stretch":
    case "expand":
      return [0, 0, window.size.x, window.size.y];
    case "keep":
      return [left, bottom, left+this.size.x, bottom+this.size.y];
    case "height":
      var ret = [left, 0, this.size.x*(window.size.y/this.size.y), window.size.y];
      ret[0] = (window.size.x-(ret[2]-ret[0]))/2;
      return ret;
    case "width":
      var ret = [0, bottom, window.size.x, this.size.y*(window.size.x/this.size.x)];
      ret[1] = (window.size.y-(ret[3]-ret[1]))/2;
      return ret;
  }

  return [0, 0, window.size.x, window.size.y];
}

// pos is pixels on the screen, lower left[0,0]
function camscreen2world(pos)
{
  var view = this.screen2cam(pos);
  view.x *= this.size.x;
  view.y *= this.size.y;
  view = view.sub([this.size.x/2, this.size.y/2]);
  view = view.add(this.pos.xy);
  return view;
}

camscreen2world.doc = "Convert a view position for a camera to world."

function screen2cam(pos)
{
  var viewport = this.view();
  var width = viewport[2]-viewport[0];
  var height = viewport[3]-viewport[1];
  var left = pos.x-viewport[0];
  var bottom = pos.y-viewport[1];
  var p = [left/width, bottom/height];
  return p;
}

screen2cam.doc = "Convert a screen space position in pixels to a normalized viewport position in a camera."

function make_camera()
{
  var cam = world.spawn();
  cam.near = 0.1;
  cam.far = 1000;
  cam.ortho = true;
  cam.viewport = [0,0,1,1];
  cam.size = window.size.slice(); // The render size of this camera in pixels
  // In ortho mode, this determines how many pixels it will see
  cam.mode = "stretch";
  cam.screen2world = camscreen2world;
  cam.screen2cam = screen2cam;

  cam.mousepos = function() { return this.screen2world(input.mouse.screenpos()); }
  cam.view = camviewport;
  cam.offscreen = false;
  return cam;
}

var camera;
var hudcam;
var appcam;
var screencolor;

prosperon.render = function()
{
  profile.frame("world");
  render.set_camera(camera);
  profile.frame("sprites");
  render.sprites();
  profile.endframe();
  profile.frame("draws");
  prosperon.draw();
  profile.endframe();
  hudcam.size = camera.size;
  hudcam.transform.pos = [hudcam.size.x/2, hudcam.size.y/2, -100];
  render.set_camera(hudcam);

  profile.endframe();
  profile.frame("hud");

  prosperon.hud();
  render.flush_text();

  render.end_pass();

  profile.endframe();

  profile.frame("post process");
  /* draw the image of the game world first */
  render.glue_pass();
  render.viewport(...camera.view());
  render.use_shader(render.postshader);
  render.use_mat({diffuse:screencolor});
  render.draw(shape.quad);

  profile.endframe();

  profile.frame("app");

  // Flush & render
  appcam.transform.pos = [window.size.x/2, window.size.y/2, -100];
  appcam.size = window.size.slice();
  if (os.sys() !== 'macos')
    appcam.size.y *= -1;

  render.set_camera(appcam);
  render.viewport(...appcam.view());

  // Call gui functions
  mum.style = mum.dbg_style;
  prosperon.gui();
  if (mum.drawinput) mum.drawinput();
  prosperon.gui_dbg();
  render.flush_text();
  mum.style = mum.base;

  profile.endframe();

  profile.frame("imgui");

  render.imgui_new(window.size.x, window.size.y, 0.01);
  prosperon.imgui();
  render.imgui_end();

  profile.endframe();

  render.end_pass();
  render.commit();
}

function process() {
  profile.frame("frame");
  var dt = profile.secs(profile.now()) - frame_t;
  frame_t = profile.secs(profile.now());

  profile.frame("app update");
  prosperon.appupdate(dt);
  profile.endframe();

  profile.frame("input");
  input.procdown();
  profile.endframe();

  if (sim.mode === "play" || sim.mode === "step") {
    profile.frame("update");  
    prosperon.update(dt * game.timescale);
    profile.endframe();
    if (sim.mode === "step") sim.pause();

    profile.frame("physics");
    physlag += dt;

    while (physlag > physics.delta) {
      physlag -= physics.delta;
      prosperon.phys2d_step(physics.delta * game.timescale);
      prosperon.physupdate(physics.delta * game.timescale);
    }
    profile.endframe();
  }

  profile.frame("render");
  prosperon.window_render(window.size);
  prosperon.render();
  profile.endframe();

  profile.endframe();
}

globalThis.fps = function () {
  return 0;
//  var sum = 0;
//  for (var i = 0; i < frames.length; i++) sum += frames[i];
//  return frames.length / sum;
};

game.timescale = 1;

var eachobj = function (obj, fn) {
  var val = fn(obj);
  if (val) return val;
  for (var o in obj.objects) {
    if (obj.objects[o] === obj)
      console.error(`Object ${obj.toString()} is referenced by itself.`);
    val = eachobj(obj.objects[o], fn);
    if (val) return val;
  }
};

game.all_objects = function (fn, startobj = world) {
  return eachobj(startobj, fn);
};
game.find_object = function (fn, startobj = world) {};

game.tags = {};
game.tag_add = function (tag, obj) {
  game.tags[tag] ??= {};
  game.tags[tag][obj.guid] = obj;
};

game.tag_rm = function (tag, obj) {
  delete game.tags[tag][obj.guid];
};

game.tag_clear_guid = function (guid) {
  for (var tag in game.tags) delete game.tags[tag][guid];
};

game.objects_with_tag = function (tag) {
  if (!game.tags[tag]) return [];
  return Object.values(game.tags[tag]);
};

game.doc = {};
game.doc.object = "Returns the entity belonging to a given id.";
game.doc.pause = "Pause game simulation.";
game.doc.play = "Resume or start game simulation.";
game.doc.camera = "Current camera.";

game.texture = function (path, force = false) {
  if (force && game.texture.cache[path]) return game.texture.cache[path];

  if (!io.exists(path)) {
    console.warn(`Missing texture: ${path}`);
    game.texture.cache[path] = game.texture("icons/no_tex.gif");
  } else game.texture.cache[path] ??= os.make_texture(path);

  return game.texture.cache[path];
};
game.texture.cache = {};

prosperon.semver = {};
prosperon.semver.valid = function (v, range) {
  v = v.split(".");
  range = range.split(".");
  if (v.length !== 3) return undefined;
  if (range.length !== 3) return undefined;

  if (range[0][0] === "^") {
    range[0] = range[0].slice(1);
    if (parseInt(v[0]) >= parseInt(range[0])) return true;

    return false;
  }

  if (range[0] === "~") {
    range[0] = range[0].slice(1);
    for (var i = 0; i < 2; i++)
      if (parseInt(v[i]) < parseInt(range[i])) return false;
    return true;
  }

  return prosperon.semver.cmp(v.join("."), range.join(".")) === 0;
};

prosperon.semver.cmp = function (v1, v2) {
  var ver1 = v1.split(".");
  var ver2 = v2.split(".");

  for (var i = 0; i < 3; i++) {
    var n1 = parseInt(ver1[i]);
    var n2 = parseInt(ver2[i]);
    if (n1 > n2) return 1;
    else if (n1 < n2) return -1;
  }

  return 0;
};

prosperon.semver.doc =
  "Functions for semantic versioning numbers. Semantic versioning is given as a triple digit number, as MAJOR.MINOR.PATCH.";
prosperon.semver.cmp.doc =
  "Compare two semantic version numbers, given like X.X.X.";
prosperon.semver.valid.doc = `Test if semantic version v is valid, given a range.
Range is given by a semantic versioning number, prefixed with nothing, a ~, or a ^.
~ means that MAJOR and MINOR must match exactly, but any PATCH greater or equal is valid.
^ means that MAJOR must match exactly, but any MINOR and PATCH greater or equal is valid.`;

prosperon.iconified = function (icon) {};
prosperon.focus = function (focus) {};
prosperon.resize = function (dimensions) {
  window.size.x = dimensions.x;
  window.size.y = dimensions.y;
};
prosperon.suspended = function (sus) {};
prosperon.mouseenter = function () {};
prosperon.mouseleave = function () {};
prosperon.touchpress = function (touches) {};
prosperon.touchrelease = function (touches) {};
prosperon.touchmove = function (touches) {};
prosperon.clipboardpaste = function (str) {};
prosperon.quit = function () {
  if (profile.disabled) return;
  
  say("===START CACHE REPORTS===\n");
  for (var i in profile.report_cache) {
    say(profile.printreport(profile.report_cache[i],i));
  }

  say("===FRAME AVERAGES===\n");
  say(profile.print_frame_avg());
  say("\n");

  profile.print_cpu_instr();
};

window.size = [640, 480];
window.mode = "keep";
window.toggle_fullscreen = function() { window.fullscreen = !window.fullscreen; }

window.set_icon.doc = "Set the icon of the window using the PNG image at path.";

window.doc = {};
window.doc.dimensions = "Window width and height packaged in an array [width,height]";
window.doc.title = "Name in the title bar of the window.";
window.doc.boundingbox = "Boundingbox of the window, with top and right being its height and width.";

global.mixin("scripts/input");
global.mixin("scripts/std");
global.mixin("scripts/diff");
global.mixin("scripts/color");
global.mixin("scripts/gui");
global.mixin("scripts/tween");
global.mixin("scripts/ai");
global.mixin("scripts/particle");

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

  add_cb(name, e_event = false) {
    var n = {};
    var fns = [];

    n.register = function (fn, oname) {
      if (!(fn instanceof Function)) return;

      var dofn = function(...args) {
        var st = profile.now();
        fn(...args);
	      profile.addreport(name, oname, st);
      }
      
      fns.push(dofn);
      return function () {
        fns.remove(dofn);
      };
    };
    
    prosperon[name] = function (...args) {
      fns.forEach(x => x(...args));
    };
    
    prosperon[name].fns = fns;
    n.clear = function () {
      fns = [];
    };

    Register[name] = n;
    Register.registries[name] = n;

    return n;
  },
};

Register.add_cb("appupdate", true);
Register.add_cb("update", true).doc = "Called once per frame.";
Register.add_cb("physupdate", true);
Register.add_cb("gui", true);
Register.add_cb("hud", true);
Register.add_cb("draw_dbg", true);
Register.add_cb("gui_dbg", true);
Register.add_cb("hud_dbg", true);
Register.add_cb("draw", true);
Register.add_cb("imgui", true);

var Event = {
  events: {},

  observe(name, obj, fn) {
    this.events[name] ??= [];
    this.events[name].push([obj, fn]);
  },

  unobserve(name, obj) {
    this.events[name] = this.events[name].filter((x) => x[0] !== obj);
  },

  rm_obj(obj) {
    Object.keys(this.events).forEach((name) => Event.unobserve(name, obj));
  },

  notify(name, ...args) {
    if (!this.events[name]) return;
    this.events[name].forEach(function (x) {
      x[1].call(x[0], ...args);
    });
  },
};

global.mixin("scripts/spline");
global.mixin("scripts/components");
global.mixin("scripts/actor");
global.mixin("scripts/entity");

function world_start() {
  globalThis.world = Object.create(entity);
  world.transform = os.make_transform();
  world.objects = {};
  world.toString = function () {
    return "world";
  };
  world.ur = "world";
  world.kill = function () {
    this.clear();
  };
  world.phys = 2;
  world.zoom = 1;
  world._ed = { selectable: false };
  world.ur = {};
  world.ur.fresh = {};
  game.cam = world;
}

global.mixin("scripts/physics");
global.mixin("scripts/widget");
global.mixin("scripts/mum");

window.title = `Prosperon v${prosperon.version}`;
window.size = [500, 500];
