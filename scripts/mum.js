globalThis.mum = {};

var panel;

var selected = undefined;

mum.inputs = {};
mum.inputs.lm = function()
{
  if (!selected) return;
  if (!selected.action) return;
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
  scale: 1,
  angle: 0,
  anchor: [0,1],
  background_image: null,
  slice: null,
  hovered: {},
  text_shadow: {
    pos: [0,0],
    color: Color.white,
  },
  text_outline: 1, /* outline in pixels */
  text_align: "left", /* left, center, right */
  text_shader: null,
  color: Color.white,
  margin: [0,0], /* Distance between elements for things like columns */
  size: null,
  max_width: Infinity,
  max_height: Infinity,
  image_repeat: false,
  image_repeat_offset: [0,0],
  debug: false, /* set to true to draw debug boxes */
  hide: false,
  tooltip: null,
}

mum.debug = false;

var post = function() {};
var posts = [];

var context = mum.base;
var contexts = [];

var cursor = [0,0];

var pre = function(data)
{
  if (data.hide) return true;
  data.__proto__ = mum.base;
  if (context)
    contexts.push(context);
    
  context = data;
}

var end = function()
{
  var old = context;
  context = contexts.pop();
  post(old);
}

mum.list = function(fn, data = {})
{
  if (pre(data)) return;
  cursor = context.pos;
  cursor = cursor.add(context.offset);
  posts.push(post);
  post = mum.list.post;
  
  fn();

  context.bb.l -= context.padding.x;
  context.bb.r += context.padding.x;
  context.bb.t += context.padding.y;
  context.bb.b -= context.padding.y;

  if (mum.debug)
    render.boundingbox(context.bb);

  post = posts.pop();
  end();
}

mum.list.post = function(e)
{
  cursor.y -= (e.bb.t - e.bb.b);
  cursor.y -= e.padding.y;
  
  if (context.bb)
    context.bb = bbox.expand(context.bb,e.bb)
  else
    context.bb = e.bb;
}

mum.label = function(str, data = {})
{
  if (pre(data)) return;

  render.set_font(context.font, context.font_size);

  context.bb = render.text_bb(str, context.scale, -1, cursor);
  
  if (mum.debug)
    render.boundingbox(context.bb);

  if (bbox.pointin(context.bb, input.mouse.screenpos())) {
    if (context.hover) {
      context.hover.__proto__ = context;
      context = context.hover;
      selected = context;
    }
  }

  context.bb = render.text(str, cursor, context.scale, context.color);
  
  end();
}

mum.image = function(path, data = {})
{
  if (pre(data)) return;
  
  var tex = game.texture(path);
  if (context.slice)
    render.slice9(tex, cursor, context.slice, context.scale);
  else
    context.bb = render.image(tex, cursor, context.scale);
  
  end();
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
    render.text(str, cursor.add(context.padding), context.scale, context.color);
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
    size:[400,400],
    color: Color.black
  }, data);

  if (pre(data)) return;

  cursor = context.pos;
  render.rectangle(cursor, cursor.add(context.size), context.color);
  cursor.y += context.height;
  cursor = cursor.add(context.padding);
  context.pos = cursor.slice();
  fn();
  end();
}

mum.div = function(fn, data = {})
{
  if (pre(data)) return;
  fn();
  end();
}
