var Gizmos = {
  pick_gameobject_points(worldpos, gameobject, points) {
    var idx = grab_from_points(worldpos, points.map(gameobject.this2world,gameobject), 25);
    if (idx === -1) return undefined;
    return points[idx];
  },
};

var Shape = {
  circle(pos, radius, color) {
    cmd(115, pos, radius, color);
  },
};

var Debug = {
  draw_grid(width, span, color) {
    color = color ? color : Color.green;
    cmd(47, width, span, color);
  },
  
  point(pos, size, color) {
    color = color ? color : Color.blue;
    Shape.circle(pos, size, color);
//    cmd(51, pos, size,color);
  },
  
  arrow(start, end, color, capsize) {
    color = color ? color : Color.red;
    if (!capsize)
      capsize = 4;
    cmd(81, start, end, color, capsize);
  },

  poly(points, color) {
    cmd_points(0,points,color);
  },

  boundingbox(bb, color) {
    color ??= Color.white;
    cmd_points(0, bb2points(bb), color);
  },
  
  box(pos, wh, color) {
    color ??= Color.white;
    cmd(53, pos, wh, color);
  },

  numbered_point(pos, n) {
    Debug.point(world2screen(pos), 3);
    GUI.text(n, world2screen(pos).add([0,4]), 1);
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

  line(points, color, type, thickness) {
    thickness ??= 1;
    
    if (!type)
      type = 0;

    if (!color)
      color = Color.white;
      
    switch (type) {
      case 0:
        cmd(83, points, color, thickness);
    }
  },

  draw_bb: false,
  draw_gizmos: false,
  draw_names: false,

  draw() {
    if (this.draw_bb)
      Game.objects.forEach(function(x) { Debug.boundingbox(x.boundingbox(), Color.Debug.boundingbox.alpha(0.05)); });

    if (Game.paused()) GUI.text("PAUSED", [0,0],1);

    if (this.draw_gizmos)
      Game.objects.forEach(function(x) {
        if (!x.icon) return;
        gui_img(x.icon, world2screen(x.pos));
      });

    if (this.draw_names)
      Game.objects.forEach(function(x) {
        GUI.text(x, world2screen(x.pos).add([0,32]), 1, Color.Debug.names);
      });

    if (Debug.Options.gif.rec) {
      gui_text("REC", [0,40], 1);
      gui_text(Time.seconds_to_timecode(Time.time - Debug.Options.gif.start_time, Debug.Options.gif.fps), [0,30], 1);
    }

    GUI.text(Game.playing() ? "PLAYING"
                         : Game.stepping() ?
			 "STEP" :
			 Game.paused() ?
			 "PAUSED; EDITING" : 
			 "EDIT", [0, 0], 1);
  },
};

Debug.Options = { };
Debug.Options.Color = {
  set trigger(x) { cmd(17,x); },
  set debug(x) { cmd(16, x); },
};

var Gizmos = {
  pick_gameobject_points(worldpos, gameobject, points) {
    var idx = grab_from_points(worldpos, points.map(gameobject.this2world,gameobject), 25);
    if (idx === -1) return null;
    return points[idx];
  },
};

var Profile = {
  tick_now() { return cmd(127); },
  ns(ticks) { return cmd(128, ticks); },
  us(ticks) { return cmd(129, ticks); },
  ms(ticks) { return cmd(130, ticks); },
  cpu(fn, times, q) {
    times ??= 1;
    q ??= "ns";
    var start = Profile.tick_now();
    for (var i = 0; i < times; i++)
      fn();
    var elapsed = Profile.tick_now() - start;
    Log.say(`Profiled in ${Profile[q](elapsed)/times} avg ${q}.`);
  },

  get fps() { return sys_cmd(8); },
};

/* These controls are available during editing, and during play of debug builds */
var DebugControls = {};
DebugControls.inputs = {};
DebugControls.inputs.f1 = function () { Debug.draw_phys(!Debug.phys_drawing); };
DebugControls.inputs.f1.doc = "Draw physics debugging aids.";
//DebugControls.inputs.f3 = function() { Debug.draw_bb = !Debug.draw_bb; };
//DebugControls.inputs.f3.doc = "Toggle drawing bounding boxes.";
DebugControls.inputs.f4 = function() {
  Debug.draw_names = !Debug.draw_names;
  Debug.draw_gizmos = !Debug.draw_gizmos;
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

Player.players[0].control(DebugControls);
Register.gui.register(Debug.draw, Debug);
