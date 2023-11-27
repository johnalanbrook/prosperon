var AI = {
  tick() {
    
  },

  sequence(list) {
    
  },
  
  race(list) {
    return function() {
      var good = false;
      list.forEach(x => if (x()) good = true);
      return good;
    };
  },
  
  do(times, list) {
    
  },
  
  sync(list) {
    return function() {
      var good = true;
      list.forEach(x => if (!x()) good = false);
      return good;
    };
  },

  moveto(pos) {
    return function() {
    var dir = pos.sub(this.pos);
    if (Vector.length(dir) < 10) return true;
    
    this.velocity = Vector.normalize(pos.sub(this.pos)).scale(20);
    return false;
    }
  },

  wait(secs) {
    secs ??= 1;
    var accum = 0;
    return function() {
      accum += Game.dt;
      if (accum >= secs)
        return true;
	
      return false;
    };
  },
};
