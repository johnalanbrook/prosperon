function compile_env(str, env, file)
{
  file ??= "unknown";
  return cmd(123, str, env, file);
}

function fcompile_env(file, env)
{
  return compile_env(IO.slurp(file), env, file);
}

var OS = {
  get cwd() { return cmd(144); },
};
OS.exec = function(s)
{
  cmd(143, s);
}

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
    Log.write(msg);
    Log.write('\n');
  },

  console(msg) {
    cmd(142, msg + '\n');
  },    

  stack(skip = 0) {
    var stack = (new Error()).stack;
    var n = stack.next('\n',0)+1;
    for (var i = 0; i < skip; i++)
      n = stack.next('\n', n)+1;

    this.write(stack.slice(n));
  },
};

var IO = {
  exists(file) { return cmd(65, file);},
  slurp(file) {
    if (IO.exists(file))
      return cmd(38,file);
    else
      throw new Error(`File ${file} does not exist; can't slurp`);
  },
  slurpwrite(str, file) { return cmd(39, str, file); },
  extensions(ext) {
    var paths = IO.ls();
    paths = paths.filter(function(str) { return str.ext() === ext; });
    return paths;
  },
  ls() { return cmd(66); },
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


Cmdline.register_cmd("p", function() { Game.edit = false; }, "Launch engine in play mode.");
Cmdline.register_cmd("v", function() { Log.say(cmd(120)); Game.quit(); }, "Display engine info.");
Cmdline.register_cmd("c", function() {}, "Redirect logging to console.");
Cmdline.register_cmd("l", function(n) {
  Log.level = n;
}, "Set log level.");
Cmdline.register_cmd("h", function(str) {
  for (var cmd of Cmdline.cmds) {
    Log.say(`-${cmd.flag}:  ${cmd.doc}`);
  }

  Game.quit();
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
  Game.quit();
}, "Pack the game into the given name.");

Cmdline.register_cmd("e", function(pawn) {
  run("scripts/editor.js");
  eval(`Log.write(Input.print_md_kbm(${pawn}));`);
  Game.quit();
}, "Print input documentation for a given object in a markdown table." );

Cmdline.register_cmd("t", function() {
  Log.warn("Testing not implemented yet.");
  Game.quit();
}, "Test suite.");

