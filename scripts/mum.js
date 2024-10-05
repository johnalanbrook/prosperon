globalThis.mum = {};

var panel;

var selected = undefined;

mum.inputs = {};
mum.inputs.lm = function () {
  if (!selected) return;
  if (!selected.action) return;
  selected.action();
};

mum.base = {
  pos: null, // If set, puts the cursor to this position before drawing the element
  offset: [0, 0], // Move x,y to the right and down before drawing
  padding: [0, 0], // Pad inwards after drawing, to prepare for the next element
  font: "fonts/c64.ttf",
  selectable: false,
  selected: false,
  font_size: 16,
  scale: 1,
  angle: 0,
  anchor: [0, 1], // where to draw the item from, relative to the cursor. [0,1] is from the top left corner. [1,0] is from the bottom right
  background_image: null,
  slice: null, // pass to slice an image as a 9 slice. see render.slice9 for its format
  hover: {
    color: Color.red,
  },
  text_shadow: {
    pos: [0, 0],
    color: Color.white,
  },
  border: 0, // Draw a border around the element. For text, an outline.
  overflow: "wrap", // how to deal with overflow from parent element
  wrap: -1,
  text_align: "left" /* left, center, right */,
  shader: null, // Use this shader, instead of the engine provided one
  color: Color.white,
  opacity: 1,
  width: 0,
  height: 0,
  max_width: Infinity,
  max_height: Infinity,
  image_repeat: false,
  image_repeat_offset: [0, 0],
  debug: false /* set to true to draw debug boxes */,
  hide: false,
  child_gap: 0,
  child_layout: 'top2bottom', /* top2bottom, left2right */
  tooltip: null,
  children: [],
};

// data is passed into each function, and various stats are generated
// drawpos: the point to start the drawing from
// wh: an array of [width,height]
// bound: a boundingbox around the drawn UI element
// extent: a boundingbox around the total extents of the element (ie before padding)

function show_debug() {
  return prosperon.debug && mum.debug;
}

var context = mum.base;
var context_stack = [];
var cursor = [0,0];

function computeContainerSize(context)
{
  var sizing = context.sizing.value;
  var content_dim = [0,0];
  var max_child_dim = [0,0];

  if (context.layout === 'left2right') {
    for (var child of context.children) {
      content_dim.x += child.size.x + context.child_gap;
      if (child.size.y > max_child_dim.y)
        max_child_dim.y = child.size.y;
    }

    content_dim.width -= context.child_gap;  // remove extra gap after last child
    content_dim.y = max_child_dim.y;
  } else {
    for (var child of context.children) {
      content_dim.y += child.size.y + context.child_gap;
      if (child.size.x > max_child_dim.x)
        max_child_dim.x = child.size.x;
    }

    content_dim.y -= child_gap;
    content_dim.x = max_child_dim.x;
  }

  content_dim = content_dim.add(context.padding.scale(2));

  var container_size = [0,0];
  if (context.sizing.x.type === 'fit')
    container_size.x = content_dim.x;
  else if (container.sizing.x.type === 'grow') {
  }
    
}

mum.container = function(data, cb) {
  context_stack.push(context);
  data.__proto__ = mum.base;
  data.children = [];
  var container_context = {
    pos:cursor.slice(),
    size:[0,0],
    children:[]
  };
  
  context = data;

  cb();

  computeContainerSize(context);
}

mum.debug = false;

var post = function () {};
var posts = [];

mum.style = mum.base;

var cursor = [0, 0];

var pre = function (data) {
  if (data.hide) return true;
  data.__proto__ = mum.style;

  if (data.pos) cursor = data.pos.slice();
  data.drawpos = cursor.slice().add(data.offset);

  if (data.opacity && data.opacity !== 1) {
    data.color = data.color.slice();
    data.color[3] = data.opacity;
  }

  data.wh = [data.width, data.height];
};

var anchor_calc = function (data) {
  var aa = [0, 1].sub(data.anchor);
  data.drawpos = data.drawpos.add([data.width, data.height]).scale(aa);
};

var end = function (data) {
  cursor = cursor.add(data.padding);
  post(data);
};

mum.list = function (fn, data = {}) {
  if (pre(data)) return;
  var aa = [0, 1].sub(data.anchor);
  cursor = cursor.add([data.width, data.height].scale(aa)).add(data.offset).add(data.padding);

  posts.push(post);
  post = mum.list.post.bind(data);

  if (show_debug())
    render.boundingbox({
      t: cursor.y,
      b: cursor.y - data.height,
      l: cursor.x,
      r: cursor.x + data.width,
    });

  //if (data.background_image) mum.image(null, Object.create(data))
  if (data.background_image) {
    var imgpos = data.pos.slice();
    imgpos.y -= data.height / 2;
    imgpos.x -= data.width / 2;
    var imgscale = [data.width, data.height];
    if (data.slice) render.slice9(game.texture(data.background_image), imgpos, data.slice, imgscale);
    else render.image(game.texture(data.background_image), imgpos, [data.width, data.height]);
  }

  fn();

  data.bb.l -= data.padding.x;
  data.bb.r += data.padding.x;
  data.bb.t += data.padding.y;
  data.bb.b -= data.padding.y;

  if (show_debug()) render.boundingbox(data.bb);

  post = posts.pop();
  end(data);
};

mum.list.post = function (e) {
  cursor.y -= e.bb.t - e.bb.b;
  cursor.y -= e.padding.y;

  if (this.bb) this.bb = bbox.expand(this.bb, e.bb);
  else this.bb = e.bb;
};

mum.label = function (str, data = {}) {
  if (pre(data)) return;

  render.set_font(data.font, data.font_size);

  data.bb = render.text_bb(str, data.scale, -1, cursor);
  data.wh = bbox.towh(data.bb);

  var aa = [0, 1].sub(data.anchor);

  data.drawpos.y -= data.bb.t - cursor.y;
  data.drawpos = data.drawpos.add(data.wh.scale(aa)).add(data.offset);

  data.bb = render.text_bb(str, data.scale, data.wrap, data.drawpos);

  if (data.action && bbox.pointin(data.bb, input.mouse.screenpos())) {
    if (data.hover) {
      data.hover.__proto__ = data;
      data = data.hover;
      selected = data;
    }
  }

  data.bb = render.text(str, data.drawpos, data.scale, data.color, data.wrap);

  if (show_debug()) render.boundingbox(data.bb);

  end(data);
};

mum.image = function (path, data = {}) {
  if (pre(data)) return;
  path ??= data.background_image;
  
  var image = game.texture(path);
  var tex = image.texture;

  if (!data.height)
    if (data.width) data.height = tex.height * (data.width / tex.width);
    else data.height = tex.height;

  if (!data.width)
    if (data.height) data.width = tex.width * (data.height / tex.height);
    else data.height = tex.height;

  if (!data.width) data.width = tex.width;
  if (!data.height) data.height = tex.height;

  var aa = [0, 1].sub(data.anchor);
  data.drawpos = data.drawpos.add(aa.scale([data.width, data.height]));

  if (data.slice) render.slice9(tex, data.drawpos, data.slice, [data.width, data.height]);
  else data.bb = render.image(image, data.drawpos, [data.width, data.height]);

  end(data);
};

mum.rectangle = function (data = {}, cb) {
  if (pre(data)) return;
  var aa = [0, 0].sub(data.anchor);
  data.drawpos = data.drawpos.add(aa.scale([data.width, data.height]));

  render.rectangle(data.drawpos, data.drawpos.add([data.width, data.height]), data.color);

  end(data);
};

var btnbb;
var btnpost = function () {
  btnbb = data.bb;
};

mum.button = function (str, data = { padding: [4, 4], color: Color.black }) {
  if (pre(data)) return;
  posts.push(post);
  post = btnpost;
  if (typeof str === "string") render.text(str, cursor.add(data.padding), data.scale, data.color);
  else str();

  if (data.action && data.hover && bbox.pointin(btnbb, input.mouse.screenpos())) {
    data.hover.__proto__ = data;
    data = data.hover;
  }
  render.rectangle([btnbb.l - data.padding.x, btnbb.b - data.padding.y], [btnbb.r + data.padding.y, btnbb.t + data.padding.y], data.color);
  data.bb = btnbb;

  post = posts.pop();
  end(data);
};

mum.window = function (fn, data = {}) {
  if (pre(data)) return;

  render.rectangle(cursor, cursor.add(data.size), data.color);
  cursor.y += data.height;
  cursor = cursor.add(data.padding);
  fn();
  end(data);
};

mum.ex_hud = function () {
  mum.label("TOP LEFT", { pos: [0, game.size.y], anchor: [0, 1] });
  mum.label("BOTTOM LEFT", { pos: [0, 0], anchor: [0, 0] });
  mum.label("TOP RIGHT", { pos: game.size, anchor: [1, 1] });
  mum.label("BOTTOM RIGHT", { pos: [game.size.x, 0], anchor: [1, 0] });
};

mum.drawinput = undefined;
var ptext = "";
var panpan = {
  draw() {
    mum.rectangle({
      pos: [0, 0],
      anchor: [0, 0],
      height: 20,
      width: window.size.x,
      padding: [10, 16],
      color: Color.black,
    });
    mum.label("input level: ");
    mum.label(ptext, { offset: [50, 0], color: Color.red });
  },
  inputs: {
    block: true,
    char(c) {
      ptext += c;
    },
    enter() {
      delete mum.drawinput;
      player[0].uncontrol(panpan);
    },
    escape() {
      delete mum.drawinput;
      player[0].uncontrol(panpan);
    },
    backspace() {
      ptext = ptext.slice(0, ptext.length - 1);
    },
  },
};

mum.textinput = function (fn, str = "") {
  mum.drawinput = panpan.draw;
  ptext = str;
  player[0].control(panpan);
  panpan.inputs.enter = function () {
    fn(ptext);
    delete mum.drawinput;
    player[0].uncontrol(panpan);
  };
};
