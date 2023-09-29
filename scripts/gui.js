var GUI = {
  text(str, pos, size, color, wrap) {
    size ??= 1;
    color ??= Color.white;
    wrap ??= -1;

    var bb = cmd(118, str, size, wrap);
    var opos = [bb.r, bb.t];
        
    var h = ui_text(str, pos, size, color, wrap);

    return bb;
  },

  text_cursor(str, pos, size, cursor) {
    cursor_text(str,pos,size,Color.white,cursor);
  },

  image(path,pos) {
    var wh = cmd(64,path);
    gui_img(path,pos, [1.0,1.0], 0.0, 0.0, [0.0,0.0], 0.0, Color.black);
    return cwh2bb([0,0], wh);
  },

  image_fn(defn) {
    var def = Object.create(this.defaults);
    Object.assign(def,defn);
    if (!def.path) {
      Log.warn("GUI image needs a path.");
      def.draw = function(){};
      return def;
    }

    var tex_wh = cmd(64,def.path);
    var wh = tex_wh.slice();

    if (def.width !== 0)
      wh.x = def.width;

    if (def.height !== 0)
      wh.y = def.height;

    wh = wh.scale(def.scale);

    var sendscale = [];
    sendscale.x = wh.x / tex_wh.x;
    sendscale.y = wh.y / tex_wh.y;

    def.draw = function(pos) {
      def.calc_bb(pos);
      gui_img(def.path, pos.sub(def.anchor.scale(wh)), sendscale, def.angle, def.image_repeat, def.image_repeat_offset, def.color);
    };

    def.calc_bb = function(cursor) {
      def.bb = cwh2bb(wh.scale([0.5,0.5]), wh);
      def.bb = movebb(def.bb, cursor.sub(wh.scale(def.anchor)));
    };

    return def;
  },

  defaults: {
    padding:[2,2], /* Each element inset with this padding on all sides */
    font: "fonts/LessPerfectDOSVGA.ttf",
    font_size: 1,
    text_align: "left",
    scale: 1,
    angle: 0,
    anchor: [0,0],
    text_shadow: {
      pos: [0,0],
      color: Color.white,
    },
    text_outline: 1, /* outline in pixels */
    color: Color.white,
    margin: [5,5], /* Distance between elements for things like columns */
    width: 0,
    height: 0,
    image_repeat: false,
    image_repeat_offset: [0,0],
    debug: false, /* set to true to draw debug boxes */
  },

  text_fn(str, defn)
  {
    var def = Object.create(this.defaults);
    Object.assign(def,defn);
    
    def.draw = function(cursor) {
      def.calc_bb(cursor);

      if (def.debug)
        Debug.boundingbox(def.bb, def.debug_colors.bounds);
      
      var old = def;
      def = Object.create(def);

/*      if (pointinbb(def.bb, Mouse.screenpos)) {
        Object.assign(def, def.hovered);
	def.calc_bb(cursor);
	GUI.selected = def;
	def.selected = true;
      }
*/
      if (def.selected) {
        Object.assign(def, def.hovered);
	def.calc_bb(cursor);
      }

      var pos = cursor.sub(bb2wh(def.bb).scale(def.anchor));

      ui_text(str, pos, def.font_size, def.color, def.width);

      def = old;
    };

    def.calc_bb = function(cursor) {
      var bb = cmd(118, str, def.font_size, def.width);
      var wh = bb2wh(bb);
      var pos = cursor.sub(wh.scale(def.anchor));
      def.bb = movebb(bb,pos);
    };

    return def;
  },

  column(defn) {
    var def = Object.create(this.defaults);
    Object.assign(def,defn);

    if (!def.items) {
      Log.warn("Columns needs items.");
      def.draw = function(){};
      return def;
    };

    def.items.forEach(function(item,idx) {
      def.items[idx].__proto__ = def;
      if (def.items[idx-1])
        def.up = def.items[idx-1];

      if (def.items[idx+1])
        def.down = def.items[idx+1];
    });

    def.draw = function(pos) {
        def.items.forEach(function(item) {
	  item.draw.call(this,pos);
          var wh = bb2wh(item.bb);
          pos.y -= wh.y;
          pos.y -= def.padding.x*2;
        });
    };

    return def;
  },

  input_lmouse_pressed() {
    if (GUI.selected)
      GUI.selected.action();
  },

  input_s_pressed() {
    if (GUI.selected?.down) {
      GUI.selected.selected = false;
      GUI.selected = GUI.selected.down;
      GUI.selected.selected = true;
    }
  },

  input_w_pressed() {
    if (GUI.selected?.up) {
      GUI.selected.selected = false;
      GUI.selected = GUI.selected.up;
      GUI.selected.selected = true;
    }
  },

  input_enter_pressed() {
    if (GUI.selected) {
      GUI.selected.action();
    }
  }
};

GUI.defaults.debug_colors = {
  bounds: Color.red.slice(),
  margin: Color.blue.slice(),
  padding: Color.green.slice()
};

Object.values(GUI.defaults.debug_colors).forEach(function(v) { v.a = 100; });


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

var Tween = {
  default: {
    loop: "restart", /* none, restart, yoyo, circle */ 
    time: 1, /* seconds to do */
    ease: Ease.linear,
    whole: true,
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
      defn.pct = (defn.accum % defn.time) / defn.time;

      var t = defn.whole ? defn.ease(defn.pct) : defn.pct;

      var nval = t / slicelen;
      var i = Math.trunc(nval);
      nval -= i;

      if (!defn.whole)
        nval = defn.ease(nval);

      obj[target] = tvals[i].lerp(tvals[i+1], nval);
    };

    defn.restart = function() { defn.accum = 0; };
    defn.stop = function() { defn.pause(); defn.restart(); };
    defn.pause = function() { Register.update.unregister(defn.fn); };

    Register.update.register(defn.fn, defn);

    return defn;
  },

  embed(obj, target, tvals, options) {
    var defn = Object.create(this.default);
    Object.assign(defn, options);

    defn.update_vals = function(vals) {
      defn.vals = vals;
      
      if (defn.loop === 'circle')
        defn.vals.push(defn.vals[0]);
      else if (defn.loop === 'yoyo') {
        for (var i = defn.vals.length-2; i >= 0; i--)
          defn.vals.push(defn.vals[i]);
      }

      defn.slices = defn.vals.length - 1;
      defn.slicelen = 1 / defn.slices;
    };

    defn.update_vals(tvals);

    defn.time_s = Date.now();

    Object.defineProperty(obj, target, {
      get() {
        defn.accum = (Date.now() - defn.time_s)/1000;
	defn.pct = (defn.accum % defn.time) / defn.time;
	var t = defn.whole ? defn.ease(defn.pct) : defn.pct;

	var nval = t / defn.slicelen;
	var i = Math.trunc(nval);
	nval -= i;

	if (!defn.whole)
	  nval = defn.ease(nval);

	return defn.vals[i].lerp(defn.vals[i+1],nval);
      },
    });

    return defn;
  },
};
