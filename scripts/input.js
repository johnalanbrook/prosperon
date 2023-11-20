var Input = {
  setgame() { cmd(77); },
  setnuke() { cmd(78); },
};

var Mouse = {
  get pos() {
    return cmd(45);
  },

  get worldpos() {
    return screen2world(cmd(45));
  },
  
  disabled() {
    cmd(46, 1);
  },
  
  normal() {
    cmd(46, 0);
  },
};

Mouse.doc = {};
Mouse.doc.pos = "The screen position of the mouse.";
Mouse.doc.worldpos = "The position in the game world of the mouse.";
Mouse.disabled.doc = "Set the mouse to hidden. This locks it to the game and hides it, but still provides movement and click events.";
Mouse.normal.doc = "Set the mouse to show again after hiding.";

var Keys = {
  shift() {
    return cmd(50, 340);// || cmd(50, 344);
  },
  
  ctrl() {
    return cmd(50, 341);// || cmd(50, 344);
  },
  
  alt() {
    return cmd(50, 342);// || cmd(50, 346);
  },

  super() {
    return cmd(50, 343);// || cmd(50, 347);
  },
};

Input.state2str = function(state) {
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

Input.print_pawn_kbm = function(pawn) {
  if (!('inputs' in pawn)) return;
  var str = "";
  for (var key in pawn.inputs) {
    if (!pawn.inputs[key].doc) continue;
    str += `${key} | ${pawn.inputs[key].doc}\n`;
  }
  return str;
};

Input.print_md_kbm = function(pawn) {
  if (!('inputs' in pawn)) return;

  var str = "";
  str += "|control|description|\n|---|---|\n";

  for (var key in pawn.inputs) {
    str += `|${key}|${pawn.inputs[key].doc}|`;
    str += "\n";
  }

  return str;
};

Input.has_bind = function(pawn, bind) {
  return (typeof pawn.inputs?.[bind] === 'function');
};

var Action = {
  add_new(name) {
    var action = Object.create(Action);
    action.name = name;
    action.inputs = [];
    this.actions.push(action);

    return action;
  },
  actions: [],
};

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

      var fn = null;

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
      Log.say(pawn.toString());
  },

  create() {
    var n = Object.create(this);
    n.pawns = [];
    n.gamepads = [];
    this.players.push(n);
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
