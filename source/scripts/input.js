
var Input = {
  setgame() { cmd(77); },
  setnuke() { cmd(78); },
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

  raw_input(cmd, state, ...args) {
    for (var pawn of this.pawns.reverse()) {
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

      if (typeof fn === 'function')
        fn.call(pawn, ... args);
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
