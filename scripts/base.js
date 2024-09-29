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

/*
Object.getPrototypeOf = undefined;
Object.setPrototypeOf = undefined;
Reflect = undefined
Symbol = undefined;
URIError = undefined;
Proxy = undefined;
Map = undefined;
WeakMap = undefined;
Promise = undefined;
Set = undefined;
WeakSet = undefined;
*/

Number.roman = {
  M: 1000,
  D: 500,
  C: 100,
  L: 50,
  X: 10,
  V: 5,
  I: 1,
};

var convert = {};
convert.romanize = function (num) {
  if (!+num) return false;
  var digits = String(+num).split("");
  var key = ["", "C", "CC", "CCC", "CD", "D", "DC", "DCC", "DCCC", "CM", "", "X", "XX", "XXX", "XL", "L", "LX", "LXX", "LXXX", "XC", "", "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX"];
  var roman = "",
    i = 3;
  while (i--) roman = (key[+digits.pop() + i * 10] || "") + roman;
  return Array(+digits.join("") + 1).join("M") + roman;
};

convert.deromanize = function (str) {
  var str = str.toUpperCase();
  var validator = /^M*(?:D?C{0,3}|C[MD])(?:L?X{0,3}|X[CL])(?:V?I{0,3}|I[XV])$/;
  var token = /[MDLV]|C[MD]?|X[CL]?|I[XV]?/g;
  var key = {
    M: 1000,
    CM: 900,
    D: 500,
    CD: 400,
    C: 100,
    XC: 90,
    L: 50,
    XL: 40,
    X: 10,
    IX: 9,
    V: 5,
    IV: 4,
    I: 1,
  };
  var num = 0,
    m;
  if (!(str && validator.test(str))) return false;
  while ((m = token.exec(str))) num += key[m[0]];
  return num;
};

convert.buf2hex = function (buffer) {
  // buffer is an ArrayBuffer
  return [...new Uint8Array(buffer)].map(x => x.toString(16).padStart(2, "0")).join(" ");
};

/* Time values are always expressed in terms of real earth-seconds */
Object.assign(time, {
  hour2minute() {
    return this.hour / this.minute;
  },
  day2hour() {
    return this.day / this.hour;
  },
  minute2second() {
    return this.minute / this.second;
  },
  week2day() {
    return this.week / this.day;
  },
});

time.strparse = {
  yyyy: "year",
  mm: "month",
  m: "month",
  eee: "yday",
  dd: "day",
  d: "day",
  v: "weekday",
  hh: "hour",
  h: "hour",
  nn: "minute",
  n: "minute",
  ss: "second",
  s: "second",
};

time.doc = {
  doc: "Functions for manipulating time.",
  second: "Earth-seconds in a second.",
  minute: "Seconds in a minute.",
  hour: "Seconds in an hour.",
  day: "Seconds in a day.",
  week: "Seconds in a week.",
  weekdays: "Names of the days of the week.",
  monthstr: "Full names of the months of the year.",
  epoch: "Times are expressed in terms of day 0 at hms 0 of this year.",
  now: "Get the time now.",
  computer_zone: "Get the time zone of the running computer.",
  computer_dst: "Return true if the computer is in daylight savings.",
  yearsize: "Given a year, return the number of days in that year.",
  monthdays: "Number of days in each month.",
  fmt: "Default format for time.",
  record: "Given a time, return an object with time fields.",
  number: "Return the number representation of a given time.",
  text: "Return a text formatted time.",
};

time.second = 1;
time.minute = 60;
time.hour = 3_600;
time.day = 86_400;
time.week = 604_800;
time.weekdays = ["Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"];
time.monthstr = ["January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"];

time.epoch = 1970;
time.isleap = function (year) {
  return this.yearsize(year) === 366;
};
time.isleap.doc = "Return true if the given year is a leapyear.";

time.yearsize = function (y) {
  if (y % 4 === 0 && (y % 100 != 0 || y % 400 === 0)) return 366;
  return 365;
};

time.timecode = function (t, fps = 24) {
  var s = Math.trunc(t);
  t -= s;
  return `${s}:${Math.trunc(fps * s)}`;
};

time.monthdays = [31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31];
time.zones = {};
time.zones["-12"] = "IDLW";
time.record = function (num, zone = this.computer_zone()) {
  if (typeof num === "object") return num;
  else if (typeof num === "number") {
    var monthdays = this.monthdays.slice();
    var rec = {
      second: 0,
      minute: 0,
      hour: 0,
      yday: 0,
      year: 0,
      zone: 0,
    };
    rec.zone = zone;
    num += zone * this.hour;

    var hms = num % this.day;
    var day = parseInt(num / this.day);

    if (hms < 0) {
      hms += this.day;
      day--;
    }
    rec.second = hms % this.minute;
    var d1 = Math.floor(hms / this.minute);
    rec.minute = d1 % this.minute;
    rec.hour = Math.floor(d1 / this.minute);

    /* addend%7 is 4 */
    rec.weekday = (day + 4503599627370496 + 2) % 7;

    var d1 = this.epoch;
    if (day >= 0) for (d1 = this.epoch; day >= this.yearsize(d1); d1++) day -= this.yearsize(d1);
    else for (d1 = this.epoch; day < 0; d1--) day += this.yearsize(d1 - 1);

    rec.year = d1;
    if (rec.year <= 0) rec.ce = "BC";
    else rec.ce = "AD";

    rec.yday = day;

    if (this.yearsize(d1) === 366) monthdays[1] = 29;

    var d0 = day;
    for (d1 = 0; d0 >= monthdays[d1]; d1++) d0 -= monthdays[d1];

    monthdays[1] = 28;
    rec.day = d0 + 1;

    rec.month = d1;
    return rec;
  }
};

time.number = function (rec) {
  if (typeof rec === "number") return rec;
  else if (typeof rec === "object") {
    var c = 0;
    var year = rec.year ? rec.year : 0;
    var hour = rec.hour ? rec.hour : 0;
    var minute = rec.minute ? rec.minute : 0;
    var second = rec.second ? rec.second : 0;
    var zone = rec.zone ? rec.zone : 0;

    if (year > this.epoch) for (var i = this.epoch; i < year; i++) c += this.day * this.yearsize(i);
    else if (year < this.epoch) {
      for (var i = this.epoch - 1; i > year; i--) c += this.day * this.yearsize(i);

      c += (this.yearsize(year) - yday - 1) * this.day;
      c += (this.day2hour() - hour - 1) * this.hour;
      c += (this.hour2minute() - minute - 1) * this.minute;
      c += this.minute2second() - second;
      c += zone * this.hour;
      c *= -1;
      return c;
    }

    c += second;
    c += minute * this.minute;
    c += hour * this.hour;
    c += yday * this.day;
    c -= zone * this.hour;

    return c;
  }
};

/* Time formatting
  yyyy - year in a 4 digit field
  y - as many digits as necessary
  mm - month (1-12)
  mB - month name
  mb - abbreviated month name
  dd - day (1-31)
  d
  c - if the year is <= 0, BC. Otherwise, AD.
  hh - hour
  h
  nn - minutes
  n
  ss - seconds
  s
  v - day of the week (0-6)
  vB - weekday name
  vb - abbreviated weekday name
  a - am/pm
  z - zone, -12 to +11
*/

time.fmt = "vB mB d h:nn:ss TZz a y c";

/* If num is a number, converts to a rec first. */
time.text = function (num, fmt = this.fmt, zone) {
  var rec = num;

  if (typeof rec === "number") rec = time.record(num, zone);

  zone = rec.zone;

  if (fmt.match("a")) {
    if (rec.hour >= 13) {
      rec.hour -= 12;
      fmt = fmt.replaceAll("a", "PM");
    } else if (rec.hour === 12) fmt = fmt.replaceAll("a", "PM");
    else if (rec.hour === 0) {
      rec.hour = 12;
      fmt = fmt.replaceAll("a", "AM");
    } else fmt = fmt.replaceAll("a", "AM");
  }
  var year = rec.year > 0 ? rec.year : rec.year - 1;
  if (fmt.match("c")) {
    if (year < 0) {
      year = Math.abs(year);
      fmt = fmt.replaceAll("c", "BC");
    } else fmt = fmt.replaceAll("c", "AD");
  }

  fmt = fmt.replaceAll("yyyy", year.toString().padStart(4, "0"));
  fmt = fmt.replaceAll("y", year);
  fmt = fmt.replaceAll("eee", rec.yday + 1);
  fmt = fmt.replaceAll("dd", rec.day.toString().padStart(2, "0"));
  fmt = fmt.replaceAll("d", rec.day);
  fmt = fmt.replaceAll("hh", rec.hour.toString().padStart(2, "0"));
  fmt = fmt.replaceAll("h", rec.hour);
  fmt = fmt.replaceAll("nn", rec.minute.toString().padStart(2, "0"));
  fmt = fmt.replaceAll("n", rec.minute);
  fmt = fmt.replaceAll("ss", rec.second.toString().padStart(2, "0"));
  fmt = fmt.replaceAll("s", rec.second);
  fmt = fmt.replaceAll("z", zone >= 0 ? "+" + zone : zone);
  fmt = fmt.replaceAll(/mm[^bB]/g, rec.month + 1);
  fmt = fmt.replaceAll(/m[^bB]/g, rec.month + 1);
  fmt = fmt.replaceAll(/v[^bB]/g, rec.weekday);
  fmt = fmt.replaceAll("mb", this.monthstr[rec.month].slice(0, 3));
  fmt = fmt.replaceAll("mB", this.monthstr[rec.month]);
  fmt = fmt.replaceAll("vB", this.weekdays[rec.weekday]);
  fmt = fmt.replaceAll("vb", this.weekdays[rec.weekday].slice(0, 3));

  return fmt;
};

Object.methods = function (o) {
  var m = [];
  Object.keys(o).forEach(function (k) {
    if (typeof o[k] === "function") m.push(k);
  });
  return m;
};
Object.methods.doc = "Retun an array of all functions an object has access to.";

Object.dig = function (obj, path, def = {}) {
  var pp = path.split(".");
  for (var i = 0; i < pp.length - 1; i++) {
    obj = obj[pp[i]] = obj[pp[i]] || {};
  }
  obj[pp[pp.length - 1]] = def;
  return def;
};

Object.rkeys = function (o) {
  var keys = [];
  Object.keys(o).forEach(function (key) {
    keys.push(key);
    if (Object.isObject(o[key])) keys.push(Object.rkeys(o[key]));
  });
  return keys;
};

Object.readonly = function (o, name, msg) {
  var tmp = {};
  var prop = Object.getOwnPropertyDescriptor(o, name);
  if (!prop) {
    console.error(`Attempted to make property ${name} readonly, but it doesn't exist on ${o}.`);
    return;
  }
  Object.defineProperty(tmp, name, prop);
  prop.get = function () {
    return tmp[name];
  };
  prop.set = function () {
    console.warn(`Attempted to set readonly property ${name}`);
  };
  Object.defineProperty(o, name, prop);
};

Object.mixin = function (target, source) {
  if (typeof source !== "object") return target;
  Object.defineProperties(target, Object.getOwnPropertyDescriptors(source));
  return target;
};

Object.mix = function (...objs) {
  var n = {};
  for (var o of objs) Object.mixin(n, o);

  return n;
};

Object.deepmixin = function (target, source) {
  var o = source;
  while (o !== Object.prototype) {
    Object.mixin(target, o);
    o = o.__proto__;
  }
};

Object.deepfreeze = function (obj) {
  for (var key in obj) {
    if (typeof obj[key] === "object") Object.deepfreeze(obj[key]);
  }
  Object.freeze(obj);
};

/* Goes through each key and overwrites if it's present */
Object.dainty_assign = function (target, source) {
  Object.keys(source).forEach(function (k) {
    if (typeof source[k] === "function") return;
    if (!(k in target)) return;
    if (Array.isArray(source[k])) target[k] = deep_copy(source[k]);
    else if (Object.isObject(source[k])) Object.dainty_assign(target[k], source[k]);
    else target[k] = source[k];
  });
};

Object.isObject = function (o) {
  return o instanceof Object && !(o instanceof Array);
};

Object.setter_assign = function (target, source) {
  for (var key in target) if (Object.isAccessor(target, key) && typeof source[key] !== "undefined") target[key] = source[key];
};

Object.containingKey = function (obj, prop) {
  if (typeof obj !== "object") return undefined;
  if (!(prop in obj)) return undefined;

  var o = obj;
  while (o.__proto__ && !Object.hasOwn(o, prop)) o = o.__proto__;

  return o;
};

Object.access = function (obj, name) {
  var dig = name.split(".");

  for (var i of dig) {
    obj = obj[i];
    if (!obj) return undefined;
  }

  return obj;
};

Object.isAccessor = function (obj, prop) {
  var o = Object.containingKey(obj, prop);
  if (!o) return false;

  var desc = Object.getOwnPropertyDescriptor(o, prop);
  if (!desc) return false;
  if (desc.get || desc.set) return true;
  return false;
};

Object.mergekey = function (o1, o2, k) {
  if (!o2) return;
  if (typeof o2[k] === "object") {
    if (Array.isArray(o2[k])) o1[k] = deep_copy(o2[k]);
    else {
      if (!o1[k]) o1[k] = {};
      if (typeof o1[k] === "object") Object.merge(o1[k], o2[k]);
      else o1[k] = o2[k];
    }
  } else o1[k] = o2[k];
};

/* Same as merge from Ruby */
/* Adds objs key by key to target */
Object.merge = function (target, ...objs) {
  for (var obj of objs) for (var key of Object.keys(obj)) Object.mergekey(target, obj, key);

  return target;
};

Object.totalmerge = function (target, ...objs) {
  for (var obj of objs) for (var key in obj) Object.mergekey(target, obj, key);

  return target;
};

/* Returns a new object with undefined, null, and empty values removed. */
Object.compact = function (obj) {};

Object.totalassign = function (to, from) {
  for (var key in from) to[key] = from[key];
};

/* Prototypes out an object and assigns values */
Object.copy = function (proto, ...objs) {
  var c = Object.create(proto);
  for (var obj of objs) Object.mixin(c, obj);
  return c;
};

/* OBJECT DEFININTIONS */
Object.defHidden = function (obj, prop) {
  Object.defineProperty(obj, prop, { enumerable: false, writable: true });
};

Object.hide = function (obj, ...props) {
  for (var prop of props) {
    var p = Object.getOwnPropertyDescriptor(obj, prop);
    if (!p) continue;
    p.enumerable = false;
    Object.defineProperty(obj, prop, p);
  }
};

Object.enumerable = function (obj, val, ...props) {
  for (var prop of props) {
    p = Object.getOwnPropertyDescriptor(obj, prop);
    if (!p) continue;
    p.enumerable = val;
    Object.defineProperty(obj, prop, p);
  }
};

Object.unhide = function (obj, ...props) {
  for (var prop of props) {
    var p = Object.getOwnPropertyDescriptor(obj, prop);
    if (!p) continue;
    p.enumerable = true;
    Object.defineProperty(obj, prop, p);
  }
};

Object.defineProperty(Object.prototype, "obscure", {
  value: function (name) {
    Object.defineProperty(this, name, { enumerable: false });
  },
});

Object.defineProperty(Object.prototype, "mixin", {
  value: function (obj) {
    if (typeof obj === "string") obj = use(obj);

    if (obj) Object.mixin(this, obj);
  },
});

Object.defineProperty(Object.prototype, "hasOwn", {
  value: function (x) {
    return this.hasOwnProperty(x);
  },
});

Object.defineProperty(Object.prototype, "defn", {
  value: function (name, val) {
    Object.defineProperty(this, name, {
      value: val,
      writable: true,
      configurable: true,
    });
  },
});

Object.defineProperty(Object.prototype, "nulldef", {
  value: function (name, val) {
    if (!this.hasOwnProperty(name)) this[name] = val;
  },
});

Object.defineProperty(Object.prototype, "prop_obj", {
  value: function () {
    return JSON.parse(JSON.stringify(this));
  },
});

/* defc 'define constant'. Defines a value that is not writable. */
Object.defineProperty(Object.prototype, "defc", {
  value: function (name, val) {
    Object.defineProperty(this, name, {
      value: val,
      writable: false,
      enumerable: true,
      configurable: false,
    });
  },
});

Object.defineProperty(Object.prototype, "stick", {
  value: function (prop) {
    Object.defineProperty(this, prop, { writable: false });
  },
});

Object.defineProperty(Object.prototype, "harden", {
  value: function (prop) {
    Object.defineProperty(this, prop, {
      writable: false,
      configurable: false,
      enumerable: false,
    });
  },
});

Object.defineProperty(Object.prototype, "deflock", {
  value: function (prop) {
    Object.defineProperty(this, prop, { configurable: false });
  },
});

Object.defineProperty(Object.prototype, "forEach", {
  value: function (fn) {
    Object.values(this).forEach(fn);
  },
});

Object.empty = function (obj) {
  return Object.keys(obj).length === 0;
};

Object.defineProperty(Object.prototype, "nth", {
  value: function (x) {
    if (this.empty || x >= Object.keys(this).length) return null;

    return this[Object.keys(this)[x]];
  },
});

Object.defineProperty(Object.prototype, "filter", {
  value: function (fn) {
    return Object.values(this).filter(fn);
  },
});

Object.defineProperty(Object.prototype, "push", {
  value: function (val) {
    var str = val.toString();
    str = str.replaceAll(".", "_");
    var n = 1;
    var t = str;
    while (Object.hasOwn(this, t)) {
      t = str + n;
      n++;
    }
    this[t] = val;
    return t;
  },
});

/* STRING DEFS */

Object.defineProperty(String.prototype, "next", {
  value: function (char, from) {
    if (!Array.isArray(char)) char = [char];
    if (from > this.length - 1) return -1;
    else if (!from) from = 0;

    var find = this.slice(from).search(char[0]);

    if (find === -1) return -1;
    else return from + find;

    var i = 0;
    var c = this.charAt(from + i);
    while (!char.includes(c)) {
      i++;
      if (from + i > this.length - 1) return -1;
      c = this.charAt(from + i);
    }

    return from + i;
  },
});

Object.defineProperty(String.prototype, "prev", {
  value: function (char, from, count) {
    if (from > this.length - 1) return -1;
    else if (!from) from = this.length - 1;

    if (!count) count = 0;

    var find = this.slice(0, from).lastIndexOf(char);

    while (count > 1) {
      find = this.slice(0, find).lastIndexOf(char);
      count--;
    }

    if (find === -1) return 0;
    else return find;
  },
});

Object.defineProperty(String.prototype, "shift", {
  value: function (n) {
    if (n === 0) return this.slice();

    if (n > 0) return this.slice(n);

    if (n < 0) return this.slice(0, this.length + n);
  },
});

Object.defineProperty(String.prototype, "strip_ext", {
  value: function () {
    return this.tolast(".");
  },
});

Object.defineProperty(String.prototype, "ext", {
  value: function () {
    return this.fromlast(".");
  },
});

Object.defineProperty(String.prototype, "set_ext", {
  value: function (val) {
    return this.strip_ext() + val;
  },
});

Object.defineProperty(String.prototype, "folder_same_name", {
  value: function () {
    var dirs = this.dir().split("/");
    return dirs.last() === this.name();
  },
});

Object.defineProperty(String.prototype, "up_path", {
  value: function () {
    var base = this.base();
    var dirs = this.dir().split("/");
    dirs.pop();
    return dirs.join("/") + base;
  },
});

Object.defineProperty(String.prototype, "fromlast", {
  value: function (val) {
    var idx = this.lastIndexOf(val);
    if (idx === -1) return "";
    return this.slice(idx + 1);
  },
});

Object.defineProperty(String.prototype, "tofirst", {
  value: function (val) {
    var idx = this.indexOf(val);
    if (idx === -1) return this.slice();
    return this.slice(0, idx);
  },
});

Object.defineProperty(String.prototype, "fromfirst", {
  value: function (val) {
    var idx = this.indexOf(val);
    if (idx === -1) return this;
    return this.slice(idx + val.length);
  },
});

Object.defineProperty(String.prototype, "name", {
  value: function () {
    var idx = this.indexOf("/");
    if (idx === -1) return this.tolast(".");
    return this.fromlast("/").tolast(".");
  },
});

Object.defineProperty(String.prototype, "set_name", {
  value: function (name) {
    var dir = this.dir();
    return this.dir() + "/" + name + "." + this.ext();
  },
});

Object.defineProperty(String.prototype, "base", {
  value: function () {
    return this.fromlast("/");
  },
});

Object.defineProperty(String.prototype, "splice", {
  value: function (index, str) {
    return this.slice(0, index) + str + this.slice(index);
  },
});

Object.defineProperty(String.prototype, "sub", {
  value: function (index, str) {
    return this.slice(0, index) + str + this.slice(index + str.length);
  },
});

Object.defineProperty(String.prototype, "updir", {
  value: function () {
    if (this.lastIndexOf("/") === this.length - 1) return this.slice(0, this.length - 1);

    var dir = (this + "/").dir();
    return dir.dir();
  },
});

Object.defineProperty(String.prototype, "trimchr", {
  value: function (chars) {
    return vector.trimchr(this, chars);
  },
});

Object.defineProperty(String.prototype, "uc", {
  value: function () {
    return this.toUpperCase();
  },
});
Object.defineProperty(String.prototype, "lc", {
  value: function () {
    return this.toLowerCase();
  },
});

/* ARRAY DEFS */
Object.defineProperty(Array.prototype, "copy", {
  value: function () {
    var c = [];

    this.forEach(function (x, i) {
      c[i] = deep_copy(x);
    });

    return c;
  },
});

Object.defineProperty(Array.prototype, "forFrom", {
  value: function (n, fn) {
    for (var i = n; i < this.length; i++) fn(this[i]);
  },
});

Object.defineProperty(Array.prototype, "forTo", {
  value: function (n, fn) {
    for (var i = 0; i < n; i++) fn(this[i]);
  },
});

Object.defineProperty(Array.prototype, "dofilter", {
  value: function (fn) {
    for (let i = 0; i < this.length; i++) {
      if (!fn.call(this, this[i], i, this)) {
        this.splice(i, 1);
        i--;
      }
    }
    return this;
  },
});

Object.defineProperty(Array.prototype, "reversed", {
  value: function () {
    var c = this.slice();
    return c.reverse();
  },
});

Object.defineProperty(Array.prototype, "rotate", {
  value: function (a) {
    return Vector.rotate(this, a);
  },
});

function make_swizz() {
  function setelem(n) {
    return {
      get: function () {
        return this[n];
      },
      set: function (x) {
        this[n] = x;
      },
    };
  }

  function arrsetelem(str, n) {
    Object.defineProperty(Array.prototype, str, setelem(n));
  }

  var arr_elems = ["x", "y", "z", "w"];
  var quat_elems = ["i", "j", "k"];
  var color_elems = ["r", "g", "b", "a"];

  arr_elems.forEach(function (x, i) {
    arrsetelem(x, i);
  });
  quat_elems.forEach(function (x, i) {
    arrsetelem(x, i);
  });
  color_elems.forEach(function (x, i) {
    arrsetelem(x, i);
  });

  var nums = [0, 1, 2, 3];

  var swizz = [];

  for (var i of nums) for (var j of nums) swizz.push([i, j]);

  swizz.forEach(function (x) {
    var str = "";
    for (var i of x) str += arr_elems[i];

    Object.defineProperty(Array.prototype, str, {
      get() {
        return [this[x[0]], this[x[1]]];
      },
      set(j) {
        this[x[0]] = j[0];
        this[x[1]] = j[1];
      },
    });

    str = "";
    for (var i of x) str += color_elems[i];
    Object.defineProperty(Array.prototype, str, {
      get() {
        return [this[x[0]], this[x[1]]];
      },
      set(j) {
        this[x[0]] = j[0];
        this[x[1]] = j[1];
      },
    });
  });

  swizz = [];
  for (var i of nums) for (var j of nums) for (var k of nums) swizz.push([i, j, k]);

  swizz.forEach(function (x) {
    var str = "";
    for (var i of x) str += arr_elems[i];

    Object.defineProperty(Array.prototype, str, {
      get() {
        return [this[x[0]], this[x[1]], this[x[2]]];
      },
      set(j) {
        this[x[0]] = j[0];
        this[x[1]] = j[1];
        this[x[2]] = j[2];
      },
    });

    str = "";
    for (var i of x) str += color_elems[i];
    Object.defineProperty(Array.prototype, str, {
      get() {
        return [this[x[0]], this[x[1]], this[x[2]]];
      },
      set(j) {
        this[x[0]] = j[0];
        this[x[1]] = j[1];
        this[x[2]] = j[2];
      },
    });
  });

  swizz = [];
  for (var i of nums) for (var j of nums) for (var k of nums) for (var w of nums) swizz.push([i, j, k, w]);

  swizz.forEach(function (x) {
    var str = "";
    for (var i of x) str += arr_elems[i];

    Object.defineProperty(Array.prototype, str, {
      get() {
        return [this[x[0]], this[x[1]], this[x[2]], this[x[3]]];
      },
      set(j) {
        this[x[0]] = j[0];
        this[x[1]] = j[1];
        this[x[2]] = j[2];
        this[x[3]] = j[3];
      },
    });

    str = "";
    for (var i of x) str += color_elems[i];
    Object.defineProperty(Array.prototype, str, {
      get() {
        return [this[x[0]], this[x[1]], this[x[2]], this[x[3]]];
      },
      set(j) {
        this[x[0]] = j[0];
        this[x[1]] = j[1];
        this[x[2]] = j[2];
        this[x[3]] = j[3];
      },
    });
  });
}
make_swizz();

Object.defineProperty(Array.prototype, "normalized", {
  value: function () {
    return vector.norm(this);
  },
});

Object.defineProperty(Array.prototype, "newfirst", {
  value: function (i) {
    var c = this.slice();
    if (i >= c.length) return c;

    do {
      c.push(c.shift());
      i--;
    } while (i > 0);

    return c;
  },
});

Object.defineProperty(Array.prototype, "doubleup", {
  value: function (n) {
    var c = [];
    this.forEach(function (x) {
      for (var i = 0; i < n; i++) c.push(x);
    });

    return c;
  },
});

Object.defineProperty(Array.prototype, "mult", {
  value: function (arr) {
    var c = [];
    for (var i = 0; i < this.length; i++) {
      c[i] = this[i] * arr[i];
    }
    return c;
  },
});

Object.defineProperty(Array.prototype, "apply", {
  value: function (fn) {
    this.forEach(function (x) {
      x[fn].apply(x);
    });
  },
});

Object.defineProperty(Array.prototype, "sorted", {
  value: function () {
    return this.toSorted();
  },
});

Object.defineProperty(Array.prototype, "equal", {
  value: function (b) {
    if (this.length !== b.length) return false;
    if (b == null) return false;
    if (this === b) return true;

    return JSON.stringify(this.sorted()) === JSON.stringify(b.sorted());

    for (var i = 0; i < this.length; i++) {
      if (!this[i] === b[i]) return false;
    }

    return true;
  },
});

Object.defineProperty(Array.prototype, "mapc", {
  value: function (fn) {
    return this.map(x => fn(x));
  },
});

Object.defineProperty(Array.prototype, "mapvec", {
  value: function (fn, b) {
    return this.map((x, i) => fn(x, b[i]));
  },
});

Object.defineProperty(Array.prototype, "remove", {
  value: function (b) {
    var idx = this.indexOf(b);

    if (idx === -1) return false;

    this.splice(idx, 1);

    return true;
  },
});

Object.defineProperty(Array.prototype, "set", {
  value: function (b) {
    if (this.length !== b.length) return;

    b.forEach(function (val, i) {
      this[i] = val;
    }, this);
  },
});

Object.defineProperty(Array.prototype, "flat", {
  value: function () {
    return [].concat.apply([], this);
  },
});

/* Return true if array contains x */
Object.defineProperty(Array.prototype, "empty", {
  get: function () {
    return this.length === 0;
  },
});

Object.defineProperty(Array.prototype, "push_unique", {
  value: function (x) {
    var inc = !this.includes(x);
    if (inc) this.push(x);
    return inc;
  },
});

Object.defineProperty(Array.prototype, "unique", {
  value: function () {
    var c = [];
    this.forEach(function (x) {
      c.push_unique(x);
    });
    return c;
  },
});

Object.defineProperty(Array.prototype, "unduped", {
  value: function () {
    return [...new Set(this)];
  },
});

Object.defineProperty(Array.prototype, "findIndex", {
  value: function (fn) {
    var idx = -1;
    this.every(function (x, i) {
      if (fn(x)) {
        idx = i;
        return false;
      }

      return true;
    });

    return idx;
  },
});

Object.defineProperty(Array.prototype, "find", {
  value: function (fn) {
    var ret;

    this.every(function (x) {
      if (fn(x)) {
        ret = x;
        return false;
      }

      return true;
    });

    return ret;
  },
});

Object.defineProperty(Array.prototype, "search", {
  value: function (val) {
    for (var i = 0; i < this.length; i++) if (this[i] === val) return i;

    return undefined;
  },
});

Object.defineProperty(Array.prototype, "last", {
  value: function () {
    return this[this.length - 1];
  },
});

Object.defineProperty(Array.prototype, "at", {
  value: function (x) {
    return x < 0 ? this[this.length + x] : this[x];
  },
});

Object.defineProperty(Array.prototype, "wrapped", {
  value: function (x) {
    var c = this.slice(0, this.length);

    for (var i = 0; i < x; i++) c.push(this[i]);

    return c;
  },
});

Object.defineProperty(Array.prototype, "wrap_idx", {
  value: function (x) {
    while (x >= this.length) {
      x -= this.length;
    }

    return x;
  },
});

Object.defineProperty(Array.prototype, "mirrored", {
  value: function (x) {
    var c = this.slice(0);
    if (c.length <= 1) return c;
    for (var i = c.length - 2; i >= 0; i--) c.push(c[i]);

    return c;
  },
});

Math.lerp = vector.lerp;
Math.gcd = vector.gcd;
Math.lcm = vector.lcm;
Math.sum = vector.sum;
Math.mean = vector.mean;
Math.sigma = vector.sigma;
Math.median = vector.median;

vector.v2one = [1,1];
vector.v3one = [1,1,1];
vector.v2zero = [0,0];
vector.v3zero = [0,0,0];

Math.variance = function (series) {
  var mean = Math.mean(series);
  var vnce = 0;
  for (var i = 0; i < series.length; i++) vnce += Math.pow(series[i] - mean, 2);
  return vnce / series.length;
};

Math.ci = function (series) {
  return (3 * Math.sigma(series)) / Math.sqrt(series.length);
};

Math.grab_from_points = function (pos, points, slop) {
  var shortest = slop;
  var idx = -1;
  points.forEach(function (x, i) {
    if (Vector.length(pos.sub(x)) < shortest) {
      shortest = Vector.length(pos.sub(x));
      idx = i;
    }
  });
  return idx;
};

Math.nearest = function (n, incr) {
  return Math.round(n / incr) * incr;
};

Math.places = function (n, digits) {
  var div = Math.pow(10, digits);
  return Math.round(n * div) / div;
};

Number.hex = function (n) {
  var s = Math.floor(n).toString(16);
  if (s.length === 1) s = "0" + s;
  return s.uc();
};

Object.defineProperty(Object.prototype, "lerp", {
  value: function (to, t) {
    var self = this;
    var obj = {};

    Object.keys(self).forEach(function (key) {
      obj[key] = self[key].lerp(to[key], t);
    });

    return obj;
  },
});

/* MATH EXTENSIONS */
Object.defineProperty(Number.prototype, "lerp", {
  value: function (to, t) {
    return Math.lerp(this, to, t);
  },
});

Object.defineProperty(Number.prototype, "clamp", {
  value: function (from, to) {
    return Math.clamp(this, from, to);
  },
});

Math.clamp = vector.clamp;

Math.random_range = vector.random_range;
Math.rand_int = function (max = 9007199254740991) {
  return Math.floor(Math.random() * max);
};

Math.snap = function (val, grid) {
  if (!grid || grid === 1) return Math.round(val);

  var rem = val % grid;
  var d = val - rem;
  var i = Math.round(rem / grid) * grid;
  return d + i;
};

Math.angledist = vector.angledist;
Math.angledist.doc = "Find the shortest angle between two angles.";
Math.TAU = Math.PI * 2;
Math.deg2rad = function (deg) {
  return deg * 0.0174533;
};
Math.rad2deg = function (rad) {
  return rad / 0.0174533;
};
Math.turn2rad = function (x) {
  return x * Math.TAU;
};
Math.rad2turn = function (x) {
  return x / Math.TAU;
};
Math.turn2deg = function (x) {
  return x * 360;
};
Math.deg2turn = function (x) {
  return x / 360;
};
Math.randomint = function (max) {
  return Math.clamp(Math.floor(Math.random() * max), 0, max - 1);
};
Math.variate = vector.variate;

/* BOUNDINGBOXES */
var bbox = {};

bbox.overlap = function (box1, box2) {
  return box1.l > box2.l && box1.r < box2.r && box1.t < box2.t && box1.b > box2.b;
  return box1.l > box2.r || box1.r < box2.l || box1.t > box2.b || box1.b < box2.t;
};

bbox.fromcwh = function (c, wh) {
  return {
    t: c.y + wh.y / 2,
    b: c.y - wh.y / 2,
    l: c.x - wh.x / 2,
    r: c.x + wh.x / 2,
  };
};

bbox.frompoints = function (points) {
  var b = { t: 0, b: 0, l: 0, r: 0 };

  points.forEach(function (x) {
    if (x.y > b.t) b.t = x.y;
    if (x.y < b.b) b.b = x.y;
    if (x.x > b.r) b.r = x.x;
    if (x.x < b.l) b.l = x.x;
  });

  return b;
};

bbox.topoints = function (bb) {
  return [
    [bb.l, bb.t],
    [bb.r, bb.t],
    [bb.r, bb.b],
    [bb.l, bb.b],
  ];
};

bbox.tocwh = function (bb) {
  if (!bb) return undefined;
  var cwh = {};

  var w = bb.r - bb.l;
  var h = bb.t - bb.b;
  cwh.wh = [w, h];
  cwh.c = [bb.l + w / 2, bb.b + h / 2];

  return cwh;
};

bbox.towh = function (bb) {
  return [bb.r - bb.l, bb.t - bb.b];
};

bbox.pointin = function (bb, p) {
  if (bb.t < p.y || bb.b > p.y || bb.l > p.x || bb.r < p.x) return false;

  return true;
};

bbox.zero = function (bb) {
  var newbb = Object.assign({}, bb);
  newbb.r -= newbb.l;
  newbb.t -= newbb.b;
  newbb.b = 0;
  newbb.l = 0;
  return newbb;
};

bbox.move = function (bb, pos) {
  var newbb = Object.assign({}, bb);
  newbb.t += pos.y;
  newbb.b += pos.y;
  newbb.l += pos.x;
  newbb.r += pos.x;
  return newbb;
};

bbox.moveto = function (bb, pos) {
  bb = bbox.zero(bb);
  return bbox.move(bb, pos);
};

bbox.expand = function (oldbb, x) {
  if (!oldbb || !x) return;
  var bb = {};
  Object.assign(bb, oldbb);

  if (bb.t < x.t) bb.t = x.t;
  if (bb.r < x.r) bb.r = x.r;
  if (bb.b > x.b) bb.b = x.b;
  if (bb.l > x.l) bb.l = x.l;

  return bb;
};

bbox.blwh = function (bl, wh) {
  return {
    b: bl.y,
    l: bl.x,
    r: bl.x + wh.x,
    t: bl.y + wh.y,
  };
};

bbox.blwh.doc = "Bounding box from (bottom left, width height)";

bbox.fromobjs = function (objs) {
  var bb = objs[0].boundingbox;
  objs.forEach(function (obj) {
    bb = bbox.expand(bb, obj.boundingbox);
  });
  return bb;
};

/* VECTORS */
var Vector = {};
Vector.length = vector.length;
Vector.norm = vector.norm;
Vector.project = vector.project;
Vector.dot = vector.dot;
Vector.random = function () {
  var vec = [Math.random() - 0.5, Math.random() - 0.5];
  return Vector.norm(vec);
};

Vector.angle_between = vector.angle_between;
Vector.rotate = vector.rotate;

vector.direction = function (from, to) {
  return vector.norm(to.sub(from));
};

Vector.equal = function (v1, v2, tol) {
  if (!tol) return v1.equal(v2);

  var eql = true;
  var c = v1.sub(v2);

  c.forEach(function (x) {
    if (!eql) return;
    if (Math.abs(x) > tol) eql = false;
  });

  return eql;
};

Vector.reflect = function (vec, plane) {
  var p = Vector.norm(plane);
  return vec.sub(p.scale(2 * Vector.dot(vec, p)));
};

Vector.reflect_point = function (vec, point) {
  return point.add(vec.sub(point).scale(-1));
};

/* POINT ASSISTANCE */

function points2cm(points) {
  var x = 0;
  var y = 0;
  var n = points.length;
  points.forEach(function (p) {
    x = x + p[0];
    y = y + p[1];
  });

  return [x / n, y / n];
}

Math.sortpointsccw = function (points) {
  var cm = points2cm(points);
  var cmpoints = points.map(function (x) {
    return x.sub(cm);
  });
  var ccw = cmpoints.sort(function (a, b) {
    var aatan = Math.atan2(a.y, a.x);
    var batan = Math.atan2(b.y, b.x);
    return aatan - batan;
  });

  return ccw.map(function (x) {
    return x.add(cm);
  });
};

var yaml = {};
yaml.tojson = function (yaml) {
  // Replace key value pairs that are strings with quotation marks around them
  yaml = yaml.replace(/(\w+):/g, '"$1":');
  yaml = yaml.replace(/: ([\w\.\/]+)/g, ': "$1"'); // TODO: make this more general

  yaml = yaml.split("\n");
  var cont = {};
  var cur = 0;
  for (var i = 0; i < yaml.length; i++) {
    var line = yaml[i];
    var indent = line.search(/\S/);

    if (indent > cur) {
      if (line[indent] == "-") {
        cont[indent] = "array";
        yaml[i] = line.sub(indent, "[");
      } else {
        cont[indent] = "obj";
        yaml[i] = line.sub(indent - 1, "{");
      }
    }

    if (indent < cur) {
      while (cur > indent) {
        if (cont[cur] === "obj") yaml[i - 1] = yaml[i - 1] + "}";
        else if (cont[cur] === "array") yaml[i - 1] = yaml[i - 1] + "]";

        delete cont[cur];
        cur--;
      }
    }

    if (indent === cur) {
      if (yaml[i][indent] === "-") yaml[i] = yaml[i].sub(indent, ",");
      else yaml[i - 1] = yaml[i - 1] + ",";
    }

    cur = indent;
  }
  yaml = "{" + yaml.join("\n") + "}";
  yaml = yaml.replace(/\s/g, "");
  yaml = yaml.replace(/,}/g, "}");
  yaml = yaml.replace(/,]/g, "]");
  yaml = yaml.replace(/,"[^"]+"\:,/g, ",");
  return yaml;
};

Math.sign = function (n) {
  return n >= 0 ? 1 : -1;
};

var lodash = {};
lodash.get = function (obj, path, defValue) {
  if (!path) return undefined;
  // Check if path is string or array. Regex : ensure that we do not have '.' and brackets.
  var pathArray = Array.isArray(path) ? path : path.match(/([^[.\]])+/g);
  var result = pathArray.reduce((prevObj, key) => prevObj && prevObj[key], obj);
  return result === undefined ? defValue : result;
};

return {
  convert,
  time,
  Vector,
  bbox,
  yaml,
  lodash,
};
