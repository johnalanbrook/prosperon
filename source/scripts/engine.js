var files = {};
function load(file) {
  var modtime = cmd(0, file);

  if (modtime === 0) {
    Log.stack();
    return false;
  }
  files[file] = modtime;
}

var Cmdline = {};

Cmdline.cmds = [];
Cmdline.register_cmd = function(flag, fn, desc) {
  Cmdline.cmds.push({
    flag: flag,
    fn: fn,
    desc: desc
  });
};

function cmd_args(cmdargs)
{
  var play = false;
  var cmds = cmdargs.split(" ");

  Cmdline.play = false;

  for (var i = 0; i < cmds.length; i++) {
    if (cmds[i][0] !== '-')
      continue;

    var c = Cmdline.cmds.find(function(cmd) { return cmd.flag === cmds[i].slice(1); });
    if (c && c.fn)
      c.fn();
  }

  if (Cmdline.play)
    run("scripts/play.js");
  else
    run("scripts/editor.js");
}

Cmdline.register_cmd("p", function() { Cmdline.play = true; }, "Launch engine in play mode.");
Cmdline.register_cmd("v", function() { Log.warn(cmd(120)); }, "Display engine info.");
Cmdline.register_cmd("c", null, "Redirect logging to console.");
Cmdline.register_cmd("l", null, "Set logging file name.");
Cmdline.register_cmd("h", function() { Log.warn("Helping."); exit();}, "Help.");

function run(file)
{
//  var text = IO.slurp(file);
//  eval?.(`"use strict";${text}`);
//  return;
  var modtime = cmd(119, file);
  if (modtime === 0) {
    Log.stack();
    return false;
  }

  files[file] = modtime;
  return cmd(117, file);
}

load("scripts/base.js");

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
    var stack = (new Error()).stack;
    var n = stack.next('\n',0)+1;
    for (var i = 0; i < skip; i++)
      n = stack.next('\n', n)+1;

    this.write(stack.slice(n));
  },
};


load("scripts/diff.js");

var Physics = {
  dynamic: 0,
  kinematic: 1,
  static: 2,
};

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


var Color = {
  white: [255,255,255,255],
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

var GUI = {
  text(str, pos, size, color, wrap) {
    size = size ? size : 1;
    color = color ? color : [255,255,255,255];
    wrap = wrap ? wrap : -1;

    var bb = cmd(118, str, size, wrap);
    var opos = [bb.r, bb.t];
        
    var h = ui_text(str, pos.sub(opos), size, color, wrap);

    return bb;
  },

  text_cursor(str, pos, size, cursor) {
    cursor_text(str,pos,size,[255,255,255],cursor);
  },

  image(path,pos) {
    let wh = cmd(64,path);
    gui_img(path,pos.slice().sub(wh), 1.0, 0.0);
    return cwh2bb([0,0], wh);
  },

  image_fn(defn) {
    var def = Object.create(this.defaults);
    Object.assign(def,defn);
    if (!def.path) {
      Log.warn("GUI image needs a path.");
      def.draw = function(){};
      return def;
    }

    var tex_wh = cmd(64,def.path);
    var wh = tex_wh.slice();

    if (def.width !== 0)
      wh.x = def.width;

    if (def.height !== 0)
      wh.y = def.height;

    wh = wh.scale(def.scale);

    var sendscale = [];
    sendscale.x = wh.x / tex_wh.x;
    sendscale.y = wh.y / tex_wh.y;

    def.draw = function(pos) {
      def.calc_bb(pos);
      gui_img(def.path, pos.sub(def.anchor.scale(wh)), sendscale, def.angle, def.image_repeat, def.image_repeat_offset, def.color);
    };

    def.calc_bb = function(cursor) {
      def.bb = cwh2bb(wh.scale([0.5,0.5]), wh);
      def.bb = movebb(def.bb, cursor.sub(wh.scale(def.anchor)));
    };

    return def;
  },

  defaults: {
    padding:[2,2], /* Each element inset with this padding on all sides */
    font: "fonts/LessPerfectDOSVGA.ttf",
    font_size: 1,
    text_align: "left",
    scale: 1,
    angle: 0,
    anchor: [0,0],
    text_shadow: {
      pos: [0,0],
      color: [255,255,255,255]
    },
    text_outline: 1, /* outline in pixels */
    color: [255,255,255,255],
    margin: [5,5], /* Distance between elements for things like columns */
    width: 0,
    height: 0,
    image_repeat: false,
    image_repeat_offset: [0,0],
    debug: false, /* set to true to draw debug boxes */
  },

  text_fn(str, defn)
  {
    var def = Object.create(this.defaults);
    Object.assign(def,defn);
    
    def.draw = function(cursor) {
      def.calc_bb(cursor);

      if (def.debug)
        Debug.boundingbox(def.bb, def.debug_colors.bounds);
      
      var old = def;
      def = Object.create(def);

/*      if (pointinbb(def.bb, Mouse.screenpos)) {
        Object.assign(def, def.hovered);
	def.calc_bb(cursor);
	GUI.selected = def;
	def.selected = true;
      }
*/
      if (def.selected) {
        Object.assign(def, def.hovered);
	def.calc_bb(cursor);
      }

      var pos = cursor.sub(bb2wh(def.bb).scale(def.anchor));

      ui_text(str, pos, def.font_size, def.color, def.width);

      def = old;
    };

    def.calc_bb = function(cursor) {
      var bb = cmd(118, str, def.font_size, def.width);
      var wh = bb2wh(bb);
      var pos = cursor.sub(wh.scale(def.anchor));
      def.bb = movebb(bb,pos);
    };

    return def;
  },

  column(defn) {
    var def = Object.create(this.defaults);
    Object.assign(def,defn);

    if (!def.items) {
      Log.warn("Columns needs items.");
      def.draw = function(){};
      return def;
    };

    def.items.forEach(function(item,idx) {
      Object.setPrototypeOf(def.items[idx], def);

      if (def.items[idx-1])
        def.up = def.items[idx-1];

      if (def.items[idx+1])
        def.down = def.items[idx+1];
    });

    def.draw = function(pos) {
        def.items.forEach(function(item) {
	  item.draw.call(this,pos);
          var wh = bb2wh(item.bb);
          pos.y -= wh.y;
          pos.y -= def.padding.x*2;
        });
    };

    return def;
  },

  input_lmouse_pressed() {
    if (GUI.selected)
      GUI.selected.action();
  },

  input_s_pressed() {
    if (GUI.selected.down) {
      GUI.selected.selected = false;
      GUI.selected = GUI.selected.down;
      GUI.selected.selected = true;
    }
  },

  input_w_pressed() {
    if (GUI.selected.up) {
      GUI.selected.selected = false;
      GUI.selected = GUI.selected.up;
      GUI.selected.selected = true;
    }
  },

  input_enter_pressed() {
    if (GUI.selected) {
      GUI.selected.action();
    }
  }
};



GUI.defaults.debug_colors = {
  bounds: Color.red.slice(),
  margin: Color.blue.slice(),
  padding: Color.green.slice()
};

Object.values(GUI.defaults.debug_colors).forEach(function(v) { v.a = 100; });

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
  guardfn(fn) {
    if (typeof fn === 'function')
      fn();
    else {
      Log.warn("TIMER TRYING TO EXECUTE WIHTOUT!!!");
      Log.warn(this);
      this.kill();
    }
  },

  make(fn, secs,obj,loop) {
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
    t.id = make_timer(guardfn, secs, obj);
    
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
  get pct() { return this.remain / this.time; },
};

/* Take numbers from 0 to 1 and remap them to easing functions */
var Ease = {
  linear(t) { return t; },

  in(t) { return t*t; },

  out(t) {
    var d = 1-t;
    return 1 - d*d
  },

  inout(t) {
    var d = -2*t + 2;
    return t < 0.5 ? 2 * t * t : 1 - (d * d) / 2;
  },
};

function make_easing_fns(num) {
  var obj = {};

  obj.in = function(t) {
    return Math.pow(t,num);
  };

  obj.out = function(t) {
    return 1 - Math.pow(1 - t, num);
  };

  var mult = Math.pow(2, num-1);

  obj.inout = function(t) {
    return t < 0.5 ? mult * Math.pow(t, num) : 1 - Math.pow(-2 * t + 2, num) / 2;
  };

  return obj;
};

Ease.quad = make_easing_fns(2);
Ease.cubic = make_easing_fns(3);
Ease.quart = make_easing_fns(4);
Ease.quint = make_easing_fns(5);

Ease.expo = {
  in(t) {
    return t === 0 ? 0 : Math.pow(2, 10 * t - 10);
  },

  out(t) {
    return t === 1 ? 1 : 1 - Math.pow(2, -10 * t);
  },

  inout(t) {
    return t === 0 ? 0 : t === 1 ? 1 : t < 0.5 ? Math.pow(2, 20 * t - 10) / 2 : (2 - Math.pow(2, -20 * t + 10)) / 2;
  }
};

Ease.bounce = {
  in(t) {
    return 1 - this.out(t - 1);
  },

  out(t) {
    var n1 = 7.5625;
    var d1 = 2.75;

    if (t < 1 / d1) { return n1 * t * t; }
    else if (t < 2 / d1) { return n1 * (t -= 1.5 / d1) * t + 0.75; }
    else if (t < 2.5 / d1) { return n1 * (t -= 2.25 / d1) * t + 0.9375; }
    else
      return n1 * (t -= 2.625 / d1) * t + 0.984375;
  },

  inout(t) {
    return t < 0.5 ? (1 - this.out(1 - 2 * t)) / 2 : (1 + this.out(2 * t - 1)) / 2;
  }
};

Ease.sine = {
  in(t) { return 1 - Math.cos((t * Math.PI)/2); },

  out(t) { return Math.sin((t*Math.PI)/2); },

  inout(t) { return -(Math.cos(Math.PI*t) - 1) / 2; }
};

Ease.elastic = {
  in(t) {
    return t === 0 ? 0 : t === 1 ? 1 : -Math.pow(2, 10*t-10) * Math.sin((t * 10 - 10.75) * this.c4);
  },

  out(t) {
    return t === 0 ? 0 : t === 1 ? 1 : Math.pow(2, -10*t) * Math.sin((t * 10 - 0.75) * this.c4) + 1;
  },

  inout(t) {
    t === 0 ? 0 : t === 1 ? 1 : t < 0.5 ?
      -(Math.pow(2, 20 * t - 10) * Math.sin((20 * t - 11.125) * this.c5)) / 2
      : (Math.pow(2, -20 * t + 10) * Math.sin((20 * t - 11.125) * this.c5)) / 2 + 1;
  },
};

Ease.elastic.c4 = 2*Math.PI/3;
Ease.elastic.c5 = 2*Math.PI / 4.5;

var Tween = {
  default: {
    loop: "restart", /* none, restart, yoyo, circle */ 
    time: 1, /* seconds to do */
    ease: Ease.linear,
    whole: true,
  },

  start(obj, target, tvals, options)
  {
    var defn = Object.create(this.default);
    Object.assign(defn, options);

    if (defn.loop === 'circle')
      tvals.push(tvals[0]);
    else if (defn.loop === 'yoyo') {
      for (var i = tvals.length-2; i >= 0; i--)
        tvals.push(tvals[i]);
    }

    defn.accum = 0;

    var slices = tvals.length - 1;
    var slicelen = 1 / slices;

    defn.fn = function(dt) {
      defn.accum += dt;
      defn.pct = (defn.accum % defn.time) / defn.time;

      var t = defn.whole ? defn.ease(defn.pct) : defn.pct;

      var nval = t / slicelen;
      var i = Math.trunc(nval);
      nval -= i;

      if (!defn.whole)
        nval = defn.ease(nval);

      obj[target] = tvals[i].lerp(tvals[i+1], nval);
    };

    defn.restart = function() { defn.accum = 0; };
    defn.stop = function() { defn.pause(); defn.restart(); };
    defn.pause = function() { unregister_update(defn.fn); };

    register_update(defn.fn, defn);

    return defn;
  },

  embed(obj, target, tvals, options) {
    var defn = Object.create(this.default);
    Object.assign(defn, options);

    defn.update_vals = function(vals) {
      defn.vals = vals;
      
      if (defn.loop === 'circle')
        defn.vals.push(defn.vals[0]);
      else if (defn.loop === 'yoyo') {
        for (var i = defn.vals.length-2; i >= 0; i--)
          defn.vals.push(defn.vals[i]);
      }

      defn.slices = defn.vals.length - 1;
      defn.slicelen = 1 / defn.slices;
    };

    defn.update_vals(tvals);

    defn.time_s = Date.now();

    Object.defineProperty(obj, target, {
      get() {
        defn.accum = (Date.now() - defn.time_s)/1000;
	defn.pct = (defn.accum % defn.time) / defn.time;
	var t = defn.whole ? defn.ease(defn.pct) : defn.pct;

	var nval = t / defn.slicelen;
	var i = Math.trunc(nval);
	nval -= i;

	if (!defn.whole)
	  nval = defn.ease(nval);

	return defn.vals[i].lerp(defn.vals[i+1],nval);
      },
    });

    return defn;
  },
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

  get screenpos() {
    var p = this.pos;
    p.y = Window.dimensions.y - p.y;
    return p;
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

function screen2world(screenpos) { return Yugine.camera.view2world(screenpos); }
function world2screen(worldpos) { return Yugine.camera.world2view(worldpos); }

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
  
  updates: [],
  update(dt) {
    this.wraploop(() => this.updates.forEach(x => x[0].call(x[1], dt)));
  },

  physupdates: [],
  physupdate(dt) {
    this.wraploop(() => this.physupdates.forEach(x => x[0].call(x[1], dt)));
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
//    Log.warn(`Unregister ${JSON.stringify(obj)}`);  
    this.updates = this.updates.filter(x => x[1] !== obj);
    this.guis = this.guis.filter(x => x[1] !== obj);
    this.nk_guis = this.nk_guis.filter(x => x[1] !== obj);
    this.debugs = this.debugs.filter(x => x[1] !== obj);
    this.physupdates = this.physupdates.filter(x => x[1] !== obj);
    this.draws = this.draws.filter(x => x[1] !== obj);
    Player.players.forEach(x => x.uncontrol(obj));
  },

  draws: [],
  draw() {
    this.draws.forEach(x => x[0].call(x[1]));
  },

  endofloop(fn) {
    if (!this.inloop)
      fn();
    else {
      this.loopcbs.push(fn);
    }
  },
};

register(0, Register.update, Register);
register(1, Register.physupdate, Register);
register(2, Register.gui, Register);
register(3, Register.nk_gui, Register);
register(6, Register.debug, Register);
register(7, Register.kbm_input, Register);
register(8, Register.gamepad_input, Register);
register(9, Log.stack, this);
register(10, Register.draw, Register);

Register.gamepad_playermap[0] = Player.players[0];

function register_update(fn, obj) {
  Register.updates.push([fn, obj ? obj : null]);
};

function unregister_update(fn) {
  Register.updates = Register.updates.filter(function(updatefn) {
    return fn === updatefn;
  });
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
  Register.guis = Register.guis.filter(x => x[0] !== fn || x[1] !== obj);
};

function register_nk_gui(fn, obj) {
  Register.nk_guis.push([fn, obj ? obj : this]);
};

function unregister_nk_gui(fn, obj) {
  Register.nk_guis = Register.nk_guis.filter(x => x[0] !== fn && x[1] !== obj);
};

function register_draw(fn,obj) {
  Register.draws.push([fn, obj ? obj : this]);
}

register_update(Yugine.exec, Yugine);

/* These functions are the "defaults", and give control to player0 */
function set_pawn(obj, player = Player.players[0]) {
  player.control(obj);
}

function unset_pawn(obj, player = Player.players[0]) {
  player.uncontrol(obj);
}

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
  width: 0,
  height: 0,
  dimensions:[0,0],
};

Signal.register("window_resize", function(w, h) {
  Window.width = w;
  Window.height = h;
  Window.dimensions = [w, h];
});

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

    Log.warn(`MAKING LEVEL ${file}`);

    if (IO.exists(file)) {
      newlevel.filejson = IO.slurp(file);
      
      try {
	newlevel.filelvl = JSON.parse(newlevel.filejson);
	newlevel.load(newlevel.filelvl);
      } catch (e) {
        newlevel.ed_gizmo = function() { GUI.text("Invalid level file: " + newlevel.file, world2screen(newlevel.pos), 1, Color.red); };
	newlevel.selectable = false;
	throw e;
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

    objs.forEach(x => {
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
      var prototype = gameobjects[x.from];
      if (!prototype) {
        Log.error(`Prototype for ${x.from} does not exist.`);
	return;
      }

      var newobj = this.spawn(gameobjects[x.from]);

      delete x.from;

      dainty_assign(newobj, x);
      
      if (x._pos)
        newobj.pos = x._pos;
	

      if (x._angle)
        newobj.angle = x._angle;
      for (var key in newobj.components)
        if ('sync' in newobj.components[key]) newobj.components[key].sync();

      newobj.sync();

      created.push(newobj);
    });

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
  scale: 1.0,

  save: true,
  
  selectable: true,

  layer: 0, /* Collision layer; should probably have been called "mask" */
  layer_nuke() {
    Nuke.label("Collision layer");
    Nuke.newline(Collision.num);
    for (var i = 0; i < Collision.num; i++)
      this.layer = Nuke.radio(i, this.layer, i);
  },

  draw_layer: 1,
  draw_layer_nuke() {
    Nuke.label("Draw layer");
    Nuke.newline(5);
    for (var i = 0; i < 5; i++)
      this.draw_layer = Nuke.radio(i, this.draw_layer, i);
  },

  in_air() {
    return q_body(7, this.body);
  },

  on_ground() { return !this.in_air(); },

  name: "gameobject",

  toString() { return this.name; },

  clone(name, ext) {
    var obj = Object.create(this);
    complete_assign(obj, ext);
    gameobjects[name] = obj;
    obj.defc('name', name);
    obj.from = this.name;
    obj.defn('instances', []);
    obj.obscure('from');
	
    return obj;
  },

  instandup() {
    var dup = World.spawn(gameobjects[this.from]);
    dup.pos = this.pos;
    dup.velocity = this.velocity;
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

  mass: 1,
  bodytype: {
    dynamic: 0,
    kinematic: 1,
    static: 2
  },

  get moi() { return q_body(6, this.body); },
  set moi(x) { set_body(13, this.body, x); },
  
  phys: 2,
  phys_nuke() {
    Nuke.newline(1);
    Nuke.label("phys");
    Nuke.newline(3);
    this.phys = Nuke.radio("dynamic", this.phys, 0);
    this.phys = Nuke.radio("kinematic", this.phys, 1);
    this.phys = Nuke.radio("static", this.phys, 2);
  },
  friction: 0,
  elasticity: 0,
  flipx: false,
  flipy: false,
  
  body: -1,
  controlled: false,

  set_center(pos) {
    var change = pos.sub(this.pos);
    this.pos = pos;
    
    for (var key in this.components) {
      this.components[key].finish_center(change);
    }
  },

  varname: "",
  
  pos: [0,0],
  
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
  
  angle: 0,
  
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
  },

  syncall() {
    this.instances.forEach(function(x) { x.sync(); });
  },
  
  pulse(vec) { /* apply impulse */
    set_body(4, this.body, vec);
  },

  push(vec) { /* apply force */
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
    
    if (boxes.empty) return cwh2bb([0,0], [0,0]);
    
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

  set width(x) {},
  get width() {
    var bb = this.boundingbox;
    return bb.r - bb.l;
  },
  set height(x) {},
  get height() {
    var bb = this.boundingbox;
    return bb.t-bb.b;
  },

  stop() {},

  

  kill() {
    if (this.body === -1) {
      Log.warn(`Object is already dead!`);
      return;
    }
    Register.endofloop(() => {
      cmd(2, this.body);
      delete Game.objects[this.body];
    
      if (this.level)
        this.level.unregister(this);
      
      this.uncontrol();
      this.instances.remove(this);
      Register.unregister_obj(this);
//      Signal.clear_obj(this);
    
      this.body = -1;
      for (var key in this.components) {
        Register.unregister_obj(this.components[key]);
        this.components[key].kill();
      }

      this.stop();
    });
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

  check_registers(obj) {
    Register.unregister_obj(this);
  
    if (typeof obj.update === 'function')
      register_update(obj.update, obj);

    if (typeof obj.physupdate === 'function')
      register_physupdate(obj.physupdate, obj);

    if (typeof obj.collide === 'function')
      obj.register_hit(obj.collide, obj);

    if (typeof obj.separate === 'function')
      obj.register_separate(obj.separate, obj);

    if (typeof obj.draw === 'function')
      register_draw(obj.draw,obj);

    if (typeof obj.debug === 'function')
      register_debug(obj.debug, obj);

    obj.components.forEach(function(x) {
      if (typeof x.collide === 'function')
        register_collide(1, x.collide, x, obj.body, x.shape);
    });
  },

  make(props, level) {
    level ??= World;
    var obj = Object.create(this);
    this.instances.push(obj);
    obj.toString = function() {
      var props = obj.prop_obj();
      for (var key in props)
        if (typeof props[key] === 'object' && !props[key] === null && props[key].empty)
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

    cmd(113, obj.body, obj);

    complete_assign(obj, {
      set scale(x) { cmd(36, this.body, x); },
      get scale() { return cmd(103, this.body); },
      get flipx() { return cmd(104,this.body); },
      set flipx(x) { cmd(55, this.body, x); },
      get flipy() { return cmd(105,this.body); },
      set flipy(x) { cmd(56, this.body, x); },

      get angle() { return Math.rad2deg(q_body(2,this.body))%360; },
      set angle(x) { set_body(0,this.body, Math.deg2rad(x)); },

      set pos(x) { set_body(2,this.body,x); },
      get pos() { return q_body(1,this.body); },

      get elasticity() { return cmd(107,this.body); },
      set elasticity(x) { cmd(106,this.body,x); },

      get friction() { return cmd(109,this.body); },
      set friction(x) { cmd(108,this.body,x); },

      set mass(x) { set_body(7,this.body,x); },
      get mass() { return q_body(5, this.body); },

      set phys(x) { set_body(1, this.body, x); },
      get phys() { return q_body(0,this.body); },
    });

    for (var prop in obj) {
       if (typeof obj[prop] === 'object' && 'make' in obj[prop]) {
	   if (prop === 'flipper') return;
           obj[prop] = obj[prop].make(obj.body);
	   obj[prop].defn('gameobject', obj);
	   obj.components[prop] = obj[prop];
       }
    };

    obj.check_registers(obj);

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


var locks = ['height', 'width', 'visible', 'body', 'controlled', 'selectable', 'save', 'velocity', 'angularvelocity', 'alive', 'boundingbox', 'name', 'scale', 'angle', 'properties', 'moi', 'relpos', 'relangle', 'up', 'down', 'right', 'left', 'bodytype', 'gizmo', 'pos'];
locks.forEach(x => gameobject.obscure(x));
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

Yugine.camera = World.spawn(camera2d);
cmd(61, Yugine.camera.id);

win_make(Game.title, Game.resolution[0], Game.resolution[1]);
//win_icon("icon.png");

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


if (IO.exists("config.js"))
  load("config.js");

var prototypes = {};
if (IO.exists("proto.json"))
  prototypes = JSON.parse(slurp("proto.json"));

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

var Gamestate = {};
