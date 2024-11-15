globalThis.gamestate = {};

global.check_registers = function check_registers(obj) {
  for (var reg in Register.registries) {
    if (!Register.registries[reg].register) return;
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

globalThis.timers = []
global.setTimeout = function(fn,seconds, ...args) {
  return prosperon.add_timer(globalThis, _ => fn.apply(undefined,args), seconds);
}
global.clearTimeout = function(id) { id(); }

prosperon.delay = setTimeout;

global.obscure("global");
global.mixin("render");
global.mixin("debug");
global.mixin('layout')
globalThis.parseq = use('parseq');

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

prosperon.SIGABRT = function()
{
  console.log("ABORT");
  console.stack();
}

prosperon.SIGSEGV = function()
{
  console.stack();
  os.exit(1);
}

prosperon.init = function () {
  render.init();
  imgui.init();

  globalThis.audio = use("sound.js");
  world_start();
  prosperon.camera = prosperon.make_camera();
  prosperon.camera.transform.pos = [0, 0, -100];
  prosperon.camera.mode = "keep";
  prosperon.camera.break = "fit";
  prosperon.camera.size = game.size;

      shape.quad = {
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
          0, 1,
          0, 0,
          1, 1,
          1, 0], 2),
        index: os.make_buffer([0, 1, 2, 2, 1, 3], 1),
        count: 6,
      };
  if (io.exists("game.js")) global.app = actor.spawn("game.js");
  else global.app = actor.spawn("nogame.js");
};

prosperon.release_mode = function () {
  prosperon.debug = false;
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

function calc_image_size(img)
{
  if (!img.texture || !img.rect) return;
  return [img.texture.width*img.rect.width, img.texture.height*img.rect.height];
}

game.tex_hotreload = function tex_hotreload(file) {
  if (!(file in game.texture.cache)) return;
  var data = io.slurpbytes(file);
  var tex;
  if (file.endsWith('.gif')) {
    var anim = os.make_gif(data);
    if (anim.frames.length !== 1) return;
    console.info(json.encode(anim));
    tex = anim.frames[0].texture;
  } else if (file.endsWith('.ase') || file.endsWith('.aseprite')) {
    var anim = os.make_aseprite(data);
    if (anim.texture) // single picture
      tex = anim.texture;
    else {
      var oldanim = game.texture.cache[file];
      // load all into gpu
      for (var a in anim) {
        oldanim[a] = anim[a];
        for (let frame of anim[a].frames)
          frame.texture.load_gpu();
      }
      return;
    }
  } else
    tex = os.make_texture(data);
  
  var img = game.texture.cache[file];
  tex.load_gpu();
  console.info(`replacing ${json.encode(img)}`)
  img.texture = tex;
  img.rect = {
    x:0,
    y:0,
    width:1,
    height:1
  };
};

var image = {};
image.dimensions = function()
{
  return [this.texture.width, this.texture.height].scale([this.rect[2], this.rect[3]]);
}

texture_proto.copy = function(src, pos, rect)
{
  var pixel_rect = {
    x: rect.x*src.width,
    y: rect.y*src.height,
    width: rect.width*src.width,
    height: rect.height*src.height
  };

  this.blit(src, {
    x: pos[0],
    y: pos[1],
    width: rect.width*src.width,
    height: rect.height*src.height
  }, pixel_rect, false);
}

var spritesheet;
var sheet_frames = [];
var sheetsize = 1024;

function pack_into_sheet(images)
{
  return;
  if (!Array.isArray(images)) images = [images];
  if (images[0].texture.width > 300 && images[0].texture.height > 300) return;
  sheet_frames = sheet_frames.concat(images);
  var sizes = sheet_frames.map(x => [x.rect.width*x.texture.width, x.rect.height*x.texture.height]);
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
    sheet_frames[i].rect.x = pos[i][0] / newsheet.width;
    sheet_frames[i].rect.y = pos[i][1] / newsheet.height;
    sheet_frames[i].rect.width = sizes[i][0] / newsheet.width;
    sheet_frames[i].rect.height = sizes[i][1] / newsheet.height;
    sheet_frames[i].texture = newsheet;
  }

  newsheet.load_gpu();
  spritesheet = newsheet;
  return spritesheet;
}

var SpriteAnim = {};
globalThis.SpriteAnim = SpriteAnim;
SpriteAnim.aseprite = function (path) {
  function aseframeset2anim(frameset, meta) {
    var anim = {};
    anim.frames = [];
    var dim = meta.size;

    var ase_make_frame = function (ase_frame) {
      var f = ase_frame.frame;
      var frame = {};
      frame.rect = {
        x: f.x / dim.w,
        y: f.y / dim.h,        
        width: f.w / dim.w,
        height: f.h / dim.h,
      };
      frame.time = ase_frame.duration / 1000;
      anim.frames.push(frame);
    };

    frameset.forEach(ase_make_frame);
    anim.dim = frameset[0].sourceSize;
    anim.loop = true;
    return anim;
  }

  var data = json.decode(io.slurp(path));
  if (!data?.meta?.app.includes("aseprite")) return;
  var anims = {};
  var frames = Array.isArray(data.frames) ? data.frames : Object.values(data.frames);

  if (!data.meta.frameTags || data.meta.frameTags.length === 0) {
    anims[0] = aseframeset2anim(frames, data.meta);
    return anims;
  }
  for (var tag of data.meta.frameTags)
    anims[tag.name] = aseframeset2anim(frames.slice(tag.from, tag.to + 1), data.meta);

  return anims;
};

game.is_image = function(obj)
{
  if (obj.texture && obj.rect) return true;
}

// Any request to it returns an image, which is a texture and rect. But they can
game.texture = function texture(path) {
  if (typeof path !== 'string') throw new Error('need a string for game.texture')
  var parts = path.split(':');
  path = Resources.find_image(parts[0]);

  // Look for a cached version
  var frame;
  var anim_str;
  if (parts.length > 1) {
    // it's an animation
    parts = parts[1].split('_'); // For a gif, it might be 'water.gif:3', but for an ase it might be 'water.ase:run_3', meaning the third frame of the 'run' animation
    if (parts.length === 1)
      frame = Number(parts[0]);
    else {
      anim_str = parts[0];
      frame = Number(parts[1]);
    }
  } else
    parts = undefined;

  var ret;
  if (ret = game.texture.cache[path]) {
    if (ret.texture) return ret;
    if (!parts) return ret;

    return ret[anim_str].frames[frame];
  }

  if (!path) return game.texture("no_tex.gif");

  if (!io.exists(path)) {
    console.error(`Missing texture: ${path}`);
    game.texture.cache[path] = game.texture("core/icons/no_tex.gif");
    game.texture.time_cache[path] = io.mod(path);
    return game.texture.cache[path];
  }

  // Cache not found; add to the spritesheet
    
  var ext = path.ext();    

  if (ext === 'ase' || ext === 'aseprite') {
    anim = os.make_aseprite(io.slurpbytes(path));

    if (!anim) return;
    if (anim.texture) {
      // it was a single image
      anim.texture.load_gpu();
      game.texture.cache[path] = anim;
      anim.size = calc_image_size(anim);
      return anim;
    }
    // load all into gpu
    for (var a in anim) 
      for (let frame of anim[a].frames) {
        frame.texture.load_gpu();
        frame.size = calc_image_size(img);
      }

    game.texture.cache[path] = anim;
    ret = game.texture.cache[path];
    if (!parts) return ret;
    return ret[anim_str].frames[frame];
  }

  if (ext === 'gif') {
    anim = os.make_gif(io.slurpbytes(path));
    if (!anim) return;
    if (anim.frames.length === 1) {
      // in this case, it's just a single image
      anim.texture = anim.frames[0].texture;
      anim.rect = anim.frames[0].rect;
      anim.frames = undefined;
      anim.texture.load_gpu();
      anim.size = calc_image_size(anim);
      game.texture.cache[path] = anim;
      return anim;
    }
    game.texture.cache[path] = anim;
    anim.frames[0].size = calc_image_size(anim.frames[0]);
    anim.frames[0].texture.load_gpu();
    return anim;
  }

  var tex = os.make_texture(io.slurpbytes(path));
  if (!tex) throw new Error(`Could not make texture from ${path}`);
  tex.path = path;
  var image;
  var anim;

  if (io.exists(path.set_ext(".json")))
    anim = SpriteAnim.aseprite(path.set_ext(".json"));

  if (!anim) {
    image = {
      texture: tex,
      rect:{x:0,y:0,width:1,height:1}
    };
    pack_into_sheet([image]);
  } else if (Object.keys(anim).length === 1) {
    image = Object.values(anim)[0];
    image.frames.forEach(x => x.texture = tex);
    pack_into_sheet(image.frames);
  } else {
    var allframes = [];
    for (var a in anim)
      allframes = allframes.concat(anim[a].frames);

    for (var frame of allframes) frame.texture = tex;
    pack_into_sheet(allframes);
    image = anim;
  }
  
  game.texture.cache[path] = image;
  game.texture.time_cache[path] = io.mod(path);

  image.size = calc_image_size(image);
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
prosperon.suspended = function (sus) {};
prosperon.mouseenter = function () {};
prosperon.mouseleave = function () {};
prosperon.touchpress = function (touches) {};
prosperon.touchrelease = function (touches) {};
prosperon.touchmove = function (touches) {};
prosperon.clipboardpaste = function (str) {};

global.mixin("input");
global.mixin("std");
global.mixin("diff");
global.mixin("color");
global.mixin("tween");
global.mixin("particle");
//global.mixin("physics");
global.mixin("geometry");

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
        fn(...args);
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
//        profile.fiber_enter(vector.fib);
        fns.forEach(fn => fn(...args));
//        profile.fiber_leave(vector.fib);
      };
    } else
      prosperon[name] = function name(...args) {
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
Register.add_cb("draw", true, render.flush);
Register.add_cb("imgui", true, render.flush);
Register.add_cb("app", true, render.flush);

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

global.mixin("spline");
global.mixin("components");
global.mixin("actor");
global.mixin("entity");

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

return {
  Register,
  sim,
  frame_t,
  physlag,
  Event,
};
