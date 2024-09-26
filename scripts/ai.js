var ai = {
  race(list) {
    return function (dt) {
      var good = false;
      for (var i = 0; i < list.length; i++) if (list[i].call(this, dt)) good = true;
      return good;
    };
  },

  sequence(list) {
    var i = 0;
    var fn = function (dt) {
      while (i !== list.length) {
        if (list[i].call(this, dt)) i++;
        else return false;
      }
      if (fn.done) fn.done();
      return true;
    };

    fn.restart = function () {
      i = 0;
    };
    return fn;
  },

  parallel(list) {
    return function (dt) {
      var good = true;
      list.forEach(function (x) {
        if (!x.call(this, dt)) good = false;
      }, this);
      return good;
    };
  },

  dofor(secs, fn) {
    return ai.race([ai.wait(secs), fn]);
  },

  wait(secs = 1) {
    var accum = 0;
    return function (dt) {
      accum += dt;
      if (accum >= secs) {
        accum = 0;
        return true;
      }

      return false;
    };
  },
};

return { ai };
