globalThis.mum = {};
var panel;

var selected = undefined;

mum.inputs = {};
mum.inputs.lm = function()
{
  if (!selected) return;
  selected.action();
}

mum.base = {
  padding:[0,0], /* Each element inset with this padding on all sides */
  offset:[0,0],
  pos: [0,0],
  font: "fonts/c64.ttf",
  selectable: false,
  selected: false,
  font_size: 16,
  text_align: "left", /* left, center, right */
  scale: 1,
  angle: 0,
  anchor: [0,1],
  background_image: null,
  hovered: {},
  text_shadow: {
    pos: [0,0],
    color: Color.white,
  },
  text_outline: 1, /* outline in pixels */
  color: Color.white,
  margin: [0,0], /* Distance between elements for things like columns */
  width: null,
  height: null,
  max_width: Infinity,
  max_height: Infinity,
  image_repeat: false,
  image_repeat_offset: [0,0],
  debug: false, /* set to true to draw debug boxes */
  hide: false,
  tooltip: null,
}

var post = function() {};
var posts = [];

var context = mum.base;
var contexts = [];

var cursor = [0,0];

var end = function()
{
  post();
  context = contexts.pop();
  if (!context) context = mum.base;
}

var pre = function(data)
{
  if (data.hide || context.hide) return true;
  data.__proto__ = context;
  contexts.push(context);
  context = data;
}

var listpost = function()
{
  var height = 0;
  height += (context.bb.t - context.bb.b);
  cursor.y -= height;
  cursor.y -= context.padding.y; 
}

mum.list = function(fn, data = {})
{
  if (pre(data)) return;
  
  cursor = context.pos;
  cursor = cursor.add(context.offset);
  posts.push(post);
  post = listpost;
  
  fn();
  post = posts.pop();
  end();
}

mum.image = function(path, data = {})
{
  if (pre(data)) return;
  
  var tex = game.texture(path);
  context.bb = render.image(tex, cursor, context.size);
  
  end();
}

mum.slice9 = function(path, data = {})
{
}

var btnbb;
var btnpost = function()
{
  btnbb = context.bb;
}

mum.button = function(str, data = {padding:[4,4], color:Color.black, hover:{color:Color.red}})
{
  if (pre(data)) return;
  posts.push(post);
  post = btnpost;
  if (typeof str === 'string')
    render.text(str, cursor.add(context.padding), context.size, context.color);
  else
    str();

  if (data.hover && bbox.pointin(btnbb, input.mouse.screenpos())) {
    data.hover.__proto__ = data;
    context = data.hover;
  }
  render.rectangle([btnbb.l-context.padding.x, btnbb.b-context.padding.y], [btnbb.r+context.padding.y, btnbb.t+context.padding.y], context.color);
  context.bb = btnbb;

  post = posts.pop();
  end();
}

mum.window = function(fn, data = {})
{
  data = Object.assign({
    width:400,
    height:400,
    color: Color.black
  }, data);

  if (pre(data)) return;

  cursor = context.pos;
  render.rectangle(cursor, cursor.add([context.width,context.height]), context.color);
  cursor.y += context.height;
  cursor = cursor.add(context.padding);
  context.pos = cursor.slice();
  fn();
  end();
}

mum.label = function(str, data = {})
{
  if (pre(data)) return;
  if (false) {
    context.hover.__proto__ = context;
    context = context.hover;
  }

  context.bb = render.text_bb(str, context.size, -1, cursor);

  if (bbox.pointin(context.bb, input.mouse.screenpos())) {
    if (context.hover) {
      context.hover.__proto__ = context;
      context = context.hover;
      selected = context;
    }
  }

  render.set_font(context.font, context.font_size);
  context.bb = render.text(str, cursor, context.size, context.color);
  
  end();
}

mum.div = function(fn, data = {})
{
  if (pre(data)) return;
  fn();
  end();
}
