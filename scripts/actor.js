var actor = {};

var actor_urs = {};

var actor_spawns = {};

function class_eval(script)
{
  var file = Resources.find_script(script);

  if (!file) {
    var ret = Object.create(base);
    if (callback) callback(ret);
    return ret;
  }

  if (!actor_urs[file]) {
    var newur = Object.create(base);
    actor_urs[file] = newur;
    actor_spawns[file] = [];
  }

  var padawan = Object.create(actor_urs[file]);
  actor_spawns[file].push(padawan);
  padawan._file = file;
  padawan._root = file.dir();

  if (callback) callback(padawan);

  var script = Resources.replstrs(file);
  script = `(function use_${file.name()}() { var self = this; var $ = this.__proto__; ${script}; })`;

  var fn = os.eval(file, script);
  return {
    usefn: fn,
    file: file,
    parent: actor_urs[file]
  };
}.hashify();

globalThis.class_use = function class_use(script, config, base, callback) {
  var prog = class_eval(script);
  var padawan = Object.create(prog.parent);
  actor_spawns[file].push(padawan);
  padawan._file = prog.file;
  padawan._root = prog.file.dir();

  if (callback) callback(padawan);
  prog.fn.call(padawan);

  if (typeof config === "object") Object.merge(padawan, config);

  return padawan;
};

globalThis.rmactor = function (e) {
  if (!actor_spawns[e._file]) return;
  actor_spawns[e._file].remove(e);
};

actor.__stats = function () {
  var total = 0;
  var stats = {};
  game.all_objects(obj => {
    if (!actor_spawns[obj._file]) return;
    stats[obj._file] ??= 0;
    stats[obj._file]++;
    total++;
  });
/*  for (var i in actor_spawns) {
    stats[i] = actor_spawns[i].length;
    total += stats[i];
  }*/
//  stats.total = total;
  return stats;
};

actor.hotreload = function hotreload(file) {
  var script = Resources.replstrs(file);
  script = `(function() { var self = this; var $ = this.__proto__;${script};})`;
  var fn = os.eval(file, script);

  for (var obj of actor_spawns[file]) {
    var a = obj;
    a.timers.forEachRight(t=>t());
    a.timers = [];
    var save = json.decode(json.encode(a));
    fn.call(a);
    Object.merge(a, save);
    check_registers(a);
  }
};

actor.spawn = function (script, config) {
  if (typeof script !== "string") return undefined;

  var padawan = class_use(script, config, actor);

  padawan.padawans = [];
  padawan.timers = [];
  padawan.master = this;
  Object.hide(padawan, "master", "padawans");
  padawan.toString = function () {
    return script;
  };
  check_registers(padawan);
  this.padawans.push(padawan);
  if (padawan.awake) padawan.awake();
  return padawan;
};

actor.tween = function (from, to, time, fn) {
  var stop = tween(from, to, time, fn);
  this.timers.push(stop);
  return stop;
};

actor.spawn.doc = `Create a new actor, using this actor as the master, initializing it with 'script' and with data (as a JSON or Nota file) from 'config'.`;

actor.rm_pawn = function (pawn) {
  this.padawans.remove(pawn);
};

actor.timers = [];
actor.kill = function () {
  if (this.__dead__) return;
  this.timers.forEach(t => t());
  input.do_uncontrol(this);
  Event.rm_obj(this);
  if (this.master) this.master.rm_pawn(this);
  this.padawans.forEach(p => p.kill());
  this.padawans = [];
  this.__dead__ = true;
  actor_spawns[this._file].remove(this);
  if (typeof this.die === "function") this.die();
  if (typeof this.stop === "function") this.stop();
  if (typeof this.garbage === "function") this.garbage();
  if (typeof this.then === "function") this.then();
};

actor.kill.doc = `Remove this actor and all its padawans from existence.`;

actor.delay = function (fn, seconds) { prosperon.add_timer(this, fn, seconds); }
actor.delay.doc = `Call 'fn' after 'seconds' with 'this' set to the actor.`;

actor.interval = function (fn, seconds) {
  var self = this;
  var stop;
  var usefn = function () {
    fn();
    stop = self.delay(usefn, seconds);
  };
  stop = self.delay(usefn, seconds);

  return stop;
};

actor.padawans = [];

global.app = Object.create(actor);
app.die = function () {
  os.exit(0);
};

return { actor, app };
