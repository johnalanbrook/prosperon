var Gizmos = {
  pick_gameobject_points(worldpos, gameobject, points) {
    var idx = grab_from_points(worldpos, points.map(gameobject.this2world,gameobject), 25);
    if (idx === -1) return null;
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
    gui_text(n, world2screen(pos).add([0,4]), 1);
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
    register_debug(fn,obj);
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
      Game.objects.forEach(function(x) { bb_draw(x.boundingbox); });

    if (Game.paused()) gui_text("PAUSED", [0,0],1);

    if (this.draw_gizmos)
      Game.objects.forEach(function(x) {
        if (!x.icon) return;
        gui_img(x.icon, world2screen(x.pos));
      });

    if (this.draw_names)
      Game.objects.forEach(function(x) {
        GUI.text(x.fullpath(), world2screen(x.pos).add([0,32]), 1, [84,110,255]);
      });

    gui_text(Game.playing() ? "PLAYING"
                         : Game.paused() ?
			 "PAUSED" :
			 "STOPPED", [0, 0], 1);
			
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
  cpu(fn, times) {
    times ??= 1;
    var start = Date.now();
    for (var i = 0; i < times; i++)
      fn();

    Log.warn(`Profiled in ${(Date.now()-start)/1000} seconds.`);
  },

  get fps() { return sys_cmd(8); },
};

var Nuke = {
  newline(cols) { nuke(3, cols ? cols : 1); },
  newrow(height) { nuke(13,height); },
  
  wins: {},
  curwin:"",
  
  prop(str, v) {
    var ret = nuke(2, str, v);
    if (Number.isFinite(ret)) return ret;
    return 0;
  },

  treeid: 0,

  tree(str) { var on = nuke(11, str, this.treeid); this.treeid++; return on; },
  tree_pop() { nuke(12);},

  prop_num(str, num) { return nuke(2, str, num, -1e10, 1e10, 0.01); },
  prop_bool(str, val) { return nuke(4, str, val); },
  checkbox(val) { return nuke(4,"",val); },
  label(str) { nuke(5, str); },
  textbox(str) { return nuke(7, str); },
  scrolltext(str) { nuke(14,str); },
  
  defaultrect: { x:10, y:10, w:400, h:600 },
  window(name) {
    this.curwin = name;
    var rect;
    if (name in this.wins)
      rect = this.wins[name];
    else
      rect = { x:10, y:10, w:400, h:600 };

    nuke(0, name, rect);
  },
  button(name) { return nuke(6, name); },
  radio(name, val, cmp) { return nuke(9, name, val, cmp); },
  img(path) { nuke(8, path); },
  end() {
    this.wins[this.curwin] = nuke(10);
    this.treeid = 0;
    nuke(1);
  },

  pprop(str, p, nonew) {
    switch(typeof p) {
      case 'number':
        if (!nonew) Nuke.newline();
        return Nuke.prop_num(str, p);
	break;

      case 'boolean':
        if (!nonew) Nuke.newline();
        return Nuke.prop_bool(str, p);

      case 'object':
        if (Array.isArray(p)) {
	  var arr = [];
          Nuke.newline(p.length+1);
	  Nuke.label(str);
	  arr[0] = Nuke.pprop("#x", p[0], true);
	  arr[1] = Nuke.pprop("#y", p[1], true);
	  return arr;
	
        } else {
          if (!nonew)Nuke.newline(2);
  	  Nuke.label(str);
          Nuke.label(p);
        }
	break;

      case 'string':
        if (!nonew) Nuke.newline();
        Nuke.label(str);
        return Nuke.textbox(p);

      default:
        if (!nonew) Nuke.newline(2);
        Nuke.label(str);
        Nuke.label(p);
     }
  },
};

Object.defineProperty(Nuke, "curwin", {enumerable:false});
Object.defineProperty(Nuke, "defaultrect", {enumerable:false});

var DebugControls = {};
DebugControls.inputs = {};
DebugControls.inputs.f1 = function () { Debug.draw_phys(!Debug.phys_drawing); };
DebugControls.inputs.f1.doc = "Draw physics debugging aids.";
DebugControls.inputs.f3 = function() { Debug.draw_bb = !Debug.draw_bb; };
DebugControls.inputs.f3.doc = "Toggle drawing bounding boxes.";
DebugControls.inputs.f4 = function() {
  Debug.draw_names = !Debug.draw_names;
  Debug.draw_gizmos = !Debug.draw_gizmos;
};
DebugControls.inputs.f4.doc = "Toggle drawing gizmos and names of objects.";
DebugControls.inputs.f10 = function() { Time.timescale = 0.1; };
DebugControls.inputs.f10.doc = "Toggle timescale to 1/10.";
DebugControls.inputs.f10.released = function () { Time.timescale = 1.0; Log.warn("SET TIMESCALE");};
DebugControls.inputs.f12 = function() { GUI.defaults.debug = !GUI.defaults.debug; Log.warn("GUI toggle debug");};
DebugControls.inputs.f12.doc = "Toggle drawing GUI debugging aids.";

DebugControls.inputs.f5 = function() {
  if (Game.paused())
    Game.play();
  else
    Game.pause();
};
DebugControls.inputs.f5.doc = "Pause or play game simulation."

DebugControls.inputs.f6 = function() {
  if (Game.paused())
    Game.step();
};
DebugControls.inputs.f6.doc = "Do one step through game while paused.";
 
DebugControls.inputs['M-1'] = Render.normal;
Render.normal.doc = "Render mode for enabling all shaders and lighting effects.";
DebugControls.inputs['M-2'] = Render.wireframe;
Render.wireframe.doc = "Render mode to see wireframes of all models.";

var Time = {
  set timescale(x) { cmd(3, x); },
  set updateMS(x) { cmd(6, x); },
  set physMS(x) { cmd(7, x); },
  set renderMS(x) { cmd(5, x); },
};

set_pawn(DebugControls);
register_gui(Debug.draw, Debug);