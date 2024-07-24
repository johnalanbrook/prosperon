var actor = {};

actor.spawn = function(script, config, callback){
  if (typeof script !== 'string') return undefined;
  var padawan = Object.create(actor);
  use(script, padawan);

  if (typeof config === 'object')
    Object.merge(padawan,config);

  padawan.padawans = [];
  padawan.timers = [];
  padawan.master = this;
  Object.hide(padawan, "master", "timers", "padawans");
  padawan.toString = function() { return script; }  
  check_registers(padawan);
  this.padawans.push(padawan);
  return padawan;
};

actor.tween = function(from,to,time,fn) {
  var stop = tween(from,to,time,fn);
  this.timers.push(stop);
  return stop;
}

actor.spawn.doc = `Create a new actor, using this actor as the master, initializing it with 'script' and with data (as a JSON or Nota file) from 'config'.`;

actor.rm_pawn = function(pawn)
{
  this.padawans.remove(pawn);
}

actor.timers = [];
actor.kill = function(){
  if (this.__dead__) return;
  this.timers.forEach(t => t());
  input.do_uncontrol(this);
  Event.rm_obj(this);
  if (this.master) this.master.rm_pawn(this);
  this.padawans.forEach(p => p.kill());
  this.padawans = [];
  this.__dead__ = true;
  if (typeof this.die === 'function') this.die();
  if (typeof this.stop === 'function') this.stop();
  if (typeof this.garbage === 'function') this.garbage();
};

actor.kill.doc = `Remove this actor and all its padawans from existence.`;

actor.delay = function(fn, seconds) {
  var timers = this.timers;
  var stop = function() {
    timers.remove(stop);
    rm();
  }

  function execute() {
    if (fn) fn();
    if (stop.then) stop.then();
    stop();
  }
  
  stop.remain = seconds;
  stop.seconds = seconds;
  stop.pct = function() { return 1-(stop.remain / stop.seconds); };
  
  function update(dt) {
    stop.remain -= dt;
    if (stop.remain <= 0)
     execute();
  }
  
  var rm = Register.appupdate.register(update);
  
  timers.push(stop);
  return stop;
};

actor.delay.doc = `Call 'fn' after 'seconds' with 'this' set to the actor.`;

actor.padawans = [];

global.app = Object.create(actor);
app.die = function() { os.quit(); }

return {actor, app};
