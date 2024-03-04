/*
  GUI functions take screen space coordinates
*/

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
  
  image(path,pos,color) {
    color ??= Color.black;
    var wh = cmd(64,path);
    gui_img(path,pos, [1.0,1.0], 0.0, false, [0.0,0.0], Color.white);
    return bbox.fromcwh([0,0], wh);
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

GUI.controls = {};
GUI.controls.toString = function() { return "GUI controls"; };
GUI.controls.update = function() { };

GUI.controls.set_mum = function(mum)
{
  mum.selected = true;
  
  if (this.selected && this.selected !== mum)
    this.selected.selected = false;

  this.selected = mum;
}
GUI.controls.check_bb = function(mum)
{
  if (bbox.pointin(mum.bb, Mouse.pos))
    GUI.controls.set_mum(mum);
}
GUI.controls.inputs = {};
GUI.controls.inputs.fallthru = false;
GUI.controls.inputs.mouse = {};
GUI.controls.inputs.mouse.move = function(pos,dpos)
{
}
GUI.controls.inputs.mouse.scroll = function(scroll)
{
}

GUI.controls.check_submit = function() {
  if (this.selected && this.selected.action)
    this.selected.action(this.selected);
}

var Mum = {
    padding:[0,0], /* Each element inset with this padding on all sides */
    offset:[0,0],
    font: "fonts/LessPerfectDOSVGA.ttf",
    selectable: false,
    selected: false,
    font_size: 1,
    text_align: "left", /* left, center, right */
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
    margin: [0,0], /* Distance between elements for things like columns */
    width: 0,
    height: 0,
    max_width: Infinity,
    max_height: Infinity,
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
    var fn = function(def) {
      var p = n.make(def);
      p.prestart();
      p.start();
      return p;
    };
    fn._int = n;
    return fn;
  },
}

Mum.text = Mum.extend({
  draw(cursor, cnt) {
    cursor ??= [0,0];  
    cnt ??= Mum;
    if (this.hide) return;
    if (this.selectable) GUI.controls.check_bb(this);
    this.caret ??= -1;

/*    if (!this.bb)
      this.calc_bb(cursor);
    else
      this.update_bb(cursor);
*/
    var params = this.selected ? this.hovered : this;

    this.width = Math.min(params.max_width, cnt.max_width);

    this.calc_bb(cursor);
    this.height = this.wh.y;
    var aa = [0,1].sub(params.anchor);
    var pos = cursor.add(params.wh.scale(aa)).add(params.offset);
    cmd(263, params.font);
    ui_text(params.str, pos, params.font_size, params.color, this.width, params.caret);
  },
  
  update_bb(cursor) {
    this.bb = bbox.move(this.bb, cursor.sub(this.wh.scale(this.anchor)));
  },
  
  calc_bb(cursor) {
    var bb = cmd(118,this.str, this.font_size, this.width);
    this.wh = bbox.towh(bb);
    var pos = cursor.add(this.wh.scale([0,1].sub(this.anchor))).add(this.offset);    
    this.bb = bbox.move(bb,pos.add([this.wh.x/2,0]));
  },
  start() {
    this.calc_bb([0,0]);
  },
});

Mum.button = Mum.text._int.extend({
 selectable: true,
 color: Color.blue,
 hovered:{
   color: Color.red
 },
 action() { console.warn("Button has no action."); },
});

Mum.window = Mum.extend({
  start() {
    this.wh = [this.width, this.height];
    this.bb = bbox.fromcwh([0,0], this.wh);
  },
  draw(cursor, cnt) {
    cursor ??= [0,0];
    cnt ??= Mum;
    var p = cursor.sub(this.wh.scale(this.anchor)).add(this.padding);    
    GUI.window(p,this.wh, this.color);
    this.bb = bbox.blwh(p, this.wh);
    GUI.flush();
    GUI.scissor(p.x,p.y,this.wh.x,this.wh.y);
    this.max_width = this.width;
    if (this.selectable) GUI.controls.check_bb(this);
    var pos = [this.bb.l, this.bb.t].add(this.padding);
    this.items.forEach(function(item) {
      if (item.hide) return;
      item.draw(pos.slice(),this);
    }, this);
    GUI.flush();
    GUI.scissor_win();
  },
});

Mum.image = Mum.extend({
  start() {
    if (!this.path) {
      console.warn("Mum image needs a path.");
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
    this.bb = bbox.fromcwh(this.wh.scale([0.5,0.5]), wh);
    this.bb = bbox.move(this.bb, pos.sub(this.wh.scale(this.anchor)));
  }
});

Mum.column = Mum.extend({
  draw(cursor, cnt) {
    cursor ??= [0,0];  
    cnt ??= Mum;
    if (this.hide) return;
    cursor = cursor.add(this.offset);
    this.max_width = cnt.width;
    
    this.items.forEach(function(item) {
      if (item.hide) return;
      item.draw(cursor, this);
      cursor.y -= item.height;
      cursor.y -= this.padding.y;
    }, this);
  },
});

GUI.window = function(pos, wh, color)
{
  var p = pos.slice();
  p.x += wh.x/2;
  p.y += wh.y/2;
  render.box(p,wh,color);
}

GUI.flush = function() { cmd(141); };

Mum.debug_colors = {
  bounds: Color.red.slice(),
  margin: Color.blue.slice(),
  padding: Color.green.slice()
};

Object.values(Mum.debug_colors).forEach(function(v) { v.a = 100; });

return {
  GUI,
  Mum
};
