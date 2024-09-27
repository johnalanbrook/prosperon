var component = {};

var make_point_obj = function (o, p) {
  return {
    pos: p,
    move(d) {
      d = o.gameobject.dir_world2this(d);
      p.x += d.x;
      p.y += d.y;
    },
    sync: o.sync.bind(o),
  };
};

var fullrect = [0, 0, 1, 1];

var sprite_addbucket = function (sprite) {
  var layer = 1000000 + sprite.gameobject.drawlayer * 1000 - sprite.gameobject.pos.y;
  sprite_buckets[layer] ??= {};
  sprite_buckets[layer][sprite.path] ??= [];
  sprite_buckets[layer][sprite.path].push(sprite);
  sprite._oldlayer = layer;
  sprite._oldpath = sprite.path;
};

var sprite_rmbucket = function (sprite) {
  if (sprite._oldlayer && sprite._oldpath) sprite_buckets[sprite._oldlayer][sprite._oldpath].remove(sprite);
  else for (var layer of Object.values(sprite_buckets)) for (var path of Object.values(layer)) path.remove(sprite);
};

var sprite = {
  loop: true,
  rect: fullrect,
  anim: {},
  playing: 0,
  anim_speed: 1,
  play(str = 0, fn, loop = true, reverse = false) {
    this.del_anim?.();
    var self = this;
    var stop;
    self.del_anim = function () {
      self.del_anim = undefined;
      self = undefined;
      advance = undefined;
      stop?.();
    };
    var playing = self.anim[str];
    if (!playing) return;
    var f = 0;
    if (reverse) f = playing.frames.length - 1;

    self.path = playing.path;

    function advance() {
      if (!self) return;
      if (!self.gameobject) return;
      self.frame = playing.frames[f].rect;
      self.rect = [self.frame.x, self.frame.y, self.frame.w, self.frame.h];
      self.update_dimensions();
      var done = false;
      if (reverse) {
        f = (((f - 1) % playing.frames.length) + playing.frames.length) % playing.frames.length;
        if (f === playing.frames.length - 1) done = true;
      } else {
        f = (f + 1) % playing.frames.length;
        if (f === 0) done = true;
      }

      if (done) {
        fn?.();
        if (!loop) {
          self?.stop();
          return;
        }
        //        self?.anim_done?.();
        //        if (!self.loop) { self.stop(); return; }
      }
      //      if (self) stop = self.gameobject.delay(advance, playing.frames[f].time / self.anim_speed);
    }

    advance();
  },
  tex_sync() {
    if (this.anim) this.stop();
    this.rect = fullrect;
    var anim = SpriteAnim.make(this.path);
    this.update_dimensions();
    this.sync();

    if (!anim) return;
    this.anim = anim;
    this.play();

    this.pos = this.dimensions().scale(this.anchor);
  },
  stop() {
    this.del_anim?.();
  },
  set path(p) {
    p = Resources.find_image(p, this.gameobject._root);
    if (!p) {
      console.warn(`Could not find image ${p}.`);
      return;
    }
    if (p === this.path) return;

    this._p = p;

    this.del_anim?.();
    this.texture = game.texture(p);

    this.diffuse = this.texture;

    this.tex_sync();
  },
  get path() {
    return this._p;
  },
  kill() {
    sprite_rmbucket(this);
    this.del_anim?.();
    this.anim = undefined;
    this.gameobject = undefined;
    this.anim_done = undefined;
    allsprites.remove(this);
  },
  move(d) {
    this.pos = this.pos.add(d);
  },
  grow(x) {
    this.scale = this.scale.scale(x);
    this.pos = this.pos.scale(x);
  },
  anchor: [0, 0],
  sync() {
    var layer = 1000000 + this.gameobject.drawlayer * 1000 - this.gameobject.pos.y;
    if (layer === this._oldlayer && this.path === this._oldpath) return;

    sprite_rmbucket(this);
    sprite_addbucket(this);
  },
  pick() {
    return this;
  },
  boundingbox() {
    var dim = this.dimensions();
    dim = dim.scale(this.gameobject.scale);
    var realpos = dim.scale(0.5).add(this.pos);
    return bbox.fromcwh(realpos, dim);
  },

  update_dimensions() {
    this._dimensions = [this.texture.width * this.rect[2], this.texture.height * this.rect[3]];
    component.sprite_dim_hook?.(this);
  },

  dimensions() {
    return this._dimensions;
  },
  width() {
    return this.dimensions().x;
  },
  height() {
    return this.dimensions().y;
  },
};
globalThis.allsprites = [];
var sprite_buckets = [];

component.sprite_buckets = function () {
  return sprite_buckets;
};

component.dynamic_sprites = [];

sprite.doc = {
  path: "Path to the texture.",
  color: "Color to mix with the sprite.",
  pos: "The offset position of the sprite, relative to its entity.",
};

sprite.setanchor = function (anch) {
  var off = [0, 0];
  switch (anch) {
    case "ll":
      break;
    case "lm":
      off = [-0.5, 0];
      break;
    case "lr":
      off = [-1, 0];
      break;
    case "ml":
      off = [0, -0.5];
      break;
    case "mm":
      off = [-0.5, -0.5];
      break;
    case "mr":
      off = [-1, -0.5];
      break;
    case "ul":
      off = [0, -1];
      break;
    case "um":
      off = [-0.5, -1];
      break;
    case "ur":
      off = [-1, -1];
      break;
  }
  this.anchor = off;
  this.pos = this.dimensions().scale(off);
};

sprite.inputs = {};
sprite.inputs.kp9 = function () {
  this.setanchor("ll");
};
sprite.inputs.kp8 = function () {
  this.setanchor("lm");
};
sprite.inputs.kp7 = function () {
  this.setanchor("lr");
};
sprite.inputs.kp6 = function () {
  this.setanchor("ml");
};
sprite.inputs.kp5 = function () {
  this.setanchor("mm");
};
sprite.inputs.kp4 = function () {
  this.setanchor("mr");
};
sprite.inputs.kp3 = function () {
  this.setanchor("ur");
};
sprite.inputs.kp2 = function () {
  this.setanchor("um");
};
sprite.inputs.kp1 = function () {
  this.setanchor("ul");
};

component.sprite = function (obj) {
  var sp = Object.create(sprite);
  sp.gameobject = obj;
  sp.transform = obj.transform;
  sp.guid = prosperon.guid();
  allsprites.push(sp);
  if (component.sprite.make_hook) component.sprite.make_hook(sp);
  sprite_addbucket(sp);
  return sp;
};

sprite.shade = [1, 1, 1, 1];

Object.mixin(os.make_seg2d(), {
  sync() {
    this.set_endpoints(this.points[0], this.points[1]);
  },
});

/* sprite anim returns a data structure for the given file path
  frames: array of frames
    rect: frame rectangle
    time: miliseconds to hold the frame for
  loop: true if it should be looped
*/
var animcache = {};
var SpriteAnim = {};
SpriteAnim.make = function (path) {
  var anim;
  if (io.exists(path.set_ext(".ase"))) anim = SpriteAnim.aseprite(path.set_ext(".ase"));
  else if (io.exists(path.set_ext(".json"))) anim = SpriteAnim.aseprite(path.set_ext(".json"));
  else if (path.ext() === "ase") anim = SpriteAnim.aseprite(path);
  else if (path.ext() === "gif") anim = SpriteAnim.gif(path);
  else anim = undefined;

  animcache[path] = anim;
  return animcache[path];
};
SpriteAnim.gif = function (path) {
  var anim = {};
  anim.frames = [];
  anim.path = path;
  var tex = game.texture(path);
  var frames = tex.frames;
  if (frames === 1) return undefined;
  var yslice = 1 / frames;
  for (var f = 0; f < frames; f++) {
    var frame = {};
    frame.rect = {
      x: 0,
      w: 1,
      y: yslice * f,
      h: yslice,
    };
    frame.time = 0.05;
    anim.frames.push(frame);
  }
  var times = tex.delays;
  for (var i = 0; i < frames; i++) anim.frames[i].time = times[i] / 1000;
  anim.loop = true;
  var dim = [tex.width, tex.height];
  dim.y /= frames;
  anim.dim = dim;
  return { 0: anim };
};

SpriteAnim.strip = function (path, frames, time = 0.05) {
  var anim = {};
  anim.frames = [];
  anim.path = path;
  var xslice = 1 / frames;
  for (var f = 0; f < frames; f++) {
    var frame = {};
    frame.rect = { s0: xslice * f, s1: xslice * (f + 1), t0: 0, t1: 1 };
    frame.time = time;
    anim.frames.push(frame);
  }
  anim.dim = Resources.texture.dimensions(path);
  anim.dim.x /= frames;
  return anim;
};

SpriteAnim.aseprite = function (path) {
  function aseframeset2anim(frameset, meta) {
    var anim = {};
    anim.frames = [];
    anim.path = path.folder() + meta.image;
    var dim = meta.size;

    var ase_make_frame = function (ase_frame) {
      var f = ase_frame.frame;
      var frame = {};
      frame.rect = {
        x: f.x / dim.w,
        w: f.w / dim.w,
        y: f.y / dim.h,
        h: f.h / dim.h,
      };
      frame.time = ase_frame.duration / 1000;
      anim.frames.push(frame);
    };

    frameset.forEach(ase_make_frame);
    anim.dim = frameset[0].sourceSize;
    anim.loop = true;
    return anim;
  }

  var data = json.decode(io.slurp(path));
  if (!data?.meta?.app.includes("aseprite")) return;
  var anims = {};
  var frames = Array.isArray(data.frames) ? data.frames : Object.values(data.frames);
  var f = 0;
  if (!data.meta.frameTags || data.meta.frameTags.length === 0) {
    anims[0] = aseframeset2anim(frames, data.meta);
    return anims;
  }
  for (var tag of data.meta.frameTags) {
    anims[tag.name] = aseframeset2anim(frames.slice(tag.from, tag.to + 1), data.meta);
    anims[f] = anims[tag.name];
    f++;
  }

  return anims;
};

SpriteAnim.validate = function (anim) {
  if (!Object.isObject(anim)) return false;
  if (typeof anim.path !== "string") return false;
  if (typeof anim.dim !== "object") return false;
  return true;
};

SpriteAnim.find = function (path) {
  if (!io.exists(path + ".asset")) return;
  var asset = JSON.parse(io.slurp(path + ".asset"));
};

SpriteAnim.doc = "Functions to create Primum animations from varying sources.";
SpriteAnim.gif.doc = "Convert a gif.";
SpriteAnim.strip.doc = "Given a path and number of frames, converts a horizontal strip animation, where each cell is the same width.";
SpriteAnim.aseprite.doc = "Given an aseprite json metadata, returns an object of animations defined in the aseprite file.";
SpriteAnim.find.doc = "Given a path, find the relevant animation for the file.";

var collider2d = {};
collider2d.inputs = {};
collider2d.inputs["M-s"] = function () {
  this.sensor = !this.sensor;
};
collider2d.inputs["M-s"].doc = "Toggle if this collider is a sensor.";

collider2d.inputs["M-t"] = function () {
  this.enabled = !this.enabled;
};
collider2d.inputs["M-t"].doc = "Toggle if this collider is enabled.";

Object.mix(os.make_poly2d(), {
  boundingbox() {
    return bbox.frompoints(this.spoints());
  },

  /* EDITOR */
  spoints() {
    var spoints = this.points.slice();

    if (this.flipx) {
      spoints.forEach(function (x) {
        var newpoint = x.slice();
        newpoint.x = -newpoint.x;
        spoints.push(newpoint);
      });
    }

    if (this.flipy) {
      spoints.forEach(function (x) {
        var newpoint = x.slice();
        newpoint.y = -newpoint.y;
        spoints.push(newpoint);
      });
    }
    return spoints;
  },

  gizmo() {
    this.spoints().forEach(x => render.point(this.gameobject.this2screen(x), 3, Color.green));
    this.points.forEach((x, i) => render.coordinate(this.gameobject.this2screen(x)));
  },

  pick(pos) {
    if (!Object.hasOwn(this, "points")) this.points = deep_copy(this.__proto__.points);

    var i = Gizmos.pick_gameobject_points(pos, this.gameobject, this.points);
    var p = this.points[i];
    if (p) return make_point_obj(this, p);

    return undefined;
  },
});

function pointscaler(x) {
  if (typeof x === "number") return;
  this.points = this.points.map(p => p.mult(x));
}

Object.mixin(os.make_poly2d(), {
  sync() {
    this.setverts(this.points);
  },
  grow: pointscaler,
});

var polyinputs = Object.create(collider2d.inputs);
os.make_poly2d().inputs = polyinputs;

polyinputs = {};
polyinputs.f10 = function () {
  this.points = Math.sortpointsccw(this.points);
};
polyinputs.f10.doc = "Sort all points to be CCW order.";

polyinputs["C-lm"] = function () {
  this.points.push(this.gameobject.world2this(input.mouse.worldpos()));
};
polyinputs["C-lm"].doc = "Add a point to location of mouse.";
polyinputs.lm = function () {};
polyinputs.lm.released = function () {};

polyinputs["C-M-lm"] = function () {
  var idx = Math.grab_from_points(
    input.mouse.worldpos(),
    this.points.map(p => this.gameobject.this2world(p)),
    25,
  );
  if (idx === -1) return;
  this.points.splice(idx, 1);
};
polyinputs["C-M-lm"].doc = "Remove point under mouse.";

polyinputs["C-b"] = function () {
  this.points = this.spoints;
  this.flipx = false;
  this.flipy = false;
};
polyinputs["C-b"].doc = "Freeze mirroring in place.";

var edge2d = {
  dimensions: 2,
  thickness: 1,
  /* if type === -1, point to point */
  type: Spline.type.catmull,
  C: 1 /* when in bezier, continuity required. 0, 1 or 2. */,
  looped: false,
  angle: 0.5 /* smaller for smoother bezier */,
  elasticity: 0,
  friction: 0,
  sync() {
    var ppp = this.sample();
    this.segs ??= [];
    var count = ppp.length - 1;
    this.segs.length = count;
    for (var i = 0; i < count; i++) {
      this.segs[i] ??= os.make_seg2d(this.body);
      this.segs[i].set_endpoints(ppp[i], ppp[i + 1]);
      this.segs[i].set_neighbors(ppp[i], ppp[i + 1]);
      this.segs[i].radius = this.thickness;
      this.segs[i].elasticity = this.elasticity;
      this.segs[i].friction = this.friction;
      this.segs[i].collide = this.collide;
    }
  },

  flipx: false,
  flipy: false,

  hollow: false,
  hollowt: 0,

  spoints() {
    if (!this.points) return [];
    var spoints = this.points.slice();

    if (this.flipx) {
      if (Spline.is_bezier(this.type)) spoints.push(Vector.reflect_point(spoints.at(-2), spoints.at(-1)));

      for (var i = spoints.length - 1; i >= 0; i--) {
        var newpoint = spoints[i].slice();
        newpoint.x = -newpoint.x;
        spoints.push(newpoint);
      }
    }

    if (this.flipy) {
      if (Spline.is_bezier(this.type)) spoints.push(Vector.reflect(point(spoints.at(-2), spoints.at(-1))));

      for (var i = spoints.length - 1; i >= 0; i--) {
        var newpoint = spoints[i].slice();
        newpoint.y = -newpoint.y;
        spoints.push(newpoint);
      }
    }

    if (this.hollow) {
      var hpoints = vector.inflate(spoints, this.hollowt);
      if (hpoints.length === spoints.length) return spoints;
      var arr1 = hpoints.filter(function (x, i) {
        return i % 2 === 0;
      });
      var arr2 = hpoints.filter(function (x, i) {
        return i % 2 !== 0;
      });
      return arr1.concat(arr2.reverse());
    }

    if (this.looped) spoints = spoints.wrapped(1);

    return spoints;
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
      if (this.looped) spoints = Spline.catmull_loop(spoints);
      else spoints = Spline.catmull_caps(spoints);

      return Spline.sample_angle(this.type, spoints, this.angle);
    }

    if (this.looped && Spline.is_bezier(this.type)) spoints = Spline.bezier_loop(spoints);

    return Spline.sample_angle(this.type, spoints, this.angle);
  },

  boundingbox() {
    return bbox.frompoints(this.points.map(x => x.scale(this.gameobject.scale)));
  },

  /* EDITOR */
  gizmo() {
    if (this.type === Spline.type.catmull || this.type === -1) {
      this.spoints().forEach(x => render.point(this.gameobject.this2screen(x), 3, Color.teal));
      this.points.forEach((x, i) => render.coordinate(this.gameobject.this2screen(x)));
    } else {
      for (var i = 0; i < this.points.length; i += 3) render.coordinate(this.gameobject.this2screen(this.points[i]), 1, Color.teal);

      for (var i = 1; i < this.points.length; i += 3) {
        render.coordinate(this.gameobject.this2screen(this.points[i]), 1, Color.green);
        render.coordinate(this.gameobject.this2screen(this.points[i + 1]), 1, Color.green);
        render.line([this.gameobject.this2screen(this.points[i - 1]), this.gameobject.this2screen(this.points[i])], Color.yellow);
        render.line([this.gameobject.this2screen(this.points[i + 1]), this.gameobject.this2screen(this.points[i + 2])], Color.yellow);
      }
    }
  },

  finish_center(change) {
    this.points = this.points.map(function (x) {
      return x.sub(change);
    });
  },

  pick(pos) {
    var i = Gizmos.pick_gameobject_points(pos, this.gameobject, this.points);
    var p = this.points[i];
    if (!p) return undefined;

    if (Spline.is_catmull(this.type) || this.type === -1) return make_point_obj(this, p);

    var that = this.gameobject;
    var me = this;
    if (p) {
      var o = {
        pos: p,
        sync: me.sync.bind(me),
      };
      if (Spline.bezier_is_handle(this.points, i))
        o.move = function (d) {
          d = that.dir_world2this(d);
          p.x += d.x;
          p.y += d.y;
          Spline.bezier_cp_mirror(me.points, i);
        };
      else
        o.move = function (d) {
          d = that.dir_world2this(d);
          p.x += d.x;
          p.y += d.y;
          var pp = Spline.bezier_point_handles(me.points, i);
          pp.forEach(ph => (me.points[ph] = me.points[ph].add(d)));
        };
      return o;
    }
  },

  rm_node(idx) {
    if (idx < 0 || idx >= this.points.length) return;
    if (Spline.is_catmull(this.type)) this.points.splice(idx, 1);

    if (Spline.is_bezier(this.type)) {
      assert(Spline.bezier_is_node(this.points, idx), "Attempted to delete a bezier handle.");
      if (idx === 0) this.points.splice(idx, 2);
      else if (idx === this.points.length - 1) this.points.splice(this.points.length - 2, 2);
      else this.points.splice(idx - 1, 3);
    }
  },

  add_node(pos) {
    pos = this.gameobject.world2this(pos);
    var idx = 0;
    if (Spline.is_catmull(this.type) || this.type === -1) {
      if (this.points.length >= 2) idx = physics.closest_point(pos, this.points, 400);

      if (idx === this.points.length) this.points.push(pos);
      else this.points.splice(idx, 0, pos);
    }

    if (Spline.is_bezier(this.type)) {
      idx = physics.closest_point(pos, Spline.bezier_nodes(this.points), 400);

      if (idx < 0) return;

      if (idx === 0) {
        this.points.unshift(pos.slice(), pos.add([-100, 0]), Vector.reflect_point(this.points[1], this.points[0]));
        return;
      }
      if (idx === Spline.bezier_node_count(this.points)) {
        this.points.push(Vector.reflect_point(this.points.at(-2), this.points.at(-1)), pos.add([-100, 0]), pos.slice());
        return;
      }
      idx = 2 + (idx - 1) * 3;
      var adds = [pos.add([100, 0]), pos.slice(), pos.add([-100, 0])];
      this.points.splice(idx, 0, ...adds);
    }
  },

  pick_all() {
    var picks = [];
    this.points.forEach(x => picks.push(make_point_obj(this, x)));
    return picks;
  },
};

component.edge2d = function (obj) {
  if (!obj.body) obj.rigidify();
  var edge = Object.create(edge2d);
  edge.body = obj.body;
  return edge;
};

edge2d.spoints.doc = "Returns the controls points after modifiers are applied, such as it being hollow or mirrored on its axises.";
edge2d.inputs = {};
edge2d.inputs.h = function () {
  this.hollow = !this.hollow;
};
edge2d.inputs.h.doc = "Toggle hollow.";

edge2d.inputs["C-g"] = function () {
  if (this.hollowt > 0) this.hollowt--;
};
edge2d.inputs["C-g"].doc = "Thin the hollow thickness.";
edge2d.inputs["C-g"].rep = true;

edge2d.inputs["C-f"] = function () {
  this.hollowt++;
};
edge2d.inputs["C-f"].doc = "Increase the hollow thickness.";
edge2d.inputs["C-f"].rep = true;

edge2d.inputs["M-v"] = function () {
  if (this.thickness > 0) this.thickness--;
};
edge2d.inputs["M-v"].doc = "Decrease spline thickness.";
edge2d.inputs["M-v"].rep = true;

edge2d.inputs["C-y"] = function () {
  this.points = this.spoints();
  this.flipx = false;
  this.flipy = false;
  this.hollow = false;
};
edge2d.inputs["C-y"].doc = "Freeze mirroring,";
edge2d.inputs["M-b"] = function () {
  this.thickness++;
};
edge2d.inputs["M-b"].doc = "Increase spline thickness.";
edge2d.inputs["M-b"].rep = true;

edge2d.inputs.plus = function () {
  if (this.angle <= 1) {
    this.angle = 1;
    return;
  }
  this.angle *= 0.9;
};
edge2d.inputs.plus.doc = "Increase the number of samples of this spline.";
edge2d.inputs.plus.rep = true;

edge2d.inputs.minus = function () {
  this.angle *= 1.1;
};
edge2d.inputs.minus.doc = "Decrease the number of samples on this spline.";
edge2d.inputs.minus.rep = true;

edge2d.inputs["C-r"] = function () {
  this.points = this.points.reverse();
};
edge2d.inputs["C-r"].doc = "Reverse the order of the spline's points.";

edge2d.inputs["C-l"] = function () {
  this.looped = !this.looped;
};
edge2d.inputs["C-l"].doc = "Toggle spline being looped.";

edge2d.inputs["C-c"] = function () {
  switch (this.type) {
    case Spline.type.bezier:
      this.points = Spline.bezier2catmull(this.points);
      break;
  }
  this.type = Spline.type.catmull;
};

edge2d.inputs["C-c"].doc = "Set type of spline to catmull-rom.";

edge2d.inputs["C-b"] = function () {
  switch (this.type) {
    case Spline.type.catmull:
      this.points = Spline.catmull2bezier(Spline.catmull_caps(this.points));
      break;
  }
  this.type = Spline.type.bezier;
};

edge2d.inputs["C-o"] = function () {
  this.type = -1;
};
edge2d.inputs["C-o"].doc = "Set spline to linear.";

edge2d.inputs["C-M-lm"] = function () {
  if (Spline.is_catmull(this.type)) {
    var idx = Math.grab_from_points(
      input.mouse.worldpos(),
      this.points.map(p => this.gameobject.this2world(p)),
      25,
    );
    if (idx === -1) return;
  } else {
  }

  this.points = this.points.newfirst(idx);
};
edge2d.inputs["C-M-lm"].doc = "Select the given point as the '0' of this spline.";

edge2d.inputs["C-lm"] = function () {
  this.add_node(input.mouse.worldpos());
};
edge2d.inputs["C-lm"].doc = "Add a point to the spline at the mouse position.";

edge2d.inputs["C-M-lm"] = function () {
  var idx = -1;
  if (Spline.is_catmull(this.type))
    idx = Math.grab_from_points(
      input.mouse.worldpos(),
      this.points.map(p => this.gameobject.this2world(p)),
      25,
    );
  else {
    var nodes = Spline.bezier_nodes(this.points);
    idx = Math.grab_from_points(
      input.mouse.worldpos(),
      nodes.map(p => this.gameobject.this2world(p)),
      25,
    );
    idx *= 3;
  }

  this.rm_node(idx);
};
edge2d.inputs["C-M-lm"].doc = "Remove point from the spline.";

edge2d.inputs.lm = function () {};
edge2d.inputs.lm.released = function () {};

edge2d.inputs.lb = function () {
  var np = [];

  this.points.forEach(function (c) {
    np.push(Vector.rotate(c, Math.deg2rad(-1)));
  });

  this.points = np;
};
edge2d.inputs.lb.doc = "Rotate the points CCW.";
edge2d.inputs.lb.rep = true;

edge2d.inputs.rb = function () {
  var np = [];

  this.points.forEach(function (c) {
    np.push(Vector.rotate(c, Math.deg2rad(1)));
  });

  this.points = np;
};
edge2d.inputs.rb.doc = "Rotate the points CW.";
edge2d.inputs.rb.rep = true;

/* CIRCLE */

function shape_maker(maker) {
  return function (obj) {
    if (!obj.body) obj.rigidify();
    return maker(obj.body);
  };
}
component.circle2d = shape_maker(os.make_circle2d);
component.poly2d = shape_maker(os.make_poly2d);
component.seg2d = shape_maker(os.make_seg2d);

Object.mix(os.make_circle2d(), {
  boundingbox() {
    return bbox.fromcwh(this.offset, [this.radius, this.radius]);
  },

  set scale(x) {
    this.radius = x;
  },
  get scale() {
    return this.radius;
  },

  get pos() {
    return this.offset;
  },
  set pos(x) {
    this.offset = x;
  },

  grow(x) {
    if (typeof x === "number") this.scale *= x;
    else if (typeof x === "object") this.scale *= x[0];
  },
});

return { component, SpriteAnim };
