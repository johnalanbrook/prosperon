var component = {
  toString() {
    if ('gameobject' in this)
      return this.name + " on " + this.gameobject;
    else
      return this.name;
  },
  name: "component",
  component: true,
  enabled: true,
  enable() { this.enabled = true; },
  disable() { this.enabled = false; },
  
  make(go) { },
  kill() { Log.info("Kill not created for this component yet"); },
  gui() { },
  gizmo() { },
  
  prepare_center() {},
  finish_center() {},
  clone(spec) {
    return clone(this, spec);
  },
};

var sprite = clone(component, {
  name: "sprite",
  path: "",
  layer: 0,
  pos: [0,0],
  get visible() { return this.enabled; },
  set visible(x) { this.enabled = x; },
  set asset(str) { this.path = str; Log.warn(`SET ${str} ON THE SPRITE`); this.sync();},
  angle: 0,
  rect: {s0:0, s1: 1, t0: 0, t1: 1},

  get dimensions() { return cmd(64,this.path); },
  set dimensions(x) {},

  make(go) {
    var sprite = Object.create(this);
    sprite.id = make_sprite(go,this.path,this.pos);
    complete_assign(sprite, {
      get enabled() { return cmd(114,this.id); },
      set enabled(x) { cmd(20,this.id,x); },
      set color(x) { cmd(96,this.id,x); },
      get pos() { return cmd(111, this.id); },
      set pos(x) { cmd(37,this.id,x); },
      set layer(x) { cmd(60, this.id, x); },
      get layer() { return this.gameobject.draw_layer; },

      get boundingbox() {
        var dim = this.dimensions;
	dim = dim.scale(this.gameobject.scale);
	var realpos = this.pos.copy();
	realpos.x = realpos.x * dim.x + (dim.x/2);
	realpos.y = realpos.y * dim.y + (dim.y/2);
	return cwh2bb(realpos,dim);
      },

      sync() {
        if (this.path)
          cmd(12,this.id,this.path,this.rect);
      },

      kill() { cmd(9,this.id); },
    });
    sprite = new Proxy(sprite, sync_proxy);
    sprite.obscure('boundingbox');
    return sprite;
  },

  input_kp9_pressed() { this.pos = [0,0]; },
  input_kp8_pressed() { this.pos = [-0.5, 0]; },
  input_kp7_pressed() { this.pos = [-1,0]; },
  input_kp6_pressed() { this.pos = [0,-0.5]; },
  input_kp5_pressed() { this.pos = [-0.5,-0.5]; },
  input_kp4_pressed() { this.pos = [-1,-0.5]; },
  input_kp3_pressed() { this.pos = [0, -1]; },
  input_kp2_pressed() { this.pos = [-0.5,-1]; },
  input_kp1_pressed() { this.pos = [-1,-1]; },
});

/* Container to play sprites and anim2ds */
var char2d = clone(sprite, {
  clone(anims) {
    var char = clone(this);
    char.anims = anims;
    return char;
  },
  
  name: "char 2d",
  
  frame2rect(frames, frame) {
    var rect = {s0:0,s1:1,t0:0,t1:1};
    
    var frameslice = 1/frames;
    rect.s0 = frameslice*frame;
    rect.s1 = frameslice*(frame+1);

    return rect;
  },
  
  make(go) {
    var char = clone(this, {
      get enabled() { return cmd(114,this.id); },
      set enabled(x) { cmd(20,this.id,x); },
      set color(x) { cmd(96,this.id,x); },
      get pos() { return cmd(111, this.id); },
      set pos(x) { cmd(37,this.id,x); },
      set layer(x) { cmd(60, this.id, x); },
      get layer() { return this.gameobject.draw_layer; },

      get boundingbox() {
        var dim = cmd(64,this.path);
	dim = dim.scale(this.gameobject.scale);	
	dim.x *= 1/6;
	var realpos = [0,0];
//	var realpos = this.pos.slice();

//	realpos.x = realpos.x * dim.x + (dim.x/2);
//	realpos.y = realpos.y * dim.y + (dim.y/2);
	return cwh2bb(realpos,dim);
      },

      sync() {
        if (this.path)
	  cmd(12,this.id,this.path,this.rect);
      },

      kill() { cmd(9,this.id); },
    });

    char.curplaying = char.anims.array()[0];
    char.obscure('curplaying');
    char.id = make_sprite(go, char.curplaying.path, this.pos);    

    char.obscure('id');
    char.frame = 0;
    char.timer = timer.make(char.advance.bind(char), 1/char.curplaying.fps);
    char.timer.loop = true;
    char.obscure('timer');
//    char.obscure('rect');
    char.rect = {};
    char.setsprite();
    return char;
  },
  
  frame: 0,
  
  play(name) {
    if (!(name in this.anims)) {
      Log.info("Can't find an animation named " + name);
      return;
    }
    
    if (this.curplaying === this.anims[name]) {
      this.timer.start();
      return;
    }
    
    this.curplaying = this.anims[name];
    this.timer.time = 1/this.curplaying.fps;
    this.timer.start();
    this.frame = 0;
    this.setsprite();
  },
  
  setsprite() {
    this.path = this.curplaying.path;
    this.rect = this.frame2rect(this.curplaying.frames, this.frame);
    cmd(12, this.id, this.path, this.rect);
  },

  advance() {
    this.frame = (this.frame + 1) % this.curplaying.frames;
    this.setsprite();

    if (this.frame === 0 && !this.curplaying.loop)
      this.timer.pause();
  },

  devance() {
    this.frame = (this.frame - 1);
    if (this.frame === -1) this.frame = this.curplaying.frames-1;
    this.setsprite();
  },

  setframe(frame) {
    this.frame = frame;
    this.setsprite();
  },
  
  pause() {
    this.timer.pause();
  },

  stop() {
    this.setframe(0);
    this.timer.stop();
  },
  
  kill() {
    this.timer.kill();
    cmd(9, this.id);
  },
});

/* Returns points specifying this geometry, with ccw */
var Geometry = {
  box(w, h) {
    w /= 2;
    h /= 2;

    var points = [
      [w,h],
      [-w,h],
      [-w,-h],
      [w,-h]
    ];

    return points;
  }
};

/* For all colliders, "shape" is a pointer to a phys2d_shape, "id" is a pointer to the shape data */
var collider2d = clone(component, {
  name: "collider 2d",
  sensor: false,

  input_s_pressed() {
    if (!Keys.alt()) return;

    this.sensor = !this.sensor;
  },

  input_t_pressed() {
    if (!Keys.alt()) return;

    this.enabled = !this.enabled;
  },

  kill() {}, /* No killing is necessary - it is done through the gameobject's kill */

  register_hit(fn, obj) {
    register_collide(1, fn, obj, this.gameobject.body, this.shape);
  },

  make_fns: {
    set sensor(x) { cmd(18,this.shape,x); },
    get sensor() { return cmd(21,this.shape); },
    set enabled(x) { cmd(22,this.shape,x); },
    get enabled() { return cmd(23,this.shape); }
  },
  
});

var sync_proxy = {
  set(t,p,v) {
    t[p] = v;
    t.sync();
    return true;
  }
};

var polygon2d = clone(collider2d, {
  name: "polygon 2d",
  points: [],
  mirrorx: false,
  mirrory: false,

  clone(spec) {
    var obj = Object.create(this);
    obj.points = this.points.copy();
    Object.assign(obj, spec);
    return obj;
  },

  make(go) {
    var poly = Object.create(this);
    Object.assign(poly, make_poly2d(go, this.points));
    
    complete_assign(poly, this.make_fns);
    complete_assign(poly, {
      get boundingbox() {
        return points2bb(this.spoints);
      },

      sync() { cmd_poly2d(0, this.id, this.spoints); }
    });
    
    poly.obscure('boundingbox');

    poly.defn('points', this.points.copy());

    poly = new Proxy(poly, sync_proxy);

    Object.defineProperty(poly, 'id', {enumerable:false});
    Object.defineProperty(poly, 'shape', {enumerable:false});

    poly.sync();

    return poly;
  },

  /* EDITOR */  
  help: "Ctrl-click Add a point\nShift-click Remove a point",  
  
  input_f10_pressed() {
    this.points = sortpointsccw(this.points);
  },

  input_b_pressed() {
    if (!Keys.ctrl()) return;
    
    this.points = this.spoints;
    this.mirrorx = false;
    this.mirrory = false;
  },
  
  get spoints() {
    var spoints = this.points.slice();
    
    if (this.mirrorx) {
      spoints.forEach(function(x) {
        var newpoint = x.slice();
	newpoint.x = -newpoint.x;
        spoints.push(newpoint);
      });
    }
    
    if (this.mirrory) {
      spoints.forEach(function(x) {
        var newpoint = x.slice();
	newpoint.y = -newpoint.y;
	spoints.push(newpoint);
      });
    }

    return spoints;
  },
  
  gizmo() {
    if (!this.hasOwn('points')) this.points = this.__proto__.points.copy();
    
    this.spoints.forEach(function(x) {
      Debug.point(world2screen(this.gameobject.this2world(x)), 3, Color.green);
    }, this);
    
    this.points.forEach(function(x, i) {
      Debug.numbered_point(this.gameobject.this2world(x), i);
    }, this);
  },
  
  input_lmouse_pressed() {
    if (Keys.ctrl()) {
      this.points.push(this.gameobject.world2this(Mouse.worldpos));
    } else if (Keys.shift()) {
      var idx = grab_from_points(screen2world(Mouse.pos), this.points.map(this.gameobject.this2world,this.gameobject), 25);
      if (idx === -1) return;
      this.points.splice(idx, 1);
    }
  },

  pick(pos) {
    return Gizmos.pick_gameobject_points(pos, this.gameobject, this.points);
  },
  
  query() {
    return cmd(80, this.shape);
  },
  
  input_m_pressed() {
    if (Keys.ctrl())
      this.mirrory = !this.mirrory;
    else
      this.mirrorx = !this.mirrorx;
  },
});

var bucket = clone(collider2d, {
  name: "bucket",
  clone(spec) {
    var obj = Object.create(this);
    obj.cpoints = this.cpoints.copy(); 
    dainty_assign(obj, spec);
    return obj;
  },

  cpoints:[],
  degrees:2,
  dimensions:2,
  /* open: 0
     clamped: 1
     beziers: 2
     looped: 3
  */
  type: 3,
  typeid: {
    open: 0,
    clamped: 1,
    beziers: 2,
    looped: 3
  },
  
  mirrorx: false,
  mirrory: false,
  
  hollow: false,
  hollowt: 0,
  
  get spoints() {
    var spoints = this.cpoints.slice();
    
    if (this.mirrorx) {
      for (var i = spoints.length-1; i >= 0; i--) {
        var newpoint = spoints[i].slice();
	newpoint.x = -newpoint.x;
        spoints.push(newpoint);
      }
    }
    
    if (this.mirrory) {
      for (var i = spoints.length-1; i >= 0; i--) {
        var newpoint = spoints[i].slice();
	newpoint.y = -newpoint.y;
	spoints.push(newpoint);
      }
    }

    return spoints;

    if (this.hollow) {
      var hpoints = [];
      var inflatep = inflate_cpv(spoints, spoints.length, this.hollowt); 
     inflatep[0].slice().reverse().forEach(function(x) { hpoints.push(x); });
      
      inflatep[1].forEach(function(x) { hpoints.push(x); });
      return hpoints;
    }
    
    return spoints;
  },

  sample(n) {
    var spoints = this.spoints;

    this.degrees = Math.clamp(this.degrees, 1, spoints.length-1);
    
    if (spoints.length === 2)
      return spoints;
    if (spoints.length < 2)
      return [];
    if (this.degrees < 2) {
      if (this.type === 3)
        return spoints.wrapped(1);
	
      return spoints;
    }

    /*
      order = degrees+1
      knots = spoints.length + order
      assert knots%order != 0
    */

    if (this.type === bucket.typeid.looped)
      return spline_cmd(0, this.degrees, this.dimensions, 0, spoints.wrapped(this.degrees), n);

    return spline_cmd(0, this.degrees, this.dimensions, this.type, spoints, n);
  },  

  samples: 10,
  points:[],
  thickness:0, /* Number of pixels out the edge is */  
  
  make(go) {
    var edge = Object.create(this);
    Object.assign(edge, make_edge2d(go, this.points, this.thickness));
    edge = new Proxy(edge, sync_proxy);    
    complete_assign(edge, {
      set thickness(x) {
        cmd_edge2d(1,this.id,x);
      },
      get thickness() { return cmd(112,this.id); },

      get boundingbox() {
        return points2bb(this.points.map(x => x.scale(this.gameobject.scale)));
      },

      sync() {
        var sensor = this.sensor;
        this.points = this.sample(this.samples);
        cmd_edge2d(0,this.id,this.points);
	this.sensor = sensor;
      },
    });

    edge.obscure('boundingbox');
    
    complete_assign(edge, this.make_fns);

    Object.defineProperty(edge, 'id', {enumerable:false});
    Object.defineProperty(edge, 'shape', {enumerable:false});

    edge.defn('points', []);

    return edge;
  },
  
  /* EDITOR */
  gizmo() {
    if (!this.hasOwn('cpoints')) this.cpoints = this.__proto__.cpoints.copy();
    
    this.spoints.forEach(function(x) {
      Debug.point(world2screen(this.gameobject.this2world(x)), 3, Color.green);
    }, this);
    
    this.cpoints.forEach(function(x, i) {
      Debug.numbered_point(this.gameobject.this2world(x), i);
    }, this);
  },
  
  help: "Ctrl-click Add a point\nShift-click Remove a point\n+,- Increase/decrease spline segs\nCtrl-+,- Inc/dec spline degrees\nCtrl-b,v Inc/dec spline thickness",
  
  input_h_pressed() {
    this.hollow = !this.hollow;
  },
  
  input_m_pressed() {
    if (Keys.ctrl()) {
      this.mirrory = !this.mirrory;
    } else {
      this.mirrorx = !this.mirrorx;
    }
  },
  
  input_g_pressed() {
    if (!Keys.ctrl()) return;
    this.hollowt--;
    if (this.hollowt < 0) this.hollowt = 0;
  },

  input_f_pressed() {
    if (!Keys.ctrl()) return;
    this.hollowt++;
  },

  input_v_pressrep() {
    if (!Keys.alt()) return;
    this.thickness--;
  },

  input_b_pressrep() {
    if (Keys.alt()) {
      this.thickness++;
    } else if (Keys.ctrl()) {
      this.cpoints = this.spoints;
      this.mirrorx = false;
      this.mirrory = false;
    }
  },
  
  finish_center(change) {
    this.cpoints = this.cpoints.map(function(x) { return x.sub(change); });
  },

  input_plus_pressrep() {
    if (Keys.ctrl())
      this.degrees++;
    else
      this.samples += 1;
  },
  
  input_minus_pressrep() {
    if (Keys.ctrl())
      this.degrees--;
    else {
      this.samples -= 1;
      if (this.samples < 1) this.samples = 1;
    }
  },

  input_r_pressed() {
    if (!Keys.ctrl()) return;

    this.cpoints = this.cpoints.reverse();
  },

  input_l_pressed() {
    if (!Keys.ctrl()) return;
    this.type = 3;
  },

  input_c_pressed() {
    if (!Keys.ctrl()) return;
    this.type = 1;
  }, 

  input_o_pressed() {
    if (!Keys.ctrl()) return;
    this.type = 0;
  },

  input_lmouse_pressed() {
    if (Keys.ctrl()) {
      if (Keys.alt()) {
        var idx = grab_from_points(Mouse.worldpos, this.cpoints.map(this.gameobject.world2this,this.gameobject), 25);
	if (idx === -1) return;

	this.cpoints = this.cpoints.newfirst(idx);
	return;
      }
      var idx = 0;

      if (this.cpoints.length >= 2) {
        idx = cmd(59, screen2world(Mouse.pos).sub(this.gameobject.pos), this.cpoints, 1000);
	if (idx === -1) return;
      }

      if (idx === this.cpoints.length)
        this.cpoints.push(this.gameobject.world2this(screen2world(Mouse.pos)));
      else
        this.cpoints.splice(idx, 0, this.gameobject.world2this(screen2world(Mouse.pos)));
      return;
    } else if (Keys.shift()) {
      var idx = grab_from_points(screen2world(Mouse.pos), this.cpoints.map(function(x) {return x.add(this.gameobject.pos); }, this), 25);
      if (idx === -1) return;

      this.cpoints.splice(idx, 1);
    }
  },

  pick(pos) { return Gizmos.pick_gameobject_points(pos, this.gameobject, this.cpoints); },

  input_lbracket_pressrep() {
    var np = [];

    this.cpoints.forEach(function(c) {
      np.push(Vector.rotate(c, Math.deg2rad(-1)));
    });

    this.cpoints = np;
  },

  input_rbracket_pressrep() {
    var np = [];

    this.cpoints.forEach(function(c) {
      np.push(Vector.rotate(c, Math.deg2rad(1)));
    });

    this.cpoints = np;
  },
});

var circle2d = clone(collider2d, {
  name: "circle 2d",
  radius: 10,
  offset: [0,0],

  get scale() { return this.radius; },
  set scale(x) { this.radius = x; },
  
  get pos() { return this.offset; },
  set pos(x) { this.offset = x; },
    
  make(go) {
    var circle = clone(this);
    var circ = make_circle2d(go, circle.radius, circle.offset);
    Object.assign(circle, circ);
    Object.defineProperty(circle, 'id', {enumerable:false});
    Object.defineProperty(circle, 'shape', {enumerable:false});

    complete_assign(circle, {
      set radius(x) { cmd_circle2d(0,this.id,x); },
      get radius() { return cmd_circle2d(2,this.id); },

      set offset(x) { cmd_circle2d(1,this.id,this.offset); },
      get offset() { return cmd_circle2d(3,this.id); },

      get boundingbox() {
        var diameter = this.radius*2*this.gameobject.scale;
        return cwh2bb(this.offset.scale(this.gameobject.scale), [this.radius,this.radius]);
      },
    });

    complete_assign(circle, this.make_fns);

    circle.obscure('boundingbox');
    
    return circle;
  },

  gui() {
    Nuke.newline();
    Nuke.label("circle2d");
    this.radius = Nuke.pprop("Radius", this.radius);
    this.offset = Nuke.pprop("offset", this.offset);
  },
});


/* ASSETS */

var Texture = {
  mipmaps(path, x) {
    cmd(94, path, x);
  },

  sprite(path, x) {
    cmd(95, path, x);
  },
};

var Resources = {
  load(path) {
  if (path in this)
    return this[path];

  var src = {};
  this[path] = src;
  src.path = path;

  if (!IO.exists(`${path}.asset`))
    return this[path];

  var data = JSON.parse(IO.slurp(`${path}.asset`));
  Object.assign(src,data);
  return this[path];

  },
};
