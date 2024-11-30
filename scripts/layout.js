// Layout code
// Contain is for how it will treat its children. If they should be laid out as a row, or column, or in a flex style, etc.

var lay_ctx = layout.make_context();

var clay_base = {
  font: undefined,
  background_image: undefined,
  slice: 0,
  font: 'smalle.16',
  font_size: undefined,
  color: [1,1,1,1],
  spacing:0,
  padding:0,
  margin:0,
  offset:[0,0],
  size:undefined,
};

var root_item;
var root_config;
var boxes = [];
globalThis.clay = {};

clay.normalizeSpacing = function normalizeSpacing(spacing) {
  if (typeof spacing === 'number') {
    return {l: spacing, r: spacing, t: spacing, b: spacing};
  } else if (Array.isArray(spacing)) {
    if (spacing.length === 2) {
      return {l: spacing[0], r: spacing[0], t: spacing[1], b: spacing[1]};
    } else if (spacing.length === 4) {
      return {l: spacing[0], r: spacing[1], t: spacing[2], b: spacing[3]};
    }
  } else if (typeof spacing === 'object') {
    return {l: spacing.l || 0, r: spacing.r || 0, t: spacing.t || 0, b: spacing.b || 0};
  } else {
    return {l:0, r:0, t:0, b:0};
  }
}

clay.draw = function draw(size, fn)
{
  lay_ctx.reset();
  boxes = [];
  var root = lay_ctx.item();
  lay_ctx.set_size(root,size);
  lay_ctx.set_contain(root,layout.contain.row);
  root_item = root;
  root_config = Object.assign({}, clay_base);
  boxes.push({
    id:root,
    config:root_config
  });
  fn();
  lay_ctx.run();

  // Adjust bounding boxes for padding
  for (var i = 0; i < boxes.length; i++) {
    var box = boxes[i];
    box.content = lay_ctx.get_rect(box.id);
    box.boundingbox = Object.assign({}, box.content);

    var padding = clay.normalizeSpacing(box.config.padding || 0);
    if (padding.l || padding.r || padding.t || padding.b) {
      // Adjust the boundingbox to include the padding
      box.boundingbox.x -= padding.l;
      box.boundingbox.y -= padding.t;
      box.boundingbox.width += padding.l + padding.r;
      box.boundingbox.height += padding.t + padding.b;
    }
    box.marginbox = Object.assign({}, box.content);
    var margin = clay.normalizeSpacing(box.config.margin || 0);
    box.marginbox.x -= margin.l;
    box.marginbox.y -= margin.t;
    box.marginbox.width += margin.l+margin.r;
    box.marginbox.height += margin.t+margin.b;
    box.content.anchor_y = 1;
    box.boundingbox.anchor_y = 1;
  }

  return boxes;
}

function create_view_fn(base_config)
{
  var base = Object.assign(Object.create(clay_base), base_config);
  return function view(config = {}, fn) {
    config.__proto__ = base;
    var item = add_item(config);

    var prev_item = root_item;
    var prev_config = root_config;
    root_item = item;
    root_config = config;
    root_config._childIndex = 0; // Initialize child index
    fn?.();
    root_item = prev_item;
    root_config = prev_config;
  }
}

clay.vstack = create_view_fn({
  contain: layout.contain.column | layout.contain.start,
});

clay.hstack = create_view_fn({
  contain: layout.contain.row | layout.contain.start,
});

clay.spacer = create_view_fn({
  behave: layout.behave.hfill | layout.behave.vfill
});

function image_size(img)
{
  return [img.rect[2]*img.texture.width, img.rect[3]*img.texture.height]; 
}

function add_item(config)
{
  // Normalize the child's margin
  var margin = clay.normalizeSpacing(config.margin || 0);
  var padding = clay.normalizeSpacing(config.padding || 0);
  var childGap = root_config.child_gap || 0;

  // Adjust for child_gap
  if (root_config._childIndex > 0) {
    var parentContain = root_config.contain || 0;
    var isVStack = (parentContain & layout.contain.column) !== 0;
    var isHStack = (parentContain & layout.contain.row) !== 0;

    if (isVStack) {
      margin.t += childGap;
    } else if (isHStack) {
      margin.l += childGap;
    }
  }

  var use_config = Object.create(config);

  use_config.margin = {
    t: margin.t+padding.t,
    b: margin.b+padding.b,
    r:margin.r+padding.r,
    l:margin.l+padding.l
  };

  var item = lay_ctx.item();
  lay_ctx.set_margins(item, use_config.margin);
  lay_ctx.set_size(item,use_config.size);
  lay_ctx.set_contain(item,use_config.contain);
  lay_ctx.set_behave(item,use_config.behave);
  boxes.push({
    id:item,
    config:use_config
  });
  lay_ctx.insert(root_item,item);

  // Increment the parent's child index
  root_config._childIndex++;
  return item;
}

function rectify_configs(config_array)
{
  if (config_array.length === 0)
    config_array = [{}];

  for (var i = config_array.length-1; i > 0; i--)
    config_array[i].__proto__ = config_array[i-1];

  config_array[0].__proto__ = clay_base;
  var cleanobj = Object.create(config_array[config_array.length-1]);

  return cleanobj;
}

clay.image = function image(path, ...configs)
{
  var config = rectify_configs(configs);
  var image = game.texture(path);
  config.image = image;  
  config.size ??= [image.texture.width, image.texture.height];
  add_item(config);
}

clay.text = function text(str, ...configs)
{
  var config = rectify_configs(configs);
  var tsize = render.text_size(str, config.font);
  config.size ??= [0,0];
  config.size = config.size.map((x,i) => Math.max(x, tsize[i]));
  config.text = str;
  add_item(config);
}

/*
  For a given size,
  the layout engine should "see" size + margin
  but its interior content should "see" size - padding
  hence, the layout box should be size-padding, with margin of margin+padding
*/

var button_base = Object.assign(Object.create(clay_base), {
  padding:0,
  hovered:{
  }
});
clay.button = function button(str, action, config = {})
{
  config.__proto__ = button_base;
  config.size = render.text_size(str,config.font);
  add_item(config);
  config.text = str;
  config.action = action;
}

var hovered = undefined;
layout.newframe = function() { hovered = undefined; }
layout.draw_commands = function draw_commands(cmds, pos = [0,0], mousepos)
{
  for (var cmd of cmds) {
    var boundingbox = geometry.rect_move(cmd.boundingbox,pos);
//    boundingbox.x -= boundingbox.width*pos.anchor_x;
//    boundingbox.y += boundingbox.height*pos.anchor_y;
    var content = geometry.rect_move(cmd.content,pos);
//    content.x -= content.width*pos.anchor_x;
//    content.y += content.height*pos.anchor_y;
    var config = cmd.config;

    if (config.hovered && geometry.rect_point_inside(boundingbox, mousepos)) {
      config.hovered.__proto__ = config;
      config = config.hovered;
      hovered = config;
    }

    if (config.background_image)
      if (config.slice)
        render.slice9(config.background_image, boundingbox, config.slice, config.background_color);
      else
        render.image(config.background_image, boundingbox, 0, config.color);
    else if (config.background_color)
      render.rectangle(boundingbox, config.background_color);
      
    if (config.text)
      render.text(config.text, content, config.font, config.font_size, config.color);
    if (config.image)
      render.image(config.image, content, 0, config.color);
  }
}

var dbg_colors = {};
layout.debug_colors = dbg_colors;
dbg_colors.content = [1,0,0,0.1];
dbg_colors.boundingbox = [0,1,0,0,0.1];
dbg_colors.margin = [0,0,1,0.1];
layout.draw_debug = function draw_debug(cmds, pos = [0,0])
{
  for (var i = 0; i < cmds.length; i++) {
    var cmd = cmds[i];
    var boundingbox = geometry.rect_move(cmd.boundingbox,pos);
//    boundingbox.x -= boundingbox.width*pos.anchor_x;
//    boundingbox.y += boundingbox.height*pos.anchor_y;
    var content = geometry.rect_move(cmd.content,pos);
//    content.x -= content.width*pos.anchor_x;
//    content.y += content.height*pos.anchor_y;
    render.rectangle(content, dbg_colors.content);
    render.rectangle(boundingbox, dbg_colors.boundingbox);
//    render.rectangle(geometry.rect_move(cmd.marginbox,pos), dbg_colors.margin);
  }
}

layout.inputs = {};
layout.inputs.lm = function()
{
  if (hovered && hovered.action) hovered.action();
}

layout.toString = _ => "layout"

return layout;

