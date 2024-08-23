/* Take numbers from 0 to 1 and remap them to easing functions */
var Ease = {
  linear(t) { return t; },

  in(t) { return t*t; },

  out(t) {
    var d = 1-t;
    return 1 - d*d
  },

  inout(t) {
    var d = -2*t + 2;
    return t < 0.5 ? 2 * t * t : 1 - (d * d) / 2;
  },
};


function make_easing_fns(num) {
  var obj = {};

  obj.in = function(t) {
    return Math.pow(t,num);
  };

  obj.out = function(t) {
    return 1 - Math.pow(1 - t, num);
  };

  var mult = Math.pow(2, num-1);

  obj.inout = function(t) {
    return t < 0.5 ? mult * Math.pow(t, num) : 1 - Math.pow(-2 * t + 2, num) / 2;
  };

  return obj;
};

Ease.quad = make_easing_fns(2);
Ease.cubic = make_easing_fns(3);
Ease.quart = make_easing_fns(4);
Ease.quint = make_easing_fns(5);

Ease.expo = {
  in(t) {
    return t === 0 ? 0 : Math.pow(2, 10 * t - 10);
  },

  out(t) {
    return t === 1 ? 1 : 1 - Math.pow(2, -10 * t);
  },

  inout(t) {
    return t === 0 ? 0 : t === 1 ? 1 : t < 0.5 ? Math.pow(2, 20 * t - 10) / 2 : (2 - Math.pow(2, -20 * t + 10)) / 2;
  }
};

Ease.bounce = {
  in(t) {
    return 1 - this.out(t - 1);
  },

  out(t) {
    var n1 = 7.5625;
    var d1 = 2.75;

    if (t < 1 / d1) { return n1 * t * t; }
    else if (t < 2 / d1) { return n1 * (t -= 1.5 / d1) * t + 0.75; }
    else if (t < 2.5 / d1) { return n1 * (t -= 2.25 / d1) * t + 0.9375; }
    else
      return n1 * (t -= 2.625 / d1) * t + 0.984375;
  },

  inout(t) {
    return t < 0.5 ? (1 - this.out(1 - 2 * t)) / 2 : (1 + this.out(2 * t - 1)) / 2;
  }
};

Ease.sine = {
  in(t) { return 1 - Math.cos((t * Math.PI)/2); },

  out(t) { return Math.sin((t*Math.PI)/2); },

  inout(t) { return -(Math.cos(Math.PI*t) - 1) / 2; }
};

Ease.elastic = {
  in(t) {
    return t === 0 ? 0 : t === 1 ? 1 : -Math.pow(2, 10*t-10) * Math.sin((t * 10 - 10.75) * this.c4);
  },

  out(t) {
    return t === 0 ? 0 : t === 1 ? 1 : Math.pow(2, -10*t) * Math.sin((t * 10 - 0.75) * this.c4) + 1;
  },

  inout(t) {
    t === 0 ? 0 : t === 1 ? 1 : t < 0.5 ?
      -(Math.pow(2, 20 * t - 10) * Math.sin((20 * t - 11.125) * this.c5)) / 2
      : (Math.pow(2, -20 * t + 10) * Math.sin((20 * t - 11.125) * this.c5)) / 2 + 1;
  },
};

Ease.elastic.c4 = 2*Math.PI/3;
Ease.elastic.c5 = 2*Math.PI / 4.5;

var tween = function(from, to, time, fn, endfn)
{
  var start = profile.secs(profile.now());
  var update = function(dt) {
    profile.frame("tween");
    var elapsed = profile.secs(profile.now()) - start;
    fn(from.lerp(to,elapsed/time));
    if (elapsed >= time) {
      fn(to);
      if (stop.then) stop.then();
      stop();
      endfn?.();
    }
    profile.endframe();
  };
  var stop = Register.update.register(update);
  return stop;
}

var Tween = {
  default: {
    loop: "hold",
    /*
      loop types
      none: when done, return to first value
      hold: hold last value of tween
      restart: restart at beginning, looping
      yoyo: go up and then back down
      circle: go up and back down, looped
    */
    time: 1, /* seconds to do */
    ease: Ease.linear,
    whole: true, /* True if time is for the entire tween, false if each stage */
    cb: function(){},
  },

  start(obj, target, tvals, options)
  {
    var defn = Object.create(this.default);
    Object.assign(defn, options);

    if (defn.loop === 'circle')
      tvals.push(tvals[0]);
    else if (defn.loop === 'yoyo') {
      for (var i = tvals.length-2; i >= 0; i--)
        tvals.push(tvals[i]);
    }

    defn.accum = 0;

    var slices = tvals.length - 1;
    var slicelen = 1 / slices;

    defn.fn = function(dt) {
      defn.accum += dt;
      if (defn.accum >= defn.time && defn.loop === 'hold') {
        if (typeof target === 'string')
          obj[target] = tvals[tvals.length-1];
        else
          target(tvals[tvals.length-1]);
	  
        defn.pause();
	      defn.cb.call(obj);
	      return;
      }

      defn.pct = (defn.accum % defn.time) / defn.time;
      if (defn.loop === 'none' && defn.accum >= defn.time)
        defn.stop();

      var t = defn.whole ? defn.ease(defn.pct) : defn.pct;

      var nval = t / slicelen;
      var i = Math.trunc(nval);
      nval -= i;

      if (!defn.whole)
        nval = defn.ease(nval);

      if (typeof target === 'string')
        obj[target] = tvals[i].lerp(tvals[i+1], nval);
      else
        target(tvals[i].lerp(tvals[i+1],nval));
    };

    var playing = false;
    
    defn.play = function() {
      if (playing) return;
      defn._end = Register.update.register(defn.fn.bind(defn));
      playing = true;
    };
    defn.restart = function() {
      defn.accum = 0;
      if (typeof target === 'string')
        obj[target] = tvals[0];
      else
        target(tvals[0]);
    };
    defn.stop = function() { if (!playing) return; defn.pause(); defn.restart(); };
    defn.pause = function() {
      defn._end();
      if (!playing) return;

      playing = false;
    };

    return defn;
  },
};

Tween.make = Tween.start;

return {Tween, Ease, tween};
