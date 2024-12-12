var downkeys = {};

function keyname(key)
{
  var str = input.keyname(key);
  return str.toLowerCase();
}

function modstr(mod = input.keymod()) {
  var s = "";
  if (mod.ctrl) s += "C-";
  if (mod.alt) s += "M-";
  if (mod.super) s += "S-";
  return s;
}

prosperon.key_down = function key_down(e) {
  downkeys[e.key] = true;
  var emacs = modstr(e.mod) + keyname(e.key);
  if (e.repeat) player[0].raw_input(emacs, "rep");
  else player[0].raw_input(emacs, "pressed");
};

prosperon.quit = function()
{
  os.exit(0);
}

prosperon.key_up = function key_up(e) {
  delete downkeys[e.key];
  var emacs = modstr(e.mod) + keyname(e.key);
  player[0].raw_input(emacs, "released");
};

prosperon.drop_file = function (path) {
  player[0].raw_input("drop", "pressed", path);
};

var mousepos = [0, 0];

prosperon.text_input = function (e) {
  player[0].raw_input("char", "pressed", e.text);
};

prosperon.mouse_motion = function (e)
{
  mousepos = e.pos;
  player[0].mouse_input("move", e.pos, e.d_pos);
};
prosperon.mouse_wheel = function mousescroll(e) {
  player[0].mouse_input(modstr() + "scroll", e.scroll);
};

prosperon.mouse_button_down = function(e)
{
  player[0].raw_input(modstr() + e.button, "pressed");  
}

prosperon.mouse_button_up = function(e)
{
  player[0].raw_input(modstr() + e.button, "released");
}

input.mouse = {};
input.mouse.screenpos = function mouse_screenpos() {
  return mousepos.slice();
};
input.mouse.worldpos = function mouse_worldpos() {
  return prosperon.camera.screen2world(mousepos);
};
input.mouse.viewpos = function mouse_viewpos()
{
  var world = input.mouse.worldpos();
  
  return mousepos.slice();
}
input.mouse.disabled = function mouse_disabled() {
  input.mouse_mode(1);
};
input.mouse.normal = function mouse_normal() {
  input.mouse_mode(0);
};
input.mouse.mode = function mouse_mode(m) {
  if (input.mouse.custom[m]) input.cursor_img(input.mouse.custom[m]);
  else input.mouse_cursor(m);
};

input.mouse.set_custom_cursor = function mouse_cursor(img, mode = input.mouse.cursor.default) {
  if (!img) delete input.mouse.custom[mode];
  else {
    input.cursor_img(img);
    input.mouse.custom[mode] = img;
  }
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

input.procdown = function procdown() {
  for (var k in downkeys) player[0].raw_input(keyname(k), "down");

  for (var i in joysticks) {
    var joy = joysticks[i];
    var x = joy.ux - joy.dx;
    var y = joy.uy - joy.dy;
    player[0].joy_input(i, joysticks[i]);
  }
};

input.print_md_kbm = function print_md_kbm(pawn) {
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

input.tabcomplete = function tabcomplete(val, list) {
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
    this.pawns = this.pawns.filter(x => x.inputs);
    for (var pawn of this.pawns.reversed()) {
      var inputs = pawn.inputs;

      if (!inputs[cmd]) {
        if (inputs.block) return;
        continue;
      }

      var fn = null;

      switch (state) {
        case "pressed":
          fn = inputs[cmd];
          break;
        case "rep":
          fn = inputs[cmd].rep ? inputs[cmd] : null;
          break;
        case "released":
          fn = inputs[cmd].released;
          break;
        case "down":
          if (typeof inputs[cmd].down === "function") fn = inputs[cmd].down;
          else if (inputs[cmd].down) fn = inputs[cmd];
      }

      var consumed = false;
      if (typeof fn === "function") {
        fn.call(pawn, ...args);
        consumed = true;
      }
      if (state === "released") inputs.release_post?.call(pawn);
      if (inputs.block) return;
      if (consumed) return;
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
