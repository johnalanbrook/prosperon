var keycodes = {
  259: "back",
  258: "tab",
  257: "enter",
  1: "escape",
  32: "space",
  266: "pgup",    
  267: "pgdown",
  268: "home",
  269: "end",
  263: "left",
  265: "up",
  262: "right",
  265: "down",
  260: "insert",
  261: "delete",
  45: "minus",
};

var codekeys = {};
for (var code in keycodes)
  codekeys[keycodes[code]] = code;
  
var mod = {
  shift: 0,
  ctrl: 0,
  alt: 0,
  super: 0
};

/*
released
rep
pressed
pressrep
down
*/

function keyname_extd(key)
{
  if (key > 289 && key < 302) {
    var num = key-289;
    return `f${num}`;
  }
  
  if (key >= 320 && key <= 329) {
    var num = key-320;
    return `kp${num}`;
  }
  
  if (keycodes[key]) return keycodes[key];
  if (key >= 32 && key <= 126) return String.fromCharCode(key).lc();
  
  return undefined;
}

prosperon.keys = [];

function modstr()
{
  var s = "";
  if (mod.ctrl) s += "C-";
  if (mod.alt) s += "M-";
  if (mod.super) s += "S-";
  return s;
}

prosperon.keydown = function(key, repeat)
{
  prosperon.keys[key] = true;
  
  if (key == 341 || key == 345)
    mod.ctrl = 1;
  else if (key == 342 || key == 346)
    mod.alt = 1;
  else if (key == 343 || key == 347)
    mod.super = 1;
  else if (key == 340 || key == 344)
    mod.shift = 1;
  else {
    var emacs = modstr() + keyname_extd(key);
    player[0].raw_input(emacs, "pressrep");
    if (repeat)
      player[0].raw_input(emacs, "rep");
    else
      player[0].raw_input(emacs, "pressed");
  }
}

prosperon.keyup = function(key)
{
  prosperon.keys[key] = false;
  if (key == 341 || key == 345)
    mod.ctrl = 0;
  else if (key == 342 || key == 346)
    mod.alt = 0;
  else if (key == 343 || key == 347)
    mod.super = 0;
  else if (key == 340 || key == 344)
    mod.shift = 0;
  else {
    var emacs = modstr() + keyname_extd(key);
    player[0].raw_input(emacs, "released");
  }
}

prosperon.droppedfile = function(path)
{
  player[0].raw_input("drop", "pressed", path);
}

var mousepos = [0,0];

prosperon.textinput = function(){};
prosperon.mousemove = function(pos, dx){
  mousepos = pos;
  player[0].mouse_input(modstr() + "move", pos, dx);
};
prosperon.mousescroll = function(dx){
  player[0].mouse_input(modstr() + "scroll", dx);
};
prosperon.mousedown = function(b){
  player[0].raw_input(modstr() + Mouse.button[b], "pressed");
};
prosperon.mouseup = function(b){
  player[0].raw_input(modstr() + Mouse.button[b], "released");
};

var Mouse = {
  screenpos() { return mousepos.slice(); },
  worldpos() { return Window.screen2world(mousepos); },
  disabled() { cmd(46, 1); },
  normal() { cmd(46, 0);},

  mode(m) {
    if (Mouse.custom[m])
      cmd(97, Mouse.custom[m]);
    else
      cmd(17, m);
  },
  
  set_custom_cursor(img, mode) {
    mode ??= Mouse.cursor.default;
    if (!img)
      delete Mouse.custom[mode];
    else {
      cmd(97, img);
      Mouse.custom[mode] = img;
    }
  },
  
  button: { /* left, right, middle mouse */
    0: "lm", 
    1: "rm",
    2: "mm"
  },
  custom:[],
  cursor: {
    default: 0,
    arrow: 1,
    ibeam: 2,
    cross: 3,
    hand: 4,
    ew: 5,
    ns: 6,
    nwse: 7,
    nesw: 8,
    resize: 9,
    no: 10
  },
};

Mouse.doc = {};
Mouse.doc.pos = "The screen position of the mouse.";
Mouse.doc.worldpos = "The position in the game world of the mouse.";
Mouse.disabled.doc = "Set the mouse to hidden. This locks it to the game and hides it, but still provides movement and click events.";
Mouse.normal.doc = "Set the mouse to show again after hiding.";

var Keys = {
  down(code) {
    return prosperon.keys[code];
  },
};

var input = {};

input.state2str = function(state) {
  if (typeof state === 'string') return state;
  switch (state) {
    case 0:
      return "down";
    case 1:
      return "pressed";
    case 2:
      return "released";
  }
}

input.print_pawn_kbm = function(pawn) {
  if (!('inputs' in pawn)) return;
  var str = "";
  for (var key in pawn.inputs) {
    if (!pawn.inputs[key].doc) continue;
    str += `${key} | ${pawn.inputs[key].doc}\n`;
  }
  return str;
};

input.print_md_kbm = function(pawn) {
  if (!('inputs' in pawn)) return;

  var str = "";
  str += "|control|description|\n|---|---|\n";

  for (var key in pawn.inputs) {
    str += `|${key}|${pawn.inputs[key].doc}|`;
    str += "\n";
  }

  return str;
};

input.has_bind = function(pawn, bind) {
  return (typeof pawn.inputs?.[bind] === 'function');
};

input.action = {
  add_new(name) {
    var action = Object.create(input.action);
    action.name = name;
    action.inputs = [];
    this.actions.push(action);

    return action;
  },
  actions: [],
};

input.keyboard_show = function(show)
{
  cmd(250,show); 
}

input.keyboard_shown = function() { return cmd(248); }

/* May be a human player; may be an AI player */
var Player = {
  players: [],
  input(fn, ...args) {
    this.pawns.forEach(x => x[fn]?.(...args));
  },

  mouse_input(type, ...args) {
    for (var pawn of this.pawns.reversed()) {
      if (typeof pawn.inputs?.mouse?.[type] === 'function') {
        pawn.inputs.mouse[type].call(pawn,...args);
	      pawn.inputs.post?.call(pawn);
	      if (!pawn.inputs.fallthru)
          return;
      }
    }
  },

  char_input(c) {
    for (var pawn of this.pawns.reversed()) {
      if (typeof pawn.inputs?.char === 'function') {
        pawn.inputs.char.call(pawn, c);
	      pawn.inputs.post?.call(pawn);
	      if (!pawn.inputs.fallthru)	
	      return;
      }
    };
  },

  raw_input(cmd, state, ...args) {
    for (var pawn of this.pawns.reversed()) {
      if (typeof pawn.inputs?.any === 'function') {
        pawn.inputs.any(cmd);
        
	      if (!pawn.inputs.fallthru)	
          return;
      }

      if (!pawn.inputs?.[cmd]) {
        if (pawn.inputs?.block) return;
      	continue;
      }

      var   fn = null;

      switch (state) {
        case 'pressed':
	        fn = pawn.inputs[cmd];
	        break;
      	case 'rep':
    	    fn = pawn.inputs[cmd].rep ? pawn.inputs[cmd] : null;
	        break;
      	case 'released':
      	  fn = pawn.inputs[cmd].released;
    	    break;
      	case 'down':
    	   fn = pawn.inputs[cmd].down;
      }

      if (typeof fn === 'function') {
        fn.call(pawn, ... args);
      	pawn.inputs.post?.call(pawn);
      }

      switch (state) {
        case 'released':
	      pawn.inputs.release_post?.call(pawn);
	      break;
      }

      if (!pawn.inputs.fallthru) return;
      if (pawn.inputs.block) return;      
    }
  },
  
  do_uncontrol(pawn) {
    this.players.forEach(function(p) {
      p.pawns = p.pawns.filter(x => x !== pawn);
    });
  },

  obj_controlled(obj) {
    for (var p in Player.players) {
      if (p.pawns.contains(obj))
        return true;
    }

    return false;
  },

  print_pawns() {
    for (var pawn of this.pawns.reversed())
      say(pawn.toString());
  },

  create() {
    var n = Object.create(this);
    n.pawns = [];
    n.gamepads = [];
    this.players.push(n);
    this[this.players.length-1] = n;
    return n;
  },

  pawns: [],

  control(pawn) {
    this.pawns.push_unique(pawn);
  },

  uncontrol(pawn) {
    this.pawns = this.pawns.filter(x => x !== pawn);
  },
};

for (var i = 0; i < 4; i++) {
  Player.create();
}

Player.control.doc = "Control a provided object, if the object has an 'inputs' object.";
Player.uncontrol.doc = "Uncontrol a previously controlled object.";
Player.print_pawns.doc = "Print out a list of the current pawn control stack.";
Player.doc = {};
Player.doc.players = "A list of current players.";

var player = Player;

return {
  Mouse,
  Keys,
  input,
  Player,
  player,
  keycodes,
  codekeys
};
