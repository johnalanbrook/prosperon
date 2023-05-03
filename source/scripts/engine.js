"use strict";

var Log = {
  set level(x) { cmd(92,x); },
  get level() { return cmd(93); },
  print(msg, lvl) {
    var lg;
    if (typeof msg === 'object') {
      lg = JSON.stringify(msg, null, 2);
    } else {
      lg = msg;
    }

    var stack = (new Error()).stack;
    var n = stack.next('\n',0)+1;
    n = stack.next('\n', n)+1;
    var nnn = stack.slice(n);
    var fmatch = nnn.match(/\(.*\:/);
    var file = fmatch ? fmatch[0].shift(1).shift(-1) : "nofile";
    var lmatch = nnn.match(/\:\d*\)/);
    var line = lmatch ? lmatch[0].shift(1).shift(-1) : "0";

    yughlog(lvl, lg, file, line);
  },
  
  info(msg) {
    this.print(msg, 0);
  },

  warn(msg) {
    this.print(msg, 1);
  },

  error(msg) {
    this.print(msg, 2);
    this.stack(1);
  },

  critical(msg) {
    this.print(msg,3);
    this.stack(1);
  },

  write(msg) {
    cmd(91,msg);
  },

  stack(skip = 0) {
    Log.warn("Printing stack");
    var stack = (new Error()).stack;
    var n = stack.next('\n',0)+1;
    for (var i = 0; i < skip; i++)
      n = stack.next('\n', n)+1;

    this.write(stack.slice(n));
  },
};


var files = {};
function load(file) {
  var modtime = cmd(0, file);

  if (modtime === 0) {
    Log.stack();
    return false;
  }
  files[file] = modtime;
}

load("scripts/base.js");
load("scripts/diff.js");
load("scripts/debug.js");

function win_icon(str) {
  cmd(90, str);
};

function sim_start() {
  sys_cmd(1);
/*  
  Game.objects.forEach(function(x) {
    if (x.start) x.start(); });

  Level.levels.forEach(function(lvl) {
    lvl.run();
  });
*/
}

function sim_stop() { sys_cmd(2); }
function sim_pause() { sys_cmd(3); }
function sim_step() { sys_cmd(4); }
function sim_playing() { return sys_cmd(5); }
function sim_paused() { return sys_cmd(6); }
function phys_stepping() { return cmd(79); }

function quit() { sys_cmd(0); };

function set_cam(id) {
  cmd(61, id);
};

var Window = {
  get width() {
    return cmd(48);
  },

  get height() {
    return cmd(49);
  },
  
  get dimensions() {
    return [this.width, this.height];
  }
};

var Color = {
  white: [255,255,255],
  blue: [84,110,255],
  green: [120,255,10],
  yellow: [251,255,43],
  red: [255,36,20],
  teal: [96, 252, 237],
  gray: [181, 181,181],
};

var GUI = {
  text(str, pos, size, color, wrap) {
    size = size ? size : 1;
    color = color ? color : [255,255,255];
    wrap = wrap ? wrap : 500;
    var h = ui_text(str, pos, size, color, wrap);

    return [wrap,h];
  },

  text_cursor(str, pos, size, cursor) {
    cursor_text(str,pos,size,[255,255,255],cursor);
  },
};

function listbox(pos, item) {
  pos.y += (item[1] - 20);
};

var Yugine = {
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

var timer = {
  make(fn, secs,obj,loop) {
    if (secs === 0) {
      fn.call(obj);
      return;
    }
      
    var t = clone(this);
    t.id = make_timer(fn, secs, obj);
    
    return t;
  },

  oneshot(fn, secs,obj) {
    var t = this.make(() => {
      fn.call();
      t.kill();
    },secs);
    t.loop = 0;
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
};

var animation = {
  time: 0,
  loop: false,
  playtime: 0,
  playing: false,
  keyframes: [],

  create() {
    var anim = Object.create(animation);
    register_update(anim.update, anim);
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

var sound = {
  play() {
    this.id = cmd(14,this.path);
  },

  stop() {

  },
};

var Music = {
  play(path) {
    Log.info("Playing " + path);
    cmd(87,path);
  },

  stop() {
    cmd(89);
  },

  pause() {
    cmd(88);
  },

  set volume(x) {
  },
};

var Sound = {
  play(file) {
    var s = Object.create(sound);
    s.path = file;
    s.play();
    return s;
  },
  
  music(midi, sf) {
    cmd(13, midi, sf);
  },

  musicstop() {
    cmd(15);
  },

  /* Between 0 and 100 */
  set volume(x) { cmd(19, x); },

  killall() {
    Music.stop();
    this.musicstop();
    /* TODO: Kill all sound effects that may still be running */
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

var Mouse = {
  get pos() {
    return cmd(45);
  },

  get worldpos() {
    return screen2world(cmd(45));
  },
  
  disabled() {
    cmd(46, 212995);
  },
  
  hidden() {
    cmd(46, 212994);
  },
  
  normal() {
    cmd(46, 212993);
  },
};

var Keys = {
  shift() {
    return cmd(50, 340) || cmd(50, 344);
  },
  
  ctrl() {
    return cmd(50, 341) || cmd(50, 344);
  },
  
  alt() {
    return cmd(50, 342) || cmd(50, 346);
  },
};

var Input = {
  setgame() { cmd(77); },
  setnuke() { cmd(78); },
};

function screen2world(screenpos) { return editor.camera.view2world(screenpos); }
function world2screen(worldpos) { return editor.camera.world2view(worldpos); }

var physics = {
  set gravity(x) { cmd(8, x); },
  get gravity() { return cmd(72); },
  set damping(x) { cmd(73,Math.clamp(x,0,1)); },
  get damping() { return cmd(74); },
  pos_query(pos) {
    return cmd(44, pos);
  },
  
  /* Returns a list of body ids that a box collides with */
  box_query(box) {
    var pts = cmd(52,box.pos,box.wh);
    return cmd(52, box.pos, box.wh);
  },
  
  box_point_query(box, points) {
    if (!box || !points)
      return [];

    return cmd(86, box.pos, box.wh, points, points.length);
  },
};

var Action = {
  add_new(name) {
    var action = Object.create(Action);
    action.name = name;
    action.inputs = [];
    this.actions.push(action);

    return action;
  },
  actions: [],
};

/* May be a human player; may be an AI player */
var Player = {
  players: [],
  input(fn, ...args) {
    this.pawns.forEach(x => x[fn]?.(...args));
  },

  control(pawn) {
    this.pawns.push_unique(pawn);
  },

  uncontrol(pawn) {
    this.pawns = this.pawns.filter(x => x !== pawn);
  },
};

for (var i = 0; i < 4; i++) {
  var player1 = Object.create(Player);
  player1.pawns = [];
  player1.gamepads = [];
  Player.players.push(player1);
}

function state2str(state) {
  if (typeof state === 'string') return state;
  switch (state) {
    case 0:
      return "down";
    case 1:
      return "pressed";
    case 2:
      return "released";
  }
}

var Register = {
  updates: [],
  update(dt) {
    this.updates.forEach(x => x[0].call(x[1], dt));
  },

  physupdates: [],
  physupdate(dt) {
    this.physupdates.forEach(x => x[0].call(x[1], dt));
  },

  guis: [],
  gui() {
    this.guis.forEach(x => x[0].call(x[1]));
  },

  nk_guis: [],
  nk_gui() {
    this.nk_guis.forEach(x => x[0].call(x[1]));
  },

  kbm_input(src, btn, state, ...args) {
    var input = `${src}_${btn}_${state}`;
    Player.players[0].input(input, ...args);
  },

  gamepad_playermap: [],
  gamepad_input(pad, btn, state, ...args) {
    var player = this.gamepad_playermap[pad];
    if (!player) return;

    var statestr = state2str(state);

    var rawfn = `gamepad_${btn}_${statestr}`;
    player.input(rawfn, ...args);

    Action.actions.forEach(x => {
      if (x.inputs.includes(btn))
        player.input(`action_${x.name}_${statestr}`, ...args);
    });
  },

  debugs: [],
  debug() {
    this.debugs.forEach(x => x[0].call(x[1]));
  },

  unregister_obj(obj) {
    this.updates = this.updates.filter(x => x[1] !== obj);
    this.guis = this.guis.filter(x => x[1] !== obj);
    this.nk_guis = this.nk_guis.filter(x => x[1] !== obj);
    this.debugs = this.debugs.filter(x => x[1] !== obj);
    this.physupdates = this.physupdates.filter(x => x[1] !== obj);

    Player.players.forEach(x => x.uncontrol(obj));
  },
};

Register.unregister_obj(null);
register(0, Register.update, Register);
register(1, Register.physupdate, Register);
register(2, Register.gui, Register);
register(3, Register.nk_gui, Register);
register(6, Register.debug, Register);
register(7, Register.kbm_input, Register);
register(8, Register.gamepad_input, Register);
register(9, Log.stack, this);

Register.gamepad_playermap[0] = Player.players[0];

function register_update(fn, obj) {
  Register.updates.push([fn, obj ? obj : null]);
};

function register_physupdate(fn, obj) {
  Register.physupdates.push([fn, obj ? obj : null]);
};

function register_gui(fn, obj) {
  Register.guis.push([fn, obj ? obj : this]);
};

function register_debug(fn, obj) {
  Register.debugs.push([fn, obj ? obj : this]);
};

function unregister_gui(fn, obj) {
  Register.guis = Register.guis.filter(x => x[0] !== fn && x[1] !== obj);
};

function register_nk_gui(fn, obj) {
  Register.nk_guis.push([fn, obj ? obj : this]);
};

function unregister_nk_gui(fn, obj) {
  Register.nk_guis = Register.nk_guis.filter(x => x[0] !== fn && x[1] !== obj);
};

register_update(Yugine.exec, Yugine);

/* These functions are the "defaults", and give control to player0 */
function set_pawn(obj, player = Player.players[0]) {
  player.control(obj);
}

function unset_pawn(obj, player = Player.players[0]) {
  player.uncontrol(obj);
}

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

  clear_obj(obj) {
    this.signals.filter(function(x) { return x[1] !== obj; });
  },
};

var IO = {
  exists(file) { return cmd(65, file);},
  slurp(file) {
    if (!this.exists(file)) {
      Log.warn(`File ${file} does not exist; can't slurp.`);
      return "";
    }
    return cmd(38,file);
  },
  slurpwrite(str, file) { return cmd(39, str, file); },
  extensions(ext) { return cmd(66, "." + ext); },
};

function slurp(file) { return IO.slurp(file);}
function slurpwrite(str, file) { return IO.slurpwrite(str, file); }

function reloadfiles() {
  Object.keys(files).forEach(function (x) { load(x); });
}


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
};


var Level = {
  levels: [],
  objects: [],
  alive: true,
  selectable: true,
  toString() {
    if (this.file)
      return this.file;

    return "Loose level";
  },
  get boundingbox() {
    return bb_from_objects(this.objects);
  },

  varname2obj(varname) {
    for (var i = 0; i < this.objects.length; i++)
      if (this.objects[i].varname === varname)
        return this.objects[i];

    return null;
  },

  run() {
    var objs = this.objects.slice();
    var scene = {};
    var self = this;
        
    objs.forEach(function(x) {
      if (x.hasOwn('varname')) {
        scene[x.varname] = x;
	this[x.varname] = x;
      }
    },this);

    eval(this.script);

    if (typeof extern === 'object')
      Object.assign(this, extern);
      
    if (typeof update === 'function')
      register_update(update, this);

    if (typeof gui === 'function')
      register_gui(gui, this);

    if (typeof nk_gui === 'function')
      register_nk_gui(nk_gui, this);
  },

  revert() {
    delete this.unique;
    this.load(this.filelvl);
  },

  /* Returns how many objects this level created are still alive */
  object_count() {
    return objects.length();
  },

  /* Save a list of objects into file, with pos acting as the relative placement */
  saveas(objects, file, pos) {
    if (!pos) pos = find_com(objects);

    objects.forEach(function(obj) {
      obj.pos = obj.pos.sub(pos);
    });

    var newlvl = Level.create();

    objects.forEach(function(x) { newlvl.register(x); });

    var save = newlvl.save();
    slurpwrite(save, file);
  },

  clean() {
    for (var key in this.objects)
      clean_object(this.objects[key]);

    for (var key in gameobjects)
      clean_object(gameobjects[key]);
  },
  
  sync_file(file) {
    var openlvls = this.levels.filter(function(x) { return x.file === file && x !== editor.edit_level; });

    openlvls.forEach(function(x) {
      x.clear();
      x.load(IO.slurp(x.file));
      x.flipdirty = true;
      x.sync();
      x.flipdirty = false;
      x.check_dirty();
    });    
  },

  save() {
    this.clean();
    var pos = this.pos;
    var angle = this.angle;

    this.pos = [0,0];
    this.angle = 0;
    if (this.flipx) {
      this.objects.forEach(function(obj) {
        this.mirror_x_obj(obj);
      }, this);
    }

    if (this.flipy) {
      this.objects.forEach(function(obj) {
        this.mirror_y_obj(obj);
      }, this);
    }
    
    var savereturn = JSON.stringify(this.objects, replacer_empty_nil, 1);

        if (this.flipx) {
      this.objects.forEach(function(obj) {
        this.mirror_x_obj(obj);
      }, this);
    }

    if (this.flipy) {
      this.objects.forEach(function(obj) {
        this.mirror_y_obj(obj);
      }, this);
    }

    this.pos = pos;
    this.angle = angle;
    return savereturn;
  },

  mirror_x_obj(obj) {
    obj.flipx = !obj.flipx;
    var rp = obj.relpos;
    obj.pos = [-rp.x, rp.y].add(this.pos);
    obj.angle = -obj.angle;
  },

  mirror_y_obj(obj) {
    var rp = obj.relpos;
    obj.pos = [rp.x, -rp.y].add(this.pos);
    obj.angle = -obj.angle;
  },

  toJSON() {
    var obj = {};
    obj.file = this.file;
    obj.pos = this._pos;
    obj.angle = this._angle;
    obj.from = "group";
    obj.flipx = this.flipx;
    obj.flipy = this.flipy;
    obj.scale = this.scale;

    if (this.varname)
      obj.varname = this.varname;

    if (!this.unique)
      return obj;

    obj.objects = {};

    this.objects.forEach(function(x,i) {
      obj.objects[i] = {};
      var adiff = Math.abs(x.relangle - this.filelvl[i]._angle) > 1e-5;
      if (adiff)
        obj.objects[i].angle = x.relangle;

      var pdiff = Vector.equal(x.relpos, this.filelvl[i]._pos, 1e-5);
      if (!pdiff)
        obj.objects[i].pos = x._pos.sub(this.pos);

      if (obj.objects[i].empty)
        delete obj.objects[i];
    }, this);

    return obj;
  },

  register(obj) {
    if (obj.level)
      obj.level.unregister(obj);
      
    this.objects.push(obj);
  },

  make() {
    return Level.loadfile(this.file, this.pos);
  },

  spawn(prefab) {
    var newobj = prefab.make();
    newobj.defn('level', this);
    this.objects.push(newobj);
    Game.register_obj(newobj);
    newobj.setup?.();
    newobj.start?.();
    return newobj;
  },

  create() {
    var newlevel = Object.create(this);
    newlevel.objects = [];
    newlevel._pos = [0,0];
    newlevel._angle = 0;
    newlevel.color = Color.green;
    newlevel.toString = function() {
      return (newlevel.unique ? "#" : "") + newlevel.file;
    };
    newlevel.filejson = newlevel.save();
    return newlevel;
  },

  addfile(file) {
    var lvl = this.loadfile(file);
    this.objects.push(lvl);
    lvl.level = this;
    return lvl;
  },

  check_dirty() {
    this.dirty = this.save() !== this.filejson;
  },

  start() {
    this.objects.forEach(function(x) { if ('start' in x) x.start(); });
  },

  loadlevel(file) {
    var lvl = Level.loadfile(file);
    if (lvl && sim_playing())
      lvl.start();

    return lvl;
  },

  loadfile(file) {
    if (!file.endsWith(".lvl")) file = file + ".lvl";
    var newlevel = Level.create();

    if (IO.exists(file)) {
      newlevel.filejson = IO.slurp(file);
      
      try {
	newlevel.filelvl = JSON.parse(newlevel.filejson);
	newlevel.load(newlevel.filelvl);
      } catch (e) {
        newlevel.ed_gizmo = function() { GUI.text("Invalid level file: " + newlevel.file, world2screen(newlevel.pos), 1, Color.red); };
	newlevel.selectable = false;
      }
      newlevel.file = file;
      newlevel.dirty = false;
    }
    
    var scriptfile = file.replace('.lvl', '.js');
    if (IO.exists(scriptfile))
      newlevel.script = IO.slurp(scriptfile);

    newlevel.run();

    return newlevel;
  },

  /* Spawns all objects specified in the lvl json object */
  load(lvl) {
    this.clear();
    this.levels.push_unique(this);
    
    if (!lvl) {
      Log.warn("Level is " + lvl + ". Need a better formed one.");

      return;
    }

    var opos = this.pos;
    var oangle = this.angle;
    this.pos = [0,0];
    this.angle = 0;

    var objs;
    var created = [];

    if (typeof lvl === 'string')
      objs = JSON.parse(lvl);
    else
      objs = lvl;

   if (typeof objs === 'object')
     objs = objs.array();

    objs.forEach(function(x) {
      if (x.from === 'group') {
        var loadedlevel = Level.loadfile(x.file);
	if (!loadedlevel) {
          Log.error("Error loading level: file " + x.file + " not found.");
	  return;
	}
	if (!IO.exists(x.file)) {
	  loadedlevel.ed_gizmo = function() { GUI.text("MISSING LEVEL " + x.file, world2screen(loadedlevel.pos) ,1, Color.red) };
	}
	var objs = x.objects;
	delete x.objects;
	Object.assign(loadedlevel, x);

	if (objs) {
   	  objs.array().forEach(function(x, i) {
	    if (x.pos)
  	      loadedlevel.objects[i].pos = x.pos.add(loadedlevel.pos);

	    if (x.angle)
	      loadedlevel.objects[i].angle = x.angle + loadedlevel.angle;
	  });

	  loadedlevel.unique = true;
	}
	loadedlevel.level = this;
	loadedlevel.sync();
	created.push(loadedlevel);
	this.objects.push(loadedlevel);
	return;
      }

      var newobj = this.spawn(gameobjects[x.from]);

      dainty_assign(newobj, x);
      if (x._pos)
        newobj.pos = x._pos;

      if (x._angle)
        newobj.angle = x._angle;
      for (var key in newobj.components)
        if ('sync' in newobj.components[key]) newobj.components[key].sync();

      newobj.sync();

      created.push(newobj);
    }, this);

    created.forEach(function(x) {
      if (x.varname)
        this[x.varname] = x;
    },this);

    this.pos = opos;
    this.angle = oangle;

    return created;
  },

  clear() {
    for (var i = this.objects.length-1; i >= 0; i--)
      if (this.objects[i].alive)
        this.objects[i].kill();

    this.levels.remove(this);
  },

  clear_all() {
    this.levels.forEach(function(x) { x.kill(); });
  },

  kill() {
    if (this.level)
      this.level.unregister(this);
    
    Register.unregister_obj(this);
      
    this.clear();
  },
  
  unregister(obj) {
    var removed = this.objects.remove(obj);

    if (removed && obj.varname)
      delete this[obj.varname];
  },

  get pos() { return this._pos; },
  set pos(x) {
    var diff = x.sub(this._pos);
    this.objects.forEach(function(x) { x.pos = x.pos.add(diff); });
    this._pos = x;
  },

  get angle() { return this._angle; },
  set angle(x) {
    var diff = x - this._angle;
    this.objects.forEach(function(x) {
      x.angle = x.angle + diff;
      var pos = x.pos.sub(this.pos);
      var r = Vector.length(pos);
      var p = Math.rad2deg(Math.atan2(pos.y, pos.x));
      p += diff;
      p = Math.deg2rad(p);
      x.pos = this.pos.add([r*Math.cos(p), r*Math.sin(p)]);
    },this);
    this._angle = x;
  },

  flipdirty: false,
  
  sync() {
    this.flipx = this.flipx;
    this.flipy = this.flipy;
  },

  _flipx: false,
  get flipx() { return this._flipx; },
  set flipx(x) {
    if (this._flipx === x && (!x || !this.flipdirty)) return;
    this._flipx = x;
    
    this.objects.forEach(function(obj) {
      obj.flipx = !obj.flipx;
      var rp = obj.relpos;
      obj.pos = [-rp.x, rp.y].add(this.pos);
      obj.angle = -obj.angle;
    },this);
  },
  
  _flipy: false,
  get flipy() { return this._flipy; },
  set flipy(x) {
    if (this._flipy === x && (!x || !this.flipdirty)) return;
    this._flipy = x;
    
    this.objects.forEach(function(obj) {
      var rp = obj.relpos;
      obj.pos = [rp.x, -rp.y].add(this.pos);
      obj.angle = -obj.angle;
    },this);
  },

  _scale: 1.0,
  get scale() { return this._scale; },
  set scale(x) {
    var diff = (x - this._scale) + 1;
    this._scale = x;

    this.objects.forEach(function(obj) {
      obj.scale *= diff;
      obj.relpos = obj.relpos.scale(diff);
    }, this);
  },
  
  get up() {
    return [0,1].rotate(Math.deg2rad(this.angle));
  },
  
  get down() {
    return [0,-1].rotate(Math.deg2rad(this.angle));
  },
  
  get right() {
    return [1,0].rotate(Math.deg2rad(this.angle));
  },
  
  get left() {
    return [-1,0].rotate(Math.deg2rad(this.angle));
  },
};

var World = Level.create();
World.name = "World";

var gameobjects = {};

/* Returns the index of the smallest element in array, defined by a function that returns a number */
Object.defineProperty(Array.prototype, 'min', {
  value: function(fn) {
    
  },
});

function grab_from_points(pos, points, slop) {
  var shortest = slop;
  var idx = -1;
  points.forEach(function(x,i) {
    if (Vector.length(pos.sub(x)) < shortest) {
      shortest = Vector.length(pos.sub(x));
      idx = i;
    }
  });
  return idx;
};

var gameobject = {
  get scale() { return this._scale; },
  set scale(x) {
    this._scale = Math.max(0,x);
    if (this.body > -1)
      cmd(36, this.body, this._scale);

    this.sync();
  },
  _scale: 1.0,

  save: true,
  
  selectable: true,

  layer: 0, /* Collision layer; should probably have been called "mask" */
  layer_nuke() {
    Nuke.label("Collision layer");
    Nuke.newline(Collision.num);
    for (var i = 0; i < Collision.num; i++)
      this.layer = Nuke.radio(i, this.layer, i);
  },

  _draw_layer: 1,
  set draw_layer(x) {
    if (x < 0) x = 0;
    if (x > 4) x = 4;
    this._draw_layer = x;
  },
  _draw_layer_nuke() {
    Nuke.label("Draw layer");
    Nuke.newline(5);
    for (var i = 0; i < 5; i++)
      this.draw_layer = Nuke.radio(i, this.draw_layer, i);
  },

  in_air() {
    return q_body(7, this.body);
  },

  get draw_layer() { return this._draw_layer; },

  name: "gameobject",

  toString() { return this.name; },

  clone(name, ext) {
    var obj = Object.create(this);
    complete_assign(obj, ext);
    gameobjects[name] = obj;
    obj.defc('name', name);
    obj.from = this.name;
    obj.defn('instances', []);
	
    return obj;
  },
  
  ed_locked: false,
  
  _visible:  true,
  get visible(){ return this._visible; },
  set visible(x) {
    this._visible = x; 
    for (var key in this.components) {
      if ('visible' in this.components[key]) {
        this.components[key].visible = x;
      }
    }
  },

  _mass: 1,
  set mass(x) { this._mass = Math.max(0,x); },
  get mass() { return this._mass; },
  bodytype: {
    dynamic: 0,
    kinematic: 1,
    static: 2
  },

  get moi() { return q_body(6, this.body); },
  
  phys: 2,
  phys_nuke() {
    Nuke.newline(1);
    Nuke.label("phys");
    Nuke.newline(3);
    this.phys = Nuke.radio("dynamic", this.phys, 0);
    this.phys = Nuke.radio("kinematic", this.phys, 1);
    this.phys = Nuke.radio("static", this.phys, 2);
  },
  _friction: 0,
  set friction(x) { this._friction = Math.max(0,x); },
  get friction() { return this._friction; },
  _elasticity: 0,
  set elasticity(x) { this._elasticity = Math.max(0, x); },
  get elasticity() { return this._elasticity; },
  
  _flipx: false,
  _flipy: false,
  get flipx() { return this._flipx; },
  set flipx(x) { this._flipx = x; if (this.alive) cmd(55, this.body, x); this.sync(); },
  get flipy() { return this._flipy; },
  set flipy(x) { this._flipy = x; if (this.alive) cmd(56, this.body, x); this.sync(); },
  
  body: -1,
  controlled: false,

  get properties() {
    var keys = [];
    for (var key of Object.keys(this)) {
      if (key.startsWith("_"))
        keys.push(key.substring(1));
      else
        keys.push(key);
    }

    return keys;
  },

  toJSON() {
    var obj = {};
    for (var key of this.properties)
      obj[key] = this[key];

    return obj;
  },
  
  set_center(pos) {
    var change = pos.sub(this.pos);
    this.pos = pos;
    
    for (var key in this.components) {
      this.components[key].finish_center(change);
    }
  },

  varname: "",
  
  _pos: [0,0],
  set pos(x) { this._pos = x; set_body(2, this.body, x); this.sync(); },
  get pos() {
    if (this.body !== -1)
      return q_body(1, this.body);
    else
      return this._pos;
  },

  set relpos(x) {
    if (!this.level) {
      this.pos = x;
      return;
    }

    this.pos = Vector.rotate(x, Math.deg2rad(this.level.angle)).add(this.level.pos);
  },

  get relpos() {
    if (!this.level) return this.pos;

    var offset = this.pos.sub(this.level.pos);
    return Vector.rotate(offset, -Math.deg2rad(this.level.angle));
  },
  
  _angle: 0,
  set angle(x) { this._angle = x; set_body(0, this.body, Math.deg2rad(x)); this.sync(); },
  get angle() {
    if (this.body !== -1)
      return Math.rad2deg(q_body(2, this.body)) % 360;
    else
      return this._angle;
  },

  get relangle() {
    if (!this.level) return this.angle;

    return this.angle - this.level.angle;
  },
  
  get velocity() { return q_body(3, this.body); },
  set velocity(x) { set_body(9, this.body, x); },
  get angularvelocity() { return Math.rad2deg(q_body(4, this.body)); },
  set angularvelocity(x) { if (this.alive) set_body(8, this.body, Math.deg2rad(x)); },

  get alive() { return this.body >= 0; },

  disable() {
    this.components.forEach(function(x) { x.disable(); });
    
  },

  enable() {
    this.components.forEach(function(x) { x.enable(); });
  },
  
  sync() {
    if (this.body === -1) return;
    cmd(55, this.body, this.flipx);
    cmd(56, this.body, this.flipy);
    set_body(2, this.body, this.pos);
    set_body(0, this.body, Math.deg2rad(this.angle));
    cmd(36, this.body, this.scale);
    set_body(10,this.body,this.elasticity);
    set_body(11,this.body,this.friction);
    set_body(1, this.body, this.phys);
    cmd(75,this.body,this.layer);
    cmd(54, this.body);
    if (this.components)
      for (var key in this.components)
        this.components[key].sync();
  },

  syncall() {
    this.instances.forEach(function(x) { x.sync(); });
  },
  
  pulse(vec) {
    set_body(4, this.body, vec);
  },

  push(vec) {
    set_body(12,this.body,vec);
  },

  gizmo: "", /* Path to an image to draw for this gameobject */

  set_pawn() {
    this.controlled = true;
    set_pawn(this);
  },

  uncontrol() {
    if (!this.controlled) return;
    unset_pawn(this);
  },

  /* Bounding box of the object in world dimensions */
  get boundingbox() {
    var boxes = [];
    boxes.push({t:0, r:0,b:0,l:0});
    for (var key in this.components) {
      if ('boundingbox' in this.components[key])
        boxes.push(this.components[key].boundingbox);
    }
    
    if (boxes.empty) return;
    
    var bb = boxes[0];
    
    boxes.forEach(function(x) {
      bb = bb_expand(bb, x);
    });
    
    var cwh = bb2cwh(bb);
    
    if (!bb) return;
    
    if (this.flipx) cwh.c.x *= -1;
    if (this.flipy) cwh.c.y *= -1;
    
    cwh.c = cwh.c.add(this.pos);
    bb = cwh2bb(cwh.c, cwh.wh);
    
    return bb ? bb : cwh2bb([0,0], [0,0]);
  },

  kill() {
    Log.warn(`Killing ${this.toString()}`);
    cmd(2, this.body);

    delete Game.objects[this.body];
    
    if (this.level)
      this.level.unregister(this);
      
    this.uncontrol();
    this.instances.remove(this);
    Register.unregister_obj(this);
    Signal.clear_obj(this);
    
    this.body = -1;
    for (var key in this.components) {
      Register.unregister_obj(this.components[key]);
      this.components[key].kill();
    }
  },

  prop_obj() {
    var obj = JSON.parse(JSON.stringify(this));
    delete obj.name;
    delete obj._pos;
    delete obj._angle;
    delete obj.from;
    return obj;
  },
  
  get up() {
    return [0,1].rotate(Math.deg2rad(this.angle));
  },
  
  get down() {
    return [0,-1].rotate(Math.deg2rad(this.angle));
  },
  
  get right() {
    return [1,0].rotate(Math.deg2rad(this.angle));
  },
  
  get left() {
    return [-1,0].rotate(Math.deg2rad(this.angle));
  },

  /* Make a unique object the same as its prototype */
  revert() {
    unmerge(this, this.prop_obj());
    this.sync();
  },

  gui() {
    var go_guis = walk_up_get_prop(this, 'go_gui');
    Nuke.newline();

    go_guis.forEach(function(x) { x.call(this); }, this);

    for (var key in this) {
      if (typeof this[key] === 'object' && 'gui' in this[key]) this[key].gui();
    }
  },

  world2this(pos) { return cmd(70, this.body, pos); },
  this2world(pos) { return cmd(71, this.body,pos); },

  make(props, level) {
    level ??= World;
    var obj = Object.create(this);
    this.instances.push(obj);
    obj.toString = function() {
      var props = obj.prop_obj();
      for (var key in props)
        if (typeof props[key] === 'object' && props[key].empty)
	  delete props[key];
	  
      var edited = !props.empty;
      return (edited ? "#" : "") + obj.name + " object " + obj.body + ", layer " + obj.draw_layer + ", phys " + obj.layer;
    };
    obj.deflock('toString');
    obj.defc('from', this.name);
    obj.defn('body', make_gameobject(this.scale,
                           this.phys,
                           this.mass,
                           this.friction,
                           this.elasticity) );
    complete_assign(obj, props);
    obj.sync();
    obj.defn('components', {});

    for (var prop in obj) {
       if (typeof obj[prop] === 'object' && 'make' in obj[prop]) {
	   if (prop === 'flipper') return;
           obj[prop] = obj[prop].make(obj.body);
	   obj[prop].defn('gameobject', obj);
	   obj.components[prop] = obj[prop];
       }
    };

    if (typeof obj.update !== 'undefined')
      register_update(obj.update, obj);

    if (typeof obj.physupdate === 'function')
      register_physupdate(obj.physupdate, obj);

    if (typeof obj.collide === 'function')
      obj.register_hit(obj.collide, obj);

    if (typeof obj.separate === 'function')
      obj.register_separate(obj.separate, obj);

    obj.components.forEach(function(x) {
      if (typeof x.collide === 'function')
        register_collide(1, x.collide, x, obj.body, x.shape);
    });

    if ('begin' in obj) obj.begin();

    return obj;
  },

  register_hit(fn, obj) {
    if (!obj)
      obj = this;

    Signal.obj_begin(fn, obj, this);
  },

  register_separate(fn, obj) {
    if (!obj)
      obj = this;

    Signal.obj_separate(fn,obj,this);
  },
}


var locks = ['draw_layer', 'friction','elasticity', 'visible', 'body', 'flipx', 'flipy', 'controlled', 'selectable', 'save', 'velocity', 'angularvelocity', 'alive', 'boundingbox', 'name', 'scale', 'angle', 'properties', 'moi', 'relpos', 'relangle', 'up', 'down', 'right', 'left', 'bodytype', 'gizmo', 'pos'];
locks.forEach(function(x) {
  Object.defineProperty(gameobject, x, {enumerable:false});
});

/* Load configs */
function load_configs(file) {
  var configs = JSON.parse(IO.slurp(file));
  for (var key in configs) {
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

var Collision = {
  types: {},
  num: 10,
  set_collide(a, b, x) {
    this.types[a][b] = x;
    this.types[b][a] = x;
  },
  sync() {
    for (var i = 0; i < this.num; i++)
      cmd(76,i,this.types[i]);
  },
  types_nuke() {
    Nuke.newline(this.num+1);
    Nuke.label("");
    for (var i = 0; i < this.num; i++) Nuke.label(i);
    
    for (var i = 0; i < this.num; i++) {
      Nuke.label(i);
      for (var j = 0; j < this.num; j++) {
        if (j < i)
	  Nuke.label("");
	else {
          this.types[i][j] = Nuke.checkbox(this.types[i][j]);
  	  this.types[j][i] = this.types[i][j];
	}
      }
    }
  },
};

for (var i = 0; i < Collision.num; i++) {
  Collision.types[i] = [];
  for (var j = 0; j < Collision.num; j++)
    Collision.types[i][j] = false;
};

if (IO.exists("game.config"))
  load_configs("game.config");

var camera2d = gameobject.clone("camera2d", {
    phys: gameobject.bodytype.kinematic,
    speed: 300,
    
    get zoom() { return this._zoom; },
    set zoom(x) {
      if (x <= 0) return;
      this._zoom = x;
      cmd(62, this._zoom);
    },
    _zoom: 1.0,
    speedmult: 1.0,
    
    selectable: false,
    
    view2world(pos) {
      return pos.mapc(mult, [1,-1]).add([-Window.width,Window.height].scale(0.5)).scale(this.zoom).add(this.pos);
    },
    
    world2view(pos) {
      return pos.sub(this.pos).scale(1/this.zoom).add(Window.dimensions.scale(0.5));
    },
});

win_make(Game.title, Game.resolution[0], Game.resolution[1]);
win_icon("icon.png");

/* Default objects */
gameobject.clone("polygon2d", {
  polygon2d: polygon2d.clone(),
});

gameobject.clone("edge2d", {
  edge2d: bucket.clone(),
});

gameobject.clone("sprite", {
  sprite: sprite.clone(),
});

load("config.js");

var prototypes = JSON.parse(slurp("proto.json"));
for (var key in prototypes) {
  if (key in gameobjects)
    dainty_assign(gameobjects[key], prototypes[key]);
  else {
    /* Create this gameobject fresh */
    Log.info("Making new prototype: " + key + " from " + prototypes[key].from);
    var newproto = gameobjects[prototypes[key].from].clone(key);
    gameobjects[key] = newproto;

    for (var pkey in newproto)
      if (typeof newproto[pkey] === 'object' && newproto[pkey] && 'clone' in newproto[pkey])
        newproto[pkey] = newproto[pkey].clone();

    dainty_assign(gameobjects[key], prototypes[key]);
  }
}

function save_gameobjects_as_prototypes() { slurpwrite(JSON.stringify(gameobjects,null,2), "proto.json"); };
