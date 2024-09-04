"use math";

os.mem_limit.doc = "Set the memory limit of the runtime in bytes.";
os.gc_threshold.doc = "Set the threshold before a GC pass is triggered in bytes. This is set to malloc_size + malloc_size>>1 after a GC pass.";
os.max_stacksize.doc = "Set the max stack size in bytes.";

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

Resources.rm_fn = function rm_fn(fn, text)
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

Resources.replpath = function replpath(str, path) {
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

Resources.replstrs = function replstrs(path) {
  if (!path) return;
  var script = io.slurp(path);
  var regexp = /"[^"\s]*?\.[^"\s]+?"/g;

  var stem = path.dir();

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

Resources.is_sound = function(path) {
  var ext = path.ext();
  return Resources.sounds.any(x => x === ext);
}

Resources.is_animation = function(path)
{
  if (path.ext() === 'gif' && Resources.gif.frames(path) > 1) return true;
  if (path.ext() === 'ase') return true;
  
  return false;
}

Resources.is_path = function(str)
{
  return !/[\\\/:*?"<>|]/.test(str);
}

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
  return Resources.images.some(x => x === ext);
};

function find_ext(file, ext, root = "") {
  if (!file) return;

  var file_ext = file.ext();
  if (ext.some(x => x === file_ext)) return file;
  for (var e of ext) {
    var nf = `${file}.${e}`;
    if (io.exists(nf))
      return nf;
  }

  var all_files = io.glob(`**/${file}.*`);
  var find = undefined;
  for (var e of ext) {
    var finds = all_files.filter(x => x.ext() === e);
    if (finds.length > 1)
      console.warn(`Found conflicting files when searching for '${file}': ${json.encode(finds)}. Returning the first one.`);
    if (finds.length > 0) {
      find = finds[0];
      break;
    }
  }

  return find;
}

var hashhit = 0;
var hashmiss = 0;

Object.defineProperty(Function.prototype, 'hashify', {
  value: function() {
    var hash = new Map();
    var fn = this;
    function ret() {
      if (!hash.has(arguments[0]))
        hash.set(arguments[0], fn(...arguments));

      return hash.get(arguments[0]);
    }
    return ret;
  }
});

Resources.find_image = function (file, root = "") {
  return find_ext(file, Resources.images, root);
}.hashify();

Resources.find_sound = function (file, root = "") {
  return find_ext(file, Resources.sounds, root);
}.hashify();

Resources.find_script = function (file, root = "") {
  return find_ext(file, Resources.scripts, root);
}.hashify();

console.transcript = "";
console.say = function (msg) {
  msg += "\n";
  console.print(msg);
  if (debug.termout) console.term_print(msg);
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
  var stack = console.stackstr(skip+1);
  console.log(stack);
  return stack;
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

globalThis.use = function use(file) {
  file = Resources.find_script(file);
  profile.cache("USE", file);

  if (use_cache[file]) {
    var ret = use_cache[file]();
    profile.endcache(" [cached]");
    return ret;
  }
  var script = Resources.replstrs(file);
  script = `(function() { var self = this; ${script}; })`;
  var fn = os.eval(file, script);
  use_cache[file] = fn;
  var ret = fn();
  profile.endcache();

  return ret;
}

function stripped_use (file, script) {
  file = Resources.find_script(file);

  if (use_cache[file]) {
    var ret = use_cache[file]();
    return ret;
  }
  script ??= Resources.replstrs(file);

  script = `(function() { var self = this; ${script}; })`;
  var fn = os.eval(file, script);
  var ret = fn();
  profile.endcache();
  
  return ret;
}

function bare_use(file)
{
  var script = io.slurp(file);
  if (!script) return;
  script = `(function() { var self = this; ${script}; })`;
  Object.assign(globalThis, os.eval(file, script)());
}

globalThis.debug = {};

profile.enabled = true;
console.enabled = true;
debug.enabled = true;

bare_use("scripts/base.js");
bare_use("scripts/profile.js");

prosperon.release = function()
{
  profile.enabled = false;
  console.enabled = false;
  debug.enabled = false;
}

bare_use("preconfig.js");

if (!profile.enabled)
  use = stripped_use;

Object.assign(globalThis, use("scripts/prosperon.js"));
