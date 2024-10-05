globalThis.gamestate = {};

global.check_registers = function (obj) {
  for (var reg in Register.registries) {
    if (typeof obj[reg] === "function") {
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
      window.set_icon(game.texture("moon").texture);
      Object.readonly(window.__proto__, "vsync");
      Object.readonly(window.__proto__, "enable_dragndrop");
      Object.readonly(window.__proto__, "enable_clipboard");
      Object.readonly(window.__proto__, "high_dpi");
      Object.readonly(window.__proto__, "sample_count");

      prosperon.camera = prosperon.make_camera();
      var camera = prosperon.camera;
      camera.transform.pos = [0, 0, -100];
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
      appcam.transform.pos = [window.size.x, window.size.y, -100];
      prosperon.screencolor = render.screencolor();

      globalThis.imgui = render.imgui_init();

      s();

      shape.quad = {
        pos: os.make_buffer([
          0, 0, 0,
          0, 1, 0,
          1, 0, 0,
          1, 1, 0], 0),
        verts: 4,
        uv: os.make_buffer([
          0, 0,
          0, 1,
          1, 0,
          1, 1], 2),
        index: os.make_buffer([0, 1, 2, 2, 1, 3], 1),
        count: 6,
      };

      shape.flipquad = {
        pos: os.make_buffer([
          0, 0, 0,
          0, 1, 0,
          1, 0, 0,
          1, 1, 0], 0),
        verts: 4,
        uv: os.make_buffer([
          0, 1,
          0, 0,
          1, 1,
          1, 0], 2),
        index: os.make_buffer([0, 1, 2, 2, 1, 3], 1),
        count: 6,
      };

      shape.triangle = {
        pos: os.make_buffer([0, 0, 0, 0.5, 1, 0, 1, 0, 0], 0),
        uv: os.make_buffer([0, 0, 0.5, 1, 1, 0], 2),
        verts: 3,
        count: 3,
        index: os.make_buffer([0, 1, 2], 1),
      };

      shape.centered_quad = {
        pos: os.make_buffer([
          -0.5, -0.5, -0.5,
          -0.5, 0.5, -0.5,
          0.5, -0.5, -0.5,
          0.5, 0.5, -0.5], 0),
        verts: 4,
        uv: os.make_buffer([
          0, 0,
          0, 1,
          1, 0,
          1, 1], 2),
        index: os.make_buffer([0, 1, 2, 2, 1, 3], 1),
        count: 6,
      };

      render.init();
    },
    prosperon.process,
    window.size.x,
    window.size.y,
  );
};

game.startengine = 0;

prosperon.release_mode = function () {
  prosperon.debug = false;
  mum.debug = false;
  debug.kill();
};
prosperon.debug = true;

game.timescale = 1;

var eachobj = function (obj, fn) {
  var val = fn(obj);
  if (val) return val;
  for (var o in obj.objects) {
    if (obj.objects[o] === obj) console.error(`Object ${obj.toString()} is referenced by itself.`);
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

game.tex_hotreload = function () {
  for (var path in game.texture.cache) {
    if (io.mod(path) > game.texture.time_cache[path]) {
      var tex = game.texture.cache[path];
      game.texture.time_cache[path] = io.mod(path);
      SpriteAnim.hotreload(path);
      os.texture_swap(path, game.texture.cache[path]);
      for (var sprite of Object.values(allsprites)) {
        if (sprite.texture == tex) {
          sprite.tex_sync();
        }
      }
    }
  }
};

var image = {};
image.dimensions = function()
{
  return [this.texture.width, this.texture.height].scale([this.rect[2], this.rect[3]]);
}

texture_proto.copy = function(src, pos, rect)
{
  var pixel_rect = {
    x: rect[0]*src.width,
    y: rect[1]*src.height,
    w: rect[2]*src.width,
    h: rect[3]*src.height
  };

  this.blit(src, {
    x: pos[0],
    y: pos[1],
    w: rect[2]*src.width,
    h: rect[3]*src.height
  }, pixel_rect, false);
}

var spritesheet;
var sheet_frames = [];
var sheetsize = 1024;

function pack_into_sheet(images)
{
  if (!Array.isArray(images)) images = [images];
  if (images[0].texture.width > 300 && images[0].texture.height > 300) return;
  sheet_frames = sheet_frames.concat(images);
  var sizes = sheet_frames.map(x => [x.rect[2]*x.texture.width, x.rect[3]*x.texture.height]);
  var pos = os.rectpack(sheetsize, sheetsize, sizes);
  if (!pos) {
    console.error(`did not make spritesheet properly from images ${images}`);
    console.info(sizes);
    return;
  }

  var newsheet = os.make_tex_data(sheetsize,sheetsize);

  for (var i = 0; i < pos.length; i++) {
    // Copy the texture to the new sheet
    newsheet.copy(sheet_frames[i].texture, pos[i], sheet_frames[i].rect);
    
    // Update the frame's rect to the new position in normalized coordinates
    sheet_frames[i].rect[0] = pos[i][0] / newsheet.width;
    sheet_frames[i].rect[1] = pos[i][1] / newsheet.height;
    sheet_frames[i].rect[2] = sizes[i][0] / newsheet.width;
    sheet_frames[i].rect[3] = sizes[i][1] / newsheet.height;
    sheet_frames[i].texture = newsheet;
  }

  newsheet.load_gpu();
  spritesheet = newsheet;
  return spritesheet;
}

game.texture = function (path) {
  if (!path) return game.texture("icons/no_tex.gif");
  path = Resources.find_image(path);

  if (!io.exists(path)) {
    console.error(`Missing texture: ${path}`);
    game.texture.cache[path] = game.texture("icons/no_tex.gif");
    game.texture.time_cache[path] = io.mod(path);
    return game.texture.cache[path];
  }
  if (game.texture.cache[path]) return game.texture.cache[path];
  var tex = os.make_texture(path);
  
  var image;
  
  var anim = SpriteAnim.make(path, tex);
  if (!anim) {
    image = {
      texture: tex,
      rect:[0,0,1,1]
    };
    if (pack_into_sheet([image]))
      tex = spritesheet;
  } else if (Object.keys(anim).length === 1) {
    image = Object.values(anim)[0].frames;
    image.forEach(x => x.texture = tex);
    if (pack_into_sheet(image))
      tex = spritesheet;
  } else {
    image = {};
    var packs = [];
    for (var a in anim) {
      image[a] = anim[a].frames.slice();
      image[a].forEach(x => x.texture = tex);
      packs = packs.concat(image[a]);
    }
    if (pack_into_sheet(packs))
      tex = spritesheet;
  }
  
  game.texture.cache[path] = image;
  game.texture.time_cache[path] = io.mod(path);

  tex.load_gpu();

  return game.texture.cache[path];
};
game.texture.cache = {};
game.texture.time_cache = {};

game.texture.total_size = function()
{
  var size = 0;
//  Object.values(game.texture.cache).forEach(x => size += x.texture.inram() ? x..texture.width*x.texture.height*4 : 0);
  return size;
}

game.texture.total_vram = function()
{
  var vram = 0;
//  Object.values(game.texture.cache).forEach(x => vram += x.vram);
  return vram;
}

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
    for (var i = 0; i < 2; i++) if (parseInt(v[i]) < parseInt(range[i])) return false;
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

prosperon.semver.cmp.doc = "Compare two semantic version numbers, given like X.X.X.";
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
window.toggle_fullscreen = function () {
  window.fullscreen = !window.fullscreen;
};

window.set_icon.doc = "Set the icon of the window using the PNG image at path.";

window.doc = {};
window.doc.dimensions = "Window width and height packaged in an array [width,height]";
window.doc.title = "Name in the title bar of the window.";
window.doc.boundingbox = "Boundingbox of the window, with top and right being its height and width.";
window.__proto__.toJSON = function () {
  return {
    size: this.size,
    fullscreen: this.fullscreen,
    title: this.title,
  };
};

global.mixin("scripts/input");
global.mixin("scripts/std");
global.mixin("scripts/diff");
global.mixin("scripts/color");
global.mixin("scripts/tween");
global.mixin("scripts/ai");
global.mixin("scripts/particle");
global.mixin("scripts/physics");
global.mixin("scripts/geometry");

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

      var dofn = function (...args) {
        profile.report(`call_${name}_${oname}`);
        var st = profile.now();
        fn(...args);
        profile.endreport(`call_${name}_${oname}`);   
      };

      var left = 0;
      var right = fns.length - 1;
      dofn.layer = fn.layer;
      dofn.layer ??= 0;

      while (left <= right) {
        var mid = Math.floor((left + right) / 2);
        if (fns[mid] === dofn.layer) {
          left = mid;
          break;
        } else if (fns[mid].layer < dofn.layer) left = mid + 1;
        else right = mid - 1;
      }

      fns.splice(left, 0, dofn);

      return function () {
        fns.remove(dofn);
      };
    };

    if (!flush) {
      prosperon[name] = function (...args) {
        fns.forEach(fn => fn(...args));
      };
    } else
      prosperon[name] = function (...args) {
        var layer = undefined;
        for (var fn of fns) {
          if (layer !== fn.layer) {
            flush();
            layer = fn.layer;
          }
          fn();
        }
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
    this.events[name] = this.events[name].filter(x => x[0] !== obj);
  },

  rm_obj(obj) {
    Object.keys(this.events).forEach(name => Event.unobserve(name, obj));
  },

  notify(name, ...args) {
    if (!this.events[name]) return;
    this.events[name].forEach(function (x) {
      x[1].call(x[0], ...args);
    });
  },
};

prosperon.add_timer = function(obj, fn, seconds)
{
  var timers = obj.timers;
  
  var stop = function () {
    timers.remove(stop);
    timer.fn = undefined;
    timer = undefined; 
  };

  function execute() {
    if (fn)
      timer.remain = fn(stop.seconds);

    if (!timer.remain)
      stop();
    else
      stop.seconds = timer.remain;
  }

  var timer = os.make_timer(execute);
  timer.remain = seconds;

  stop.remain = seconds;
  stop.seconds = seconds;
  
  timers.push(stop);
  return stop;
}

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
  Event,
};
