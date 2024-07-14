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
  pos: null,
  font: "fonts/c64.ttf",
  selectable: false,
  selected: false,
  font_size: 16,
  scale: 1,
  angle: 0,
  anchor: [0,1], // where to draw the item from, relative to the cursor.
  background_image: null,
  slice: null,
  hover: {
    color: Color.red,
  },
  text_shadow: {
    pos: [0,0],
    color: Color.white,
  },
  text_outline: 1, /* outline in pixels */
  text_align: "left", /* left, center, right */
  text_shader: null,
  overflow: "wrap", // clip, wrap, break
  color: Color.white,
  margin: [0,0], /* Distance between elements for things like columns */
  size: null,
  width:0,
  height:0,
  max_width: Infinity,
  max_height: Infinity,
  image_repeat: false,
  image_repeat_offset: [0,0],
  debug: false, /* set to true to draw debug boxes */
  hide: false,
  tooltip: null,
}

function show_debug() { return prosperon.debug && mum.debug; }

mum.debug = false;

var post = function() {};
var posts = [];

mum.style = mum.base;

var context = mum.style;
var container = undefined;
var contexts = [];

var cursor = [0,0];

var pre = function(data)
{
  if (data.hide) return true;
  data.__proto__ = mum.style;
  if (context)
    contexts.push(context);
    
  context = data;
  if (context.pos) cursor = context.pos.slice();
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
  var aa = [0,1].sub(context.anchor);
  if (context.pos) cursor = context.pos.slice();
  cursor = cursor.add([context.width,context.height].scale(aa)).add(context.offset).add(context.padding);

  posts.push(post);
  post = mum.list.post;
  
  if (show_debug())
    render.boundingbox({
      t:cursor.y,
      b:cursor.y-context.height,
      l:cursor.x,
      r:cursor.x+context.width
    });
    
  cursor.x += context.width/2;
    
  //if (context.background_image) mum.image(null, Object.create(context)) 
  if (context.background_image) {
    var imgpos = context.pos.slice();
    imgpos.y -= context.height/2;
    imgpos.x -= context.width/2;
    var imgscale = [context.width,context.height];
    if (context.slice)
      render.slice9(game.texture(context.background_image), imgpos, context.slice, imgscale);
    else
      render.image(game.texture(context.background_image), imgpos, [context.width,context.height]);
  }
    
  fn();
  
  context.bb.l -= context.padding.x;
  context.bb.r += context.padding.x;
  context.bb.t += context.padding.y;
  context.bb.b -= context.padding.y;

  if (show_debug())
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
  context.wh = bbox.towh(context.bb);
  
  var aa = [0,1].sub(context.anchor);  
  
  var pos = cursor.slice();  
  pos.y -= (context.bb.t-cursor.y);
  pos = pos.add(context.wh.scale(aa)).add(context.offset);

  context.bb = render.text_bb(str, context.scale, -1, pos);

  if (context.action && bbox.pointin(context.bb, input.mouse.screenpos())) {
    if (context.hover) {
      context.hover.__proto__ = context;
      context = context.hover;
      selected = context;
    }
  }

  context.bb = render.text(str, pos, context.scale, context.color);
    
  if (show_debug())
    render.boundingbox(context.bb);
  
  end();
}

mum.image = function(path, data = {})
{
  if (pre(data)) return;
  path ??= context.background_image;
  var tex = path;
  if (typeof path === 'string')
    tex = game.texture(path);
  
  if (context.slice)
    render.slice9(tex, cursor, context.slice, context.scale);
  else {
    cursor.y -= tex.height*context.scale;
    context.bb = render.image(tex, cursor, [context.scale*tex.width, context.scale*tex.height]);
  }
  
  end();
}

var btnbb;
var btnpost = function()
{
  btnbb = context.bb;
}

mum.button = function(str, data = {padding:[4,4], color:Color.black})
{
  if (pre(data)) return;
  posts.push(post);
  post = btnpost;
  if (typeof str === 'string')
    render.text(str, cursor.add(context.padding), context.scale, context.color);
  else
    str();

  if (data.action && data.hover && bbox.pointin(btnbb, input.mouse.screenpos())) {
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

  render.rectangle(cursor, cursor.add(context.size), context.color);
  cursor.y += context.height;
  cursor = cursor.add(context.padding);
  context.pos = cursor.slice();
  fn();
  end();
}

mum.ex_hud = function()
{
  mum.label("TOP LEFT", {pos:[0,game.size.y], anchor:[0,1]});
  mum.label("BOTTOM LEFT", {pos:[0,0], anchor:[0,0]});
  mum.label("TOP RIGHT", {pos:game.size, anchor:[1,1]});
  mum.label("BOTTOM RIGHT", {pos:[game.size.x, 0], anchor:[1,0]});
}
