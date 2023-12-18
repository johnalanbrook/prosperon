/* All draw in screen space */
var Shape = {
  circle(pos, radius, color) { cmd(115, pos, radius, color); },
  
  point(pos,size,color) {
    color ??= Color.blue;
    Shape.circle(pos,size,color);
  },
  
  arrow(start, end, color, capsize) {
    color ??= Color.red;
    capsize ??= 4;
    cmd(81, start, end, color, capsize);
  },
  
  poly(points, color) { cmd_points(0,points,color); },
  
  box(pos, wh, color) {
    color ??= Color.white;
    cmd(53, pos, wh, color);
  },

  line(points, color, type, thickness) {
    thickness ??= 1;
    type ??= 0;
    color ??= Color.white;
    
    switch (type) {
      case 0:
        cmd(83, points, color, thickness);
    }
  },
};

var Debug = {
  fn_break(fn, obj) {
    if (typeof fn !== 'function') return;
    obj ??= globalThis;
    
    var newfn = function() {
      console.log("broke");
      fn();
    };
    obj[fn.name] = newfn;    
  },

  draw_grid(width, span, color) {
    color = color ? color : Color.green;
    cmd(47, width, span, color);
  },
  
  coordinate(pos, size, color) { GUI.text(JSON.stringify(pos.map(p=>Math.round(p))), pos, size, color); },

  boundingbox(bb, color) {
    color ??= Color.white;
    cmd_points(0, bb2points(bb), color);
  },

  numbered_point(pos, n, color) {
    color ??= Color.white;
    Shape.point(pos, 3, color);
    GUI.text(n, pos.add([0,4]), 1, color);
  },

  phys_drawing: false,
  draw_phys(on) {
    this.phys_drawing = on;
    cmd(4, this.phys_drawing);
  },

  draw_obj_phys(obj) {
    cmd(82, obj.body);
  },

  register_call(fn, obj) {
    Register.debug.register(fn,obj);
  },

  gameobject(go) { cmd(15, go.body); },

  draw_bb: false,
  draw_gizmos: false,
  draw_names: false,

  draw() {
    if (this.draw_bb)
      Game.all_objects(function(x) { Debug.boundingbox(x.boundingbox(), Color.Debug.boundingbox.alpha(0.05)); });

    if (Game.paused()) GUI.text("PAUSED", [0,0],1);

    if (this.draw_gizmos)
      Game.all_objects(function(x) {
        if (!x.icon) return;
        GUI.image(x.icon, world2screen(x.pos));
      });

    if (this.draw_names)
      Game.all_objects(function(x) {
        GUI.text(x, world2screen(x.pos).add([0,32]), 1, Color.Debug.names);
      });

    if (Debug.Options.gif.rec) {
      GUI.text("REC", [0,40], 1);
      GUI.text(Time.seconds_to_timecode(Time.time - Debug.Options.gif.start_time, Debug.Options.gif.fps), [0,30], 1);
    }

    GUI.text(Game.playing() ? "PLAYING"
                         : Game.stepping() ?
			 "STEP" :
			 Game.paused() ?
			 "PAUSED; EDITING" : 
			 "EDIT", [0, 0], 1);
  },
};

Debug.assert = function(b, str)
{
  str ??= "";
  
  if (!b) {
    console.error(`Assertion failed. ${str}`);
    Game.quit();
  }
}

Debug.Options = { };
Debug.Options.Color = {
  set trigger(x) { cmd(17,x); },
  set debug(x) { cmd(16, x); },
};

var Gizmos = {
  pick_gameobject_points(worldpos, gameobject, points) {
    var idx = Math.grab_from_points(worldpos, points.map(gameobject.this2world,gameobject), 25);
    if (idx === -1) return undefined;
    return idx;
  },
};

var Profile = {
  tick_now() { return cmd(127); },
  ns(ticks) { return cmd(128, ticks); },
  us(ticks) { return cmd(129, ticks); },
  ms(ticks) { return cmd(130, ticks); },
  best_t(ns) {
    var e = ns;
    var qq = 'ns';
    if (e > 1000) {
      e /= 1000;
      qq = 'us';
      if (e > 1000) {
        e /= 1000;
	qq = 'ms';
      }
    }
    return {
      time: e,
      unit: qq
    };
  },
  cpu(fn, times, q) {
    times ??= 1;
    q ??= "unnamed";
    var start = Profile.tick_now();
    for (var i = 0; i < times; i++)
      fn();
      
    var elapsed = Profile.tick_now() - start;
    var avgt = Profile.best_t(elapsed/times);
    var totalt = Profile.best_t(elapsed);

    console.say(`Profile [${q}]: ${avgt.time.toFixed(3)} ${avgt.unit} average [${totalt.time.toFixed(3)} ${totalt.unit} for ${times} loops]`);
  },

  get fps() { return sys_cmd(8); },
};



Profile.test = {
  barecall() { profile(0); },
  unpack_num(n) { profile(1,n); },
  unpack_array(n) { profile(2,n); },
  pack_num() { profile(3); },
  pack_string() { profile(6); },
  unpack_string(s) { profile(4,s); },
  unpack_32farr(a) { profile(5,a); },
};

Profile.cpu.doc = `Output the time it takes to do a given function n number of times. Provide 'q' as "ns", "us", or "ms" to output the time taken in the requested resolution.`;

/* These controls are available during editing, and during play of debug builds */
var DebugControls = {};
DebugControls.toString = function() { return "Debug"; };
DebugControls.inputs = {};
DebugControls.inputs.f1 = function () { Debug.draw_phys(!Debug.phys_drawing); };
DebugControls.inputs.f1.doc = "Draw physics debugging aids.";
//DebugControls.inputs.f3 = function() { Debug.draw_bb = !Debug.draw_bb; };
//DebugControls.inputs.f3.doc = "Toggle drawing bounding boxes.";
DebugControls.inputs.f4 = function() {
//  Debug.draw_names = !Debug.draw_names;
//  Debug.draw_gizmos = !Debug.draw_gizmos;
};
DebugControls.inputs.f4.doc = "Toggle drawing gizmos and names of objects.";

Debug.Options.gif = {
  w: 640, /* Max width */
  h: 480, /* Max height */
  stretch: false, /* True if you want to stretch */
  cpf: 4,
  depth: 16,
  file: "out.gif",
  rec: false,
  secs: 6,
  start_time: 0,
  fps: 0,
  start() {
    var w = this.w;
    var h = this.h;
    if (!this.stretch) {
      var win = Window.height / Window.width;    
      var gif = h/w;
      if (gif > win)
        h = w * win;
      else
        w = h / win;
    }

    cmd(131, w, h, this.cpf, this.depth);
    this.rec = true;
    this.fps = (1/this.cpf)*100;
    this.start_time = Time.time;

    timer.oneshot(this.stop.bind(this), this.secs, this, true);
  },

  stop() {
    if (!this.rec) return;
    cmd(132, this.file);
    this.rec = false;
  },
};

DebugControls.inputs.f8 = function() {
  var now = new Date();
  Debug.Options.gif.file = now.toISOString() + ".gif";
  Debug.Options.gif.start();
};
DebugControls.inputs.f9 = function() {
  Debug.Options.gif.stop();
}

DebugControls.inputs.f10 = function() { Time.timescale = 0.1; };
DebugControls.inputs.f10.doc = "Toggle timescale to 1/10.";
DebugControls.inputs.f10.released = function () { Time.timescale = 1.0; };
DebugControls.inputs.f12 = function() { GUI.defaults.debug = !GUI.defaults.debug; Log.warn("GUI toggle debug");};
DebugControls.inputs.f12.doc = "Toggle drawing GUI debugging aids.";

DebugControls.inputs['M-1'] = Render.normal;
Render.normal.doc = "Render mode for enabling all shaders and lighting effects.";
DebugControls.inputs['M-2'] = Render.wireframe;
Render.wireframe.doc = "Render mode to see wireframes of all models.";

DebugControls.inputs['C-M-f'] = function() {};
DebugControls.inputs['C-M-f'].doc = "Enter camera fly mode.";

var Time = {
  set timescale(x) { cmd(3, x); },
  get timescale() { return cmd(121); },
  set updateMS(x) { cmd(6, x); },
  set physMS(x) { cmd(7, x); },
  set renderMS(x) { cmd(5, x); },

  get time() { return cmd(133); },

  seconds_to_timecode(secs, fps)
  {
    var s = Math.trunc(secs);
    secs -= s;
    var f = Math.trunc(fps * secs);
    return `${s}:${f}`;
  },

  pause() {
    Time.stash = Time.timescale;
    Time.timescale = 0;
  },

  play() {
    if (!Time.stash) {
      Log.warn("Tried to resume time without calling Time.pause first.");
      return;
    }
    Time.timescale = Time.stash;
  },
};

Time.doc = {};
Time.doc.timescale = "Get and set the timescale. 1 is normal time; 0.5 is half speed; etc.";
Time.doc.updateMS = "Set the ms per game update.";
Time.doc.physMS = "Set the ms per physics update.";
Time.doc.renderMS = "Set the ms per render update.";
Time.doc.time = "Seconds elapsed since the game started.";
Time.doc.pause = "Pause the game by setting the timescale to 0; remembers the current timescale on play.";
Time.doc.play = "Resume the game after using Time.pause.";

Player.players[0].control(DebugControls);
Register.gui.register(Debug.draw, Debug);

var console = Object.create(Log);
console.log = function(str)
{
  console.say(time.text(time.now(), 'yyyy-m-dd hh:nn:ss') + "  " + str);
}
console.clear = function()
{
  cmd(146);
}

var API = {};
API.doc_entry = function(obj, key)
{
  var d = obj.doc;
  var doc = "";
  var title = key;
  
  var o = obj[key];
  if (typeof o === 'undefined' && obj.impl && typeof obj.impl[key] !== 'undefined')
    o = obj.impl[key];

  var t = typeof o;
  if (t === 'object' && Array.isArray(o)) t = 'array';

  if (t === 'function') {
    title = o.toString().tofirst(')') + ")";
    if (o.doc) doc = o.doc;
    t = "";
  }
  if (t === 'undefined') t = "";

  if (t) t = "**" + t + "**";

  if (!doc) {
    if (d && d[key]) doc = d[key];
    else return "";
  }

  return `### \`${title}\`
${t}
${doc}
`;
}

API.print_doc =  function(name)
{
  var obj = eval(name);
  if (!obj.doc) {
    Log.warn(`Object has no doc sidecar.`);
    return;
  }

  var mdoc = "# " + name + " API #\n";
  if (obj.doc?.doc) mdoc += obj.doc.doc + "\n";
  else if (typeof obj.doc === 'string') mdoc += obj.doc + "\n";
  for (var key in obj) {
    if (key === 'doc') continue;
    if (key === 'toString') continue;
    mdoc += API.doc_entry(obj, key);
  }

  return mdoc;
}
