"use math";

Object.defineProperty(String.prototype, 'rm', {
  value: function(index, endidx = index+1) { return this.slice(0,index) + this.slice(endidx); }
});

Object.defineProperty(String.prototype, "tolast", {
  value: function (val) {
    var idx = this.lastIndexOf(val);
    if (idx === -1) return this.slice();
    return this.slice(0, idx);
  },
});

Object.defineProperty(String.prototype, "dir", {
  value: function () {
    if (!this.includes("/")) return "";
    return this.tolast("/");
  },
});

Object.defineProperty(String.prototype, "folder", {
  value: function () {
    var dir = this.dir();
    if (!dir) return "";
    else return dir + "/";
  },
});

globalThis.Resources = {};

Resources.rm_fn = function(fn, text)
{
  var reg = new RegExp(fn.source + "\\s*\\(");
  var match;
  while (match = text.match(reg)) {
    var last = match.index+match[0].length;
    var par = 1;
    while (par !== 0) {
      if (text[last] === '(') par++;
      if (text[last] === ')') par--;
      last++;
    }
    text = text.rm(match.index, last);
  }

  return text;
}
Resources.rm_fn.doc = "Remove calls to a given function from a given text script.";

Resources.replpath = function (str, path) {
  if (!str) return str;
  if (str[0] === "/") return str.rm(0);

  if (str[0] === "@") return os.prefpath() + "/" + str.rm(0);

  if (!path) return str;

  var stem = path.dir();
  while (stem) {
    var tr = stem + "/" + str;
    if (io.exists(tr)) return tr;
    stem = stem.updir();
  }

  return str;
};

Resources.replstrs = function (path) {
  if (!path) return;
  var script = io.slurp(path);
  var regexp = /"[^"\s]*?\.[^"\s]+?"/g;
  var stem = path.dir();
  
  // remove console statements
  if (!console.enabled)
    script = Resources.rm_fn(/console\.(spam|info|warn|error)/, script);
  
  if (!profile.enabled)
    script = Resources.rm_fn(/profile\.(cache|frame|endcache|endframe)/, script);
  
  if (!debug.enabled) {
    script = Resources.rm_fn(/assert/, script);
    script = Resources.rm_fn(/debug\.(build|fn_break)/, script);
  }
    
  script = script.replace(regexp, function (str) {
    var newstr = Resources.replpath(str.trimchr('"'), path);
    return `"${newstr}"`;
  });

  return script;
};

globalThis.json = {};
json.encode = function (value, replacer, space = 1) {
  return JSON.stringify(value, replacer, space);
};

json.decode = function (text, reviver) {
  if (!text) return undefined;
  return JSON.parse(text, reviver);
};

json.readout = function (obj) {
  var j = {};
  for (var k in obj)
    if (typeof obj[k] === "function") j[k] = "function " + obj[k].toString();
    else j[k] = obj[k];

  return json.encode(j);
};

json.doc = {
  doc: "json implementation.",
  encode: "Encode a value to json.",
  decode: "Decode a json string to a value.",
  readout: "Encode an object fully, including function definitions.",
};

Resources.scripts = ["jsoc", "jsc", "jso", "js"];
Resources.images = ["png", "gif", "jpg", "jpeg"];
Resources.sounds = ["wav", "flac", "mp3", "qoa"];
Resources.is_image = function (path) {
  var ext = path.ext();
  return Resources.images.any((x) => x === ext);
};

function find_ext(file, ext) {
  if (io.exists(file)) return file;
  for (var e of ext) {
    var nf = `${file}.${e}`;
    if (io.exists(nf)) return nf;
  }
  return;
}

Resources.find_image = function (file) {
  return find_ext(file, Resources.images);
};
Resources.find_sound = function (file) {
  return find_ext(file, Resources.sounds);
};
Resources.find_script = function (file) {
  return find_ext(file, Resources.scripts);
};

console.transcript = "";
console.say = function (msg) {
  msg += "\n";
  console.print(msg);
  console.transcript += msg;
};
console.log = console.say;
globalThis.say = console.say;
globalThis.print = console.print;

console.pprint = function (msg, lvl = 0) {
  if (typeof msg === "object") msg = JSON.stringify(msg, null, 2);

  var file = "nofile";
  var line = 0;
  console.rec(0, msg, file, line);

  var caller = new Error().stack.split("\n")[2];
  if (caller) {
    var md = caller.match(/\((.*)\:/);
    var m = md ? md[1] : "SCRIPT";
    if (m) file = m;
    md = caller.match(/\:(\d*)\)/);
    m = md ? md[1] : 0;
    if (m) line = m;
  }

  console.rec(lvl, msg, file, line);
};

console.spam = function (msg) {
  console.pprint(msg, 0);
};
console.debug = function (msg) {
  console.pprint(msg, 1);
};
console.info = function (msg) {
  console.pprint(msg, 2);
};
console.warn = function (msg) {
  console.pprint(msg, 3);
};
console.error = function (msg) {
  console.pprint(msg + "\n" + console.stackstr(2), 4);
};
console.panic = function (msg) {
  console.pprint(msg + "\n" + console.stackstr(2), 5);
};
console.stackstr = function (skip = 0) {
  var err = new Error();
  var stack = err.stack.split("\n");
  return stack.slice(skip, stack.length).join("\n");
};

console.stack = function (skip = 0) {
  console.log(console.stackstr(skip + 1));
};

console.stdout_lvl = 1;
console.trace = console.stack;

console.doc = {
  level: "Set level to output logging to console.",
  info: "Output info level message.",
  warn: "Output warn level message.",
  error: "Output error level message, and print stacktrace.",
  critical: "Output critical level message, and exit game immediately.",
  write: "Write raw text to console.",
  say: "Write raw text to console, plus a newline.",
  stack: "Output a stacktrace to console.",
  console: "Output directly to in game console.",
  clear: "Clear console.",
};

globalThis.global = globalThis;

var use_cache = {};

globalThis.use = function(file, env = {}, script) {
  file = Resources.find_script(file);
  profile.cache("USE", file);

  if (use_cache[file]) {
    var ret = use_cache[file].call(env);
    return;
  }
  script ??= Resources.replstrs(file);

  script = `(function() { var self = this; ${script}; })`;
  var fn = os.eval(file, script);
  use_cache[file] = fn;
  var ret = fn.call(env);
  profile.endcache();
  return ret;
}

function stripped_use (file, env = {}, script) {
  file = Resources.find_script(file);

  if (use_cache[file]) {
    var ret = use_cache[file].call(env);
    return;
  }
  script ??= Resources.replstrs(file);

  script = `(function() { var self = this; ${script}; })`;
  var fn = os.eval(file, script);
  var ret = fn.call(env);
  profile.endcache();
  return ret;
}

function bare_use(file)
{
  var script = io.slurp(file);
  if (!script) return;
  script = `(function() { var self = this; ${script}; })`;
  Object.assign(globalThis, os.eval(file, script).call(globalThis));
}

globalThis.debug = {};

profile.enabled = true;
console.enabled = true;
debug.enabled = true;

bare_use("scripts/base.js");
bare_use("scripts/profile.js");

bare_use("preconfig.js");

if (!profile.enabled)
  use = stripped_use;

Object.assign(globalThis, use("scripts/prosperon.js"));
