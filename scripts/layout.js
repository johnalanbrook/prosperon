// Layout code
// Contain is for how it will treat its children. If they should be laid out as a row, or column, or in a flex style, etc.

var lay_ctx = layout.make_context();
layout.contain = {};
layout.contain.row = 0x002;
layout.contain.column = 0x003;
layout.contain.layout = 0x000;
layout.contain.flex = 0x002;
layout.contain.nowrap = 0x000;
layout.contain.wrap = 0x004;
layout.contain.start = 0x008;
layout.contain.middle = 0x000;
layout.contain.end = 0x010;
layout.contain.justify = 0x018;

// Behave is for how it behaves to its parent. How it should be aligned, directions it should fill, etc.
layout.behave = {};
layout.behave.left = 0x020;
layout.behave.top = 0x040;
layout.behave.right = 0x080;
layout.behave.bottom = 0x100;
layout.behave.hfill = 0x0a0;
layout.behave.vfill = 0x140;
layout.behave.hcenter = 0x000;
layout.behave.vcenter = 0x000;
layout.behave.center = 0x000;
layout.behave.fill = 0x1e0;
layout.behave.break = 0x200;

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

clay.normalizeSpacing = function(spacing) {
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

clay.draw = function(size, fn, )
{
  lay_ctx.reset();
  boxes = [];
  var root = lay_ctx.item({
    size:size,
    contain: layout.contain.row,
  });
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
    box.content.y *= -1;
    box.boundingbox.y *= -1
    box.content.anchor_y = 1;
    box.boundingbox.anchor_y = 1;
  }

  return boxes;
}

function create_view_fn(base_config)
{
  var base = Object.assign(Object.create(clay_base), base_config);
  return function(config = {}, fn) {
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

var add_item = function(config)
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

  var item = lay_ctx.item(use_config);  
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

clay.image = function(path, ...configs)
{
  var config = rectify_configs(configs);
  var image = game.texture(path);
  config.image = path;  
  config.size ??= [image.texture.width, image.texture.height];
  add_item(config);
}

clay.text = function(str, ...configs)
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
clay.button = function(str, action, config = {})
{
  config.__proto__ = button_base;
  config.size = render.text_size(str,config.font);
  add_item(config);
  config.text = str;
  config.action = action;
}

layout.draw_commands = function(cmds, pos = [0,0])
{
  var mousepos = prosperon.camera.screen2hud(input.mouse.screenpos());
  for (var cmd of cmds) {
    cmd.boundingbox.x += pos.x;
    cmd.boundingbox.y += pos.y;
    cmd.content.x += pos.x;
    cmd.content.y += pos.y;
    var config = cmd.config;
    if (config.hovered && geometry.rect_point_inside(cmd.boundingbox, mousepos)) {
      config.hovered.__proto__ = config;
      config = config.hovered;
    }

    if (config.background_image)
      if (config.slice)
        render.slice9(config.background_image, cmd.boundingbox, config.slice, config.background_color);
      else
        render.image(config.background_image, cmd.boundingbox, 0, config.color);
    else if (config.background_color)
      render.rectangle(cmd.boundingbox, config.background_color);
      
    if (config.text)
      render.text(config.text, cmd.content, config.font, config.font_size, config.color);
    if (config.image)
      render.image(config.image, cmd.content, 0, config.color);
      
    render.rectangle(cmd.content, [1,0,0,0]);
//    render.rectangle(cmd.boundingbox, [0,1,0,0.1]);
//    render.rectangle(cmd.marginbox, [0,0,1,0.1]);
  }
}

return layout;

