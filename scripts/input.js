input.keycodes = {
  32: "space",
  45: "minus",
  256: "escape",
  257: "enter",
  258: "tab",
  259: "backspace",
  260: "insert",
  261: "delete",
  262: "right",
  263: "left",
  264: "down",
  265: "up",
  266: "pgup",
  267: "pgdown",
  268: "home",
  269: "end",
};

input.codekeys = {};
for (var code in input.keycodes) input.codekeys[input.keycodes[code]] = code;

var mod = {
  shift: 0,
  ctrl: 0,
  alt: 0,
  super: 0,
};

/*
released
rep
pressed
down
*/

function keycode(name) {
  return charCodeAt(name);
}

function keyname_extd(key) {
  if (!parseInt(key)) return key;

  if (key > 289 && key < 302) {
    var num = key - 289;
    return `f${num}`;
  }

  if (key >= 320 && key <= 329) {
    var num = key - 320;
    return `kp${num}`;
  }

  if (input.keycodes[key]) return input.keycodes[key];
  if (key >= 32 && key <= 126) return String.fromCharCode(key).lc();

  return undefined;
}

var downkeys = {};

function modstr() {
  var s = "";
  if (mod.ctrl) s += "C-";
  if (mod.alt) s += "M-";
  if (mod.super) s += "S-";
  return s;
}

prosperon.keydown = function (key, repeat) {
  downkeys[key] = true;

  if (key == 341 || key == 345) mod.ctrl = 1;
  else if (key == 342 || key == 346) mod.alt = 1;
  else if (key == 343 || key == 347) mod.super = 1;
  else if (key == 340 || key == 344) mod.shift = 1;
  else {
    var emacs = modstr() + keyname_extd(key);
    if (repeat) player[0].raw_input(emacs, "rep");
    else player[0].raw_input(emacs, "pressed");
  }
};

prosperon.keyup = function (key) {
  delete downkeys[key];

  if (key == 341 || key == 345) mod.ctrl = 0;
  else if (key == 342 || key == 346) mod.alt = 0;
  else if (key == 343 || key == 347) mod.super = 0;
  else if (key == 340 || key == 344) mod.shift = 0;
  else {
    var emacs = modstr() + keyname_extd(key);
    player[0].raw_input(emacs, "released");
  }
};

prosperon.droppedfile = function (path) {
  player[0].raw_input("drop", "pressed", path);
};

var mousepos = [0, 0];

prosperon.textinput = function (c) {
  player[0].raw_input("char", "pressed", c);
};
prosperon.mousemove = function (pos, dx) {
  mousepos = pos;
  mousepos.y = window.size.y - mousepos.y;
  player[0].mouse_input("move", pos, dx);
};
prosperon.mousescroll = function (dx) {
  player[0].mouse_input(modstr() + "scroll", dx);
};
prosperon.mousedown = function (b) {
  player[0].raw_input(modstr() + input.mouse.button[b], "pressed");
  downkeys[input.mouse.button[b]] = true;
};
prosperon.mouseup = function (b) {
  player[0].raw_input(input.mouse.button[b], "released");
  delete downkeys[input.mouse.button[b]];
};

input.mouse = {};
input.mouse.screenpos = function () {
  return mousepos.slice();
};
input.mouse.worldpos = function () {
  return game.camera.view2world(mousepos);
};
input.mouse.disabled = function () {
  input.mouse_mode(1);
};
input.mouse.normal = function () {
  input.mouse_mode(0);
};
input.mouse.mode = function (m) {
  if (input.mouse.custom[m]) input.cursor_img(input.mouse.custom[m]);
  else input.mouse_cursor(m);
};

input.mouse.set_custom_cursor = function (img, mode = input.mouse.cursor.default) {
  if (!img) delete input.mouse.custom[mode];
  else {
    input.cursor_img(img);
    input.mouse.custom[mode] = img;
  }
};

input.mouse.button = {
  /* left, right, middle mouse */ 0: "lm",
  1: "rm",
  2: "mm",
};
input.mouse.custom = [];
input.mouse.cursor = {
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
  no: 10,
};

input.mouse.doc = {};
input.mouse.doc.pos = "The screen position of the mouse.";
input.mouse.doc.worldpos = "The position in the game world of the mouse.";
input.mouse.disabled.doc = "Set the mouse to hidden. This locks it to the game and hides it, but still provides movement and click events.";
input.mouse.normal.doc = "Set the mouse to show again after hiding.";

input.keyboard = {};
input.keyboard.down = function (code) {
  if (typeof code === "number") return downkeys[code];
  if (typeof code === "string") return downkeys[code.uc().charCodeAt()] || downkeys[code.lc().charCodeAt()];
  return undefined;
};

input.state2str = function (state) {
  if (typeof state === "string") return state;
  switch (state) {
    case 0:
      return "down";
    case 1:
      return "pressed";
    case 2:
      return "released";
  }
};

input.print_pawn_kbm = function (pawn) {
  if (!("inputs" in pawn)) return;
  var str = "";
  for (var key in pawn.inputs) {
    if (!pawn.inputs[key].doc) continue;
    str += `${key} | ${pawn.inputs[key].doc}\n`;
  }
  return str;
};

var joysticks = {};

joysticks["wasd"] = {
  uy: "w",
  dy: "s",
  ux: "d",
  dx: "a",
};

input.procdown = function () {
  for (var k in downkeys) player[0].raw_input(keyname_extd(k), "down");

  for (var i in joysticks) {
    var joy = joysticks[i];
    var x = joy.ux - joy.dx;
    var y = joy.uy - joy.dy;
    player[0].joy_input(i, joysticks[i]);
  }
};

input.print_md_kbm = function (pawn) {
  if (!("inputs" in pawn)) return;

  var str = "";
  str += "|control|description|\n|---|---|\n";

  for (var key in pawn.inputs) {
    str += `|${key}|${pawn.inputs[key].doc}|`;
    str += "\n";
  }

  return str;
};

input.has_bind = function (pawn, bind) {
  return typeof pawn.inputs?.[bind] === "function";
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

input.tabcomplete = function (val, list) {
  if (!val) return val;
  list.dofilter(function (x) {
    return x.startsWith(val);
  });

  if (list.length === 1) {
    return list[0];
  }

  var ret = undefined;
  var i = val.length;
  while (!ret && !Object.empty(list)) {
    var char = list[0][i];
    if (
      !list.every(function (x) {
        return x[i] === char;
      })
    )
      ret = list[0].slice(0, i);
    else {
      i++;
      list.dofilter(function (x) {
        return x.length - 1 > i;
      });
    }
  }

  return ret ? ret : val;
};

/* May be a human player; may be an AI player */

/*
  'block' on a pawn's input blocks any input from reaching below for the 
*/

var Player = {
  players: [],
  input(fn, ...args) {
    this.pawns.forEach(x => x[fn]?.(...args));
  },

  mouse_input(type, ...args) {
    for (var pawn of this.pawns.reversed()) {
      if (typeof pawn.inputs?.mouse?.[type] === "function") {
        pawn.inputs.mouse[type].call(pawn, ...args);
        pawn.inputs.post?.call(pawn);
        if (!pawn.inputs.fallthru) return;
      }
    }
  },

  char_input(c) {
    for (var pawn of this.pawns.reversed()) {
      if (typeof pawn.inputs?.char === "function") {
        pawn.inputs.char.call(pawn, c);
        pawn.inputs.post?.call(pawn);
        if (!pawn.inputs.fallthru) return;
      }
    }
  },

  joy_input(name, joystick) {
    for (var pawn of this.pawns.reversed()) {
      if (!pawn.inputs) return;
      if (!pawn.inputs.joystick) return;
      if (!pawn.inputs.joystick[name]) return;

      var x = 0;
      if (input.keyboard.down(joystick.ux)) x++;
      if (input.keyboard.down(joystick.dx)) x--;
      var y = 0;
      if (input.keyboard.down(joystick.uy)) y++;
      if (input.keyboard.down(joystick.dy)) y--;

      pawn.inputs.joystick[name](x, y);
    }
  },

  raw_input(cmd, state, ...args) {
    for (var pawn of this.pawns.reversed()) {
      if (!pawn.inputs) {
        console.error(`pawn no longer has inputs object.`);
        continue;
      }

      var block = pawn.inputs.block;

      if (!pawn.inputs[cmd]) {
        if (pawn.inputs.block) return;
        continue;
      }

      var fn = null;

      switch (state) {
        case "pressed":
          fn = pawn.inputs[cmd];
          break;
        case "rep":
          fn = pawn.inputs[cmd].rep ? pawn.inputs[cmd] : null;
          break;
        case "released":
          fn = pawn.inputs[cmd].released;
          break;
        case "down":
          if (typeof pawn.inputs[cmd].down === "function") fn = pawn.inputs[cmd].down;
          else if (pawn.inputs[cmd].down) fn = pawn.inputs[cmd];
      }

      if (typeof fn === "function") fn.call(pawn, ...args);

      if (!pawn.inputs)
        if (block) return;
        else continue;

      if (state === "released") pawn.inputs.release_post?.call(pawn);

      if (!pawn.inputs.fallthru) return;
      if (pawn.inputs.block) return;
    }
  },

  obj_controlled(obj) {
    for (var p in Player.players) {
      if (p.pawns.contains(obj)) return true;
    }

    return false;
  },

  print_pawns() {
    for (var pawn of this.pawns.reversed()) say(pawn.toString());
  },

  create() {
    var n = Object.create(this);
    n.pawns = [];
    n.gamepads = [];
    this.players.push(n);
    this[this.players.length - 1] = n;
    return n;
  },

  pawns: [],

  control(pawn) {
    if (!pawn.inputs) {
      console.warn(`attempted to control a pawn without any input object.`);
      return;
    }
    this.pawns.push_unique(pawn);
  },

  uncontrol(pawn) {
    this.pawns = this.pawns.filter(x => x !== pawn);
  },
};

input.do_uncontrol = function (pawn) {
  Player.players.forEach(function (p) {
    p.pawns = p.pawns.filter(x => x !== pawn);
  });
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
  player,
};
