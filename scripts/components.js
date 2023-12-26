function make_point_obj(o, p)
{
  return {
    pos: p,
    move(d) {
      d = o.gameobject.dir_world2this(d);
      p.x += d.x;
      p.y += d.y;
    },
    sync: o.sync.bind(o)
  }
}

function assign_impl(obj, impl)
{
  var tmp = {};
  for (var key of Object.keys(impl))
    if (typeof obj[key] !== 'undefined' && typeof obj[key] !== 'function')
      tmp[key] = obj[key];

  Object.mixin(obj, impl);

  if (obj.sync) obj.sync();

  for (var key in tmp)
    obj[key] = tmp[key];
}

var component = {
  components: [],
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

  isComponent(c) {
    if (typeof c !== 'object') return false;
    if (typeof c.toString !== 'function') return false;
    if (typeof c.make !== 'function') return false;
    return (typeof component[c.toString()] === 'object');
  },

  hides: ['gameobject', 'id'],
  
  make(go) {
    var nc = Object.create(this);
    nc.gameobject = go;
    Object.assign(nc, this._enghook(go.body));
    nc.sync();
    assign_impl(nc,this.impl);
    Object.hide(nc, ...this.hides);
    nc.post();
    return nc;
  },
  
  kill() { Log.info("Kill not created for this component yet"); },
  sync() {},
  post(){},
  gui() { },
  gizmo() { },
  
  prepare_center() {},
  finish_center() {},
  extend(spec) { return Object.copy(this, spec); },
};

component.sprite = Object.copy(component, {
  pos:[0,0],
  color:[1,1,1,1],
  layer:0,
  enabled:true,
  path: "",
  rect: {s0:0, s1: 1, t0: 0, t1: 1},  
  toString() { return "sprite"; },
  _enghook: make_sprite,
});

component.sprite.impl = {
  set path(x) {
    //cmd(12,this.id,prototypes.resani(this.gameobject.__proto__.toString(), x),this.rect);
    cmd(12,this.id,x,this.rect);
  },
  get path() {
    var s = cmd(116,this.id);
    if (s === "icons/no_tex.gif") return undefined;
    return s;
    //return prototypes.resavi(this.gameobject.__proto__.toString(), cmd(116,this.id));
  },
  toString() { return "sprite"; },
  hide() { this.enabled = false; },
  show() { this.enabled = true; },
  asset(str) { this.path = str; },
  get enabled() { return cmd(114,this.id); },
  set enabled(x) { cmd(20,this.id,x); },
  set color(x) { cmd(96,this.id,x); },
  get color() {return cmd(148,this.id);},
  get pos() { return cmd(111, this.id); },
  set pos(x) { cmd(37,this.id,x); },
  get angle() { return cmd(217,this.id); },
  set angle(x) { cmd(218,this.id,x); },
  get scale() { return cmd(215, this.id); },
  set scale(x) { cmd(216, this.id, x); },
  set layer(x) { cmd(60, this.id, x); },
  get layer() { return undefined; },
  emissive(x) { cmd(170, this.id, x); },
  pickm() { return this; },
  move(d) { this.pos = this.pos.add(d); },

  boundingbox() {
    var dim = this.dimensions();
    dim = dim.scale(this.gameobject.gscale());
    var realpos = dim.scale(0.5).add(this.pos);
    return cwh2bb(realpos,dim);
  },

  kill() { cmd(9,this.id); },
  dimensions() { return cmd(64,this.path); },
  width() { return cmd(64,this.path).x; },
  height() { return cmd(64,this.path).y; },
};

Object.freeze(sprite);

component.model = Object.copy(component, {
  path:"",
  _enghook: make_model,
});
component.model.impl = {
  set path(x) { cmd(149, this.id, x); },
  draw() { cmd(150, this.id); },
  kill() { cmd(213, this.id); },
};

var sprite = component.sprite;

sprite.doc = {
  path: "Path to the texture.",
  color: "Color to mix with the sprite.",
  pos: "The offset position of the sprite, relative to its entity."
};

sprite.inputs = {};
sprite.inputs.kp9 = function() { this.pos = this.dimensions().scale([0,0]); };
sprite.inputs.kp8 = function() { this.pos = this.dimensions().scale([-0.5, 0]); };
sprite.inputs.kp7 = function() { this.pos = this.dimensions().scale([-1,0]); };
sprite.inputs.kp6 = function() { this.pos = this.dimensions().scale([0,-0.5]); };
sprite.inputs.kp5 = function() { this.pos = this.dimensions().scale([-0.5,-0.5]); };
sprite.inputs.kp4 = function() { this.pos = this.dimensions().scale([-1,-0.5]); };
sprite.inputs.kp3 = function() { this.pos = this.dimensions().scale([0, -1]); };
sprite.inputs.kp2 = function() { this.pos = this.dimensions().scale([-0.5,-1]); };
sprite.inputs.kp1 = function() { this.pos = this.dimensions().scale([-1,-1]); };
Object.seal(sprite);

var SpriteAnim = {
  gif(path) {
    var anim = {};
    anim.frames = [];
    anim.path = path;
    var frames = cmd(139,path);
    var yslice = 1/frames;
    for (var f = 0; f < frames; f++) {
      var frame = {};
      frame.rect = {
	s0: 0,
	s1: 1,
	t0: yslice*f,
	t1: yslice*(f+1)
      };
      frame.time = 0.05;
      anim.frames.push(frame);
    }
    anim.loop = true;
    var dim = cmd(64,path);
    dim.y /= frames;
    anim.dim = dim;
    return anim;
  },

  strip(path, frames, time=0.05) {
    var anim = {};
    anim.frames = [];
    anim.path = path;
    var xslice = 1/frames;
    for (var f = 0; f < frames; f++) {
      var frame = {};
      frame.rect = {s0:xslice*f, s1: xslice*(f+1), t0:0, t1:1};
      frame.time = time;
      anim.frames.push(frame);
    }
    anim.dim = cmd(64,path);
    anim.dim.x /= frames;
    return anim;
  },

  aseprite(path) {
  function aseframeset2anim(frameset, meta) {
    var anim = {};
    anim.frames = [];
    anim.path = meta.image;
    var dim = meta.size;

    var ase_make_frame = function(ase_frame,i) {
      var f = ase_frame.frame;
      var frame = {};
      frame.rect = {
	s0: f.x/dim.w,
	s1: (f.x+f.w)/dim.w,
	t0: f.y/dim.h,
	t1: (f.y+f.h)/dim.h
      };
      frame.time = ase_frame.duration / 1000;
      anim.frames.push(frame);
    };

    frameset.forEach(ase_make_frame);
    anim.dim = [frameset[0].sourceSize.x, frameset[0].sourceSize.y];
    anim.loop = true;
    return anim;
    };

    var json = IO.slurp(ase);
    json = JSON.parse(json);
    var anims = {};
    var frames = Array.isArray(json.frames) ? json.frames : Object.values(json.frames);  
    for (var tag of json.meta.frameTags) 
      anims[tag.name] = aseframeset2anim(frames.slice(tag.from, tag.to+1), json.meta);

    return anims;
  },

  validate(anim)
  {
    if (!Object.isObject(anim)) return false;
    if (typeof anim.path !== 'string') return false;
    if (typeof anim.dim !== 'object') return false;
    return true;
  },

  find(path) {
    if (!IO.exists(path + ".asset")) return;
    var asset = JSON.parse(IO.slurp(path + ".asset"));
    
  },
};

SpriteAnim.doc = 'Functions to create Primum animations from varying sources.';
SpriteAnim.gif.doc = 'Convert a gif.';
SpriteAnim.strip.doc = 'Given a path and number of frames, converts a horizontal strip animation, where each cell is the same width.'
SpriteAnim.aseprite.doc = 'Given an aseprite json metadata, returns an object of animations defined in the aseprite file.';
SpriteAnim.find.doc = 'Given a path, find the relevant animation for the file.';

/* Container to play sprites and anim2ds */

component.char2d = Object.create(component.sprite);
component.char2dimpl = {
  boundingbox() {
    var dim = this.acur.dim.slice();
    dim = dim.scale(this.gameobject.scale);	
    var realpos = this.pos.slice();
    realpos.x = realpos.x * dim.x + (dim.x/2);
    realpos.y = realpos.y * dim.y + (dim.y/2);
    return cwh2bb(realpos,dim);
  },

  anims:{},

  sync() {
    if (this.path)
      cmd(12,this.id,this.path,this.rect);
  },

  frame: 0,

  play_anim(anim) {
    this.acur = anim;
    this.frame = 0;
    this.gameobject.delay(this.advance.bind(this), this.acur.frames[this.frame].time);
    this.setsprite();
  },
  
  play(name) {
    if (!(name in this)) {
      Log.info("Can't find an animation named " + name);
      return;
    }
    
    if (this.acur === this[name]) {
      this.timer.start();
      return;
    }
    
    this.acur = this[name];
    this.frame = 0;
    this.timer.time = this.acur.frames[this.frame].time;
    this.timer.start();
    this.setsprite();
  },
  
  setsprite() {
    cmd(12, this.id, this.acur.path, this.acur.frames[this.frame].rect);
  },

  advance() {
    this.frame = (this.frame + 1) % this.acur.frames.length;
    this.setsprite();
    this.gameobject.delay(this.advance.bind(this), this.acur.frames[this.frame].time);
  },

  devance() {
    this.frame = (this.frame - 1);
    if (this.frame === -1) this.frame = this.acur.frames-1;
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
  
  kill() { cmd(9, this.id); },

  add_anim(anim,name) {
    if (name in this) return;
    this[name] = function() {
      this.play_anim(anim);
    }
  },
};

Object.assign(component.char2d, component.char2dimpl);

component.char2d.doc = {
  doc: "An animation player for sprites.",
  frame: "The current frame of animation.",
  anims: "A list of all animations in this player.",
  acur: "The currently playing animation object.",
  advance: "Advance the animation by one frame.",
  devance: "Go back one frame in the animation.",
  setframe: "Set a specific frame of animation.",
  stop: "Stops the animation and returns to the first frame.",
  pause: "Pauses the animation sequence in place.",
  play: "Given an animation string, play it. Equivalent to anim.[name].play().",
  play_anim: "Play a given animation object.",
  add_anim: "Add an animation object with the given name."
};

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
  },

  arc(radius, angle, n, start) {
    start ??= 0;
    start = Math.deg2rad(start);
    if (angle >= 360)
      angle = 360;

    if (n <= 1) return [];
    var points = [];
    
    angle = Math.deg2rad(angle);
    var arclen = angle/n;
    for (var i = 0; i < n; i++)
      points.push(Vector.rotate([radius,0], start + (arclen*i)));

    return points;
  },

  circle(radius, n) {
    if (n <= 1) return [];
    return Geometry.arc(radius, 360, n);
  },

  ngon(radius, n) {
    return Geometry.arc(radius,360,n);
  },
};

Geometry.doc = {
  doc: "Functions for creating a list of points for various geometric shapes.",
  box: "Create a box.",
  arc: "Create an arc, made of n points.",
  circle: "Create a circle, made of n points.",
  ngon: "Create a polygon of n sides.",
};
  
/* For all colliders, "shape" is a pointer to a phys2d_shape, "id" is a pointer to the shape data */
var collider2d = Object.copy(component, {
  name: "collider 2d",
  sensor: false,

  kill() {}, /* No killing is necessary - it is done through the gameobject's kill */

  register_hit(fn, obj) {
    register_collide(1, fn, obj, this.gameobject.body, this.shape);
  },

  impl: {
    set sensor(x) { cmd(18,this.shape,x); },
    get sensor() { return cmd(21,this.shape); },
    set enabled(x) { cmd(22,this.shape,x); },
    get enabled() { return cmd(23,this.shape); }
  },
});

Object.hide(collider2d.impl, 'enabled');

collider2d.inputs = {};
collider2d.inputs['M-s'] = function() { this.sensor = !this.sensor; }
collider2d.inputs['M-s'].doc = "Toggle if this collider is a sensor.";

collider2d.inputs['M-t'] = function() { this.enabled = !this.enabled; }
collider2d.inputs['M-t'].doc = "Toggle if this collider is enabled.";

component.polygon2d = Object.copy(collider2d, {
  toString() { return "polygon2d"; },
  flipx: false,
  flipy: false,
  
  boundingbox() {
    return points2bb(this.spoints());
  },
  
  hides: ['id', 'shape', 'gameobject'],
  _enghook: make_poly2d,
  points:[],
  setpoints(points) {
    this.points = points;
    this.sync();
  },

  /* EDITOR */  
  spoints() {
    var spoints = this.points.slice();
    
    if (this.flipx) {
      spoints.forEach(function(x) {
        var newpoint = x.slice();
	newpoint.x = -newpoint.x;
        spoints.push(newpoint);
      });
    }
    
    if (this.flipy) {
      spoints.forEach(function(x) {
        var newpoint = x.slice();
	newpoint.y = -newpoint.y;
	spoints.push(newpoint);
      });
    }
    return spoints;
  },
  
  gizmo() {
    this.spoints().forEach(x => Shape.point(this.gameobject.this2screen(x), 3, Color.green));
    this.points.forEach((x,i)=>Debug.numbered_point(this.gameobject.this2screen(x), i));
  },

  pick(pos) {
    if (!Object.hasOwn(this,'points'))
      this.points = deep_copy(this.__proto__.points);
      
    var i = Gizmos.pick_gameobject_points(pos, this.gameobject, this.points);
    var p = this.points[i];
    if (p)
      return make_point_obj(this, p);
      
    return undefined;
  },
});

component.polygon2d.impl = Object.mix(collider2d.impl, {
  sync() { cmd_poly2d(0, this.id, this.spoints()); },
  query() { return cmd(80, this.shape); },
});

var polygon2d = component.polygon2d;

polygon2d.inputs = {};
polygon2d.inputs.post = function() { this.sync(); };
polygon2d.inputs.f10 = function() {
  this.points = sortpointsccw(this.points);
};
polygon2d.inputs.f10.doc = "Sort all points to be CCW order.";

polygon2d.inputs['C-lm'] = function() {
  this.points.push(this.gameobject.world2this(Mouse.worldpos));
};
polygon2d.inputs['C-lm'].doc = "Add a point to location of mouse.";
polygon2d.inputs.lm = function(){};
polygon2d.inputs.lm.released = function(){};

polygon2d.inputs['C-M-lm'] = function() {
  var idx = Math.grab_from_points(Mouse.worldpos, this.points.map(p => this.gameobject.this2world(p)), 25);
  if (idx === -1) return;
  this.points.splice(idx, 1);
};
polygon2d.inputs['C-M-lm'].doc = "Remove point under mouse.";

polygon2d.inputs['C-b'] = function() {
  this.points = this.spoints;
  this.flipx = false;
  this.flipy = false;
};
polygon2d.inputs['C-b'].doc = "Freeze mirroring in place.";

component.edge2d = Object.copy(collider2d, {
  dimensions:2,
  thickness:0,
  /* if type === -1, point to point */
  type: Spline.type.catmull,
  C: 1, /* when in bezier, continuity required. 0, 1 or 2. */
  looped: false,
  angle: 0.5, /* smaller for smoother bezier */
  
  flipx: false,
  flipy: false,
  cpoints:[],
  toString() { return "edge2d"; },
  
  hollow: false,
  hollowt: 0,

  spoints() {
    if (!this.cpoints) return [];
    var spoints = this.cpoints.slice();
    
    if (this.flipx) {
      if (Spline.is_bezier(this.type))
        spoints.push(Vector.reflect_point(spoints.at(-2), spoints.at(-1)));

      for (var i = spoints.length-1; i >= 0; i--) {
        var newpoint = spoints[i].slice();
	newpoint.x = -newpoint.x;
        spoints.push(newpoint);
      }
    }
    
    if (this.flipy) {
      if (Spline.is_bezier(this.type))
        spoints.push(Vector.reflect(point(spoints.at(-2),spoints.at(-1))));
	
      for (var i = spoints.length-1; i >= 0; i--) {
        var newpoint = spoints[i].slice();
	newpoint.y = -newpoint.y;
	spoints.push(newpoint);
      }
    }

    if (this.hollow) {
      var hpoints = inflate_cpv(spoints, spoints.length, this.hollowt);
      if (hpoints.length === spoints.length) return spoints;
      var arr1 = hpoints.filter(function(x,i) { return i % 2 === 0; });
      var arr2 = hpoints.filter(function(x,i) { return i % 2 !== 0; });
      return arr1.concat(arr2.reverse());
    }
    
    return spoints;
  },

  setpoints(points) {
    this.cpoints = points;
    this.sync();
  },

  post() {
    this.cpoints = [];
  },

  sample() {
    var spoints = this.spoints();
    if (this.type === -1) {
      if (this.looped) spoints.push(spoints[0]);
      return spoints;
    }

    if (this.type === Spline.type.catmull) {
      if (this.looped)
        spoints = Spline.catmull_loop(spoints);
      else
        spoints = Spline.catmull_caps(spoints);

      return Spline.sample_angle(this.type, spoints,this.angle);
    }

    if (this.looped && Spline.is_bezier(this.type))
      spoints = Spline.bezier_loop(spoints);

    return Spline.sample_angle(this.type, spoints, this.angle);
  },

  boundingbox() { return points2bb(this.points.map(x => x.scale(this.gameobject.scale))); },

  hides: ['gameobject', 'id', 'shape'],
  _enghook: make_edge2d,

  /* EDITOR */
  gizmo() {
    if (this.type === Spline.type.catmull) {
      this.spoints().forEach(x => Shape.point(this.gameobject.this2screen(x), 3, Color.teal));
      this.cpoints.forEach((x,i) => Debug.numbered_point(this.gameobject.this2screen(x), i));
    } else {
      for (var i = 0; i < this.cpoints.length; i += 3)
        Debug.numbered_point(this.gameobject.this2screen(this.cpoints[i]), i, Color.teal);
	
      for (var i = 1; i < this.cpoints.length; i+=3) {
        Debug.numbered_point(this.gameobject.this2screen(this.cpoints[i]), i, Color.green);
        Debug.numbered_point(this.gameobject.this2screen(this.cpoints[i+1]), i+1, Color.green);	
        Shape.line([this.gameobject.this2screen(this.cpoints[i-1]), this.gameobject.this2screen(this.cpoints[i])], Color.yellow);
        Shape.line([this.gameobject.this2screen(this.cpoints[i+1]), this.gameobject.this2screen(this.cpoints[i+2])], Color.yellow);	
      }
    }
  },
  
  finish_center(change) { this.cpoints = this.cpoints.map(function(x) { return x.sub(change); }); },

  pick(pos) {
    var i = Gizmos.pick_gameobject_points(pos, this.gameobject, this.cpoints);
    var p = this.cpoints[i];
    if (!p) return undefined;

    if (Spline.is_catmull(this.type))
      return make_point_obj(this,p);
      
    var that = this.gameobject;
    var me = this;
    if (p) {
      var o = {
        pos: p,
	sync: me.sync.bind(me)
      };
      if (Spline.bezier_is_handle(this.cpoints,i))
	o.move = function(d) {
	  d = that.dir_world2this(d);
	  p.x += d.x;
	  p.y += d.y;
	  Spline.bezier_cp_mirror(me.cpoints,i);
	};
      else
        o.move = function(d) {
	  d = that.dir_world2this(d);
	  p.x += d.x;
	  p.y += d.y;
	  var pp = Spline.bezier_point_handles(me.cpoints,i);
	  pp.forEach(ph => me.cpoints[ph] = me.cpoints[ph].add(d));
	}
      return o;
    }
  },

  rm_node(idx) {
    if (idx < 0 || idx >= this.cpoints.length) return;
    if (Spline.is_catmull(this.type))
      this.cpoints.splice(idx,1);

    if (Spline.is_bezier(this.type)) {
      Debug.assert(Spline.bezier_is_node(this.cpoints, idx), 'Attempted to delete a bezier handle.');
      if (idx === 0)
        this.cpoints.splice(idx,2);
      else if (idx === this.cpoints.length-1)
        this.cpoints.splice(this.cpoints.length-2,2);
      else
        this.cpoints.splice(idx-1,3);
    }
  },

  add_node(pos) {
    pos = this.gameobject.world2this(pos);
    var idx = 0;
    if (Spline.is_catmull(this.type)) {
      if (this.cpoints.length >= 2)
	idx = cmd(59, pos, this.cpoints, 400);

      if (idx === this.cpoints.length)
	this.cpoints.push(pos);
      else
	this.cpoints.splice(idx, 0, pos);
    }

    if (Spline.is_bezier(this.type)) {
      idx = cmd(59, pos, Spline.bezier_nodes(this.cpoints),400);

      if (idx < 0) return;
      
      if (idx === 0) {
        this.cpoints.unshift(pos.slice(), pos.add([-100,0]), Vector.reflect_point(this.cpoints[1], this.cpoints[0]));
	return;
      }
      if (idx === Spline.bezier_node_count(this.cpoints)) {
        this.cpoints.push(Vector.reflect_point(this.cpoints.at(-2), this.cpoints.at(-1)), pos.add([-100,0]), pos.slice());
	return;
      }
      idx = 2 + (idx-1)*3;
      var adds = [pos.add([100,0]), pos.slice(), pos.add([-100,0])];
      this.cpoints.splice(idx, 0, ...adds);
    }
  },

  pick_all() {
    var picks = [];
    this.cpoints.forEach(x =>picks.push(make_point_obj(this,x)));
    return picks;
  },
});

component.edge2d.impl = Object.mix({
  set thickness(x) {
    cmd_edge2d(1,this.id,x);
  },
  get thickness() { return cmd(112,this.id); },
  sync() {
    var sensor = this.sensor;
    var points = this.sample();
    if (!points) return;
    cmd_edge2d(0,this.id,points);
    this.sensor = sensor;
  },
}, component.edge2d.impl);

var bucket = component.edge2d;
bucket.spoints.doc = "Returns the controls points after modifiers are applied, such as it being hollow or mirrored on its axises.";
bucket.inputs = {};
bucket.inputs.post = function() { this.sync(); };
bucket.inputs.h = function() { this.hollow = !this.hollow; };
bucket.inputs.h.doc = "Toggle hollow.";

bucket.inputs['C-g'] = function() { if (this.hollowt > 0) this.hollowt--; };
bucket.inputs['C-g'].doc = "Thin the hollow thickness.";
bucket.inputs['C-g'].rep = true;

bucket.inputs['C-f'] = function() { this.hollowt++; };
bucket.inputs['C-f'].doc = "Increase the hollow thickness.";
bucket.inputs['C-f'].rep = true;

bucket.inputs['M-v'] = function() { if (this.thickness > 0) this.thickness--; };
bucket.inputs['M-v'].doc = "Decrease spline thickness.";
bucket.inputs['M-v'].rep = true;

bucket.inputs['C-y'] = function() {
  this.cpoints = this.spoints();
  this.flipx = false;
  this.flipy = false;
  this.hollow = false;
};
bucket.inputs['C-y'].doc = "Freeze mirroring,";
bucket.inputs['M-b'] = function() { this.thickness++; };
bucket.inputs['M-b'].doc = "Increase spline thickness.";
bucket.inputs['M-b'].rep = true;

bucket.inputs.plus = function() {
  if (this.angle <= 1) {
    this.angle = 1;
    return;
  }
  this.angle *= 0.9;
};
bucket.inputs.plus.doc = "Increase the number of samples of this spline.";
bucket.inputs.plus.rep = true;

bucket.inputs.minus = function() { this.angle *= 1.1; };
bucket.inputs.minus.doc = "Decrease the number of samples on this spline.";
bucket.inputs.minus.rep = true;

bucket.inputs['C-r'] = function() { this.cpoints = this.cpoints.reverse(); };
bucket.inputs['C-r'].doc = "Reverse the order of the spline's points.";

bucket.inputs['C-l'] = function() { this.looped = !this.looped};
bucket.inputs['C-l'].doc = "Toggle spline being looped.";

bucket.inputs['C-c'] = function() {
  switch(this.type) {
    case Spline.type.bezier:
      this.cpoints = Spline.bezier2catmull(this.cpoints);
      break;
  }
  this.type = Spline.type.catmull;
};

bucket.inputs['C-c'].doc = "Set type of spline to catmull-rom.";

bucket.inputs['C-b'] = function() {
  switch(this.type) {
    case Spline.type.catmull:
      this.cpoints = Spline.catmull2bezier(Spline.catmull_caps(this.cpoints));
      break;
  }
  this.type = Spline.type.bezier;
};

bucket.inputs['C-o'] = function() { this.type = -1; };
bucket.inputs['C-o'].doc = "Set spline to linear.";

bucket.inputs['C-M-lm'] = function() {
  if (Spline.is_catmull(this.type)) {
    var idx = Math.grab_from_points(Mouse.worldpos, this.cpoints.map(p => this.gameobject.this2world(p)), 25);
    if (idx === -1) return;
  } else {
    
  }

  this.cpoints = this.cpoints.newfirst(idx);
};
bucket.inputs['C-M-lm'].doc = "Select the given point as the '0' of this spline.";

bucket.inputs['C-lm'] = function() { this.add_node(Mouse.worldpos); }
bucket.inputs['C-lm'].doc = "Add a point to the spline at the mouse position.";

bucket.inputs['C-M-lm'] = function() {
  var idx = -1;
  if (Spline.is_catmull(this.type))
    idx = Math.grab_from_points(Mouse.worldpos, this.cpoints.map(p => this.gameobject.this2world(p)), 25);
  else {
    var nodes = Spline.bezier_nodes(this.cpoints);
    idx = Math.grab_from_points(Mouse.worldpos, nodes.map(p => this.gameobject.this2world(p)), 25);
    idx *= 3;
  }

  this.rm_node(idx);
};
bucket.inputs['C-M-lm'].doc = "Remove point from the spline.";

bucket.inputs.lm = function(){};
bucket.inputs.lm.released = function(){};

bucket.inputs.lb = function() {
  var np = [];

  this.cpoints.forEach(function(c) {
    np.push(Vector.rotate(c, Math.deg2rad(-1)));
  });

  this.cpoints = np;
};
bucket.inputs.lb.doc = "Rotate the points CCW.";
bucket.inputs.lb.rep = true;

bucket.inputs.rb = function() {
  var np = [];

  this.cpoints.forEach(function(c) {
    np.push(Vector.rotate(c, Math.deg2rad(1)));
  });

  this.cpoints = np;
};
bucket.inputs.rb.doc = "Rotate the points CW.";
bucket.inputs.rb.rep = true;

component.circle2d = Object.copy(collider2d, {
  radius:10,
  offset:[0,0],
  toString() { return "circle2d"; },
  
  boundingbox() {
    return cwh2bb(this.offset.scale(this.gameobject.scale), [this.radius,this.radius]);
  },

  hides: ['gameobject', 'id', 'shape', 'scale'],
  _enghook: make_circle2d,
});

component.circle2d.impl = Object.mix({
  set radius(x) { cmd_circle2d(0,this.id,x); },
  get radius() { return cmd_circle2d(2,this.id); },

  set scale(x) { this.radius = x; },
  get scale() { return this.radius; },

  set offset(x) { cmd_circle2d(1,this.id,x); },
  get offset() { return cmd_circle2d(3,this.id); },

  get pos() { return this.offset; },
  set pos(x) { this.offset = x; },
  
}, collider2d.impl);;

/* ASSETS */

var Texture = {
  mipmaps(path, x) { cmd(94, path, x); },

  sprite(path, x) { cmd(95, path, x); },
};

Texture.mipmaps.doc = "Return true if the texture has mipmaps.";
Texture.sprite.do = "Return true if the texture is treated as a sprite.";
