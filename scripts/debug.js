debug.build = function(fn) { if (!debug.show) return; fn(); }
debug.show = true;
debug.urnames = false;
debug.termout = true;
debug.console = false;
debug.cheat = false;

debug.fn_break = function(fn,obj = globalThis) {
  if (typeof fn !== 'function') return;
  
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
debug.sprite_nums = false;
debug.draw = function() {
  if (this.draw_phys) game.all_objects(function(x) { debug.draw_gameobject(x); });
  
  if (this.draw_bb)
    game.all_objects(function(x) { debug.boundingbox(x.boundingbox(), Color.debug.boundingbox.alpha(0.05)); });

  if (this.draw_gizmos)
    game.all_objects(function(x) {
      if (!x.icon) return;
      gui.image(x.icon, game.camera.world2view(x.pos));
    });

  if (this.draw_names)
    game.all_objects(function(x) {
      render.text(x, game.camera.view2screen(x.pos).add([0,32]), 1, Color.debug.names);
    });

  if (debug.gif.rec) {
    render.text("REC", [0,40], 1);
    render.text(time.timecode(time.timenow() - debug.gif.start_time, debug.gif.fps), [0,30], 1);
  }
  
  return;
  
  if (sim.paused()) render.text("PAUSED", [0,0],1);  

  render.text(sim.playing() ? "PLAYING"
                       : sim.stepping() ?
		                   "STEP" :
		                   sim.paused() ?
		                   "PAUSED; EDITING" : 
		                   "EDIT", [0, 0], 1);
}

var assert = function(op, str = `assertion failed [value '${op}']`)
{
  if (!op)
    console.panic(str);
}

var Gizmos = {
  pick_gameobject_points(worldpos, gameobject, points) {
    var idx = Math.grab_from_points(worldpos, points.map(gameobject.this2world,gameobject), 25);
    if (idx === -1) return undefined;
    return idx;
  },
};

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

var gif = {
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
debug.inputs.f12 = function() { gui.defaults.debug = !gui.defaults.debug; console.warn("gui toggle debug");};
debug.inputs.f12.doc = "Toggle drawing gui debugging aids.";

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

  return `#### ${title}
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

  var keys = Object.keys(obj);
  for (var key of keys) {
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
