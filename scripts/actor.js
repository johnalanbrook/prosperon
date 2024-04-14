var actor = {};

actor.spawn = function(script, config, callback){
  if (typeof script !== 'string') return undefined;
  console.info(`spawning actor with script ${script}`);
  var padawan = Object.create(actor);
  use(script, padawan);

  if (typeof config === 'object')
    Object.merge(padawan,config);

  padawan.padawans = [];
  padawan.timers = [];
  padawan.master = this;
  Object.hide(padawan, "master", "timers", "padawans");
  check_registers(padawan);
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
  this.timers.forEach(t => t());
  input.do_uncontrol(this);
  Event.rm_obj(this);
  if (this.master) this.master.rm_pawn(this);
  this.padawans.forEach(p => p.kill());
  this.padawans = [];
  this.__dead__ = true;
  if (typeof this.die === 'function') this.die();
  if (typeof this.stop === 'function') this.stop();
};

actor.kill.doc = `Remove this actor and all its padawans from existence.`;

actor.interval = function(fn, seconds) {
  var cur;
  var stop = function() {
    cur();
  }
  
  var f = function() {
    fn.call(this);
    cur = this.delay(f,seconds);
  }
  
  cur = this.delay(f,seconds);
  
  return stop;
}

actor.delay = function(fn, seconds) {
  var timers = this.timers;
  var stop = function() {
    timers.remove(stop);
    rm();
  }

  function execute() {
    fn();
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
