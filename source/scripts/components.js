var component = {
  toString() {
    if ('gameobject' in this)
      return this.name + " on " + this.gameobject;
    else
      return this.name;
  },
  name: "component",
  component: true,
  set enabled(x) { },
  get enabled() { },
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
  _path: "",
  get path() { return this._path; },
  set path(x) { this._path = x; this.load_img(x); },
  _pos: [0, 0],
  get layer() {
    if (!this.gameobject)
      return 0;
    else
      return this.gameobject.draw_layer;
  },
  
  get pos() { return this._pos; },
  set pos(x) {
    this._pos = x;
    this.sync();
  },

  input_kp9_pressed() { this.pos = [0,0]; },
  input_kp8_pressed() { this.pos = [-0.5,0]; },
  input_kp7_pressed() { this.pos = [-1,0]; },
  input_kp6_pressed() { this.pos = [0,-0.5]; },
  input_kp5_pressed() { this.pos = [-0.5,-0.5]; },
  input_kp4_pressed() { this.pos = [-1,-0.5]; },
  input_kp3_pressed() { this.pos = [0, -1]; },
  input_kp2_pressed() { this.pos = [-0.5,-1]; },
  input_kp1_pressed() { this.pos = [-1,-1]; },
  
  get boundingbox() {
    if (!this.gameobject) return null;
    var dim = cmd(64, this.path);
    dim = dim.scale(this.gameobject.scale);
    var realpos = this.pos.copy();
    realpos.x *= dim.x;
    realpos.y *= dim.y;
    realpos.x += (dim.x/2);
    realpos.y += (dim.y/2);
    return cwh2bb(realpos, dim);
  },

  set asset(x) {
    if (!x) return;
    if (!x.endsWith(".png")) {
      Log.error("Can't set texture to a non image.");
      return;
    }

    this.path = x;
    Log.info("path is now " + x);
    this.sync();
  },

  _enabled: true,
  set enabled(x) { this._enabled = x; cmd(20, this.id, x); },
  get enabled() { return this._enabled; return cmd(21, this.id); },
  get visible() { return this.enabled; },
  set visible(x) { this.enabled = x; },
  
  _angle: 0,
  get angle() { return this._angle; },
  set angle(x) {
    this._angle = x;
    sync();
  },

  make(go) {
    var sprite = clone(this);
    Object.defineProperty(sprite, 'id', {value:make_sprite(go,this.path,this.pos)});
    sprite.sync();
    return sprite;
  },
  
  rect: {s0:0, s1: 1, t0: 0, t1: 1},

  sync() {
    if (!this.hasOwn('id')) return;
    cmd(60, this.id, this.layer);
    cmd(37, this.id, this.pos);
  },

  set color(x) {
    cmd(96, this.id, x);
  },

  load_img(img) {
    cmd(12, this.id, img, this.rect);
  },

  kill() {
    cmd(9, this.id);
  },
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
    var char = clone(this);
    char.curplaying = char.anims.array()[0];
    char.obscure('curplaying');
    Object.defineProperty(char, 'id', {value:make_sprite(go,char.curplaying.path,this.pos)});
    char.obscure('id');
    char.frame = 0;
    char.timer = timer.make(char.advance.bind(char), 1/char.curplaying.fps);
    char.timer.loop = true;
    char.obscure('timer');
    char.obscure('rect');
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

/* For all colliders, "shape" is a pointer to a phys2d_shape, "id" is a pointer to the shape data */
var collider2d = clone(component, {
  name: "collider 2d",
  
  _sensor: false,
  set sensor(x) {
    this._sensor = x;
    if (this.shape)
      cmd(18, this.shape, x);
  },
  get sensor() { return this._sensor; },

  input_s_pressed() {
    if (!Keys.alt()) return;

    this.sensor = !this.sensor;
  },

  input_t_pressed() {
    if (!Keys.alt()) return;

    this.enabled = !this.enabled;
  },

  coll_sync() {
    cmd(18, this.shape, this.sensor);
  },

  _enabled: true,
  set enabled(x) {this._enabled = x; if (this.id) cmd(22, this.id, x); },
  get enabled() { return this._enabled; },
  kill() {}, /* No killing is necessary - it is done through the gameobject's kill */

  register_hit(fn, obj) {
    register_collide(1, fn, obj, this.gameobject.body, this.shape);
  },
});


var polygon2d = clone(collider2d, {
  name: "polygon 2d",
  points: [],
  help: "Ctrl-click Add a point\nShift-click Remove a point",
  clone(spec) {
    var obj = Object.create(this);
    obj.points = this.points.copy();
    Object.assign(obj, spec);
    return obj;
  },

  make(go) {
    var poly = Object.create(this);
    Object.assign(poly, make_poly2d(go, this.points));
    Object.defineProperty(poly, 'id', {enumerable:false});
    Object.defineProperty(poly, 'shape', {enumerable:false});

    return poly;
  },
  
  get boundingbox() {
    if (!this.gameobject) return null;
    var scaledpoints = [];
    this.points.forEach(function(x) { scaledpoints.push(x.scale(this.gameobject.scale)); }, this);
    return points2bb(scaledpoints);
  },
  
  input_f10_pressed() {
    this.points = sortpointsccw(this.points);
  },

  sync() {
    if (!this.id) return;
    cmd_poly2d(0, this.id, this.spoints);
    this.coll_sync();
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
    

    this.sync();
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
  
  mirrorx: false,
  mirrory: false,
  
  input_m_pressed() {
    if (Keys.ctrl())
      this.mirrory = !this.mirrory;
    else
      this.mirrorx = !this.mirrorx;
  },
});

var bucket = clone(collider2d, {
  name: "bucket",
  help: "Ctrl-click Add a point\nShift-click Remove a point\n+,- Increase/decrease spline segs\nCtrl-+,- Inc/dec spline degrees\nCtrl-b,v Inc/dec spline thickness",
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
  
  get boundingbox() {
    if (!this.gameobject) return null;
    var scaledpoints = [];
    this.points.forEach(function(x) { scaledpoints.push(x.scale(this.gameobject.scale)); }, this);
    return points2bb(scaledpoints);
  },
  
  mirrorx: false,
  mirrory: false,
  
  input_m_pressed() {
    if (Keys.ctrl()) {
      this.mirrory = !this.mirrory;
    } else {
      this.mirrorx = !this.mirrorx;
    }
    
    this.sync();
  },
  
  hollow: false,
  input_h_pressed() {
    this.hollow = !this.hollow;
  },
  
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

  hollowt: 0,

  input_g_pressed() {
    if (!Keys.ctrl()) return;
    this.hollowt--;
    if (this.hollowt < 0) this.hollowt = 0;
  },

  input_f_pressed() {
    if (!Keys.ctrl()) return;
    this.hollowt++;
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

    if (this.type === 3)
      return spline_cmd(0, this.degrees, this.dimensions, 0, spoints.wrapped(this.degrees), n);

    return spline_cmd(0, this.degrees, this.dimensions, this.type, spoints, n);
  },  

  samples: 10,
  points:[],
  
  make(go) {
    var edge = Object.create(this);
    Object.assign(edge, make_edge2d(go, this.points, this.thickness));
    Object.defineProperty(edge, 'id', {enumerable:false});
    Object.defineProperty(edge, 'shape', {enumerable:false});
    edge.defn('points', []);
//    Object.defineProperty(edge, 'points', {enumerable:false});
    edge.sync();
    return edge;
  },
  
  sync() {
    if (!this.gameobject) return;
    this.points = this.sample(this.samples);
    cmd_edge2d(0, this.id, this.points);
    cmd_edge2d(1, this.id, this._thickness * this.gameobject.scale);
    this.coll_sync();
  },

  gizmo() {
    if (!this.hasOwn('cpoints')) this.cpoints = this.__proto__.cpoints.copy();
    
    this.spoints.forEach(function(x) {
      Debug.point(world2screen(this.gameobject.this2world(x)), 3, Color.green);
    }, this);
    
    this.cpoints.forEach(function(x, i) {
      Debug.numbered_point(this.gameobject.this2world(x), i);
    }, this);
    
    this.sync();
  },
 
  _thickness:0, /* Number of pixels out the edge is */
  get thickness() { return this._thickness; },
  set thickness(x) {
    this._thickness = Math.max(x, 0);
    cmd_edge2d(1, this.id, this._thickness);
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
    this.sync();
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
  get radius() {
    return this.rradius;
  },
  rradius: 10,
  set radius(x) {
   this.rradius = x;
   cmd_circle2d(0, this.id, this.rradius);
  },
  
  get boundingbox() {
    if (!this.gameobject) return null;
    var radius = this.radius*2*this.gameobject.scale;
    return cwh2bb(this.offset.scale(this.gameobject.scale), [radius, radius]);
  },

  get scale() { return this.radius; },
  set scale(x) { this.radius = x; },
  
  ofset: [0,0],
  get offset() { return this.ofset; },
  set offset(x) { this.ofset = x; cmd_circle2d(1, this.id, this.ofset); },
  
  get pos() { return this.ofset; },
  set pos(x) { this.offset = x; },
    
  make(go) {
    var circle = clone(this);
    var circ = make_circle2d(go, circle.radius, circle.offset);
    Object.assign(circle, circ);
    Object.defineProperty(circle, 'id', {enumerable:false});
    Object.defineProperty(circle, 'shape', {enumerable:false});
    return circle;
  },

  gui() {
    Nuke.newline();
    Nuke.label("circle2d");
    this.radius = Nuke.pprop("Radius", this.radius);
    this.offset = Nuke.pprop("offset", this.offset);
  },

  sync() {
    cmd_circle2d(0, this.id, this.rradius);
    cmd_circle2d(-1, this.id);
    this.coll_sync();  
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
