var GUI = {
  text(str, pos, size, color, wrap, anchor, cursor) {
    size ??= 1;
    color ??= Color.white;
    wrap ??= -1;
    anchor ??= [0,1];

    cursor ??= -1;

    var bb = cmd(118, str, size, wrap);
    var w = bb.r*2;
    var h = bb.t*2;

    //ui_text draws with an anchor on top left corner
    var p = pos.slice();
    p.x -= w * anchor.x;
    bb.r += (w*anchor.x);
    bb.l += (w*anchor.x);
    p.y += h * (1 - anchor.y);
    bb.t += h*(1-anchor.y);
    bb.b += h*(1-anchor.y);
    ui_text(str, p, size, color, wrap, cursor);

    return bb;
  },

  scissor(x,y,w,h) {
    cmd(140,x,y,w,h);
  },
  
  scissor_win() { cmd(140,0,0,Window.width,Window.height); },
  
  image(path,pos) {
    var wh = cmd(64,path);
    gui_img(path,pos, [1.0,1.0], 0.0, 0.0, [0.0,0.0], 0.0, Color.black);
    return cwh2bb([0,0], wh);
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

var gui_controls = {};
gui_controls.update = function() { };

gui_controls.options = [];
gui_controls.inputs = {};
gui_controls.inputs.fallthru = false;
gui_controls.inputs.mouse = {};
gui_controls.inputs.mouse.move = function(pos,dpos)
{
  var newsel = undefined;
  this.options.forEach(function(x) {
    if (pointinbb(x.bb,pos)) {
      newsel = x;
      return;
    }
  });

  if (this.selected && this.selected !== newsel)
    this.selected.selected = false;

  this.selected = newsel;
  if (this.selected)
    this.selected.selected = true;
}

gui_controls.inputs.lm = function() {
  if (this.selected) {
    Log.warn(this.selected.str);
    this.selected.action(this.selected);
  }
};

var Mum = {
    padding:[0,0], /* Each element inset with this padding on all sides */
    offset:[0,0],
    font: "fonts/LessPerfectDOSVGA.ttf",
    selectable: false,
    selected: false,
    font_size: 1,
    text_align: "left",
    scale: 1,
    angle: 0,
    anchor: [0,1],
    hovered: {},
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
    
  make(def) {
    var n = Object.create(this);
    Object.assign(n, def);
    return n;
  },

  prestart() {
    this.hovered.__proto__ = this;
  },

  start() {},

  extend(def) {
    var n = Object.create(this);
    Object.assign(n, def);
    return function(def) {
      var p = n.make(def);
      p.prestart();
      p.start();
      return p;
    };
  },
}

Mum.text = Mum.extend({
  draw(cursor) {
    if (this.hide) return;
    if (this.selectable) gui_controls.options.push_unique(this);
    this.caret ??= -1;

/*    if (!this.bb)
      this.calc_bb(cursor);
    else
      this.update_bb(cursor);
*/
    var params = this.selected ? this.hovered : this;

    this.calc_bb(cursor);
    var aa = [0,1].sub(params.anchor);
    var pos = cursor.add(params.wh.scale(aa)).add(params.offset);
    ui_text(params.str, pos, params.font_size, params.color, params.width, params.caret);
  },
  
  update_bb(cursor) {
    this.bb = movebb(this.bb, cursor.sub(this.wh.scale(this.anchor)));
  },
  
  calc_bb(cursor) {
    var bb = cmd(118,this.str, this.font_size, this.width);
    this.wh = bb2wh(bb);
    var pos = cursor.add(this.wh.scale([0,1].sub(this.anchor))).add(this.offset);    
    this.bb = movebb(bb,pos.add([this.wh.x/2,0]));
  },
  start() {
    this.calc_bb([0,0]);
  },
});

Mum.window = Mum.extend({
  start() {
    this.wh = [this.width, this.height];
    this.bb = cwh2bb([0,0], this.wh);
  },
  draw(pos) {
    var p = pos.sub(this.wh.scale(this.anchor));
    GUI.window(p,this.wh, this.color);
    this.bb = bl2bb(p, this.wh);
    var pos = [this.bb.l, this.bb.t];
    GUI.flush();
    GUI.scissor(p.x,p.y,this.wh.x,this.wh.y);
    this.items.forEach(function(item) {
      if (item.hide) return;
      item.draw(pos.slice());
    });
    GUI.flush();
    GUI.scissor_win();
  },
});

Mum.image = Mum.extend({
  start() {
    if (!this.path) {
      Log.warn("Mum image needs a path.");
      this.draw = function(){};
      return;
    }

    var tex_wh = cmd(64, this.path);
    this.wh = tex_wh.slice();
    if (this.width !== 0) this.wh.x = this.width;
    if (this.height !== 0) this.wh.y = this.height;

    this.wh = wh.scale(this.scale);
    this.sendscale = [wh.x/tex_wh.x, wh.y/tex_wh.y];
  },
  
  draw(pos) {
    this.calc_bb(pos);
    gui_img(this.path, pos.sub(this.anchor.scale([this.width, this.height])), this.sendscale, this.angle, this.image_repeat, this.image_repeat_offset, this.color);
  },

  calc_bb(pos) {
    this.bb = cwh2bb(this.wh.scale([0.5,0.5]), wh);
    this.bb = movebb(this.bb, pos.sub(this.wh.scale(this.anchor)));
  }
});

Mum.column = Mum.extend({
  draw(cursor) {
    if (this.hide) return;
    cursor = cursor.add(this.offset);
    
    this.items.forEach(function(item) {
      if (item.hide) return;
      item.draw(cursor);
      cursor.y -= item.wh.y;
      cursor.y -= this.padding.y;
    }, this);
  },
});

GUI.window = function(pos, wh, color)
{
  var p = pos.slice();
  p.x += wh.x/2;
  p.y += wh.y/2;
  Debug.box(p,wh,color);
}

GUI.flush = function() { cmd(141); };

Mum.debug_colors = {
  bounds: Color.red.slice(),
  margin: Color.blue.slice(),
  padding: Color.green.slice()
};

Object.values(Mum.debug_colors).forEach(function(v) { v.a = 100; });

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
