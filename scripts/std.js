os.cwd.doc = "Get the absolute path of the current working directory.";
os.env.doc = "Return the value of the environment variable v.";

var projectfile = ".prosperon/project.json";

var Resources = {};
Resources.images = ["png", "jpg", "jpeg", "gif"];
Resources.sounds =  ["wav", "mp3", "flac", "qoa"];
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
  "@": user path
  "/": game path
*/

var tmp = io.chmod;
io.chmod = function(file,mode) {
  return tmp(file,parseInt(mode,8));
}
  
io.mixin({
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

  glob(pat) {
    var paths = io.ls();
    pat = pat.replaceAll(/([\[\]\(\)\^\$\.\|\+])/g, "\\$1");
    pat = pat.replaceAll('**', '.*');
    pat = pat.replaceAll(/[^\.]\*/g, '[^\\/]*');
    pat = pat.replaceAll('?', '.');
    
    var regex = new RegExp("^"+pat+"$", "");
    return paths.filter(str => str.match(regex));
  },
});

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
    global.mixin("scripts/editor.js");
    use("editorconfig.js");
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
  io.slurpwrite(projectfile, json.encode(project));
  
}, "Turn the directory into a Prosperon game.");

Cmdline.register_order("debug", function() {
  Cmdline.orders.play();
}, "Play the game with debugging enabled.");

Cmdline.register_order("play", function() {
  if (!io.exists(projectfile)) {
    say("No game to play. Try making one with 'prosperon init'.");
    return;
  }

  var project = json.decode(io.slurp(projectfile));
  global.mixin("config.js");
  if (project.title) Window.title(project.title);
  
  Game.engine_start(function() {
    global.mixin("scripts/sound.js");  
    global.mixin("game.js");
    if (project.icon) Window.icon(project.icon);
  });  
}, "Play the game present in this folder.");

Cmdline.register_order("pack", function(str) {
  var packname;
  if (str.length === 0)
    packname = "game.cdb";
  else if (str.length > 1) {
    console.warn("Give me a single filename for the pack.");
    return;
  } else
    packname = str[0];

  say(`Packing into ${packname}`);
    
  cmd(124, packname);
  io.chmod(packname, 666);
}, "Pack the game into the given name.", "NAME");

Cmdline.register_order("cdb", function(argv) {
  var cdb = "game.cdb";
  if (!io.exists(cdb)) {
    say(`No 'game.cdb' present.`);
    return;
  }
  if (argv.length === 0) {
    say(`cdb name: ${cdb}`);
    
  }
}, "CDB commands.");

Cmdline.register_order("qoa", function(argv) {
  var sounds = Resources.sounds.filter(x => x !== "qoa");
  for (var file of argv) {
    if (!sounds.includes(file.ext())) continue;
    say(`converting ${file}`);
    cmd(262,file);
  }
}, "Convert file(s) to qoa.");

Cmdline.register_order("about", function(argv) {
  
  if (!argv[0]) {
    say('About your game');
    say(`Prosperon version ${prosperon.version}`);
    say(`Total entities ${ur._list.length}`);
  }
  switch (argv[0]) {
    case "entities":
      for (var i of ur._list) say(i);
      break;
  }
}, "Get information about this game.");

Cmdline.register_order("ur", function(argv) {
  for (var i of ur._list.sort()) say(i);
}, "Get information about the ur types in your game.");

Cmdline.register_order("env", function(argv) {
  if (argv.length > 2) return;
  var gg = json.decode(io.slurp(projectfile));
  if (argv.length === 0) {
    say(json.encode(gg,null,1));
    return;
  }

  if (argv.length === 1) {
    var v = gg[argv[0]];
    if (!v) {
      say(`Value ${argv[0]} not found.`);
      return;
    }
    say(`${argv[0]}:${v}`);
  } else {
    gg[argv[0]] = argv[1];
    say(`Set ${argv[0]}:${v}`);
    say(json.encode(gg,null,1));
    io.slurpwrite(projectfile, json.encode(gg));
  }
}, "Get or set game variables.");

Cmdline.register_order("unpack", function() {
  say("Unpacking not implemented.");
}, "Unpack this binary's contents into this folder for editing.");

Cmdline.register_order("build", function() {
  say("Building not implemented.");
}, "Build static assets for this project.");

Cmdline.register_order("nota", function(argv) {
  for (var file of argv) {
    if (!io.exists(file)) {
      say(`File ${file} does not exist.`);
      continue;
    }

    var obj = json.decode(io.slurp(file));
    var nn = nota.encode(obj);
    io.slurpwrite(file.set_ext(".nota"), nn);
  }
}, "Create a nota file from a json.");

Cmdline.register_order("json", function(argv) {
  for (var file of argv) {
    if (!io.exists(file)) {
      say(`File ${file} does not exist.`);
      continue;
    }
    say(file.ext());
    var obj = nota.decode(io.slurp(file));
    var nn = json.encode(obj);
    io.slurpwrite(file.set_ext(".json", nn));
  }
}, "Create a JSON from a nota.");

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
}, "Clean up a given object file.", "JSON ...");

Cmdline.register_cmd("l", function(n) {
  console.level = n;
}, "Set log level.");

return {
  console,
  Resources,
  say,
  io,
  Cmdline,
  cmd_args
};
