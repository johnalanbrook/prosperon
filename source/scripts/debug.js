var Gizmos = {
  pick_gameobject_points(worldpos, gameobject, points) {
    var idx = grab_from_points(worldpos, points.map(gameobject.this2world,gameobject), 25);
    if (idx === -1) return null;
    return points[idx];
  },
};


var Debug = {
  draw_grid(width, span) {
    cmd(47, width, span);
  },
  
  point(pos, size, color) {
    color = color ? color : Color.blue;
    cmd(51, pos, size,color);
  },
  
  arrow(start, end, color, capsize) {
    color = color ? color : Color.red;
    if (!capsize)
      capsize = 4;
    cmd(81, start, end, color, capsize);
  },
  
  box(pos, wh, color) {
    color = color ? color : Color.white;
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

  line(points, color, type) {
    if (!type)
      type = 0;

    if (!color)
      color = Color.white;
      
    switch (type) {
      case 0:
        cmd(83, points, color);
    }
  },
};

var Gizmos = {
  pick_gameobject_points(worldpos, gameobject, points) {
    var idx = grab_from_points(worldpos, points.map(gameobject.this2world,gameobject), 25);
    if (idx === -1) return null;
    return points[idx];
  },
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

