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

  make(go) {
    var nc = Object.create(this);
    nc.gameobject = go;
    Object.mixin(nc, this._enghook(go, nc));
    assign_impl(nc,this.impl);
    Object.hide(nc, 'gameobject', 'id');
    nc.post();
    nc.make = undefined;
    return nc;
  },
  
  kill() { console.info("Kill not created for this component yet"); },
  sync(){},
  post(){},
  gui(){},
  gizmo(){},
  
  finish_center() {},
  extend(spec) { return Object.copy(this, spec); },
};

var make_point_obj = function(o, p)
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

var assign_impl = function(obj, impl)
{
  var tmp = {};
  for (var key of Object.keys(impl))
    if (typeof obj[key] !== 'undefined' && typeof obj[key] !== 'function')
      tmp[key] = obj[key];

  Object.mixin(obj, impl);

  for (var key in tmp)
    obj[key] = tmp[key];
}

function json_from_whitelist(whitelist)
{
  return function() {
    var o = {};
    for (var p of whitelist)
      o[p] = this[p];
    return o;
  } 
}

Object.mixin(os.sprite(true), {
  loop: true,
  toJSON:json_from_whitelist([
    "path",
    "pos",
    "scale",
    "angle",
    "color",
    "emissive",
    "parallax",
    "frame"
  ]),
  anim:{},
  playing: 0,
  play(str = 0) {
    this.del_anim?.();
    var self = this;
    var stop;
    self.del_anim = function() {
      self.del_anim = undefined;
      self = undefined;
      advance = undefined;
      stop?.();
    }
    var playing = self.anim[str];
    if (!playing) return;
    var f = 0;
    self.path = playing.path;
    
    function advance() {
      if (!self) return;
      if (!self.gameobject) return;
      //self.path = playing.path;
      self.frame = playing.frames[f].rect;
      f = (f+1)%playing.frames.length;
      if (f === 0) {
        self.anim_done?.();
        if (!self.loop) { self.stop(); return; }
      }
      stop = self.gameobject.delay(advance, playing.frames[f].time);
    }
    this.tex(game.texture(playing.path));
    advance();
  },
  stop() {
    this.del_anim?.();
  },
  set path(p) {
    p = Resources.find_image(p);
    if (!p) {
      console.warn(`Could not find image ${p}.`);
      return;
    }
    if (p === this.path) return;
    this._p = p;    
    this.del_anim?.();
    this.texture = game.texture(p);
    this.tex(this.texture);
    
    var anim = SpriteAnim.make(p);
    if (!anim) return;
    this.anim = anim;
    this.play();
    
    this.pos = this.dimensions().scale(this.anchor);
  },
  get path() {
    return this._p;
  },
  kill() {
    this.del_anim?.();
    this.anim = undefined;
    this.gameobject = undefined;
    this.anim_done = undefined;
  },
  toString() { return "sprite"; },
  move(d) { this.pos = this.pos.add(d); },
  grow(x) {
    this.scale = this.scale.scale(x);
    this.pos = this.pos.scale(x);
  },
  anchor:[0,0],
  sync() { },
  pick() { return this; },
  boundingbox() {
    var dim = this.dimensions();
    dim = dim.scale(this.gameobject.gscale());
    var realpos = dim.scale(0.5).add(this.pos);
    return bbox.fromcwh(realpos,dim);
  },
  
  dimensions() {
    var dim = [this.texture.width, this.texture.height];
    dim.x *= this.frame.w;
    dim.y *= this.frame.h;
    return dim;
  },
  width() { return this.dimensions().x; },
  height() { return this.dimensions().y; },
});

os.sprite(true).make = function(go)
{
  var sp = os.sprite();
  sp.go = go;
  sp.gameobject = go;
  return sp;
}

component.sprite = os.sprite(true);

var sprite = component.sprite;

sprite.doc = {
  path: "Path to the texture.",
  color: "Color to mix with the sprite.",
  pos: "The offset position of the sprite, relative to its entity."
};

sprite.setanchor = function(anch) {
  var off = [0,0];
  switch(anch) {
    case "ll": break;
    case "lm": off = [-0.5,0]; break;
    case "lr": off = [-1,0]; break;
    case "ml": off = [0,-0.5]; break;
    case "mm": off = [-0.5,-0.5]; break;
    case "mr": off = [-1,-0.5]; break;
    case "ul": off = [0,-1]; break;
    case "um": off = [-0.5,-1]; break;
    case "ur": off = [-1,-1]; break;
  } 
  this.anchor = off;
  this.pos = this.dimensions().scale(off);
}

sprite.inputs = {};
sprite.inputs.kp9 = function() { this.setanchor("ll"); }
sprite.inputs.kp8 = function() { this.setanchor("lm"); }
sprite.inputs.kp7 = function() { this.setanchor("lr"); }
sprite.inputs.kp6 = function() { this.setanchor("ml"); }
sprite.inputs.kp5 = function() { this.setanchor("mm"); }
sprite.inputs.kp4 = function() { this.setanchor("mr"); }
sprite.inputs.kp3 = function() { this.setanchor("ur"); }
sprite.inputs.kp2 = function() { this.setanchor("um"); }
sprite.inputs.kp1 = function() { this.setanchor("ul"); }

Object.seal(sprite);

/* sprite anim returns a data structure for the given file path
  frames: array of frames
    rect: frame rectangle
    time: miliseconds to hold the frame for
  loop: true if it should be looped
*/
var animcache = {};
var SpriteAnim = {};
SpriteAnim.make = function(path) {
  if (path in animcache) return animcache[path];
  var anim;
  if (io.exists(path.set_ext(".ase")))
    anim = SpriteAnim.aseprite(path.set_ext(".ase"));
  else if (io.exists(path.set_ext(".json")))
    anim = SpriteAnim.aseprite(path.set_ext(".json"));
  else if (path.ext() === 'ase')  
    anim = SpriteAnim.aseprite(path);  
  else if (path.ext() === 'gif')
    anim = SpriteAnim.gif(path);
  else
    anim = undefined;
    
  animcache[path] = anim; 
  return animcache[path];
};
SpriteAnim.gif = function(path) {
  console.info(`making an anim from ${path}`);
  var anim = {};
  anim.frames = [];
  anim.path = path;
  var tex = game.texture(path);
  var frames = tex.frames;
  console.info(`frames are ${frames}`);    
  if (frames === 1) return undefined;
  var yslice = 1/frames;
  for (var f = 0; f < frames; f++) {
    var frame = {};
    frame.rect = {
      x: 0,
      w: 1,
      y: yslice*f,
      h: yslice
    };
    frame.time = 0.05;
    anim.frames.push(frame);
  }
  var times = tex.delays;
  for (var i = 0; i < frames; i++)
    anim.frames[i].time = times[i]/1000;
  anim.loop = true;
  var dim = [tex.width,tex.height];
  console.info(`dimensions are ${dim}`);
  dim.y /= frames;
  anim.dim = dim;
  return {0:anim};
};

SpriteAnim.strip = function(path, frames, time=0.05) {
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
  anim.dim = Resources.texture.dimensions(path);
  anim.dim.x /= frames;
  return anim;
};

SpriteAnim.aseprite = function(path) {
  function aseframeset2anim(frameset, meta) {
    var anim = {};
    anim.frames = [];
    anim.path = path.folder() + meta.image;
    var dim = meta.size;

    var ase_make_frame = function(ase_frame) {
      var f = ase_frame.frame;
      var frame = {};
      frame.rect = {
        x: f.x/dim.w,
        w: f.w/dim.w,
        y: f.y/dim.h,
        h: f.h/dim.h
      };
      frame.time = ase_frame.duration / 1000;
      anim.frames.push(frame);
    };

    frameset.forEach(ase_make_frame);
    anim.dim = frameset[0].sourceSize;
    anim.loop = true;
    return anim;
  };

  var data = json.decode(io.slurp(path));
  if (!data?.meta?.app.includes("aseprite")) return;
  var anims = {};
  var frames = Array.isArray(data.frames) ? data.frames : Object.values(data.frames);
  var f = 0;
  if (data.meta.frameTags.length === 0) {
    anims[0] = aseframeset2anim(frames, data.meta);
    return anims;
  }
  for (var tag of data.meta.frameTags) {
    anims[tag.name] = aseframeset2anim(frames.slice(tag.from, tag.to+1), data.meta);
    anims[f] = anims[tag.name];
    f++;
  }

  return anims;
};

SpriteAnim.validate = function(anim) {
  if (!Object.isObject(anim)) return false;
  if (typeof anim.path !== 'string') return false;
  if (typeof anim.dim !== 'object') return false;
  return true;
};

SpriteAnim.find = function(path) {
  if (!io.exists(path + ".asset")) return;
  var asset = JSON.parse(io.slurp(path + ".asset"));
};

SpriteAnim.doc = 'Functions to create Primum animations from varying sources.';
SpriteAnim.gif.doc = 'Convert a gif.';
SpriteAnim.strip.doc = 'Given a path and number of frames, converts a horizontal strip animation, where each cell is the same width.'
SpriteAnim.aseprite.doc = 'Given an aseprite json metadata, returns an object of animations defined in the aseprite file.';
SpriteAnim.find.doc = 'Given a path, find the relevant animation for the file.';

/* For all colliders, "shape" is a pointer to a phys2d_shape, "id" is a pointer to the shape data */
var collider2d = Object.copy(component, {
  impl: {
    set sensor(x) { pshape.set_sensor(this.shape,x); },
    get sensor() { return pshape.get_sensor(this.shape); },
    set enabled(x) { pshape.set_enabled(this.shape,x); },
    get enabled() { return pshape.get_enabled(this.shape); }
  },
});

Object.hide(collider2d.impl, 'enabled');

collider2d.inputs = {};
collider2d.inputs['M-s'] = function() { this.sensor = !this.sensor; }
collider2d.inputs['M-s'].doc = "Toggle if this collider is a sensor.";

collider2d.inputs['M-t'] = function() { this.enabled = !this.enabled; }
collider2d.inputs['M-t'].doc = "Toggle if this collider is enabled.";

component.polygon2d = Object.copy(collider2d, {
  toJSON:json_from_whitelist([
    'points',
    'sensor'
  ]),
  toString() { return "polygon2d"; },
  flipx: false,
  flipy: false,
  
  boundingbox() {
    return bbox.frompoints(this.spoints());
  },
  
  hides: ['id', 'shape', 'gameobject'],
  _enghook: os.make_poly2d,
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
    this.spoints().forEach(x => render.point(this.gameobject.this2screen(x), 3, Color.green));
    this.points.forEach((x,i)=>render.coordinate(this.gameobject.this2screen(x)));
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

function pointscaler(x) {
  if (typeof x === 'number') return;
  this.points = this.points.map(p => p.mult(x));
}

component.polygon2d.impl = Object.mix(collider2d.impl, {
  sync() { poly2d.setverts(this.id,this.spoints()); },
  query() { return physics.shape_query(this.shape); },
  grow: pointscaler,
});

var polygon2d = component.polygon2d;

polygon2d.inputs = {};
//polygon2d.inputs.post = function() { this.sync(); };
polygon2d.inputs.f10 = function() {
  this.points = Math.sortpointsccw(this.points);
};
polygon2d.inputs.f10.doc = "Sort all points to be CCW order.";

polygon2d.inputs['C-lm'] = function() {
  this.points.push(this.gameobject.world2this(input.mouse.worldpos()));
};
polygon2d.inputs['C-lm'].doc = "Add a point to location of mouse.";
polygon2d.inputs.lm = function(){};
polygon2d.inputs.lm.released = function(){};

polygon2d.inputs['C-M-lm'] = function() {
  var idx = Math.grab_from_points(input.mouse.worldpos(), this.points.map(p => this.gameobject.this2world(p)), 25);
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
  toJSON:json_from_whitelist([
    'sensor',
    'thickness',
    'points',
    'hollow',
    'hollowt',
    'angle',
  ]),
  dimensions:2,
  thickness:0,
  /* if type === -1, point to point */
  type: Spline.type.catmull,
  C: 1, /* when in bezier, continuity required. 0, 1 or 2. */
  looped: false,
  angle: 0.5, /* smaller for smoother bezier */
  
  flipx: false,
  flipy: false,
  points:[],
  toString() { return "edge2d"; },
  
  hollow: false,
  hollowt: 0,

  spoints() {
    if (!this.points) return [];
    var spoints = this.points.slice();
    
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
      var hpoints = vector.inflate(spoints, this.hollowt);
      if (hpoints.length === spoints.length) return spoints;
      var arr1 = hpoints.filter(function(x,i) { return i % 2 === 0; });
      var arr2 = hpoints.filter(function(x,i) { return i % 2 !== 0; });
      return arr1.concat(arr2.reverse());
    }
    
    return spoints;
  },

  setpoints(points) {
    this.points = points;
//    this.sync();
  },

  post() {
    this.points = [];
  },

  sample() {
    var spoints = this.spoints();
    if (spoints.length === 0) return [];
    
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

  boundingbox() { return bbox.frompoints(this.points.map(x => x.scale(this.gameobject.scale))); },

  hides: ['gameobject', 'id', 'shape'],
  _enghook: os.make_edge2d,

  /* EDITOR */
  gizmo() {
    if (this.type === Spline.type.catmull || this.type === -1) {
      this.spoints().forEach(x => render.point(this.gameobject.this2screen(x), 3, Color.teal));
      this.points.forEach((x,i) => render.coordinate(this.gameobject.this2screen(x)));
    } else {
      for (var i = 0; i < this.points.length; i += 3)
        render.coordinate(this.gameobject.this2screen(this.points[i]), 1, Color.teal);
	
      for (var i = 1; i < this.points.length; i+=3) {
        render.coordinate(this.gameobject.this2screen(this.points[i]), 1, Color.green);
        render.coordinate(this.gameobject.this2screen(this.points[i+1]), 1, Color.green);	
        render.line([this.gameobject.this2screen(this.points[i-1]), this.gameobject.this2screen(this.points[i])], Color.yellow);
        render.line([this.gameobject.this2screen(this.points[i+1]), this.gameobject.this2screen(this.points[i+2])], Color.yellow);	
      }
    }
  },
  
  finish_center(change) { this.points = this.points.map(function(x) { return x.sub(change); }); },

  pick(pos) {
    var i = Gizmos.pick_gameobject_points(pos, this.gameobject, this.points);
    var p = this.points[i];
    if (!p) return undefined;

    if (Spline.is_catmull(this.type) || this.type === -1)
      return make_point_obj(this,p);
      
    var that = this.gameobject;
    var me = this;
    if (p) {
      var o = {
        pos: p,
	      sync: me.sync.bind(me)
      };
      if (Spline.bezier_is_handle(this.points,i))
        o.move = function(d) {
        d = that.dir_world2this(d);
        p.x += d.x;
        p.y += d.y;
        Spline.bezier_cp_mirror(me.points,i);
	    };
      else
        o.move = function(d) {
          d = that.dir_world2this(d);
          p.x += d.x;
          p.y += d.y;
          var pp = Spline.bezier_point_handles(me.points,i);
          pp.forEach(ph => me.points[ph] = me.points[ph].add(d));
	      }
      return o;
    }
  },

  rm_node(idx) {
    if (idx < 0 || idx >= this.points.length) return;
    if (Spline.is_catmull(this.type))
      this.points.splice(idx,1);

    if (Spline.is_bezier(this.type)) {
      assert(Spline.bezier_is_node(this.points, idx), 'Attempted to delete a bezier handle.');
      if (idx === 0)
        this.points.splice(idx,2);
      else if (idx === this.points.length-1)
        this.points.splice(this.points.length-2,2);
      else
        this.points.splice(idx-1,3);
    }
  },

  add_node(pos) {
    pos = this.gameobject.world2this(pos);
    var idx = 0;
    if (Spline.is_catmull(this.type) || this.type === -1) {
      if (this.points.length >= 2)
	idx = physics.closest_point(pos, this.points, 400);

      if (idx === this.points.length)
	this.points.push(pos);
      else
	this.points.splice(idx, 0, pos);
    }

    if (Spline.is_bezier(this.type)) {
      idx = physics.closest_point(pos, Spline.bezier_nodes(this.points),400);

      if (idx < 0) return;
      
      if (idx === 0) {
        this.points.unshift(pos.slice(), pos.add([-100,0]), Vector.reflect_point(this.points[1], this.points[0]));
	return;
      }
      if (idx === Spline.bezier_node_count(this.points)) {
        this.points.push(Vector.reflect_point(this.points.at(-2), this.points.at(-1)), pos.add([-100,0]), pos.slice());
	return;
      }
      idx = 2 + (idx-1)*3;
      var adds = [pos.add([100,0]), pos.slice(), pos.add([-100,0])];
      this.points.splice(idx, 0, ...adds);
    }
  },

  pick_all() {
    var picks = [];
    this.points.forEach(x =>picks.push(make_point_obj(this,x)));
    return picks;
  },
});

component.edge2d.impl = Object.mix(collider2d.impl, {
  set thickness(x) { edge2d.set_thickness(this.id,x); },
  get thickness() { return edge2d.get_thickness(this.id); },
  grow: pointscaler,
  sync() {
    var sensor = this.sensor;
    var points = this.sample();
    if (!points) return;
    edge2d.setverts(this.id,points);
    this.sensor = sensor;
  },
});

var bucket = component.edge2d;
bucket.spoints.doc = "Returns the controls points after modifiers are applied, such as it being hollow or mirrored on its axises.";
bucket.inputs = {};
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
  this.points = this.spoints();
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

bucket.inputs['C-r'] = function() { this.points = this.points.reverse(); };
bucket.inputs['C-r'].doc = "Reverse the order of the spline's points.";

bucket.inputs['C-l'] = function() { this.looped = !this.looped};
bucket.inputs['C-l'].doc = "Toggle spline being looped.";

bucket.inputs['C-c'] = function() {
  switch(this.type) {
    case Spline.type.bezier:
      this.points = Spline.bezier2catmull(this.points);
      break;
  }
  this.type = Spline.type.catmull;
};

bucket.inputs['C-c'].doc = "Set type of spline to catmull-rom.";

bucket.inputs['C-b'] = function() {
  switch(this.type) {
    case Spline.type.catmull:
      this.points = Spline.catmull2bezier(Spline.catmull_caps(this.points));
      break;
  }
  this.type = Spline.type.bezier;
};

bucket.inputs['C-o'] = function() { this.type = -1; };
bucket.inputs['C-o'].doc = "Set spline to linear.";

bucket.inputs['C-M-lm'] = function() {
  if (Spline.is_catmull(this.type)) {
    var idx = Math.grab_from_points(input.mouse.worldpos(), this.points.map(p => this.gameobject.this2world(p)), 25);
    if (idx === -1) return;
  } else {
    
  }

  this.points = this.points.newfirst(idx);
};
bucket.inputs['C-M-lm'].doc = "Select the given point as the '0' of this spline.";

bucket.inputs['C-lm'] = function() { this.add_node(input.mouse.worldpos()); }
bucket.inputs['C-lm'].doc = "Add a point to the spline at the mouse position.";

bucket.inputs['C-M-lm'] = function() {
  var idx = -1;
  if (Spline.is_catmull(this.type))
    idx = Math.grab_from_points(input.mouse.worldpos(), this.points.map(p => this.gameobject.this2world(p)), 25);
  else {
    var nodes = Spline.bezier_nodes(this.points);
    idx = Math.grab_from_points(input.mouse.worldpos(), nodes.map(p => this.gameobject.this2world(p)), 25);
    idx *= 3;
  }

  this.rm_node(idx);
};
bucket.inputs['C-M-lm'].doc = "Remove point from the spline.";

bucket.inputs.lm = function(){};
bucket.inputs.lm.released = function(){};

bucket.inputs.lb = function() {
  var np = [];

  this.points.forEach(function(c) {
    np.push(Vector.rotate(c, Math.deg2rad(-1)));
  });

  this.points = np;
};
bucket.inputs.lb.doc = "Rotate the points CCW.";
bucket.inputs.lb.rep = true;

bucket.inputs.rb = function() {
  var np = [];

  this.points.forEach(function(c) {
    np.push(Vector.rotate(c, Math.deg2rad(1)));
  });

  this.points = np;
};
bucket.inputs.rb.doc = "Rotate the points CW.";
bucket.inputs.rb.rep = true;

component.circle2d = Object.copy(collider2d, {
  radius:10,
  offset:[0,0],
  toString() { return "circle2d"; },
  
  boundingbox() {
    return bbox.fromcwh([0,0], [this.radius,this.radius]);
  },

  hides: ['gameobject', 'id', 'shape', 'scale'],
  _enghook: os.make_circle2d,
});

component.circle2d.impl = Object.mix({
  toJSON:json_from_whitelist([
    "pos",
    "radius",
  ]),

  set radius(x) { circle2d.set_radius(this.id,x); circle2d.sync(this.id); },
  get radius() { return circle2d.get_radius(this.id); },

  set scale(x) { this.radius = x; },
  get scale() { return this.radius; },

  set offset(x) { circle2d.set_offset(this.id,x); circle2d.sync(this.id); },
  get offset() { circle2d.get_offset(this.id); },

  get pos() { return this.offset; },
  set pos(x) { this.offset = x; },

  grow(x) {
    if (typeof x === 'number') this.scale *= x;
    else if (typeof x === 'object') this.scale *= x[0];
  },
  
}, collider2d.impl);

return {component, SpriteAnim};
