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

Object.getPrototypeOf = undefined;
Object.setPrototypeOf = undefined;
Reflect = undefined;
Symbol = undefined;
URIError = undefined;
Proxy = undefined;
Map = undefined;
WeakMap = undefined;
Promise = undefined;
Set = undefined;
WeakSet = undefined;

var json = {};
json.encode = function(value, space, replacer, whitelist)
{
  return JSON.stringify(value, space, replacer);
}

json.decode = function(text, reviver)
{
  return JSON.parse(text,reviver);
}

Object.methods = function(o)
{
  var m = [];
  Object.keys(o).forEach(function(k) {
    if (typeof o[k] === 'function') m.push(k);
  });
  return m;
}
Object.methods.doc = "Retun an array of all functions an object has access to.";

Object.dig = function(obj, path, def)
{
  def ??= {};
  var pp = path.split('.');
  for (var i = 0; i < pp.length-1; i++) {
    obj = obj[pp[i]] = obj[pp[i]] || {};
  }
  obj[pp[pp.length-1]] = def;
  return def;
}

Object.samenewkeys = function(a,b)
{
  b ??= a.__proto__;
  var ret = {};
  ret.same = [];
  ret.unique = [];
  Object.keys(a).forEach(key => (key in b) ? ret.same.push(key) : ret.unique.push(key));
  return ret;
}
Object.samenewkeys.doc = "Return an object listing which keys are the same and unique on a compared to b.";

Object.rkeys = function(o)
{
  var keys = [];
  Object.keys(o).forEach(function(key) {
    keys.push(key);
    if (Object.isObject(o[key]))
      keys.push(Object.rkeys(o[key]));
  });
  return keys;
}

Object.extend = function(from)
{
  var n = {};
  Object.mixin(n, from);
  return n;
}

Object.mixin = function(target, source)
{
  if (typeof source !== 'object') return target;
  Object.defineProperties(target, Object.getOwnPropertyDescriptors(source));
  return target;
};

Object.mix = function(...objs)
{
  var n = {};
  for (var o of objs)
    Object.mixin(n,o);

  return n;
}

Object.deepmixin = function(target, source)
{
  var o = source;
  while (o !== Object.prototype) {
    Object.mixin(target, o);
    o = o.__proto__;
  };
}

Object.deepfreeze = function(obj)
{
  for (var key in obj) {
    if (typeof obj[key] === 'object')
      Object.deepfreeze(obj[key]);
  }
  Object.freeze(obj);
}

/* Goes through each key and overwrites if it's present */
Object.dainty_assign = function(target, source)
{
  Object.keys(source).forEach(function(k) {
    if (typeof source[k] === 'function') return;
    if (!(k in target)) return;
    if (Array.isArray(source[k]))
      target[k] = deep_copy(source[k]);
    else if (Object.isObject(source[k]))
      Object.dainty_assign(target[k], source[k]);
    else
      target[k] = source[k];
  });
}

Object.isObject = function(o)
{
  return (typeof o === 'object' && !Array.isArray(o));
}

Object.setter_assign = function(target, source)
{
  for (var key in target)
    if (Object.isAccessor(target,key) && typeof source[key] !== 'undefined')
      target[key] = source[key];
}

Object.containingKey = function(obj, prop)
{
  if (typeof obj !== 'object') return undefined;
  if (!(prop in obj)) return undefined;
  
  var o = obj;
  while (o.__proto__ && !Object.hasOwn(o, prop))
    o = o.__proto__;

  return o;
}

Object.isAccessor = function(obj, prop)
{
  var o = Object.containingKey(obj,prop);
  if (!o) return false;
  
  var desc = Object.getOwnPropertyDescriptor(o,prop);
  if (!desc) return false;
  if (desc.get || desc.set) return true;
  return false;
}

Object.mergekey = function(o1,o2,k)
{
  if (!o2) return;
  if (Object.isAccessor(o2,k))
    Object.defineProperty(o1, k, Object.getOwnPropertyDescriptor(o2,k));
  else if (typeof o2[k] === 'object') {
    if (Array.isArray(o2[k]))
      o1[k] = deep_copy(o2[k]);
    else {
      if (!o1[k]) o1[k] = {};
      if (typeof o1[k] === 'object')
        Object.merge(o1[k], o2[k]);
      else
        o1[k] = o2[k];
    }
   } else
     o1[k] = o2[k];
}

/* Same as merge from Ruby */
Object.merge = function(target, ...objs)
{
  for (var obj of objs)
    for (var key of Object.keys(obj))
      Object.mergekey(target,obj,key);

  return target;
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
    Object.mixin(c, obj);
  return c;
}

/* OBJECT DEFININTIONS */
Object.defHidden = function(obj, prop)
{
  Object.defineProperty(obj, prop, {enumerable:false, writable:true});
}

Object.hide = function(obj,...props)
{
  for (var prop of props) {
    var p = Object.getOwnPropertyDescriptor(obj,prop);
    if (!p) {
      Log.warn(`No property of name ${prop}.`);
      return;
    }
    p.enumerable = false;
    Object.defineProperty(obj, prop, p);
  }
}

Object.unhide = function(obj, ...props)
{
  for (var prop of props) {
    var p = Object.getOwnPropertyDescriptor(obj,prop);
    if (!p) {
      Log.warn(`No property of name ${prop}.`);
      return;
    }
    p.enumerable = true;
    Object.defineProperty(obj, prop, p);
  }
}

Object.defineProperty(Object.prototype, 'obscure', {
  value: function(name) {
    Object.defineProperty(this, name, { enumerable: false });
  }
});

Object.defineProperty(Object.prototype, 'hasOwn', {
  value: function(x) { return this.hasOwnProperty(x); }
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
    Object.values(this).forEach(fn);
  }
});

Object.defineProperty(Object.prototype, 'map', {
  value: function(fn) {
    var a = [];
    Object.values(this).forEach(function(x) {
      a.push(fn(x));
    });
    return a;
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

Object.defineProperty(Object.prototype, 'filter', {
  value: function(fn) {
    return Object.values(this).filter(fn);
  }
});
    
Object.defineProperty(Object.prototype, 'push', {
  value: function(val) {
    var str = val.toString();
    str = str.replaceAll('.', '_');
    var n = 1;
    var t = str;
    while (Object.hasOwn(this,t)) {
      t = str + n;
      n++;      
    }
    this[t] = val;
    return t;
  }
});

Object.defineProperty(Object.prototype, 'remove', {
  value: function(val) {
    delete this[val.toString()];
  }
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

Object.defineProperty(String.prototype, 'strip_ext', {
  value: function() { return this.tolast('.'); }
});

Object.defineProperty(String.prototype, 'ext', {
  value: function() { return this.fromlast('.'); }
});

Object.defineProperty(String.prototype, 'set_ext', {
  value: function(val) { return this.strip_ext() + val; }
});

Object.defineProperty(String.prototype, 'fromlast', {
  value: function(val) {
    var idx = this.lastIndexOf(val);
    if (idx === -1) return "";
    return this.slice(idx+1);
  }
});

Object.defineProperty(String.prototype, 'tolast', {
  value: function(val) {
    var idx = this.lastIndexOf(val);
    if (idx === -1) return this.slice();
    return this.slice(0,idx);
  }
});

Object.defineProperty(String.prototype, 'tofirst', {
  value: function(val) {
    var idx = this.indexOf(val);
    if (idx === -1) return this.slice();
    return this.slice(0,idx);
  }
});

Object.defineProperty(String.prototype, 'fromfirst', {
  value: function(val) {
    var idx = this.indexOf(val);
    if (idx === -1) return this;
    return this.slice(idx+val.length);
  }
});

Object.defineProperty(String.prototype, 'name', {
  value: function() {
    var idx = this.indexOf('/');
    if (idx === -1)
      return this.tolast('.');
    return this.fromlast('/').tolast('.'); }
});

Object.defineProperty(String.prototype, 'base', {
  value: function() { return this.fromlast('/'); }
});

Object.defineProperty(String.prototype, 'dir', {
  value: function() { return this.tolast('/'); }
});

Object.defineProperty(String.prototype, 'splice', {
  value: function(index, str, ) {
    return this.slice(0,index) + str + this.slice(index);
  }
});

Object.defineProperty(String.prototype, 'rm', {
  value: function(index, endidx) {
    endidx ??= index+1;
    return this.slice(0,index) + this.slice(endidx);
  }
});

Object.defineProperty(String.prototype, 'updir', {
  value: function() {
    if (this.lastIndexOf('/') === this.length-1)
      return this.slice(0,this.length-1);
    
    var dir = (this + "/").dir();
    return dir.dir();
  }
});

Object.defineProperty(String.prototype, 'startswith', {
  value: function(val) {
    if (!val) return false;
    return this.startsWith(val);
  }
});

Object.defineProperty(String.prototype, 'endswith', {
  value: function(val) {
    if (!val) return false;
    return this.endsWith(val);
  }
});

Object.defineProperty(String.prototype, 'pct', {
  value: function(val) {
  }
});

Object.defineProperty(String.prototype, 'uc', { value: function() { return this.toUpperCase(); } });

Object.defineProperty(String.prototype, 'lc', {value:function() { return this.toLowerCase(); }});


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

Object.defineProperty(Array.prototype, 'dofilter', {
  value: function(fn) {
    var j = 0;
    this.forEach(function(val,i) {
      if (fn(val)) {
        if (i !== j) this[j] = val;
	j++;
      }
    }, this);
    this.length = j;
    return this;
  }
});


function filterInPlace(a, condition, thisArg) {
  let j = 0;

  a.forEach((e, i) => { 
    if (condition.call(thisArg, e, i, a)) {
      if (i!==j) a[j] = e; 
      j++;
    }
  });

  a.length = j;
  return a;
}


Object.defineProperty(Array.prototype, 'reversed', {
  value: function() {
    var c = this.slice();
    return c.reverse();
  }
});

Object.defineProperty(Array.prototype, 'rotate', {
  value: function(a) {
    return Vector.rotate(this, a);
  }
});

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

var arr_elems = ['x', 'y', 'z', 'w'];
var quat_elems = ['i', 'j', 'k'];
var color_elems = ['r', 'g', 'b', 'a'];

arr_elems.forEach(function(x, i) { arrsetelem(x,i); });
quat_elems.forEach(function(x, i) { arrsetelem(x,i); });
color_elems.forEach(function(x, i) { arrsetelem(x,i); });

var nums = [0,1,2,3];

var swizz = [];

for (var i of nums)
  for (var j of nums)
    swizz.push([i,j]);

swizz.forEach(function(x) {
  var str = "";
  for (var i of x)
    str += arr_elems[i];
    
  Object.defineProperty(Array.prototype, str, {
    get() { return [this[x[0]], this[x[1]]]; },
    set(j) { this[x[0]] = j[0]; this[x[1]] = j[1]; },
  });

  str = "";
  for (var i of x) str += color_elems[i]; 
  Object.defineProperty(Array.prototype, str, {
    get() { return [this[x[0]], this[x[1]]]; },
    set(j) { this[x[0]] = j[0]; this[x[1]] = j[1]; },
  });

});


swizz = [];
for (var i of nums)
  for (var j of nums)
    for (var k of nums)
      swizz.push([i,j,k]);

swizz.forEach(function(x) {
  var str = "";
  for (var i of x)
    str += arr_elems[i];
    
  Object.defineProperty(Array.prototype, str, {
    get() { return [this[x[0]], this[x[1]], this[x[2]]]; },
    set(j) { this[x[0]] = j[0]; this[x[1]] = j[1]; this[x[2]] = j[2];},
  });

  str = "";
  for (var i of x) str += color_elems[i];
  Object.defineProperty(Array.prototype, str, {
    get() { return [this[x[0]], this[x[1]], this[x[2]]]; },
    set(j) { this[x[0]] = j[0]; this[x[1]] = j[1]; this[x[2]] = j[2];},
  });
});


swizz = [];
for (var i of nums)
  for (var j of nums)
    for (var k of nums)
      for (var w of nums)
        swizz.push([i,j,k,w]);

swizz.forEach(function(x) {
  var str = "";
  for (var i of x)
    str += arr_elems[i];
    
  Object.defineProperty(Array.prototype, str, {
    get() { return [this[x[0]], this[x[1]], this[x[2]], this[x[3]]];},
    set(j) { this[x[0]] = j[0]; this[x[1]] = j[1]; this[x[2]] = j[2]; this[x[3]] = j[3];},
  });

  str = "";
  for (var i of x) str += color_elems[i];
    Object.defineProperty(Array.prototype, str, {
    get() { return [this[x[0]], this[x[1]], this[x[2]], this[x[3]]];},
    set(j) { this[x[0]] = j[0]; this[x[1]] = j[1]; this[x[2]] = j[2]; this[x[3]] = j[3];},
  });

});


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

  return JSON.stringify(this.sort()) === JSON.stringify(b.sort());
  
  for (var i = 0; i < this.length; i++) {
    if (!this[i] === b[i])
      return false;
  }

  return true;
}});

function add(x,y) { return x+y; };
function mult(x,y) { return x*y; };

Object.defineProperty(Array.prototype, 'mapc', {
  value: function(fn) {
    return this.map(x => fn(x));
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

Math.grab_from_points = function(pos, points, slop) {
  var shortest = slop;
  var idx = -1;
  points.forEach(function(x,i) {
    if (Vector.length(pos.sub(x)) < shortest) {
      shortest = Vector.length(pos.sub(x));
      idx = i;
    }
  });
  return idx;
};


Number.prec = function(num)
{
  return parseFloat(num.toFixed(3));
}

Number.hex = function(n)
{
  var s = Math.floor(n).toString(16);
  if (s.length === 1) s = '0' + s;
  return s.uc();
}

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
Math.patan2 = function(p) { return Math.atan2(p.y,p.x); };
Math.deg2rad = function(deg) { return deg * 0.0174533; };
Math.rad2deg = function(rad) { return rad / 0.0174533; };
Math.randomint = function(max) { return Math.clamp(Math.floor(Math.random() * max), 0, max-1); };

/* BOUNDINGBOXES */
function cwh2bb(c, wh) {
  return {
    t: c.y+(wh.y/2),
    b: c.y-(wh.y/2),
    l: c.x-(wh.x/2),
    r: c.x+(wh.x/2)
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

function bl2bb(bl, wh)
{
  return {
    b: bl.y,
    l: bl.x,
    r: bl.x + wh.x,
    t: bl.y + wh.y
  };
}

function bb_from_objects(objs) {
  var bb = objs[0].boundingbox;
  objs.forEach(function(obj) { bb = bb_expand(bb, obj.boundingbox); });
  return bb;
};

var Boundingbox = {};
Boundingbox.width = function(bb) { return bb.r - bb.l; };
Boundingbox.height = function(bb) { return bb.t - bb.b; };
Boundingbox.bl = function(bb) { return [bb.l, bb.b] };

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
    return cmd(88,a,b);
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

  reflect(vec, plane) {
    var p = Vector.norm(plane);
    return vec.sub(p.scale(2*Vector.dot(vec, p)));
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
