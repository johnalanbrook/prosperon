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
  
  make(go) { },
  kill() { Log.info("Kill not created for this component yet"); },
  gui() { },
  gizmo() { },
  
  prepare_center() {},
  finish_center() {},
  extend(spec) {
    return Object.copy(this, spec);
  },

  /* Given a relative path, return the full path */
  resani(path) {
    if (!this.gameobject) return path;
    if (path[0] === '/') return path.slice(1);
    var res = this.gameobject.ur.toString();
    res = res.replaceAll('.', '/');
    var restry = res + "/" + path;
    while (!IO.exists(restry)) {
      res = res.updir() + "/";
      if (res === "/")
        return path;

      restry = res + path;
    }
    return restry;
  },

  /* Given the full path, return the most relative path */
  resavi(path) {
    if (!this.gameobject) return path;
    if (path[0] === '/') return path;

    var res = this.gameobject.ur.toString();
    res = res.replaceAll('.', '/');
    if (!path.startsWith(res))
      return path;

    return path.replace(res, "").slice(1);
  },
};

component.sprite = {
  pos:[0,0],
  color:[1,1,1],
  layer:0,
  enabled:true,
  path: "",
  toString() { return "sprite"; },
  make(go) {
    var nsprite = Object.create(component.sprite.maker);
    nsprite.gameobject = go;
    Object.assign(nsprite, make_sprite(go.body));    
    nsprite.ur = this;
    return nsprite;
  },
};

component.sprite.maker = Object.copy(component, {
  set path(x) {
    x = this.resani(x);
    cmd(12,this.id,x,this.rect);
  },
  get path() {
    return cmd(116,this.id);
  },
  get visible() { return this.enabled; },
  set visible(x) { this.enabled = x; },
  asset(str) { this.path = str; },
  rect: {s0:0, s1: 1, t0: 0, t1: 1},
  get enabled() { return cmd(114,this.id); },
  set enabled(x) { cmd(20,this.id,x); },
  set color(x) { cmd(96,this.id,x); },
  get color() {return undefined; },
  get pos() { return cmd(111, this.id); },
  set pos(x) { cmd(37,this.id,x); },
  set layer(x) { cmd(60, this.id, x); },
  get layer() { return undefined; },

  boundingbox() {
    return cwh2bb([0,0],[0,0]);
    var dim = this.dimensions();
    dim = dim.scale(this.gameobject.scale);
    var realpos = this.pos.copy();
    realpos.x = realpos.x * dim.x + (dim.x/2);
    realpos.y = realpos.y * dim.y + (dim.y/2);
    return cwh2bb(realpos,dim);
  },

  kill() { cmd(9,this.id); },
  dimensions() { return cmd(64,this.path); },
  width() { return cmd(64,this.path).x; },
  height() { return cmd(64,this.path).y; },
});

Object.freeze(sprite);

var sprite = component.sprite.maker;

sprite.inputs = {};
sprite.inputs.kp9 = function() { this.pos = [0,0]; };
sprite.inputs.kp8 = function() { this.pos = [-0.5, 0]; };
sprite.inputs.kp7 = function() { this.pos = [-1,0]; };
sprite.inputs.kp6 = function() { this.pos = [0,-0.5]; };
sprite.inputs.kp5 = function() { this.pos = [-0.5,-0.5]; };
sprite.inputs.kp4 = function() { this.pos = [-1,-0.5]; };
sprite.inputs.kp3 = function() { this.pos = [0, -1]; };
sprite.inputs.kp2 = function() { this.pos = [-0.5,-1]; };
sprite.inputs.kp1 = function() { this.pos = [-1,-1]; };
Object.seal(sprite);

var aseframeset2anim = function(frameset, meta)
{
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
}

var ase2anim = function(ase)
{
  var json = IO.slurp(ase);
  json = JSON.parse(json);
  var frames = Array.isArray(json.frames) ? json.frames : Object.values(json.frames);
  return aseframeset2anim(json.frames, json.meta);
}

var ase2anims = function(ase)
{
  var json = IO.slurp(ase);
  json = JSON.parse(json);
  var anims = {};
  var frames = Array.isArray(json.frames) ? json.frames : Object.values(json.frames);  
  for (var tag of json.meta.frameTags) 
    anims[tag.name] = aseframeset2anim(frames.slice(tag.from, tag.to+1), json.meta);

  return anims;
}

var gif2anim = function(gif)
{
  var anim = {};
  anim.frames = [];
  anim.path = gif;
  var frames = cmd(139,gif);
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
  var dim = cmd(64,gif);
  dim.y /= frames;
  anim.dim = dim;
  return anim;
}

var strip2anim = function(strip)
{
  var anim = {};
  anim.frames = [];
  anim.path = strip;
  var frames = 8;
  var xslice = 1/frames;
  for (var f = 0; f < frames; f++) {
    var frame = {};
    frame.rect = {s0:xslice*f, s1: slice*(f+1), t0:0, t1:1};
    frame.time = 0.05;
    anim.frames.push(frame);
  }
  anim.dim = cmd(64,strip);
  anim.dim.x /= frames;
  return anim;
}

/* Container to play sprites and anim2ds */
component.char2d = Object.copy(sprite, {
  get enabled() { return cmd(114,this.id); },
  set enabled(x) { cmd(20,this.id,x); },
  set color(x) { cmd(96,this.id,x); },
  get pos() { return cmd(111, this.id); },
  set pos(x) { cmd(37,this.id,x); },
  set layer(x) { cmd(60, this.id, x); },
  get layer() { return this.gameobject.draw_layer; },

  boundingbox() {
    var dim = this.curplaying.dim.slice();
    dim = dim.scale(this.gameobject.scale);	
    var realpos = this.pos.slice();
    realpos.x = realpos.x * dim.x + (dim.x/2);
    realpos.y = realpos.y * dim.y + (dim.y/2);
    return cwh2bb(realpos,dim);
  },

  sync() {
    if (this.path)
      cmd(12,this.id,this.path,this.rect);
  },

  kill() { cmd(9,this.id); },
  ur: {
    
  },

  make(go) {
    var char = Object.create(this);
    char.gameobject = go;
    Object.assign(char, make_sprite(go.body));
    char.frame = 0;
    char.timer = timer.make(char.advance.bind(char), 1);
    char.timer.loop = true;
    return char;
  },
  
  frame: 0,
  
  play(name) {
    if (!(name in this)) {
      Log.info("Can't find an animation named " + name);
      return;
    }
    
    if (this.curplaying === this[name]) {
      this.timer.start();
      return;
    }
    
    this.curplaying = this[name];
    this.frame = 0;
    this.timer.time = this.curplaying.frames[this.frame].time;
    this.timer.start();
    this.setsprite();
  },
  
  setsprite() {
    cmd(12, this.id, this.curplaying.path, this.curplaying.frames[this.frame].rect);
  },

  advance() {
    this.frame = (this.frame + 1) % this.curplaying.frames.length;
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
var collider2d = Object.copy(component, {
  name: "collider 2d",
  sensor: false,

  kill() {}, /* No killing is necessary - it is done through the gameobject's kill */

  register_hit(fn, obj) {
    register_collide(1, fn, obj, this.gameobject.body, this.shape);
  },
/*    set sensor(x) { cmd(18,this.shape,x); },
    get sensor() { return cmd(21,this.shape); },
    set enabled(x) { cmd(22,this.shape,x); },
    get enabled() { return cmd(23,this.shape); }
*/
});

collider2d.inputs = {};
collider2d.inputs['M-s'] = function() { this.sensor = !this.sensor; }
collider2d.inputs['M-s'].doc = "Toggle if this collider is a sensor.";

collider2d.inputs['M-t'] = function() { this.enabled = !this.enabled; }
collider2d.inputs['M-t'].doc = "Toggle if this collider is enabled.";

component.polygon2d = Object.copy(collider2d, {
  ur: {
    flipx: false,
    flipy: false
  },
  
  sync() {
    cmd_poly2d(0, this.id, this.spoints);
  },
  
  boundingbox() {
    return points2bb(this.spoints);
  },

  make(go) {
    var poly = Object.create(this);
    poly.gameobject = go;
    poly.points = [];
    poly.flipx = false;
    poly.flipy = false;
    Object.assign(poly, make_poly2d(go.body, poly.points));
    return poly;
  },

  /* EDITOR */  
  get spoints() {
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
    if (!this.hasOwn('points')) this.points = this.__proto__.points.copy();
    
    this.spoints.forEach(function(x) {
      Debug.point(world2screen(this.gameobject.this2world(x)), 3, Color.green);
    }, this);
    
    this.points.forEach(function(x, i) {
      Debug.numbered_point(this.gameobject.this2world(x), i);
    }, this);
  },

  pick(pos) {
    return Gizmos.pick_gameobject_points(pos, this.gameobject, this.points);
  },
  
  query() {
    return cmd(80, this.shape);
  },
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

polygon2d.inputs['S-lm'] = function() {
  var idx = grab_from_points(Mouse.worldpos, this.points.map(p => this.gameobject.this2world(p)), 25);
  if (idx === -1) return;
  this.points.splice(idx, 1);
};
polygon2d.inputs['S-lm'].doc = "Remove point under mouse.";

polygon2d.inputs['C-b'] = function() {
  this.points = this.spoints;
  this.flipx = false;
  this.flipy = false;
};
polygon2d.inputs['C-b'].doc = "Freeze mirroring in place.";

Object.freeze(polygon2d);

component.bucket = Object.copy(collider2d, {
  name: "bucket",
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
  
  flipx: false,
  flipy: false,
  
  hollow: false,
  hollowt: 0,
  
  get spoints() {
    var spoints = this.cpoints.slice();
    
    if (this.flipx) {
      for (var i = spoints.length-1; i >= 0; i--) {
        var newpoint = spoints[i].slice();
	newpoint.x = -newpoint.x;
        spoints.push(newpoint);
      }
    }
    
    if (this.flipy) {
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
    edge.gameobject = go;
    Object.assign(edge, make_edge2d(go.body, this.points, this.thickness));
    Object.assign(edge, {
      set thickness(x) {
        cmd_edge2d(1,this.id,x);
      },
      get thickness() { return cmd(112,this.id); },

      boundingbox() {
        return points2bb(this.points.map(x => x.scale(this.gameobject.scale)));
      },

      sync() {
        var sensor = this.sensor;
        this.points = this.sample(this.samples);
        cmd_edge2d(0,this.id,this.points);
	this.sensor = sensor;
      },
    });

    Object.assign(edge, this.make_fns);

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
  
  finish_center(change) {
    this.cpoints = this.cpoints.map(function(x) { return x.sub(change); });
  },

  pick(pos) { return Gizmos.pick_gameobject_points(pos, this.gameobject, this.cpoints); },
});

var bucket = component.bucket;
bucket.inputs = {};
bucket.inputs.h = function() { this.hollow = !this.hollow; };
bucket.inputs.h.doc = "Toggle hollow.";

bucket.inputs['C-g'] = function() {
  this.hollowt--;
  if (this.hollowt < 0) this.hollowt = 0;
};
bucket.inputs['C-g'].doc = "Thin the hollow thickness.";

bucket.inputs['C-f'] = function() { this.hollowt++; };
bucket.inputs['C-f'].doc = "Increase the hollow thickness.";

bucket.inputs['M-v'] = function() { this.thickness--; };
bucket.inputs['M-v'].doc = "Decrease spline thickness.";
bucket.inputs['M-v'].rep = true;

bucket.inputs['C-b'] = function() {
  this.cpoints = this.spoints;
  this.flipx = false;
  this.flipy = false;
};
bucket.inputs['C-b'].doc = "Freeze mirroring,";
bucket.inputs['M-b'] = function() { this.thickness++; };
bucket.inputs['M-b'].doc = "Increase spline thickness.";
bucket.inputs['M-b'].rep = true;

bucket.inputs['C-plus'] = function() { this.degrees++; };
bucket.inputs['C-plus'].doc = "Increase the degrees of this spline.";
bucket.inputs['C-plus'].rep = true;

bucket.inputs.plus = function() { this.samples++; };
bucket.inputs.plus.doc = "Increase the number of samples of this spline.";
bucket.inputs.plus.rep = true;

bucket.inputs.minus = function() {
  this.samples--;
  if (this.samples < 1) this.samples = 1;
};
bucket.inputs.minus.doc = "Decrease the number of samples on this spline.";
bucket.inputs.minus.rep = true;

bucket.inputs['C-minus'] = function() { this.degrees--; };
bucket.inputs['C-minus'].doc = "Decrease the number of degrees of this spline.";
bucket.inputs['C-minus'].rep = true;

bucket.inputs['C-r'] = function() { this.cpoints = this.cpoints.reverse(); };
bucket.inputs['C-r'].doc = "Reverse the order of the spline's points.";

bucket.inputs['C-l'] = function() { this.type = 3; };
bucket.inputs['C-l'].doc = "Set type of spline to clamped.";

bucket.inputs['C-c'] = function() { this.type = 1; };
bucket.inputs['C-c'].doc = "Set type of spline to closed.";

bucket.inputs['C-o'] = function() { this.type = 0; };
bucket.inputs['C-o'].doc = "Set spline to open.";

bucket.inputs['C-M-lm'] = function() {
  var idx = grab_from_points(Mouse.worldpos, this.cpoints.map(this.gameobject.world2this,this.gameobject), 25);
  if (idx === -1) return;

  this.cpoints = this.cpoints.newfirst(idx);
};
bucket.inputs['C-M-lm'].doc = "Select the given point as the '0' of this spline.";

bucket.inputs['C-lm'] = function() {
  var idx = 0;

  if (this.cpoints.length >= 2) {
    idx = cmd(59, screen2world(Mouse.pos).sub(this.gameobject.pos), this.cpoints, 1000);
    if (idx === -1) return;
  }

  if (idx === this.cpoints.length)
    this.cpoints.push(this.gameobject.world2this(screen2world(Mouse.pos)));
  else
    this.cpoints.splice(idx, 0, this.gameobject.world2this(screen2world(Mouse.pos)));
};
bucket.inputs['C-lm'].doc = "Add a point to the spline at the mouse position.";

bucket.inputs['S-lm'] = function() {
  var idx = grab_from_points(screen2world(Mouse.pos), this.cpoints.map(function(x) {return x.add(this.gameobject.pos); }, this), 25);
  if (idx === -1) return;

  this.cpoints.splice(idx, 1);
};
bucket.inputs['S-lm'].doc = "Remove point from the spline.";

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
  set radius(x) { cmd_circle2d(0,this.id,x); },
  get radius() { return cmd_circle2d(2,this.id); },

  set scale(x) { this.radius = x; },
  get scale() { return this.radius; },

  set offset(x) { cmd_circle2d(1,this.id,x); },
  get offset() { return cmd_circle2d(3,this.id); },

  boundingbox() {
    var diameter = this.radius*2*this.gameobject.scale;
    return cwh2bb(this.offset.scale(this.gameobject.scale), [this.radius,this.radius]);
  },
    
  make(go) {
    var circle = Object.create(this);
    circle.gameobject = go;
    Object.assign(circle, make_circle2d(go.body));
    
    return circle;
  },

  gui() {
    Nuke.newline();
    Nuke.label("circle2d");
    this.radius = Nuke.pprop("Radius", this.radius);
    this.offset = Nuke.pprop("offset", this.offset);
  },

  ur: {
    radius:10,
    offset:[0,0],
    toString() { return "circle2d"; },
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
