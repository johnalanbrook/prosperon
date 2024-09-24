globalThis.gamestate = {};

global.check_registers = function (obj) {
  for (var reg in Register.registries) {
    if (typeof obj[reg] === 'function') {
      var fn = obj[reg].bind(obj);
      fn.layer = obj[reg].layer;
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

global.obscure("global");
global.mixin("scripts/render");
global.mixin("scripts/debug");
global.mixin("scripts/repl");

var frame_t = profile.secs(profile.now());

var sim = {};
sim.mode = "play";
sim.play = function () {
  this.mode = "play";
  os.reindex_static();
  game.all_objects(o => {
    if (!o._started) {
      o._started = true;
      o.start?.();
    }
  });
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
      window.set_icon(game.texture("moon"));
      Object.readonly(window.__proto__, "vsync");
      Object.readonly(window.__proto__, "enable_dragndrop");
      Object.readonly(window.__proto__, "enable_clipboard");
      Object.readonly(window.__proto__, "high_dpi");
      Object.readonly(window.__proto__, "sample_count");

      prosperon.camera = prosperon.make_camera();
      var camera = prosperon.camera;
      camera.transform.pos = [0,0,-100];
      camera.mode = "keep";
      camera.break = "fit";
      camera.size = game.size;
      gamestate.camera = camera;

      prosperon.hudcam = prosperon.make_camera();
      var hudcam = prosperon.hudcam;
      hudcam.near = 0;
      hudcam.size = camera.size;
      hudcam.mode = "keep";
      hudcam.break = "fit";

      prosperon.appcam = prosperon.make_camera();
      var appcam = prosperon.appcam;
      appcam.near = 0;
      appcam.size = window.size;
      appcam.transform.pos = [window.size.x,window.size.y,-100];
      prosperon.screencolor = render.screencolor();
      
      globalThis.imgui = render.imgui_init();
      
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
    },
    prosperon.process,
    window.size.x,
    window.size.y,
  );
};

game.startengine = 0;

prosperon.release_mode = function()
{
  prosperon.debug = false;
  mum.debug = false;
  debug.kill();
}
prosperon.debug = true;

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

game.tex_hotreload = function()
{
  for (var path in game.texture.cache) {
    if (io.mod(path) > game.texture.time_cache[path]) {
      var tex = game.texture.cache[path];
      game.texture.time_cache[path] = io.mod(path);
      os.texture_swap(path, game.texture.cache[path]);
      for (var sprite of Object.values(allsprites)) {
        if (sprite.texture == tex) {
	        sprite.tex_sync();
	      }
      }
    }
  }
}

game.texture = function (path) {
  if (!path) return game.texture("icons/no_text.gif");
  path = Resources.find_image(path);
  
  if (!io.exists(path)) {
    console.error(`Missing texture: ${path}`);
    game.texture.cache[path] = game.texture("icons/no_tex.gif");
    game.texture.time_cache[path] = io.mod(path);
    return game.texture.cache[path];
  }
  if (game.texture.cache[path]) return game.texture.cache[path];
  game.texture.cache[path] = os.make_texture(path);
  game.texture.time_cache[path] = io.mod(path);
  return game.texture.cache[path];
};
game.texture.cache = {};
game.texture.time_cache = {};

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
  prosperon.quit_hook?.();
};

window.size = [640, 480];
window.mode = "keep";
window.toggle_fullscreen = function() { window.fullscreen = !window.fullscreen; }

window.set_icon.doc = "Set the icon of the window using the PNG image at path.";

window.doc = {};
window.doc.dimensions = "Window width and height packaged in an array [width,height]";
window.doc.title = "Name in the title bar of the window.";
window.doc.boundingbox = "Boundingbox of the window, with top and right being its height and width.";
window.__proto__.toJSON = function()
{
  return {
    size: this.size,
    fullscreen: this.fullscreen,
    title: this.title
  }; 
}

global.mixin("scripts/input");
global.mixin("scripts/std");
global.mixin("scripts/diff");
global.mixin("scripts/color");
global.mixin("scripts/tween");
global.mixin("scripts/ai");
global.mixin("scripts/particle");
global.mixin("scripts/physics");
global.mixin("scripts/geometry")

/*
Factory for creating registries. Register one with 'X.register',
which returns a function that, when invoked, cancels the registry.
*/
var Register = {
  registries: [],

  add_cb(name, e_event = false, flush = undefined) {
    var n = {};
    var fns = [];

    n.register = function (fn, oname) {
      if (!(fn instanceof Function)) return;
      
      var guid = prosperon.guid();

      var dofn = function(...args) {
        profile.cache(name,oname);
        var st = profile.now();
        fn(...args);
      	profile.endcache();
      }

      fns.push(dofn);
      dofn.layer = fn.layer;
      dofn.layer ??= 0;

      fns.sort((a,b) => a.layer > b.layer);
      
      return function () {
        fns.remove(dofn);
      };
    };

    if (!flush) {
      prosperon[name] = function(...args) {
        fns.forEach(fn => fn(...args));
      }
    }
    else
      prosperon[name] = function(...args) {
        var layer = undefined;
        for (var fn of fns) {
	  if (layer !== fn.layer) {
	    flush();
	    layer = fn.layer;
	  }
          fn();	  
        }
      }
    
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
Register.add_cb("hud", true, render.flush);
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

return {
  Register,
  sim,
  frame_t,
  physlag,
  Event
}
