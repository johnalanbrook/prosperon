os.cwd.doc = "Get the absolute path of the current working directory.";
os.env.doc = "Return the value of the environment variable v.";

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

Resources.replstrs = function(path)
{
  var script = io.slurp(path);
  var regexp = /"[^"\s]*?\.[^"\s]+?"/g;
  var stem = path.dir();

  script = script.replace(regexp,function(str) {
    if (str[1] === "/")
      return str.rm(1);

    if (str[1] === "@")
      return str.rm(1).splice(1, "playerpath/");

    return str.splice(1, stem + "/");
  });

  return script;
}

var console = {
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

  log(msg) { console.say(time.text(time.now(), 'yyyy-m-dd hh:nn:ss') + "  " + str); },
  say(msg) { console.write(msg + '\n'); },
  repl(msg) { cmd(142, msg + '\n'); },    

  stack(skip = 0) {
    var err = new Error();
    var stack = err.stack;
    var n = stack.next('\n',0)+1;
    for (var i = 0; i < skip; i++)
      n = stack.next('\n', n)+1;
    console.write(err.name);
    console.write(err.message);
    console.write(err.stack);
//    console.write(stack);
  },

  clear() {
    cmd(146);
  },

  assert(assertion, msg, obj) {
    if (!assertion) {
      console.error(msg);
      console.stack();
    }
  },
};

var say = function(msg) {
  console.say(msg);
}
say.doc = "Print to std out with an appended newline.";

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
  clear: "Clear console."
};

/*
  io path rules. Starts with, meaning:
  "@": playerpath
  "/": game room
  "#": Force look locally (instead of in db first)
    - This is combined with others. #/, #@, etc
  "": Local path relative to script defined in
*/
  
var io = {
  exists(file) { return cmd(65, file);},
  slurp(file) {
    if (io.exists(file))
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
    var paths = io.ls();
    paths = paths.filter(function(str) { return str.ext() === ext; });
    return paths;
  },
  compile(script) {
    return cmd(260, script);
  },
  run_bytecode(byte_file) {
    return cmd(261, byte_file);
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
  mkdir(dir) {
    cmd(258, dir);
  },
  glob(pat) {
    var paths = io.ls();
    pat = pat.replaceAll(/([\[\]\(\)\^\$\.\|\+])/g, "\\$1");
    pat = pat.replaceAll('**', '.*');
    pat = pat.replaceAll(/[^\.]\*/g, '[^\\/]*');
    pat = pat.replaceAll('?', '.');
    
    var regex = new RegExp("^"+pat+"$", "");
    return paths.filter(str => str.match(regex));
  },
};

io.doc = {
  doc: "Functions for filesystem input/output commands.",
  exists: "Returns true if a file exists.",
  slurp: "Returns the contents of given file as a string.",
  slurpbytes: "Return the contents of a file as a byte array.",
  slurpwrite: "Write a given string to a given file.",
  cp: "Copy file f1 to f2.",
  mv: "Rename file f1 to f2.",
  rm: "Remove file f.",
  mkdir: "Make dir.",
  ls: "List contents of the game directory.",
  glob: "Glob files in game directory.",
};

var Cmdline = {};

Cmdline.cmds = [];
Cmdline.orders = {};
Cmdline.register_cmd = function(flag, fn, doc) {
  Cmdline.cmds.push({
    flag: flag,
    fn: fn,
    doc: doc
  });
};

Cmdline.register_order = function(order, fn, doc, usage) {
  Cmdline.orders[order] = fn;
  fn.doc = doc;
  usage ??= "";
  fn.usage = `${order} ${usage}`;
}

Cmdline.register_order("edit", function() {
  if (!io.exists(".prosperon")) {
    say("No game to edit. Try making one with 'prosperon init'.");
    return;
  }
  
  Game.engine_start(function() {
    load("scripts/editor.js");
    load("editorconfig.js");
    editor.enter_editor();
  });
}, "Edit the project in this folder. Give it the name of an UR to edit that specific object.", "?UR?");

Cmdline.register_order("init", function() {
  if (io.exists(".prosperon")) {
    say("Already a game here.");
    return;
  }

  if (!(io.ls().length === 0)) {
    say("Directory is not empty. Make an empty one and init there.");
    return;
  }

  io.mkdir(".prosperon");
  var project = {};
  project.version = prosperon.version;
  project.revision = prosperon.revision;
  io.slurpwrite(".prosperon/project", json.encode(project));
  
}, "Turn the directory into a Prosperon game.");

Cmdline.register_order("play", function() {
  if (!io.exists(".prosperon/project")) {
    say("No game to play. Try making one with 'prosperon init'.");
    return;
  }

  var project = json.decode(io.slurp(".prosperon/project"));
  
  Game.engine_start(function() {
    load("config.js");
    load("game.js");
    if (project.icon) Window.icon(project.icon);
    if (project.title) Window.title(project.title);
    say(project.title);
  });  
}, "Play the game present in this folder.");

Cmdline.register_order("pack", function(str) {
  var packname;
  if (str.length === 0)
    packname = "test.cdb";
  else if (str.length > 1) {
    console.warn("Give me a single filename for the pack.");
    return;
  } else
    packname = str[0];

  say(`Packing into ${packname}`);
    
  cmd(124, packname);
}, "Pack the game into the given name.", "NAME");

Cmdline.register_order("unpack", function() {
  say("Unpacking not implemented.");
}, "Unpack this binary's contents into this folder for editing.");

Cmdline.register_order("build", function() {
  say("Building not implemented.");
}, "Build static assets for this project.");

Cmdline.register_order("api", function(obj) {
  if (!obj[0]) {
    Cmdline.print_order("api");
    return;
  }

  load("scripts/editor.js");
  var api = Debug.api.print_doc(obj[0]);
  if (!api)
    return;

  say(api);
}, "Print the API for an object as markdown. Give it a file to save the output to.", "OBJECT");

Cmdline.register_order("compile", function(argv) {
  for (var file of argv) {
    var comp = io.compile(file);
    io.slurpwrite(file + "c", comp);
  }
}, "Compile one or more provided files into bytecode.", "FILE ...");

Cmdline.register_order("input", function(pawn) {
  load("scripts/editor.js");
  say(`## Input for ${pawn}`);
  eval(`say(input.print_md_kbm(${pawn}));`);
}, "Print input documentation for a given object as markdown. Give it a file to save the output to", "OBJECT ?FILE?");

Cmdline.register_order("run", function(script) {
  script = script.join(" ");
  if (!script) {
    say("Need something to run.");
    return;
  }
  
  if (io.exists(script))
    try {
      if (script.endswith("c"))
        cmd(261, script);
      else
        load(script);
    } catch(e) { }
  else {
    var ret = eval(script);
    if (ret) say(ret);
  }
}, "Run a given script. SCRIPT can be the script itself, or a file containing the script", "SCRIPT");

Cmdline.orders.script = Cmdline.orders.run;

Cmdline.print_order = function(fn)
{
  if (typeof fn === 'string')
    fn = Cmdline.orders[fn];
    
  if (!fn) return;
  say(`Usage: prosperon ${fn.usage}`);
  say(fn.doc);
}

Cmdline.register_order("help", function(order) {
  
  if (!Object.empty(order)) {
    var orfn = Cmdline.orders[order];
    
    if (!orfn) {
      console.warn(`No command named ${order}.`);
      return;
    }

    Cmdline.print_order(orfn);
    return;
  }
  
  Cmdline.print_order("help");

  for (var cmd of Object.keys(Cmdline.orders).sort())
    say(cmd);

  Cmdline.orders.version();
}, "Give help with a specific command.", "TOPIC");

Cmdline.register_order("version", function() {
  say(`Prosperon version ${prosperon.version} [${prosperon.revision}]`);
}, "Display Prosperon info.");

function cmd_args(cmdargs)
{
  var play = false;
  var cmds = cmdargs.split(/\s+/).slice(1);

  if (cmds.length === 0)
    cmds[0] = "play";
  else if (!Cmdline.orders[cmds[0]]) {
    console.warn(`Command ${cmds[0]} not found.`);
    return;
  }
  
  Cmdline.orders[cmds[0]](cmds.slice(1));
}

Cmdline.register_order("clean", function(argv) {
  say("Cleaning not implemented.");
  return;
  
  var f = argv[0];
  if (argv.length === 0) {
    Cmdline.print_order("clean");
    return;
  }
  
  if (!io.exists(f)) {
    say(`File ${f} does not exist.`);
    return;
  }

  prototypes.generate_ur();

  var j = json.decode(io.slurp(f));

  for (var k in j)
    if (k in j.objects)
      delete j[k];

  console.warn(j);

  for (var k in j.objects) {
    var o = j.objects[k];
    samediff(o, ur[o.ur]);
  }

  say(j);
}, "Clean up a given object file.", "JSON ...");

Cmdline.register_cmd("l", function(n) {
  console.level = n;
}, "Set log level.");
