var Input = {
  setgame() { cmd(77); },
  setnuke() { cmd(78); },
};

var Mouse = {
  get pos() {
    return cmd(45);
  },

  get screenpos() {
    var p = this.pos;
    p.y = Window.dimensions.y - p.y;
    return p;
  },

  get worldpos() {
    return screen2world(cmd(45));
  },
  
  disabled() {
    cmd(46, 1);
  },
  
  hidden() {
    cmd(46, 1);
  },
  
  normal() {
    cmd(46, 0);
  },
};

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
	return;
      }
    }
  },

  char_input(c) {
    for (var pawn of this.pawns.reversed()) {
      if (typeof pawn.inputs?.char === 'function') {
        pawn.inputs.char.call(pawn, c);
	pawn.inputs.post?.call(pawn);
	return;
      }
    };
  },

  raw_input(cmd, state, ...args) {
    for (var pawn of this.pawns.reversed()) {
      if (typeof pawn.inputs?.any === 'function') {
        pawn.inputs.any(cmd);
        return;
      }
      if (!pawn.inputs?.[cmd]) continue;

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
	return;
      }
    }
  },
  
  uncontrol(pawn) {
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

  create() {
    var n = Object.create(this);
    n.pawns = [];
    n.gamepads = [];
    n.control = function(pawn) { n.pawns.push_unique(pawn); };
    n.uncontrol = function(pawn) { n.pawns = n.pawns.filter(x => x !== pawn); };
    this.players.push(n);
    return n;
  },
};

for (var i = 0; i < 4; i++) {
  Player.create();
}
