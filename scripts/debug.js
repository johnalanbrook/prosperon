debug.fn_break = function(fn,obj) {
  if (typeof fn !== 'function') return;
  obj ??= globalThis;
  
  var newfn = function() {
    console.log("broke");
    fn();
  };
  obj[fn.name] = newfn;   
}

debug.draw_phys = false;
debug.draw_bb = false;
debug.draw_gizmos = false;
debug.draw_names = false;
debug.draw = function() {
  if (this.draw_phys) game.all_objects(function(x) { debug.draw_gameobject(x.body); });
  
  if (this.draw_bb)
    game.all_objects(function(x) { debug.boundingbox(x.boundingbox(), Color.debug.boundingbox.alpha(0.05)); });



  if (this.draw_gizmos)
    game.all_objects(function(x) {
      if (!x.icon) return;
      GUI.image(x.icon, window.world2screen(x.pos));
    });

  if (this.draw_names)
    game.all_objects(function(x) {
      GUI.text(x, window.world2screen(x.pos).add([0,32]), 1, Color.debug.names);
    });

  if (debug.gif.rec) {
    GUI.text("REC", [0,40], 1);
    GUI.text(time.timecode(time.timenow() - debug.gif.start_time, debug.gif.fps), [0,30], 1);
  }
  
  return;
  
  if (sim.paused()) GUI.text("PAUSED", [0,0],1);  

  GUI.text(sim.playing() ? "PLAYING"
                       : sim.stepping() ?
		 "STEP" :
		 sim.paused() ?
		 "PAUSED; EDITING" : 
		 "EDIT", [0, 0], 1);
}

function assert(op, str)
{
  str ??= `assertion failed [value '${op}']`;
  if (!op) {
    console.error(`Assertion failed: ${str}`);
    os.quit();
  }
}

var Gizmos = {
  pick_gameobject_points(worldpos, gameobject, points) {
    var idx = Math.grab_from_points(worldpos, points.map(gameobject.this2world,gameobject), 25);
    if (idx === -1) return undefined;
    return idx;
  },
};

Object.assign(profile, {
  best_t(t) {
    var qq = 'ns';
    if (t > 1000) {
      t /= 1000;
      qq = 'us';
      if (t > 1000) {
        t /= 1000;
	qq = 'ms';
      }
    }
    return `${t.toPrecision(4)} ${qq}`;
  },
  cpu(fn, times, q) {
    times ??= 1;
    q ??= "unnamed";
    var start = profile.now();
    for (var i = 0; i < times; i++)
      fn();
      
    var elapsed = profile.now() - start;
    var avgt = profile.best_t(elapsed/times);
    var totalt = profile.best_t(elapsed);

    say(`profile [${q}]: ${profile.best_t(avgt)} average [${profile.best_t(totalt)} for ${times} loops]`);
  },

  time(fn) {
    var start = profile.now();
    fn();
    return profile.lap(start);
  },

  lap(t) {
    return profile.best_t(profile.now()-t);
  },

  measure(fn, str) {
    str ??= 'unnamed';
    var start = profile.now();
    fn();
    say(`profile [${str}]: ${profile.lap(start)}`);
  },

  secs() { return this.now()/1000000000; },
});

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

/* These controls are available during editing, and during play of debug builds */
debug.inputs = {};
debug.inputs.f1 = function () { debug.draw_phys =  !debug.draw_phys; };
debug.inputs.f1.doc = "Draw physics debugging aids.";
debug.inputs.f3 = function() { debug.draw_bb = !debug.draw_bb; };
debug.inputs.f3.doc = "Toggle drawing bounding boxes.";
debug.inputs.f4 = function() {
  debug.draw_names = !debug.draw_names;
  debug.draw_gizmos = !debug.draw_gizmos;
};
debug.inputs.f4.doc = "Toggle drawing gizmos and names of objects.";

debug.gif = {
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
      var win = window.height / window.width;    
      var gif = h/w;
      if (gif > win)
        h = w * win;
      else
        w = h / win;
    }

//    cmd(131, w, h, this.cpf, this.depth);
    this.rec = true;
    this.fps = (1/this.cpf)*100;
    this.start_time = time.now();

    timer.oneshot(this.stop.bind(this), this.secs, this, true);
  },

  stop() {
    if (!this.rec) return;
//    cmd(132, this.file);
    this.rec = false;
  },
};

debug.inputs.f8 = function() {
  var now = new Date();
  debug.gif.file = now.toISOString() + ".gif";
  debug.gif.start();
};
debug.inputs.f9 = function() {
  debug.gif.stop();
}

debug.inputs.f10 = function() { time.timescale = 0.1; };
debug.inputs.f10.doc = "Toggle timescale to 1/10.";
debug.inputs.f10.released = function () { time.timescale = 1.0; };
debug.inputs.f12 = function() { GUI.defaults.debug = !GUI.defaults.debug; console.warn("GUI toggle debug");};
debug.inputs.f12.doc = "Toggle drawing GUI debugging aids.";

debug.inputs['M-1'] = render.normal;
debug.inputs['M-2'] = render.wireframe;

debug.inputs['C-M-f'] = function() {};
debug.inputs['C-M-f'].doc = "Enter camera fly mode.";

debug.api = {};
debug.api.doc_entry = function(obj, key)
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

debug.api.print_doc =  function(name)
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

    mdoc += debug.api.doc_entry(obj, key) + "\n";
  }

  return mdoc;
}

return {
  debug,
  Gizmos,
  assert
}
