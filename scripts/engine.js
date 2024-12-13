Object.defineProperty(Object.prototype, "object_id", {
  value: function() {
    return os.value_id(this);
  }
});

os.mem_limit.doc = "Set the memory limit of the runtime in bytes.";
os.gc_threshold.doc = "Set the threshold before a GC pass is triggered in bytes. This is set to malloc_size + malloc_size>>1 after a GC pass.";
os.max_stacksize.doc = "Set the max stack size in bytes.";

Object.defineProperty(String.prototype, "rm", {
  value: function (index, endidx = index + 1) {
    return this.slice(0, index) + this.slice(endidx);
  },
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

Resources.rm_fn = function rm_fn(fn, text) {
  var reg = new RegExp(fn.source + "\\s*\\(");
  var match;
  while ((match = text.match(reg))) {
    var last = match.index + match[0].length;
    var par = 1;
    while (par !== 0) {
      if (text[last] === "(") par++;
      if (text[last] === ")") par--;
      last++;
    }
    text = text.rm(match.index, last);
  }

  return text;
};
Resources.rm_fn.doc = "Remove calls to a given function from a given text script.";

// Normalizes paths for use in prosperon
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

// Given a script path, loads it, and replaces certain function calls to conform to environment
Resources.replstrs = function replstrs(path) {
  if (!path) return;
  var script = io.slurp(path);
  if (!script) return;
  var regexp = /"[^"\s]*?\.[^"\s]+?"/g;

  var stem = path.dir();

  if (!console.enabled) script = Resources.rm_fn(/console\.(spam|info|warn|error)/, script);
  if (!profile.enabled) script = Resources.rm_fn(/profile\.(cache|frame|endcache|endframe)/, script);

  if (!debug.enabled) {
    script = Resources.rm_fn(/assert/, script);
    script = Resources.rm_fn(/debug\.(build|fn_break)/, script);
  }

  script = script.replace(regexp, function (str) {
    var newstr = Resources.replpath(str.trimchr('"'), path);
    return `"${newstr}"`;
  });

  return script;
}

Resources.is_sound = function (path) {
  var ext = path.ext();
  return Resources.sounds.any(x => x === ext);
};

Resources.is_animation = function (path) {
  if (path.ext() === "gif" && Resources.gif.frames(path) > 1) return true;
  if (path.ext() === "ase") return true;

  return false;
};

Resources.is_path = function (str) {
  return !/[\\\/:*?"<>|]/.test(str);
};

globalThis.json = {};
json.encode = function json_encode(value, replacer, space = 1) {
  return JSON.stringify(value, replacer, space);
};

json.decode = function json_decode(text, reviver) {
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
Resources.images = ["qoi", "png", "gif", "jpg", "jpeg", "ase", "aseprite"];
Resources.sounds = ["wav", "flac", "mp3", "qoa"];
Resources.fonts = ["ttf"];
Resources.is_image = function (path) {
  var ext = path.ext();
  return Resources.images.some(x => x === ext);
};

Resources.shaders = ["hlsl", "glsl", "cg"]
Resources.is_shader = function(path) {
  var ext = path.ext();
  return Resources.shaders.some(x => x === ext)
}

var res_cache = {};

// ext is a list of extensions to search
function find_ext(file, ext) {
  if (!file) return;

  var file_ext = file.ext();
  var has_ext = file_ext.length > 0;

  for (var e of ext) {
    var nf = `${file}.${e}`;
    if (io.exists(nf)) return nf;
  }

  var glob_pat = has_ext ? `**/${file}` : `**/${file}.*`;

  var all_files = io.glob(glob_pat);
  var find = undefined;
  for (var e of ext) {
    var finds = all_files.filter(x => x.ext() === e);
    if (finds.length > 1) {
      console.warn(`Found conflicting files when searching for '${file}': ${json.encode(finds)}. Returning the topmost one.`);
      finds.sort((a,b) => a.length-b.length);
      return finds[0];
    }
    if (finds.length === 1) return finds[0];
  }

  return find;
}

var hashhit = 0;
var hashmiss = 0;

globalThis.hashifier = {};
hashifier.stats = function()
{
  
}

Object.defineProperty(Function.prototype, "hashify", {
  value: function () {
    var hash = new Map();
    var fn = this;
    function hashified(...args) {
      var key = args[0];
      if (!hash.has(key)) hash.set(key, fn(...args));

      return hash.get(key);
    }
    return hashified;
  },
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

Resources.find_font = function(file, root = "") {
  return find_ext(file, Resources.fonts, root);
}.hashify();

console.transcript = "";
console.say = function (msg) {
  console.print(msg);
  if (debug.termout) console.term_print(msg);
  console.transcript += msg;
};

globalThis.say = console.say;
globalThis.print = console.print;

console.rec = function(category, priority, line, file, msg)
{
  return `${file}:${line}: [${category} ${priority}]: ${msg}`;
}

console.pprint = function pprint(msg, lvl = 0) {
  if (typeof msg === "object") msg = JSON.stringify(msg, null, 2);

  var file = "nofile";
  var line = 0;

  var caller = new Error().stack.split("\n")[2];
  if (caller) {
    var md = caller.match(/\((.*)\:/);
    var m = md ? md[1] : "SCRIPT";
    if (m) file = m;
    md = caller.match(/\:(\d*)\)/);
    m = md ? md[1] : 0;
    if (m) line = m;
  }
  var fmt = console.rec("script", lvl, line,file, msg);
  console.log(fmt);
  if (tracy) tracy.message(fmt);
};

console.spam = function spam(msg) {
  console.pprint(msg, 0);
};
console.debug = function debug(msg) {
  console.pprint(msg, 1);
};
console.info = function info(msg) {
  console.pprint(msg, 2);
};
console.warn = function warn(msg) {
  console.pprint(msg, 3);
};
console.error = function error (e) {
  console.log(e);
  console.log(e.stack);
};
console.panic = function (e) {
  console.log("PANIC!")
  console.error(e);
  os.quit();
};
console.stackstr = function (skip = 0) {
  var err = new Error();
  var stack = err.stack.split("\n");
  return stack.slice(skip, stack.length).join("\n");
};

console.stack = function (skip = 0) {
  var stack = console.stackstr(skip + 1);
  console.log(stack);
  return stack;
};

console.stdout_lvl = 1;

console.doc = {
  log: "Output directly to in game console.",
  level: "Set level to output logging to console.",
  info: "Output info level message.",
  warn: "Output warn level message.",
  error: "Output error level message, and print stacktrace.",
  critical: "Output critical level message, and exit game immediately.",
  write: "Write raw text to console.",
  say: "Write raw text to console, plus a newline.",
  stack: "Output a stacktrace to console.",
  clear: "Clear console.",
};

globalThis.global = globalThis;

var use_cache = {};

globalThis.use = function use(file) {
  file = Resources.find_script(file);

  if (use_cache[file]) {
    var ret = use_cache[file]();
    return ret;
  }
  
  var script = Resources.replstrs(file);
  var fnname = file.replace(/[^a-zA-Z0-9_$]/g, "_");
  script = `(function ${fnname}() { var self = this; ${script}; } )`;
  var fn = os.eval(file, script);
  use_cache[file] = fn;

  var ret = fn();
  
  return ret;
};

var tmpslurp = io.slurp;
io.slurp = function slurp(path)
{
  var findpath = Resources.replpath(path);
  var ret = tmpslurp(findpath, true); //|| core_db.slurp(findpath, true);
  return ret;
}

io.slurpbytes = function(path)
{
  path = Resources.replpath(path);
  var ret = tmpslurp(path);// || core_db.slurp(path);
  if (!ret) throw new Error(`Could not find file ${path} anywhere`);
  return ret;
}

var ignore = io.slurp('.prosperonignore').split('\n');
var allpaths;
var tmpglob = io.glob;
io.glob = function glob(pat) {
  if (!allpaths)
    allpaths = io.globfs(ignore);

  return allpaths.filter(str => game.glob(pat,str)).sort();
}

io.invalidate = function()
{
  allpaths = undefined;
}

function splitPath(path) {
  return path.split('/').filter(part => part.length > 0);
}

function splitPattern(pattern) {
  return pattern.split('/').filter(part => part.length > 0);
}

function matchPath(pathParts, patternParts) {
  let pathIndex = 0;
  let patternIndex = 0;
  let starPatternIndex = -1;
  let starPathIndex = -1;

  while (pathIndex < pathParts.length) {
    if (patternIndex < patternParts.length) {
                  if (patternParts[patternIndex] === '**') {
            // Record the position of '**' in the pattern
            starPatternIndex = patternIndex;
            // Record the current position in the path
            starPathIndex = pathIndex;
            // Move to the next pattern component
            patternIndex++;
            continue;
          } else if (
            patternParts[patternIndex] === '*' ||
            patternParts[patternIndex] === pathParts[pathIndex]
          ) {
            // If the pattern is '*' or exact match, move to the next component
            patternIndex++;
            pathIndex++;
            continue;
          }
    }

    if (starPatternIndex !== -1) {
      // If there was a previous '**', backtrack
      patternIndex = starPatternIndex + 1;
      starPathIndex++;
      pathIndex = starPathIndex;
      continue;
    }

    // No match and no '**' to backtrack to
    return false;
  }

  // Check for remaining '**' in the pattern
  while (patternIndex < patternParts.length && patternParts[patternIndex] === '**') {
    patternIndex++;
  }

  return patternIndex === patternParts.length;
}

function stripped_use(file, script) {
  file = Resources.find_script(file);

  if (use_cache[file]) {
    var ret = use_cache[file]();
    return ret;
  }
  script ??= Resources.replstrs(file);

  script = `(function () { var self = this; ${script}; })`;
  var fn = os.eval(file, script);
  var ret = fn();

  return ret;
}

function bare_use(file) {
  try {
  var script = io.slurp(file);
  if (!script) return;
  var fnname = file.replace(/[^a-zA-Z0-9_$]/g, "_");  
  script = `(function ${fnname}() { var self = this; ${script}; })`;
  Object.assign(globalThis, os.eval(file, script)());
  } catch(e) {
    console.log(e);
  }
}

profile.enabled = true;
console.enabled = true;
debug.enabled = true;

bare_use("core/scripts/base.js");
bare_use("core/scripts/profile.js");

prosperon.release = function () {
  profile.enabled = false;
  console.enabled = false;
  debug.enabled = false;
};

bare_use("core/scripts/preconfig.js");

if (!profile.enabled) use = stripped_use;

Object.assign(globalThis, use("core/scripts/prosperon.js"));
 
