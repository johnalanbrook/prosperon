os.cwd.doc = "Get the absolute path of the current working directory.";
os.env.doc = "Return the value of the environment variable v.";
if (os.sys() === "windows") os.user = os.env("USERNAME");
else os.user = os.env("USER");

/*var ignore;
if (ignore = io.slurp('.prosperonignore')) {
  ignore = ignore.split('\n');
  for (var ig of ignore) {
    if (!ig) continue;
    allpaths = allpaths.filter(x => !x.startsWith(ig));
  }
}
*/
//console.info(allpaths)

var appy = {};
appy.inputs = {};
if (os.sys() === "macos") {
  appy.inputs["S-q"] = os.exit;
}

appy.inputs.f7 = function () {
  debug.meta = !debug.meta;
};
appy.inputs.f8 = function () {
  debug.cheat = !debug.cheat;
};
appy.inputs.f9 = function () {
  debug.console = !debug.console;
};
appy.inputs.f10 = function () {
  debug.show = !debug.show;
};
appy.inputs.f11 = window.toggle_fullscreen;
appy.inputs.f11.doc = "Toggle window fullscreen.";
appy.inputs.f11.title = "Toggle Fullscreen";
appy.inputs["M-f4"] = os.exit;

player[0].control(appy);

os.home = os.env("HOME");

/*steam.path = {
  windows: `C:/Program Files (x86)/Steam/userdata/${steam.userid}/${steam.appid}`,
  macos: `${os.home}/Library/Application Support/Steam/userdata/${steam.userid}/${steam.appid}`,
  linux: `${os.home}/.local/share/Steam/userdata/${steam.userid}/${steam.appid}`
};
*/

var otherpath = {
  windows: `C:/Users/${os.user}/Saved Games`,
  macos: `${os.home}/Library/Application Support`,
  linux: `${os.home}/.local/share`,
};

os.prefpath = function () {
  return otherpath[os.sys()] + "/" + (game.title ? game.title : "Untitled Prosperon Game");
};

os.openurl = function (url) {
  if (os.sys() === "windows") os.system(`start ${url}`);
  else os.system(`open ${url}`);
};

var projectfile = "project.prosperon";
io.dumpfolder = ".prosperon";

Resources.texture = {};
Resources.texture.dimensions = function (path) {
  texture.dimensions(path);
};

Resources.gif = {};
Resources.gif.frames = function (path) {
  return render.gif_frames(path);
};

/*
  io path rules. Starts with, meaning:
  "@": user path
  "/": root game path
  "" : relative game path
*/

var tmpchm = io.chmod;
io.chmod = function (file, mode) {
  return tmpchm(file, parseInt(mode, 8));
};

io.mkpath = function (dir) {
  if (!dir) return;
  var mkstack = [];
  while (!io.exists(dir)) {
    mkstack.push(dir.fromlast("/"));
    dir = dir.dir();
  }
  for (var d of mkstack) {
    dir = dir + "/" + d;
    say(`making ${dir}`);
    io.mkdir(dir);
  }
};

var tmpslurpw = io.slurpwrite;
io.slurpwrite = function (path, c) {
  path = Resources.replpath(path);
//  io.mkpath(path.dir());
  return tmpslurpw(path, c);
};

var tmprm = io.rm;
io.rm = function (f) {
  tmprm(Resources.replpath(f));
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
Cmdline.register_cmd = function (flag, fn, doc) {
  Cmdline.cmds.push({
    flag: flag,
    fn: fn,
    doc: doc,
  });
};

Cmdline.register_order = function (order, fn, doc, usage = "") {
  Cmdline.orders[order] = fn;
  fn.doc = doc;
  fn.usage = `${order} ${usage}`;
};

Cmdline.register_order(
  "edit",
  function () {
    if (!io.exists(projectfile)) {
      say("No game to edit. Try making one with 'prosperon init'.");
      return;
    }

    sim.pause();

    game.engine_start(function () {
      global.mixin("editor.js");
      use("editorconfig.js");
      use("config.js");
      render.set_font("fonts/c64.ttf", 8);
      editor.enter_editor();
    });
  },
  "Edit the project in this folder. Give it the name of an UR to edit that specific object.",
  "?UR?",
);

Cmdline.register_order(
  "init",
  function () {
    if (io.exists(projectfile)) {
      say("Already a game here.");
      return;
    }

    io.mkdir(io.dumpfolder);
    var project = {};
    project.version = prosperon.version;
    project.revision = prosperon.revision;
    io.slurpwrite(projectfile, json.encode(project));
  },
  "Turn the directory into a Prosperon game.",
);

Cmdline.register_order(
  "debug",
  function () {
    Cmdline.orders.play([]);
  },
  "Play the game with debugging enabled.",
);

Cmdline.register_order(
  "web",
  function () {
    Cmdline.orders.play([]);
  },
  "Play the game in a web browser.",
);

Cmdline.register_order(
  "play",
  function (argv) {
    if (argv[0]) io.chdir(argv[0]);

    //  game.loadurs();

    if (!io.exists(projectfile)) {
      console.log("No game to play. Try making one with 'prosperon init'.");
      return;
    }

    var project = json.decode(io.slurp(projectfile));
    game.title = project.title;
    game.size = [1280, 720];
    if (io.exists("config.js")) global.mixin("config.js");
    else console.warn("No config.js file found. Starting with default parameters.");
    
    prosperon.title = project.title;
    prosperon.width = 1280;
    prosperon.height = 720;
    prosperon.cleanup = function(){}
    prosperon.event = function(e){
      switch(e.type) {
        case "mouse_move":
          prosperon.mousemove(e.mouse, e.mouse_d);
          break;
        case "mouse_scroll":
          prosperon.mousescroll(e.scroll);
          break;
        case "key_down":
          prosperon.keydown(e.key_code, e.key_repeat);
          break;
        case "key_up":
          prosperon.keyup(e.key_code);
          break;
        case "mouse_up":
          prosperon.mouseup(e.mouse_button);
          break;
        case "mouse_down":
          prosperon.mousedown(e.mouse_button);
          break;
        case "char":
          prosperon.textinput(e.char_code);
          break;
        case "resized":
          prosperon.resize([e.window_width, e.window_height]);
          break;
        case "iconified":
          prosperon.iconified(false);
          break;
        case "restored":
          prosperon.iconified(true);
          break;
        case "focused":
          prosperon.focus(true);
          break;
        case "unfocused":
          prosperon.focus(false);
          break;
        case "suspended":
          prosperon.suspended(true);
          break;
        case "quit_requested":
          os.exit(0);
          break;
        case "mouse_enter":
          prosperon.mouseenter();
          break;
        case "mouse_leave":
          prosperon.mouseleave();
          break;
      }
    }
    prosperon.frame = prosperon.process;
    prosperon.icon = os.make_texture(io.slurpbytes('moon.gif'));
    prosperon.high_dpi = 0;
    prosperon.alpha = 1;
    prosperon.fullscreen = 0;
    prosperon.enable_clipboard = true;
    prosperon.enable_dragndrop=true;
    prosperon.max_dropped_files=1;
    prosperon.swap_interval = 1;

    game.engine_start(prosperon);
  },
  "Play the game present in this folder.",
);

Cmdline.register_order(
  "pack",
  function (str) {
    var packname;
    if (str.length === 0) packname = "game.zip";
    else if (str.length > 1) {
      console.warn("Give me a single filename for the pack.");
      return;
    } else packname = str[0];

    say(`Packing into ${packname}`);

    io.pack_start(packname);
    files = allfiles.filter(f => !f.startsWith(".git"));
    files = files.filter(f => !f.startsWith(".nova"));
    files = files.filter(f => !f.includes(".DS_Store"));
    files = files.filter(f => !f.startsWith(".gitignore"));
    say(files);
    for (var f of files) io.pack_add(f);
    io.pack_end();
  },
  "Pack the game into the given name.",
  "NAME",
);

Cmdline.register_order(
  "cdb",
  function (argv) {
    var cdb = "game.zip";
    if (!io.exists(cdb)) {
      say(`No 'game.zip' present.`);
      return;
    }
    if (argv.length === 0) {
      say(`cdb name: ${cdb}`);
    }
  },
  "CDB commands.",
);

Cmdline.register_order(
  "qoa",
  function (argv) {
    var sounds = Resources.sounds.filter(x => x !== "qoa");
    for (var file of argv) {
      if (!sounds.includes(file.ext())) continue;
      say(`converting ${file}`);
      io.save_qoa(file);
    }
  },
  "Convert file(s) to qoa.",
);

Cmdline.register_order(
  "about",
  function (argv) {
    if (!argv[0]) {
      say("About your game");
      say(`Prosperon version ${prosperon.version}`);
      say(`Total entities ${ur._list.length}`);
    }
    switch (argv[0]) {
      case "entities":
        for (var i of ur._list) say(i);
        break;
    }
  },
  "Get information about this game.",
);

Cmdline.register_order(
  "ur",
  function (argv) {
    //  game.loadurs();
    for (var i of ur._list.sort()) say(i);
  },
  "Get information about the ur types in your game.",
);

Cmdline.register_order(
  "env",
  function (argv) {
    if (argv.length > 2) return;
    var gg = json.decode(io.slurp(projectfile));
    if (argv.length === 0) {
      say(json.encode(gg, null, 1));
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
      say(json.encode(gg, null, 1));
      io.slurpwrite(projectfile, json.encode(gg));
    }
  },
  "Get or set game variables.",
);

Cmdline.register_order(
  "unpack",
  function () {
    say("Unpacking not implemented.");
  },
  "Unpack this binary's contents into this folder for editing.",
);

Cmdline.register_order(
  "build",
  function () {
    say("Building not implemented.");
  },
  "Build static assets for this project.",
);

Cmdline.register_order(
  "nota",
  function (argv) {
    for (var file of argv) {
      if (!io.exists(file)) {
        say(`File ${file} does not exist.`);
        continue;
      }

      var obj = json.decode(io.slurp(file));
      var nn = nota.encode(obj);
      io.slurpwrite(file.set_ext(".nota"), nn);
    }
  },
  "Create a nota file from a json.",
);

Cmdline.register_order(
  "json",
  function (argv) {
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
  },
  "Create a JSON from a nota.",
);

Cmdline.register_order(
  "api",
  function (obj) {
    if (!obj[0]) {
      Cmdline.print_order("api");
      return;
    }

    use("editor.js");
    var api = debug.api.print_doc(obj[0]);
    if (!api) return;

    say(api);
  },
  "Print the API for an object as markdown. Give it a file to save the output to.",
  "OBJECT",
);

Cmdline.register_order(
  "input",
  function (pawn) {
    use("editor.js");
    say(`## Input for ${pawn}`);
    eval(`say(input.print_md_kbm(${pawn}));`);
  },
  "Print input documentation for a given object as markdown. Give it a file to save the output to",
  "OBJECT ?FILE?",
);

Cmdline.register_order(
  "run",
  function (script) {
    script = script.join(" ");
    if (!script) {
      say("Need something to run.");
      return;
    }

    say(eval(script));
  },
  "Run a given script. SCRIPT can be the script itself, or a file containing the script",
  "SCRIPT",
);

Cmdline.orders.script = Cmdline.orders.run;

Cmdline.print_order = function (fn) {
  if (typeof fn === "string") fn = Cmdline.orders[fn];

  if (!fn) return;
  say(`Usage: prosperon ${fn.usage}`);
  say(fn.doc);
};

Cmdline.register_order(
  "help",
  function (order) {
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

    for (var cmd of Object.keys(Cmdline.orders).sort()) say(cmd);

    Cmdline.orders.version();
  },
  "Give help with a specific command.",
  "TOPIC",
);

Cmdline.register_order(
  "version",
  function () {
    say(`Prosperon version ${prosperon.version} [${prosperon.revision}]`);
  },
  "Display Prosperon info.",
);

function cmd_args(cmdargs) {
  var play = false;
  var cmds = cmdargs.split(/\s+/).slice(1);

  if (cmds.length === 0) cmds[0] = "play";
  else if (!Cmdline.orders[cmds[0]]) {
    console.warn(`Command ${cmds[0]} not found. Playing instead.`);
    cmds[0] = "play";
  }

  Cmdline.orders[cmds[0]](cmds.slice(1));
  if (!game.startengine) os.exit(0);
}

Cmdline.register_order(
  "clean",
  function (argv) {
    say("Cleaning not implemented.");
  },
  "Clean up a given object file.",
  "JSON ...",
);

Cmdline.register_order(
  "test",
  function (argv) {
    use("test.js");
  },
  "Run tests.",
);

Cmdline.register_cmd(
  "l",
  function (n) {
    console.level = n;
  },
  "Set log level.",
);

function convertYAMLtoJSON(yamlString) {
  const lines = yamlString.split("\n");
  const jsonObj = {};

  let currentKey = "";
  let currentValue = "";
  let currentDepth = 0;

  for (let i = 0; i < lines.length; i++) {
    const line = lines[i].trim();
    if (!line || line.startsWith("#")) {
      continue;
    }

    const depth = (line.match(/^\s+/g) || [""])[0].length;
    const keyValue = line.split(":");
    const key = keyValue[0].trim();
    const value = keyValue[1].trim();

    if (depth > currentDepth) {
      jsonObj[currentKey] = convertYAMLtoJSON(currentValue);
      currentKey = key;
      currentValue = value;
    } else if (depth === currentDepth) {
      jsonObj[currentKey] = convertYAMLtoJSON(currentValue);
      currentKey = key;
      currentValue = value;
    } else {
      jsonObj[currentKey] = convertYAMLtoJSON(currentValue);
      currentKey = "";
      currentValue = "";
      i--; // To reprocess the current line with updated values
    }

    currentDepth = depth;
  }

  if (currentKey) {
    jsonObj[currentKey] = convertYAMLtoJSON(currentValue);
  }

  return jsonObj;
}

return {
  Resources,
  Cmdline,
  cmd_args,
  convertYAMLtoJSON,
};
