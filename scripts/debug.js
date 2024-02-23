/* All draw in screen space */
var Shape = {
  point(pos,size,color) {
    color ??= Color.blue;
    Shape.circle(pos,size,color);
  },

  line(points, color, thickness) {
    thickness ??= 1;
    color ??= Color.white;
    cmd(83, points, color, thickness);
  },

  poly(points, color) { cmd_points(0,points,color); },

  circle(pos, radius, color) { cmd(115, pos, radius, color); },

  /* size here is arm length - size of 2 is 4 height total */
  cross(pos, size, color) {
    color ??= Color.red;
    var a = [
      pos.add([0,size]),
      pos.add([0,-size])
    ];
    var b = [
      pos.add([size,0]),
      pos.add([-size,0])
    ];
    
    Shape.line(a,color);
    Shape.line(b,color);
  },
  
  arrow(start, end, color, wingspan, wingangle) {
    color ??= Color.red;
    wingspan ??= 4;
    wingangle ??=10;
    
    var dir = end.sub(start).normalized();
    var wing1 = [
      Vector.rotate(dir, wingangle).scale(wingspan).add(end),
      end
    ];
    var wing2 = [
      Vector.rotate(dir,-wingangle).scale(wingspan).add(end),
      end
    ];
    Shape.line([start,end],color);
    Shape.line(wing1,color);
    Shape.line(wing2,color);
  },

  rectangle(lowerleft, upperright, color) {
    var pos = lowerleft.add(upperright).map(x=>x/2);
    var wh = [upperright.x-lowerleft.x,upperright.y-lowerleft.y];
    Shape.box(pos,wh,color);
  },
  
  box(pos, wh, color) {
    color ??= Color.white;
    cmd(53, pos, wh, color);
  },

};

Shape.doc = "Draw shapes in screen space.";
Shape.circle.doc = "Draw a circle at pos, with a given radius and color.";
Shape.cross.doc = "Draw a cross centered at pos, with arm length size.";
Shape.arrow.doc = "Draw an arrow from start to end, with wings of length wingspan at angle wingangle.";
Shape.poly.doc = "Draw a concave polygon from a set of points.";
Shape.rectangle.doc = "Draw a rectangle, with its corners at lowerleft and upperright.";
Shape.box.doc = "Draw a box centered at pos, with width and height in the tuple wh.";
Shape.line.doc = "Draw a line from a set of points, and a given thickness.";

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
  call_fn_n(fn1, n) { profile(7,fn1,n,fn2); },
};

Profile.test.call_fn_n.doc = "Calls fn1 n times, and then fn2.";

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

console.assert = function(assertion, msg, objs)
{
  if (!assertion) {
    console.error(msg);
    console.stack();
  }
}

var say = function(msg) {
  console.say(msg);
}

say.doc = "Print to std out with an appended newline.";

var gist = function(o)
{
  if (typeof o === 'object') return json.encode(o,null,1);
  if (typeof o === 'string') return o;
  return o.toString();
}
gist.doc = "Return the best string gist of an object.";

var API = {};
API.doc_entry = function(obj, key)
{
  if (typeof key !== 'string') {
    console.warn("Cannot print a key that isn't a string.");
    return undefined;
  }
  
  var title = key;
  
  var o = obj[key];
  if (typeof o === 'undefined' && obj.impl && typeof obj.impl[key] !== 'undefined')
    o = obj.impl[key];

  var t = typeof o;
  if (Array.isArray(o)) t = "array";
  else if (t === 'function') {
    title = o.toString().tofirst(')') + ")";
    title = title.fromfirst('(');
    title = key + "(" + title;
    if (o.doc) doc = o.doc;
    t = "";
  } else if (t === 'undefined') t = "";

  if (t) t = "**" + t + "**\n";

  var doc = "";
  if (o.doc) doc = o.doc;
  else if (obj.doc && obj.doc[key]) doc = obj.doc[key];
  else if (Array.isArray(o)) doc = json.encode(o);

  return `## ${title}
${t}
${doc}
`;
}

API.print_doc =  function(name)
{
  var obj = name;
  if (typeof name === 'string') {
    obj = eval(name);
    if (!obj) {
      console.warn(`Cannot print the API of '${name}', as it was not found.`);
      return undefined;
    }

    obj = globalThis[name];
  }
    
    obj = eval(name);

  if (!Object.isObject(obj)) {
    console.warn("Cannot print the API of something that isn't an object.");
    return undefined;
  }

  if (!obj) {
    console.warn(`Object '${name}' does not exist.`);
    return;
  }
  
  var mdoc = "# " + name + "\n";
  if (obj.doc?.doc) mdoc += obj.doc.doc + "\n";
  else if (typeof obj.doc === 'string') mdoc += obj.doc + "\n";
  
  for (var key in obj) {
    if (key === 'doc') continue;
    if (key === 'toString') continue;

    mdoc += API.doc_entry(obj, key) + "\n";
  }

  return mdoc;
}
