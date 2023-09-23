/* Removing crud I don't like */
/* Functions that lead to bloated error prone javascript */
/* It is EMCA6 but without a lot of builtin objects and various functions. There are no:
   * Promises and so on (Generators, async)
   * WeakMaps and so on (weakset, weakref)
   * Typed arrays
   * Proxys
   * Modules
   * Symbols (use closures)
   In addition to the removal of a bunch of stuff as seen here.
   Access prototypes through __proto__ instead of the long-winded Object.getProtoTypeOf.
*/
var FNRM = function() { Log.error("removed"); };
Object.getPrototypeOf = FNRM;
Object.setPrototypeOf = FNRM;
Reflect = {};
Symbol = {};
URIError = {};

Object.complete_assign = function(target, source)
{
  if (typeof source === 'undefined') return target;
  
  Object.keys(source).forEach(function (k) {
    if (Object.isAccessor(source,k))
      Object.defineProperty(target, k, Object.getOwnPropertyDescriptor(source,k));
    else
      target[k] = source[k];
  });

  return target;
};

Object.deepfreeze = function(obj)
{
  for (var key in obj) {
    if (typeof obj[key] === 'object')
      Object.deepfreeze(obj[key]);
  }
  Object.freeze(obj);
}

/* Goes through each key and overwrites if it's present, adds if it is not */
Object.dainty_assign = function(target, source)
{
  for (var key in source) {

    if (typeof source[key] === 'function') {
      target[key] = source[key];
      continue;
    }
    if (!Object.hasOwn(target, key)) continue;
    if (!Object.getOwnPropertyDescriptor(target, key).writable) continue;
    
    if (Array.isArray(target[key]))
      target[key] = source[key];
    else if (typeof target[key] === 'object')
      Object.dainty_assign(target[key], source[key]);
    else {
      target[key] = source[key];
    }
  }
}

Object.isAccessor = function(obj, prop)
{
  var desc = Object.getOwnPropertyDescriptor(obj,prop);
  if (!desc) return false;
  if (desc.get || desc.set) return true;
  return false;
}

Object.mergekey = function(o1,o2,k)
{
  if (typeof(o2[k]) === 'object') {
    if (Array.isArray(o2[k]))
      o1[k] = o2[k].slice();
    else
      Object.merge(o1[k], o2[k]);
   } else
     o1[k] = o2[k];
}

/* Same as merge from Ruby */
Object.merge = function(target, ...objs)
{
  for (var obj of objs)
    for (var key of Object.keys(obj))
      Object.mergekey(target,obj,key);
}

Object.totalmerge = function(target, ...objs)
{
  for (var obj of objs)
    for (var key in obj)
      Object.mergekey(target,obj,key);
}

/* Returns a new object with undefined, null, and empty values removed. */
Object.compact = function(obj)
{

}

Object.totalassign = function(to, from)
{
  for (var key in from)
    to[key] = from[key];
}

/* Prototypes out an object and assigns values */
Object.copy = function(proto, ...objs)
{
  var c = Object.create(proto);
  for (var obj of objs)
    Object.complete_assign(c, obj);
  return c;
}

/* OBJECT DEFININTIONS */
Object.defHidden = function(obj, prop)
{
  Object.defineProperty(obj, prop, {enumerable:false, writable:true});
}

Object.defineProperty(Object.prototype, 'obscure', {
  value: function(name) {
    Object.defineProperty(this, name, { enumerable: false });
  }
});

Object.defineProperty(Object.prototype, 'hasOwn', {
  value: function(x) { return this.hasOwnProperty(x); }
});

Object.defineProperty(Object.prototype, 'array', {
  value: function() {
    var a = [];
    for (var key in this)
      a.push(this[key]);

    return a;
  },
});

Object.defineProperty(Object.prototype, 'defn', {
  value: function(name, val) {
    Object.defineProperty(this, name, { value:val, writable:true, configurable:true });
  }
});

Object.defineProperty(Object.prototype, 'nulldef', {
  value: function(name, val) {
    if (!this.hasOwnProperty(name)) this[name] = val;
  }
});

/*Object.defineProperty(Object.prototype, 'writable', {
  value: function(name) {
    return Object.getPropertyDescriptor(this, name).writable;
  }
});
*/

Object.defineProperty(Object.prototype, 'prop_obj', {
  value: function() { return JSON.parse(JSON.stringify(this)); }
});

/* defc 'define constant'. Defines a value that is not writable. */
Object.defineProperty(Object.prototype, 'defc', {
  value: function(name, val) {
    Object.defineProperty(this,name, {
      value: val,
      writable:false,
      enumerable:true,
      configurable:false,
    });
  }
});

Object.defineProperty(Object.prototype, 'stick', {
  value: function(prop) {
    Object.defineProperty(this, prop, {writable:false});
  }
});

Object.defineProperty(Object.prototype, 'harden', {
  value: function(prop) {
    Object.defineProperty(this, prop, {writable:false, configurable:false, enumerable: false});
  }
});

Object.defineProperty(Object.prototype, 'deflock', {
  value: function(prop) {
    Object.defineProperty(this,prop, {configurable:false});
  }
});

Object.defineProperty(Object.prototype, 'forEach', {
  value: function(fn) {
    for (var key in this)
      fn(this[key]);
  }
});


Object.defineProperty(Object.prototype, 'empty', {
  get: function() {
    return Object.keys(this).empty;
  },
});

Object.defineProperty(Object.prototype, 'nth', {
  value: function(x) {
    if (this.empty || x >= Object.keys(this).length) return null;

    return this[Object.keys(this)[x]];
  },
});

Object.defineProperty(Object.prototype, 'findIndex', {
  value: function(x) {
    var i = 0;
    for (var key in this) {
      if (this[key] === x) return i;
      i++;
    }

    return -1;
  }
});


/* STRING DEFS */

Object.defineProperty(String.prototype, 'next', {
  value: function(char, from) {
    if (!Array.isArray(char))
      char = [char];
    if (from > this.length-1)
      return -1;
    else if (!from)
      from = 0;

    var find = this.slice(from).search(char[0]);

    if (find === -1)
      return -1;
    else
      return from + find;
    
    var i = 0;
    var c = this.charAt(from+i);
    while (!char.includes(c)) {
      i++;
      if (from+i >this.length-1) return -1;
      c = this.charAt(from+i);
    }

    return from+i;
  }
});

Object.defineProperty(String.prototype, 'prev', {
  value: function(char, from, count) {
    if (from > this.length-1)
      return -1;
    else if (!from)
      from = this.length-1;

    if (!count) count = 0;

    var find = this.slice(0,from).lastIndexOf(char);

    while (count > 1) {
      find = this.slice(0,find).lastIndexOf(char);
      count--;
    }

    if (find === -1)
      return 0;
    else
      return find;
  }
});

Object.defineProperty(String.prototype, 'shift', {
  value: function(n) {
    if (n === 0) return this.slice();

    if (n > 0)
      return this.slice(n);

    if (n < 0)
      return this.slice(0, this.length+n);
  }
});


Object.defineProperty(String.prototype, 'ext', {
  value: function() {
    var idx = this.lastIndexOf('.');
    if (idx === -1) return undefined;
    return this.slice(idx);
  }
});

Object.defineProperty(String.prototype, 'set_ext', {
  value: function(val) {
    return this.dir() + "/" + this.name() + val;
  }
});

Object.defineProperty(String.prototype, 'name', {
  value: function() {
    var s = this.lastIndexOf('/');
    var e = this.lastIndexOf('.');
    if (e === -1) e = this.length;
    return this.slice(s+1,e);
  }
});

Object.defineProperty(String.prototype, 'base', {
  value: function() {
    return this.slice(this.lastIndexOf('/')+1);
  }
});

Object.defineProperty(String.prototype, 'dir', {
  value: function() {
    var e = this.lastIndexOf('/');
    return this.slice(0, e);
  }
});


/* ARRAY DEFS */

Object.defineProperty(Array.prototype, 'copy', {
  value: function() {
    var c = [];

    this.forEach(function(x, i) {
      c[i] = deep_copy(x);
    });
    
    return c;
  }
});

Object.defineProperty(Array.prototype, 'rotate', {
  value: function(a) {
    return Vector.rotate(this, a);
  }
});


Object.defineProperty(Array.prototype, '$add', {
value: function(b) {
  for (var i = 0; i < this.length; i++) {
    this[i] += b[i];
  }
}});

function setelem(n) {
  return {
    get: function() { return this[n]; },
    set: function(x) { this[n] = x; }
  }
};

function arrsetelem(str, n)
{
  Object.defineProperty(Array.prototype, str, setelem(n));
}

Object.defineProperty(Array.prototype, 'x', setelem(0));
Object.defineProperty(Array.prototype, 'y', setelem(1));
Object.defineProperty(Array.prototype, 'z', setelem(2));
Object.defineProperty(Array.prototype, 'w', setelem(3));
arrsetelem('r', 0);
arrsetelem('g', 1);
arrsetelem('b', 2);
arrsetelem('a', 3);


Object.defineProperty(Array.prototype, 'add', {
value: function(b) {
  var c = [];
  for (var i = 0; i < this.length; i++) { c[i] = this[i] + b[i]; }
  return c;
}});

Object.defineProperty(Array.prototype, 'newfirst', {
  value: function(i) {
    var c = this.slice();
    if (i >= c.length) return c;

    do {
      c.push(c.shift());
      i--;
    } while (i > 0);

    return c;
  }
});

Object.defineProperty(Array.prototype, 'doubleup', {
  value: function(n) {
    var c = [];
    this.forEach(function(x) {
      for (var i = 0; i < n; i++)
        c.push(x);
    });
    
    return c;
  }
});

Object.defineProperty(Array.prototype, 'sub', {
value: function(b) {
  var c = [];
  for (var i = 0; i < this.length; i++) { c[i] = this[i] - b[i]; }
  return c;
}});

Object.defineProperty(Array.prototype, 'mult', {
  value: function(arr) {
    var c = [];
    for (var i = 0; i < this.length; i++) { c[i] = this[i] * arr[i]; }
    return c;
}});

Object.defineProperty(Array.prototype, 'apply', {
  value: function(fn) {
    this.forEach(function(x) { x[fn].apply(x); });
  }
});

Object.defineProperty(Array.prototype, 'scale', {
  value: function(s) {
    if (Array.isArray(s)) {
      var c = this.slice();
      c.forEach(function(x,i) { c[i] = x * s[i]; });
      return c;
    }
    return this.map(function(x) { return x*s; });
}});

Object.defineProperty(Array.prototype, 'equal', {
value: function(b) {
  if (this.length !== b.length) return false;
  if (b == null) return false;
  if (this === b) return true;

  return JSON.stringify(this) === JSON.stringify(b);
  
  for (var i = 0; i < this.length; i++) {
    if (!this[i] === b[i])
      return false;
  }

  return true;
}});

function add(x,y) { return x+y; };
function mult(x,y) { return x*y; };

Object.defineProperty(Array.prototype, 'mapc', {
  value: function(fn, arr) {
      return this.map(function(x, i) {
        return fn(x, arr[i]);
      });
  }
});

Object.defineProperty(Array.prototype, 'remove', {
 value: function(b) {
  var idx = this.indexOf(b);
  
  if (idx === -1) return false;
  
  this.splice(idx, 1);

  return true;
}});

Object.defineProperty(Array.prototype, 'set', {
  value: function(b) {
    if (this.length !== b.length) return;

    b.forEach(function(val, i) { this[i] = val; }, this);
  }
});

Object.defineProperty(Array.prototype, 'flat', {
  value: function() {
    return [].concat.apply([],this);
  }
});

Object.defineProperty(Array.prototype, 'any', {
  value: function(fn) {
    var ev = this.every(function(x) {
      return !fn(x);
    });
    return !ev;
  }
});

/* Return true if array contains x */
/*Object.defineProperty(Array.prototype, 'includes', {
value: function(x) {
  return this.some(e => e === x);
}});
*/
Object.defineProperty(Array.prototype, 'empty', {
  get: function() { return this.length === 0; },
});

Object.defineProperty(Array.prototype, 'push_unique', {
  value: function(x) {
  if (!this.includes(x)) this.push(x);
}});

Object.defineProperty(Array.prototype, 'unique', {
  value: function() {
    var c = [];
    this.forEach(function(x) { c.push_unique(x); });
    return c;
  }
});


Object.defineProperty(Array.prototype, 'findIndex', {
 value: function(fn) {
  var idx = -1;
  this.every(function(x, i) {
    if (fn(x)) {
      idx = i;
      return false;
    }

    return true;
  });

  return idx;
}});

Object.defineProperty(Array.prototype, 'find', {
 value: function(fn) {
  var ret;

  this.every(function(x) {
    if (fn(x)) {
      ret = x;
      return false;
    }

    return true;
  });

  return ret;
}});

Object.defineProperty(Array.prototype, 'last', {
  get: function() { return this[this.length-1]; },
});

Object.defineProperty(Array.prototype, 'at', {
value: function(x) {
  return x < 0 ? this[this.length+x] : this[x];
}});

Object.defineProperty(Array.prototype, 'wrapped', {
  value: function(x) {
    var c = this.slice(0, this.length);

    for (var i = 0; i < x; i++)
      c.push(this[i]);
      
    return c;
}});

Object.defineProperty(Array.prototype, 'wrap_idx', {
  value: function(x) {
    while (x >= this.length) {
      x -= this.length;
    }

    return x;
  }
});

Object.defineProperty(Array.prototype, 'mirrored', {
  value: function(x) {
    var c = this.slice(0);
    if (c.length <= 1) return c;
    for (var i = c.length-2; i >= 0; i--)
      c.push(c[i]);

    return c;
  }
});

Object.defineProperty(Array.prototype, 'lerp', {
  value: function(to, t) {
    var c = [];
    this.forEach(function(x,i) {
      c[i] = (to[i] - x) * t + x;
    });
    return c;
  }
});

Math.lerp = function(s,f,t) { return (f-s)*t + s; };

Object.defineProperty(Object.prototype, 'lerp',{
  value: function(to, t) {
    var self = this;
    var obj = {};

    Object.keys(self).forEach(function(key) {
      obj[key] = self[key].lerp(to[key],t);
    });

    return obj;
}});

/* MATH EXTENSIONS */
Object.defineProperty(Number.prototype, 'lerp', {
  value: function(to, t) {
    var s = this;
    return (to - this) * t + this;
  }
});

Math.clamp = function (x, l, h) { return x > h ? h : x < l ? l : x; }

Math.random_range = function(min,max) { return Math.random() * (max-min) + min; };

Math.snap = function(val, grid) {
  if (!grid || grid === 1) return Math.round(val);

  var rem = val%grid;
  var d = val - rem;
  var i = Math.round(rem/grid)*grid;
  return d+i;
}

Math.angledist = function (a1, a2) {
    var dist = a2 - a1;
    var wrap = dist >= 0 ? dist+360 : dist-360;
    wrap %= 360;

    if (Math.abs(dist) < Math.abs(wrap))
      return dist;

    return wrap;
};
Math.angledist.doc = "Find the shortest angle between two angles.";

Math.deg2rad = function(deg) { return deg * 0.0174533; };
Math.rad2deg = function(rad) { return rad / 0.0174533; };
Math.randomint = function(max) { return Math.clamp(Math.floor(Math.random() * max), 0, max-1); };

/* BOUNDINGBOXES */
function cwh2bb(c, wh) {
  return {
    t: c.y+wh.y/2,
    b: c.y-wh.y/2,
    l: c.x-wh.x/2,
    r: c.x+wh.x/2
  };
};

function points2bb(points) {
    var b= {t:0,b:0,l:0,r:0};
    
    points.forEach(function(x) {
      if (x.y > b.t) b.t = x.y;
      if (x.y < b.b) b.b = x.y;
      if (x.x > b.r) b.r = x.x;
      if (x.x < b.l) b.l = x.x;
    });
    
    return b;
};

function bb2points(bb)
{
  return [
   [bb.l,bb.t],
   [bb.r,bb.t],
   [bb.r,bb.b],
   [bb.l,bb.b]
  ];
}

function points2cwh(start,end)
{
    var c = [];
    c[0] = (end[0] - start[0]) / 2;
    c[0] += start[0];
    c[1] = (end[1] - start[1]) / 2; 
    c[1] += start[1];
    var wh = [];
    wh[0] = Math.abs(end[0] - start[0]);
    wh[1] = Math.abs(end[1] - start[1]);
    return {c: c, wh: wh};
}

function bb2cwh(bb) {
  if (!bb) return undefined;
  var cwh = {};

  var w = bb.r - bb.l;
  var h = bb.t - bb.b;
  cwh.wh = [w, h];
  cwh.c = [bb.l + w/2, bb.b + h/2];
  
  return cwh;
};

function pointinbb(bb, p)
{
  if (bb.t < p.y || bb.b > p.y || bb.l > p.x || bb.r < p.x)
    return false;

  return true;
}

function movebb(bb, pos) {
  var newbb = Object.assign({}, bb);
  newbb.t += pos.y;
  newbb.b += pos.y;
  newbb.l += pos.x;
  newbb.r += pos.x;
  return newbb;
};

function bb_expand(oldbb, x) {
  if (!oldbb || !x) return;
  var bb = {};
  Object.assign(bb, oldbb);
  
  if (bb.t < x.t) bb.t = x.t;
  if (bb.r < x.r) bb.r = x.r;
  if (bb.b > x.b) bb.b = x.b;
  if (bb.l > x.l) bb.l = x.l;

  return bb;
};

function bb_from_objects(objs) {
  var bb = objs[0].boundingbox;
  objs.forEach(function(obj) { bb = bb_expand(bb, obj.boundingbox); });
  return bb;
};


/* VECTORS */
var Vector = {
  x: 0,
  y: 0,
  length(v) {
    var sum = v.reduce(function(acc, val) { return acc + val**2; }, 0);
    return Math.sqrt(sum);
  },
  norm(v) {
    var len = Vector.length(v);
    return [v.x/len, v.y/len];
    
  },
  make(x, y) {
    var vec = Object.create(this, {x:x, y:y});
  },

  project(a, b) {
    return cmd(85, a, b);
  },

  dot(a, b) {

  },
  
  random() {
    var vec = [Math.random()-0.5, Math.random()-0.5];
    return Vector.norm(vec);
  },
  
  angle(v) {
    return Math.atan2(v.y, v.x);
  },
  
  rotate(v,angle) {
    var r = Vector.length(v);
    var p = Vector.angle(v) + angle;
    return [r*Math.cos(p), r*Math.sin(p)];
  },

  equal(v1, v2, tol) {
    if (!tol)
      return v1.equal(v2);

    var eql = true;
    var c = v1.sub(v2);

    c.forEach(function(x) {
      if (!eql) return;
      if (Math.abs(x) > tol)
        eql = false;
    });

    return eql;
  },
  
};

/* POINT ASSISTANCE */

function points2cm(points)
{
  var x = 0;
  var y = 0;
  var n = points.length;
  points.forEach(function(p) {
    x = x + p[0];
    y = y + p[1];
  });
  
  return [x/n,y/n];
};

function sortpointsccw(points)
{
  var cm = points2cm(points);
  var cmpoints = points.map(function(x) { return x.sub(cm); });
  var ccw = cmpoints.sort(function(a,b) {
    aatan = Math.atan2(a.y, a.x);
    batan = Math.atan2(b.y, b.x);
    return aatan - batan;
  });
  
  return ccw.map(function(x) { return x.add(cm); });
}
