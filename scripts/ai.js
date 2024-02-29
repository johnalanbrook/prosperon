var ai = {
  race(list) {
    return function(dt) {
      var good = false;
      list.forEach(function(x) { if (x.call(this,dt)) good = true; }, this);
      return good;
    };
  },
  
  do(times, list) {
    
  },

  sequence(list) {
    var i = 0;
    return function(dt) {
      while (i !== list.length) {
        if (list[i].call(this,dt))
	  i++;
	else
	  return false;
      }
      return true;
    };
  },

  parallel(list) {
    return function(dt) {
      var good = true;
      list.forEach(function(x){ if (!x.call(this,dt)) good = false; },this);
      return good;
    };
  },

  moveto() {
    return function(dt) {
    var dir = this.randomloc.sub(this.pos);
    if (Vector.length(dir) < 10) return true;
    
    this.velocity = Vector.norm(this.randomloc.sub(this.pos)).scale(20);
    return False;
    }
  },

  move() {
    return function(dt) {
      this.velocity = this.left().scale(20);
      return false;
    }
  },

  wait(secs) {
    secs ??= 1;
    var accum = 0;
    return function(dt) {
      accum += dt;
      if (accum >= secs)
        return true;
	
      return false;
    };
  },
};

return {ai};
