var actor = {};
var a_db = {};

actor.spawn = function(script, config){
  if (typeof script !== 'string') return undefined;
  if (!a_db[script]) a_db[script] = io.slurp(script);
  var padawan = Object.create(actor);
  eval_env(a_db[script], padawan, script);

  if (typeof config === 'object')
    Object.merge(padawan,config);

  padawan.padawans = [];
  padawan.timers = [];
  padawan.master = this;
  Object.hide(padawan, "master","timers", "padawans");
  this.padawans.push(padawan);
  return padawan;
};

actor.spawn.doc = `Create a new actor, using this actor as the master, initializing it with 'script' and with data (as a JSON or Nota file) from 'config'.`;

actor.rm_pawn = function(pawn)
{
  this.padawans.remove(pawn);
}

actor.timers = [];
actor.kill = function(){
  if (this.__dead__) return;
  this.timers.forEach(t => t.kill());
  if (this.master) this.master.rm_pawn(this);
  this.padawans.forEach(p => p.kill());
  this.padawans = [];
  this.__dead__ = true;
  if (typeof this.die === 'function') this.die();
  if (typeof this.stop === 'function') this.stop();
};

actor.kill.doc = `Remove this actor and all its padawans from existence.`;

actor.delay = function(fn, seconds) {
  var t = Object.create(timer);
  t.remain = seconds;
  t.kill = () => {
    timer.kill.call(t);
    delete this.timers[t.toString()];
  }
  t.fire = () => {
    if (this.__dead__) return;
    fn();
    t.kill();
  };
  Register.appupdate.register(t.update, t);
  this.timers.push(t);
  return function() { t.kill(); };
};

actor.delay.doc = `Call 'fn' after 'seconds' with 'this' set to the actor.`;

actor.padawans = [];

global.app = Object.create(actor);

app.die = function()
{
  Game.quit();
}

return {actor, app};
