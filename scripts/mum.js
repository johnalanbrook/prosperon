globalThis.mum = {};
var panel;

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

var listpost = function()
{
  var height = 0;
  if (context.height) height += context.height;
  else height += (context.bb.t - context.bb.b);
  cursor.y -= height;
  cursor.y -= context.padding.y; 
}

var pre = function(data)
{
  if (data.hide || context.hide) return true;
  data.__proto__ = context;
  contexts.push(context);
  context = data;
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

mum.button = function(str, data = {padding:[4,4]})
{
  if (pre(data)) return;
  var bb = render.text(str, cursor.add(context.padding), context.size, context.color);
  render.rectangle([bb.l-context.padding.x, bb.b-context.padding.y], [bb.r+context.padding.y, bb.t+context.padding.y], Color.black);
  context.bb = bb;
  end();
}

mum.label = function(str, data = {})
{
  if (pre(data)) return;
  render.set_font(data.font, data.font_size);
  context.bb = render.text(str, cursor, context.size, context.color);
  
  end();
}