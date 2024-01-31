function compile_env(str, env, file)
{
  file ??= "unknown";
  return cmd(123, str, env, file);
}

function fcompile_env(file, env) { return compile_env(IO.slurp(file), env, file); }

function buf2hex(buffer) { // buffer is an ArrayBuffer
  return [...new Uint8Array(buffer)]
      .map(x => x.toString(16).padStart(2, '0'))
      .join(' ');
}

var OS = {};
OS.cwd = function() { return cmd(144); }
OS.exec = function(s) { cmd(143, s); }

var Resources = {};
Resources.images = ["png", "jpg", "jpeg", "gif"];
Resources.sounds =  ["wav", "mp3", "flac"];
Resources.scripts = "js";
Resources.is_image = function(path) {
  var ext = path.ext();
  return Resources.images.any(x => x === ext);
}
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

Resources.texture = {};
Resources.texture.dimensions = function(path) { return cmd(64,path); }

Resources.gif = {};
Resources.gif.frames = function(path) { return cmd(139,path); }

var Log = {
  set level(x) { cmd(92,x); },
  get level() { return cmd(93); },
  print(msg, lvl) {
    var lg;
    if (typeof msg === 'object') {
      lg = JSON.stringify(msg, null, 2);
    } else {
      lg = msg;
    }

    var stack = (new Error()).stack;
    var n = stack.next('\n',0)+1;
    n = stack.next('\n', n)+1;
    var nnn = stack.slice(n);
    var fmatch = nnn.match(/\(.*\:/);
    var file = fmatch ? fmatch[0].shift(1).shift(-1) : "nofile";
    var lmatch = nnn.match(/\:\d*\)/);
    var line = lmatch ? lmatch[0].shift(1).shift(-1) : "0";

    yughlog(lvl, lg, file, line);
  },
  
  info(msg) {
    this.print(msg, 0);
  },

  warn(msg) {
    this.print(msg, 1);
  },

  error(msg) {
    this.print(msg, 2);
    this.stack(1);
  },

  critical(msg) {
    this.print(msg,3);
    this.stack(1);
  },

  write(msg) {
    if (typeof msg === 'object')
      msg = JSON.stringify(msg,null,2);
  
    cmd(91,msg);
  },

  say(msg) {
    Log.write(msg + '\n');
  },

  repl(msg) {
    cmd(142, msg + '\n');
  },    

  stack(skip = 0) {
    var err = new Error();
    var stack = err.stack;
    var n = stack.next('\n',0)+1;
    for (var i = 0; i < skip; i++)
      n = stack.next('\n', n)+1;
    Log.write(err.name);
    Log.write(err.message);
    Log.write(err.stack);
//    Log.write(stack);
  },

  clear() {
    cmd(146);
  },
};

Log.doc = {
  level: "Set level to output logging to console.",
  info: "Output info level message.",
  warn: "Output warn level message.",
  error: "Output error level message, and print stacktrace.",
  critical: "Output critical level message, and exit game immediately.",
  write: "Write raw text to console.",
  say: "Write raw text to console, plus a newline.",
  stack: "Output a stacktrace to console.",
  console: "Output directly to in game console.",
  clear: "Clear console."
};

/*
  IO path rules. Starts with, meaning:
  "@": playerpath
  "/": game room
  "#": Force look locally (instead of in db first)
    - This is combined with others. #/, #@, etc
  "": Local path relative to script defined in
*/
  
var IO = {
  exists(file) { return cmd(65, file);},
  slurp(file) {
    if (IO.exists(file))
      return cmd(38,file);
    else
      throw new Error(`File ${file} does not exist; can't slurp`);
  },
  slurpbytes(file) {
    return cmd(81, file);
  },
  slurpwrite(file, data) {
    if (data.byteLength)
      cmd(60, data, file);
    else
      return cmd(39, data, file);
  },
  extensions(ext) {
    var paths = IO.ls();
    paths = paths.filter(function(str) { return str.ext() === ext; });
    return paths;
  },
  ls() { return cmd(66); },
  /* Only works on text files currently */
  cp(f1, f2) {
    cmd(166, f1, f2);
  },
  mv(f1, f2) {
    return cmd(163, f1, f2);
  },
  rm(f) {
    return cmd(f);
  },
  glob(pat) {
    var paths = IO.ls();
    pat = pat.replaceAll(/([\[\]\(\)\^\$\.\|\+])/g, "\\$1");
    pat = pat.replaceAll('**', '.*');
    pat = pat.replaceAll(/[^\.]\*/g, '[^\\/]*');
    pat = pat.replaceAll('?', '.');
    
    var regex = new RegExp("^"+pat+"$", "");
    return paths.filter(str => str.match(regex));
  },
};

IO.doc = {
  doc: "Functions for filesystem input/output commands.",
  exists: "Returns true if a file exists.",
  slurp: "Returns the contents of given file as a string.",
  slurpwrite: "Write a given string to a given file.",
  ls: "List contents of the game directory.",
  glob: "Glob files in game directory.",
};

var Parser = {};
Parser.replstrs = function(path)
{
  var script = IO.slurp(path);
  var regexp = /"[^"\s]*?\.[^"\s]+?"/g;
  var stem = path.dir();

  script = script.replace(regexp,function(str) {
    if (str[1] === "/")
      return str.rm(1);

    if (str[1] === "@")
      return str.rm(1).splice(1, "playerpath/");

    return str.splice(1, stem + "/");
  });
  Log.warn(script);
}

var Cmdline = {};

Cmdline.cmds = [];
Cmdline.register_cmd = function(flag, fn, doc) {
  Cmdline.cmds.push({
    flag: flag,
    fn: fn,
    doc: doc
  });
};

function cmd_args(cmdargs)
{
  var play = false;
  var cmds = cmdargs.split(" ");

  for (var i = 1; i < cmds.length; i++) {
    if (cmds[i][0] !== '-') {
      Log.warn(`Command '${cmds[i]}' should start with a '-'.`);
      continue;
    }

    var c = Cmdline.cmds.find(function(cmd) { return cmd.flag === cmds[i].slice(1); });
    if (!c) {
      Log.warn(`Command ${cmds[i]} not recognized.`);
      continue;
    }

      var sendstr = [];
      var j = i+1;
      while (cmds[j] && cmds[j][0] !== '-') {
        sendstr.push(cmds[j]);
	j++;
      }

      c.fn(sendstr);
      i = j-1;
    }
}

var STD = {};
STD.exit = function(status)
{
  cmd(147,status);
}
Cmdline.register_cmd("p", function() { Game.edit = false; }, "Launch engine in play mode.");
Cmdline.register_cmd("v", function() { Log.say(cmd(120)); STD.exit(0);}, "Display engine info.");
Cmdline.register_cmd("l", function(n) {
  Log.level = n;
}, "Set log level.");
Cmdline.register_cmd("h", function(str) {
  for (var cmd of Cmdline.cmds) {
    Log.say(`-${cmd.flag}:  ${cmd.doc}`);
  }
  STD.exit(0);
},
"Help.");
Cmdline.register_cmd("b", function(str) {
  var packname;
  if (str.length === 0)
    packname = "test.cdb";
  else if (str.length > 1) {
    Log.warn("Give me a single filename for the pack.");
    Game.quit();
  } else
    packname = str[0];

  Log.warn(`Packing into ${packname}`);
    
  cmd(124, packname);
  STD.exit(0);
}, "Pack the game into the given name.");

Cmdline.register_cmd("e", function(pawn) {
  load("scripts/editor.js");
  Log.write(`## Input for ${pawn}\n`);
  eval(`Log.write(Input.print_md_kbm(${pawn}));`);
  STD.exit(0);
}, "Print input documentation for a given object in a markdown table." );

Cmdline.register_cmd("t", function() {
  Log.warn("Testing not implemented yet.");
  STD.exit(0);  
}, "Test suite.");

Cmdline.register_cmd("d", function(obj) {
  load("scripts/editor.js");
  Log.say(API.print_doc(obj[0]));
  STD.exit(0);
}, "Print documentation for an object.");

Cmdline.register_cmd("cjson", function(json) {
  var f = json[0];
  if (!IO.exists(f)) {
    Log.warn(`File ${f} does not exist.`);
    STD.exit(1);
  }

  prototypes.generate_ur();

  var j = JSON.parse(IO.slurp(f));

  for (var k in j) {
    if (k in j.objects)
      delete j[k];
  }

  Log.warn(j);

  for (var k in j.objects) {
    var o = j.objects[k];
    samediff(o, ur[o.ur]);
  }

  Log.say(j);

  STD.exit(0);
}, "Clean up a jso file.");

Cmdline.register_cmd("r", function(script) {
  try { run(script); } catch(e) { STD.exit(0); }
  
  STD.exit(0);
}, "Run a script.");
