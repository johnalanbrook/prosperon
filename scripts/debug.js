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
    cmd_points(0, bbox.topoints(bb), color);
  },

  numbered_point(pos, n, color) {
    color ??= Color.white;
    render.point(pos, 3, color);
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
        GUI.image(x.icon, Window.world2screen(x.pos));
      });

    if (this.draw_names)
      Game.all_objects(function(x) {
        GUI.text(x, Window.world2screen(x.pos).add([0,32]), 1, Color.Debug.names);
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

function assert(op, str)
{
  str ??= `assertion failed [value '${op}']`;
  if (!op)
    console.critical(`Assertion failed: ${str}`);
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

Object.assign(performance, {
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
    var start = performance.tick_now();
    for (var i = 0; i < times; i++)
      fn();
      
    var elapsed = performance.tick_now() - start;
    var avgt = performance.best_t(elapsed/times);
    var totalt = performance.best_t(elapsed);

    console.say(`performance [${q}]: ${avgt.time.toFixed(3)} ${avgt.unit} average [${totalt.time.toFixed(3)} ${totalt.unit} for ${times} loops]`);
  },

  get fps() { return sys_cmd(8); },

  measure(fn, str) {
    str ??= 'unnamed';
    var start = performance.tick_now();
    fn();
    var elapsed = performance.tick_now()-start;
    elapsed = performance.best_t(elapsed);
    say(`performance [${str}]: ${elapsed.time.toFixed(3)} ${elapsed.unit}`);
  },
});

performance.now = performance.tick_now;

performance.test = {
  barecall() { performance(0); },
  unpack_num(n) { performance(1,n); },
  unpack_array(n) { performance(2,n); },
  pack_num() { performance(3); },
  pack_string() { performance(6); },
  unpack_string(s) { performance(4,s); },
  unpack_32farr(a) { performance(5,a); },
  call_fn_n(fn1, n) { performance(7,fn1,n,fn2); },
};

performance.test.call_fn_n.doc = "Calls fn1 n times, and then fn2.";

performance.cpu.doc = `Output the time it takes to do a given function n number of times. Provide 'q' as "ns", "us", or "ms" to output the time taken in the requested resolution.`;

/* These controls are available during editing, and during play of debug builds */
Debug.inputs = {};
Debug.inputs.f1 = function () { Debug.draw_phys(!Debug.phys_drawing); };
Debug.inputs.f1.doc = "Draw physics debugging aids.";
//Debug.inputs.f3 = function() { Debug.draw_bb = !Debug.draw_bb; };
//Debug.inputs.f3.doc = "Toggle drawing bounding boxes.";
Debug.inputs.f4 = function() {
  Debug.draw_names = !Debug.draw_names;
  Debug.draw_gizmos = !Debug.draw_gizmos;
};
Debug.inputs.f4.doc = "Toggle drawing gizmos and names of objects.";

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

Debug.inputs.f8 = function() {
  var now = new Date();
  Debug.Options.gif.file = now.toISOString() + ".gif";
  Debug.Options.gif.start();
};
Debug.inputs.f9 = function() {
  Debug.Options.gif.stop();
}

Debug.inputs.f10 = function() { Time.timescale = 0.1; };
Debug.inputs.f10.doc = "Toggle timescale to 1/10.";
Debug.inputs.f10.released = function () { Time.timescale = 1.0; };
Debug.inputs.f12 = function() { GUI.defaults.debug = !GUI.defaults.debug; console.warn("GUI toggle debug");};
Debug.inputs.f12.doc = "Toggle drawing GUI debugging aids.";

Debug.inputs['M-1'] = render.normal;
Debug.inputs['M-2'] = render.wireframe;

Debug.inputs['C-M-f'] = function() {};
Debug.inputs['C-M-f'].doc = "Enter camera fly mode.";

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
      console.warn("Tried to resume time without calling Time.pause first.");
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

Debug.api = {};
Debug.api.doc_entry = function(obj, key)
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

Debug.api.print_doc =  function(name)
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

    mdoc += Debug.api.doc_entry(obj, key) + "\n";
  }

  return mdoc;
}

return {
  Debug,
  Time,
  Gizmos,
  performance,
  assert
}
