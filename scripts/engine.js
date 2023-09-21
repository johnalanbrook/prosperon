var files = {};
function load(file) {
   var modtime = cmd(0, file);
  files[file] = modtime;
}

load("scripts/base.js");
load("scripts/std.js");

function initialize()
{
  if (!Game.edit)
    load("scripts/play.js");
  else
    load("scripts/editor.js");
}

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

load("scripts/diff.js");
Log.level = 1;

var Color = {
  white: [255,255,255,255],
  black: [0,0,0,255],
  blue: [84,110,255,255],
  green: [120,255,10,255],
  yellow: [251,255,43,255],
  red: [255,36,20,255],
  teal: [96, 252, 237,255],
  gray: [181, 181,181,255],
  cyan: [0,255,255],
};

function bb2wh(bb) {
  return [bb.r-bb.l, bb.t-bb.b];
};

load("scripts/gui.js");

var timer = {
  make(fn, secs,obj,loop,app) {
    app ??= false;
    if (secs === 0) {
      fn.call(obj);
      return;
    }
      
    var t = clone(this);
    t.callback = fn;
    var guardfn = function() {
      if (typeof t.callback === 'function')
        t.callback();
      else
        Log.warn("Timer trying to execute without a function.");
    };
    t.id = make_timer(guardfn, secs, app);
    
    return t;
  },

  oneshot(fn, secs,obj, app) {
    app ??= false;
    var t = this.make(fn, secs, obj, 0, app);
    t.start();
  },

  get remain() { return cmd(32, this.id); },
  get on() { return cmd(33, this.id); },
  get loop() { return cmd(34, this.id); },
  set loop(x) { cmd(35, this.id, x); },
  
  start() { cmd(26, this.id); },
  stop() { cmd(25, this.id); },
  pause() { cmd(24, this.id); },
  kill() { cmd(27, this.id); },
  set time(x) { cmd(28, this.id, x); },
  get time() { return cmd(29, this.id); },
  get pct() { return this.remain / this.time; },
};

var animation = {
  time: 0,
  loop: false,
  playtime: 0,
  playing: false,
  keyframes: [],

  create() {
    var anim = Object.create(animation);
    Register.update.register(anim.update, anim);
    return anim;
  },

  start() {
    this.playing = true;
    this.time = this.keyframes.last[1];
    this.playtime = 0;
  },

  interval(a, b, t) {
    return (t - a) / (b - a);
  },

  near_val(t) {
    for (var i = 0; i < this.keyframes.length-1; i++) {
      if (t > this.keyframes[i+1][1]) continue;

      return this.interval(this.keyframes[i][1], this.keyframes[i+1][1], t) >= 0.5 ? this.keyframes[i+1][0] : this.keyframes[i][0];
    }

    return this.keyframes.last[0];
  },

  lerp_val(t) {
    for (var i = 0; i < this.keyframes.length-1; i++) {
      if (t > this.keyframes[i+1][1]) continue;

      var intv = this.interval(this.keyframes[i][1], this.keyframes[i+1][1], t);
      return ((1 - intv) * this.keyframes[i][0]) + (intv * this.keyframes[i+1][0]);
    }

    return this.keyframes.last[0];
  },

  cubic_val(t) {

  },

  mirror() {
    if (this.keyframes.length <= 1) return;
    for (var i = this.keyframes.length-1; i >= 1; i--) {
      this.keyframes.push(this.keyframes[i-1]);
      this.keyframes.last[1] = this.keyframes[i][1] + (this.keyframes[i][1] - this.keyframes[i-1][1]);
    }
  },

  update(dt) {
    if (!this.playing) return;

    this.playtime += dt;
    if (this.playtime >= this.time) {
      if (this.loop)
        this.playtime = 0;
      else {
        this.playing = false;
        return;
      }
    }

    this.fn(this.lerp_val(this.playtime));
  },
};

var Render = {
  normal() {
    cmd(67);
  },
  
  wireframe() {
    cmd(68);
  },
};

load("scripts/physics.js");
load("scripts/input.js");
load("scripts/sound.js");

function screen2world(screenpos) { return Game.camera.view2world(screenpos); }
function world2screen(worldpos) { return Game.camera.world2view(worldpos); }

var Register = {
  inloop: false,
  loopcbs: [],
  finloop() {
    this.loopcbs.forEach(x => x());
    this.loopcbs = [];
  },

  wraploop(loop) {
    this.inloop = true;
    loop();
    this.inloop = false;
    this.finloop();
  },

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
      entries = entries.filter(function(e) { return e.fn !== f; });
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
Register.add_cb(1, "physupdate");
Register.add_cb(2, "gui");
Register.add_cb(3, "nk_gui");
Register.add_cb(6, "debug");
register(7, Register.kbm_input, Register);
Register.add_cb(8, "gamepad_input");
Register.add_cb(10, "draw");

register(9, Log.stack, this);

Register.gamepad_playermap[0] = Player.players[0];



Player.players[0].control(GUI);

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

  clera_obj(obj) {
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

var Window = {
  set width(w) { cmd(125, w); },
  set height(h) { cmd(126, h); },
  get width() { return cmd(48); },
  get height() { return cmd(49); },
  get dimensions() { return [this.width, this.height]; },
  set name(str) { cmd(134, str); },
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

var ur_json = function()
{
  function objdiff(from, to) {
    if (!to) return from; // Everything on from is unique
    var ret = {};
    ret.ur = to.toString();
    for (var key in from) {
      if (!(key in to)) continue;

      if (typeof from[key] === 'object') {
        if ('ur' in from[key]) {
	  ret[key] = objdiff(from[key],from[key].ur);
	  continue;
	}
	var diff = objdiff(from[key], to[key]);
	if (diff && !diff.empty) ret[key] = diff;
	continue;
      }
      
      if (from[key] !== to[key])
	ret[key] = from[key];
    }
    if (ret.empty) return undefined;
    return ret;
  }

  return objdiff(this, this.ur); 
}


load("scripts/components.js");

function replacer_empty_nil(key, val) {
  if (typeof val === 'object' && JSON.stringify(val) === '{}')
    return undefined;

//  if (typeof val === 'number')
//    return parseFloat(val.toFixed(4));

  return val;
};

function clean_object(obj) {
  Object.keys(obj).forEach(function(x) {
    if (!(x in obj.__proto__)) return;

    switch(typeof obj[x]) {
      case 'object':
        if (Array.isArray(obj[x])) {
          if (obj[x].equal(obj.__proto__[x])) {
	    delete obj[x];
	  }
	} else
	  clean_object(obj[x]);
	  
        break;

      case 'function':
        return;

      default:
        if (obj[x] === obj.__proto__[x])
	  delete obj[x];
	break;
    }
  });
};

function find_com(objects)
{
  if (!objects || objects.length === 0)
    return [0,0];
    var com = [0,0];
    com[0] = objects.reduce(function(acc, val) {
      return acc + val.pos[0];
    }, 0);
    com[0] /= objects.length;
    
    com[1] = objects.reduce(function(acc, val) {
      return acc + val.pos[1];
    }, 0);
    com[1] /= objects.length;    
    
    return com;
};

var Game = {
  objects: [],
  resolution: [1200,720],
  name: "Untitled",
  edit: true,
  register_obj(obj) {
    this.objects[obj.body] = obj;
  },
  
  /* Returns an object given an id */
  object(id) {
    return this.objects[id];
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

  groupify(objects, spec) {
    var newgroup = {
      locked: true,
      breakable: true,
      objs: objects,
//      get pos() { return find_com(objects); },
//      set pos(x) { this.objs.forEach(function(obj) { obj.pos = x; }) },
    };

    Object.assign(newgroup, spec);
    objects.forEach(function(x) {
      x.defn('group', newgroup);
    });

    var bb = bb_from_objects(newgroup.objs);
    newgroup.startbb = bb2cwh(bb);
    newgroup.bboffset = newgroup.startbb.c.sub(newgroup.objs[0].pos);

    newgroup.boundingbox = function() {
      newgroup.startbb.c = newgroup.objs[0].pos.add(newgroup.bboffset);
      return cwh2bb(newgroup.startbb.c, newgroup.startbb.wh);
    };

    if (newgroup.file)
      newgroup.color = [120,255,10];

    return newgroup;
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
    /* And return to editor .. */
    Log.warn("Stopping not implemented. Paused, and go to editor.");
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

  get dt() {
    return cmd(63);
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

Register.update.register(Game.exec, Game);

load("scripts/entity.js");

var preprimum = {};
preprimum.add_child = function() {};
var World = gameobject.make(gameobject.ur, preprimum);
var Primum = World;
Primum.selectable = false;
World.reparent = function(parent) { Log.warn("Cannot reparent the Primum."); }
World.unparent = function() { Log.warn("The Primum has no parent, always."); }

/* Load configs */
function load_configs(file) {
  Log.info(`Loading config file ${file}.`);
  var configs = JSON.parse(IO.slurp(file));
  for (var key in configs) {
    if (typeof globalThis[key] !== "object") continue;
    Object.assign(globalThis[key], configs[key]);
  }
  
  Collision.sync();
  Game.objects.forEach(function(x) { x.sync(); });
  
  if (!local_conf.mouse) {
    Log.info("disabling mouse features");
    Mouse.disabled = function() {};
    Mouse.hidden = function() {};
  };
};

var local_conf = {
  mouse: true,
};

if (IO.exists("game.config"))
  load_configs("game.config");

/* Save configs */
function save_configs() {
    Log.info("saving configs");
    var configs = {};
    configs.editor_config = editor_config;
    configs.Nuke = Nuke;
    configs.local_conf = local_conf;
    IO.slurpwrite(JSON.stringify(configs, null, 1), "editor.config");
    
    save_game_configs();
};

function save_game_configs() {
  var configs = {};
  configs.physics = physics;
  configs.Collision = Collision;
  Log.info(configs);
  IO.slurpwrite(JSON.stringify(configs,null,1), "game.config");

  Collision.sync();
  Game.objects.forEach(function(x) { x.sync(); });
};

load("scripts/physics.js");


Game.view_camera = function(cam)
{
  Game.camera = cam;
  cmd(61, Game.camera.body);
}

Game.view_camera(Primum.spawn(ur.camera2d));

Window.name = "Primum Machinam (V0.1)";
Window.width = 1280;
Window.height = 720;
