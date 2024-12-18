var unit_transform = os.make_transform();

/*
  Anatomy of rendpering an image
  render.image(path)
  Path can be a file like "toad"
  If this is a gif, this would display the entire range of the animation
  It can be a frame of animation, like "frog.0"
  If it's an aseprite, it can have multiple animations, like "frog.walk.0"
                                                          file^ frame^ idx

  render.image("frog.walk.0",
  game.image("frog.walk.0") ==> retrieve

  image = {
    texture: "spritesheet.png",
    rect: [x,y,w,h],
    time: 100 
  },

    frames: {
      toad: {
        x: 4,
        y: 5,
        w: 10,
        h: 10
      },
      frog: {
      
        walk: [
          { texture: spritesheet.png, x: 10, y:10, w:6,h:6, time: 100 },
          { texture: spritesheet.png, x:16,y:10,w:6,h:6,time:100}       <--- two frame walk animation
        ],
      },
    },
  }
  texture frog {
    texture: {"frog.png"}, <--- this is the actual thing to send to the gpu
    x:0,
    y:0,
    w:10,
    h:10
  },
*/

var sprite_mesh = {};

render.doc = {
  doc: "Functions for rendering modes.",
  normal: "Final render with all lighting.",
  wireframe: "Show only wireframes of models.",
};

var cur = {};
cur.images = [];
cur.samplers = [];

var base_pipeline = {
  vertex: "sprite.vert",
  frag: "sprite.frag",
  primitive: "triangle", // point, line, linestrip, triangle, trianglestrip
  fill: true, // false for lines
  depth: {
    compare: "always",  // never/less/equal/less_equal/greater/not_equal/greater_equal/always
    test: true,
    write: false,
    bias: 0,
    bias_slope_scale: 0,
    bias_clamp: 0
  },
  stencil: {
    enabled: true,
    front: {
      compare: "equal", // never/less/equal/less_equal/greater/neq/greq/always
      fail: "keep", // keep/zero/replace/incr_clamp/decr_clamp/invert/incr_wrap/decr_wrap
      depth_fail: "keep",
      pass: "keep"
    },
    back: {
      compare: "equal", // never/less/equal/less_equal/greater/neq/greq/always
      fail: "keep", // keep/zero/replace/incr_clamp/decr_clamp/invert/incr_wrap/decr_wrap
      depth_fail: "keep",
      pass: "keep"
    },
    test: true,
    compare_mask: 0,
    write_mask: 0
  },
  blend: {
    enabled: false,
    src_factor_rgb: "one", // zero/one/src_color/one_minus_src_color/dst_color/one_minus_dst_color/src_alpha/one_minus_src_alpha/dst_alpha/one_minus_dst_alpha/constant_color/one_minus_constant_color/src_alpha_saturate
    dst_factor_rgb: "zero",
    op_rgb: "add", // add/sub/rev_sub/min/max
    src_factor_alpha: "one",
    dst_factor_alpha: "zero",
    op_alpha: "add"
  },
  cull: "none", // none/front/back
  face: "cw", // cw/ccw
  alpha_to_coverage: false,
  multisample: {
    count: 1, // number of multisamples
    mask: 0xFFFFFFFF,
    domask: false
  },
  label: "scripted pipeline"
}

var pipeline_3d = Object.create(base_pipeline);

function use_pipeline(pipeline) {
  if (!pipeline.gpu)
    pipeline.gpu = gpu.make_pipeline(pipeline);
  render._main.set_pipeline(pipeline);
}

render.poly_prim = function poly_prim(verts) {
  var index = [];
  if (verts.length < 1) return undefined;

  for (var i = 0; i < verts.length; i++) verts[i][2] = 0;

  for (var i = 2; i < verts.length; i++) {
    index.push(0);
    index.push(i - 1);
    index.push(i);
  }

  return {
    pos: os.make_buffer(verts.flat()),
    verts: verts.length,
    index: os.make_buffer(index, 1),
    count: index.length,
  };
};

var shader_cache = {};
var shader_times = {};

function strip_shader_inputs(shader) {
  for (var a of shader.vs.inputs) a.name = a.name.slice(2);
}

render.hotreload = function shader_hotreload() {
  for (var i in shader_times) {
    if (io.mod(i) <= shader_times[i]) continue;
    say(`HOT RELOADING SHADER ${i}`);
    shader_times[i] = io.mod(i);
    var obj = create_shader_obj(i);
    obj = obj[os.sys()];
    var old = shader_cache[i];
    Object.assign(shader_cache[i], obj);
    cur.bind = undefined;
    cur.mesh = undefined;
  }
};

function make_shader(shader, ...args) {
  var file = `shaders/spirv/${shader}.spv`;
  if (shader_cache[file]) return shader_cache[file];
  shader = io.slurpbytes(file);
  shader = render._main.make_shader(shader, ...args);
  shader_cache[file] = shader;
  return shader;
}

function shader_apply_material(shader, material = {}, old = {}) {
  render.setpipeline(cur.pipeline);
  for (var p in shader.vs.unimap) {
    if (!(p in material)) continue;
    if (material[p] === old[p]) continue;
    assert(p in material, `shader ${shader.name} has no uniform for ${p}`);
    var s = shader.vs.unimap[p];

    if (p === "bones") {
      render.setunibones(0, s.slot, material[p]);
      continue;
    }

    shader_unisize[s.size](0, s.slot, material[p]);
  }

  for (var p in shader.fs.unimap) {
    if (!(p in material)) continue;
    if (material[p] === old[p]) continue;
    assert(p in material, `shader ${shader.name} has no uniform for ${p}`);
    var s = shader.fs.unimap[p];
    shader_unisize[s.size](1, s.slot, material[p]);
  }

  if (!material.diffuse) return;
  if (material.diffuse === old.diffuse) return;

  if ("diffuse_texel" in shader.fs.unimap) render.setuniv2(1, shader.fs.unimap.diffuse_texel.slot, [1,1].div([material.diffuse.width, material.diffuse.height]));

  if ("diffuse_size" in shader.fs.unimap) render.setuniv2(1, shader.fs.unimap.diffuse_size.slot, [material.diffuse.width, material.diffuse.height]);

  if ("diffuse_size" in shader.vs.unimap) render.setuniv2(0, shader.vs.unimap.diffuse_size.slot, [material.diffuse.width, material.diffuse.height]);
}

// Creates a binding object for a given mesh and shader
var bcache = new WeakMap();
function sg_bind(mesh, ssbo) {
  if (cur.bind && cur.mesh === mesh && cur.ssbo === ssbo) {
    if (ssbo)
      cur.bind.ssbo = [ssbo];
    else
      cur.bind.ssbo = undefined;
    cur.bind.images = cur.images;
    cur.bind.samplers = cur.samplers;
    render.setbind(cur.bind);
    return;
  }

/*  if (bcache.has(cur.shader) && bcache.get(cur.shader).has(mesh)) {
    cur.bind = bcache.get(cur.shader).get(mesh);
    cur.bind.images = cur.images;
    cur.bind.samplers = cur.samplers;
    if (ssbo)
    cur.bind.ssbo = [ssbo];
    render.setbind(cur.bind);
    return;
  }*/
  var bind = {};/*  if (!bcache.has(cur.shader)) bcache.set(cur.shader, new WeakMap());
  if (!bcache.get(cur.shader).has(mesh)) bcache.get(cur.shader).set(mesh, bind);*/
  cur.mesh = mesh;
  cur.ssbo = ssbo;
  cur.bind = bind;
  bind.attrib = [];
  if (cur.shader.vs.inputs)
    for (var a of cur.shader.vs.inputs) {
      if (!(a.name in mesh)) {
        console.error(`cannot draw shader ${cur.shader.name}; there is no attrib ${a.name} in the given mesh. ${json.encode(mesh)}`);
        return undefined;
      } else bind.attrib.push(mesh[a.name]);
    }

  if (cur.shader.indexed) {
    bind.index = mesh.index;
    bind.count = mesh.count;
  } else bind.count = mesh.verts;

  bind.ssbo = [];
  if (cur.shader.vs.storage_buffers) for (var b of cur.shader.vs.storage_buffers) bind.ssbo.push(ssbo);

  bind.images = cur.images;
  bind.samplers = cur.samplers;

  render.setbind(cur.bind);

  return bind;
}

render.device = {
  pc: [1920, 1080],
  macbook_m2: [2560, 1664, 13.6],
  ds_top: [400, 240, 3.53],
  ds_bottom: [320, 240, 3.02],
  playdate: [400, 240, 2.7],
  switch: [1280, 720, 6.2],
  switch_lite: [1280, 720, 5.5],
  switch_oled: [1280, 720, 7],
  dsi: [256, 192, 3.268],
  ds: [256, 192, 3],
  dsixl: [256, 192, 4.2],
  ipad_air_m2: [2360, 1640, 11.97],
  iphone_se: [1334, 750, 4.7],
  iphone_12_pro: [2532, 1170, 6.06],
  iphone_15: [2556, 1179, 6.1],
  gba: [240, 160, 2.9],
  gameboy: [160, 144, 2.48],
  gbc: [160, 144, 2.28],
  steamdeck: [1280, 800, 7],
  vita: [960, 544, 5],
  psp: [480, 272, 4.3],
  imac_m3: [4480, 2520, 23.5],
  macbook_pro_m3: [3024, 1964, 14.2],
  ps1: [320, 240, 5],
  ps2: [640, 480],
  snes: [256, 224],
  gamecube: [640, 480],
  n64: [320, 240],
  c64: [320, 200],
  macintosh: [512, 342, 9],
  gamegear: [160, 144, 3.2],
};

var sprite_stack = [];

render.device.doc = `Device resolutions given as [x,y,inches diagonal].`;
var std_sampler;

function upload_model(model)
{
  var bufs = [];
  for (var i in model) {
    if (typeof model[i] !== 'object') continue;
    if (i === 'indices') model[i].index = true;
    bufs.push(model[i]);
  }
  render._main.upload(bufs);  
}

var campos = [0,0,500];

function gpupresent()
{
  var myimg = game.texture("pockle");
  myimg.sampler = std_sampler;
  var spritemesh = render._main.make_sprite_mesh(sprite_stack);
  upload_model(spritemesh);

  sprite_stack.length = 0;
  
  var cmds = render._main.acquire_cmd_buffer();  
  var pass = cmds.render_pass();
try{  
  pass.bind_pipeline(base_pipeline.gpu);
  pass.bind_model(spritemesh);
  pass.bind_mat({diffuse:myimg});
  cmds.camera(prosperon.camera.transform);
  pass.draw(spritemesh.count,1,0,0,0);
  prosperon.camera.transform.pos = campos;
  campos.y -= 0.1;
  campos.z += 0.1;
  prosperon.camera.fov = 60;
  prosperon.camera.near = 0.1;
  prosperon.camera.far = 100000;
  cmds.camera_perspective(prosperon.camera);
  pass.bind_pipeline(pipeline_model.gpu);
  pass.bind_model(ducky);
  pass.draw(ducky.count,1,0,0,0);
  cmds.camera(prosperon.camera.transform, true);
  pass.bind_pipeline(base_pipeline.gpu);
  pass.draw(spritemesh.count,1,0,0,0);
} catch(e) { console.log(e); } finally {
  pass.end();
  cmds.submit();  
}

}

var ducky;
var pipeline_model;

render.init = function () {
  io.mount("core");
  render._main.present = gpupresent;
  ducky = os.model_buffer("Duck.glb");
  upload_model(ducky);
  for (var i in ducky) console.log(`ducky has ${i}`)
  console.log(ducky.count)
  var sprite_vert = make_shader("sprite.vert", true,0,0,0,1);
  var sprite_frag = make_shader("sprite.frag", false,1,0,0,0);
  base_pipeline.vertex = sprite_vert;
  base_pipeline.fragment = sprite_frag;
  base_pipeline.gpu = render._main.make_pipeline(base_pipeline);
  var model_vert = make_shader("model.vert", true, 0,0,0,1);
  var model_frag = make_shader("model.frag", false, 0,0,0,0);
  pipeline_model = Object.create(base_pipeline);
  pipeline_model.vertex = model_vert;
  pipeline_model.fragment = model_frag;
  pipeline_model.gpu = render._main.make_pipeline(pipeline_model);
  
  std_sampler = render._main.make_sampler({
    min_filter: "nearest",
    mag_filter: "nearest",
    mipmap_mode: "nearest",
    address_mode_u: "clamp_edge",
    address_mode_v: "clamp_edge",
    address_mode_w: "clamp_edge"
  });

/*  os.make_circle2d().draw = function () {
    render.circle(this.body().transform().pos, this.radius, [1, 1, 0, 1]);
  };

  var disabled = [148 / 255, 148 / 255, 148 / 255, 1];
  var sleep = [1, 140 / 255, 228 / 255, 1];
  var dynamic = [1, 70 / 255, 46 / 255, 1];
  var kinematic = [1, 194 / 255, 64 / 255, 1];
  var static_color = [73 / 255, 209 / 255, 80 / 255, 1];

  os.make_poly2d().draw = function () {
    var body = this.body();
    var color = body.sleeping() ? [0, 0.3, 0, 0.4] : [0, 1, 0, 0.4];
    var t = body.transform();
    render.poly(this.points, color, body.transform());
    color.a = 1;
    render.line(this.points.wrapped(1), color, 1, body.transform());
  };

  os.make_seg2d().draw = function () {
    render.line([this.a(), this.b()], [1, 0, 1, 1], Math.max(this.radius / 2, 1), this.body().transform());
  };

  joint.pin().draw = function () {
    var a = this.bodyA();
    var b = this.bodyB();
    render.line([a.transform().pos.xy, b.transform().pos.xy], [0, 1, 1, 1], 1);
  };
*/  
};

render.draw_sprites = true;
render.draw_particles = true;
render.draw_hud = true;
render.draw_gui = true;
render.draw_gizmos = true;

render.sprites = function render_sprites() {
  // bucket & draw
/*  var sorted = allsprites.sort((a,b) => {
    if (a.gameobject.drawlayer !== b.gameobject.drawlayer) return a.gameobject.drawlayer - b.gameobject.drawlayer;
    if (a.image.texture !== b.image.texture) return a.image.texture.path.localeCompare(b.image.texture.path);
    return a.gameobject.transform.pos.y - b.gameobject.transform.pos.y;
  });

  if (sorted.length === 0) return;

  var tex = undefined;
  var buckets = [];
  var group = [sorted[0]];

  for (var i = 1; i < sorted.length; i++) {
    if (sorted[i].image.texture !== sorted[i-1].image.texture) {
      buckets.push(group);
      group = [];
    }
    group.push(sorted[i]);
  }
  if (group.length>0) buckets.push(group);
  render.use_shader(spritessboshader);
  for (var img of buckets) {
    var sparray = img;
    if (sparray.length === 0) continue;
    var ss = sparray[0];
    ss.baseinstance = render.make_sprite_ssbo(sparray,sprite_ssbo);
    render.use_mat(ss);
    render.draw(shape.quad,sprite_ssbo,sparray.length);
  }
*/

  var buckets = component.sprite_buckets();
  if (buckets.length === 0) return;
  render.use_shader(spritessboshader);  
  for (var l in buckets) {
    var layer = buckets[l];
    for (var img in layer) {
      var sparray = layer[img];
      if (sparray.length === 0) continue;
      var ss = sparray[0];
      ss.baseinstance = render.make_sprite_ssbo(sparray, sprite_ssbo);
      render.use_mat(ss);
      render.draw(shape.quad, sprite_ssbo, sparray.length);
    }
  }
};

function draw_sprites()
{
  var buckets = component.sprite_buckets();
  if (buckets.length === 0) return;
  for (var l in buckets) {
    var layer = buckets[l];
    for (var img in layer) {
      var sparray = layer[img];
      if (sparray.length === 0) continue;
      var geometry = render._main.make_sprite_mesh(sparray);
      render.geometry(sparray[0], geometry);
    }
  }
}

render.circle = function render_circle(pos, radius, color, inner_radius = 1) {
  check_flush();

  if (inner_radius >= 1) inner_radius = inner_radius / radius;
  else if (inner_radius < 0) inner_radius = 1.0;

  var mat = {
    radius: radius,
    inner_r: inner_radius,
    coord: pos,
    shade: color,
  };
  render.use_shader(circleshader);
  render.use_mat(mat);
  render.draw(shape.quad);
};
render.circle.doc = "Draw a circle at pos, with a given radius and color. If inner_radius is between 0 and 1, it acts as a percentage of radius. If it is above 1, is acts as a unit (usually a pixel).";

render.poly = function render_poly(points, color, transform) {
  var buffer = render.poly_prim(points);
  var mat = { shade: color };
  render.use_shader(polyshader);
  set_model(transform);
  render.use_mat(mat);
  render.draw(buffer);
};

var nextflush = undefined;
function flush() {
  nextflush?.();
  nextflush = undefined;
}

// If flush_fn was already on deck, it does not flush. Otherwise, flushes and then sets the flush fn
function check_flush(flush_fn) {
  if (!nextflush) nextflush = flush_fn;
  else if (nextflush !== flush_fn) {
    nextflush();
    nextflush = flush_fn;
  }
}

render.flush = check_flush;
render.forceflush = function forceflush()
{
  if (nextflush) nextflush();
  nextflush = undefined;
  cur.shader =  undefined;
}

var poly_cache = [];
var poly_idx = 0;
var poly_ssbo;

function poly_e() {
  var e;
  poly_idx++;
  if (poly_idx > poly_cache.length) {
    e = {
      transform: os.make_transform(),
      color: Color.white,
    };
    poly_cache.push(e);
    return e;
  }
  var e = poly_cache[poly_idx - 1];
  e.transform.unit();
  return e;
}

function flush_poly() {
  if (poly_idx === 0) return;
  render.use_shader(queued_shader, queued_pipe);
  var base = render.make_particle_ssbo(poly_cache.slice(0, poly_idx), poly_ssbo);
  render.use_mat({baseinstance:base});  
  render.draw(shape.quad, poly_ssbo, poly_idx);
  poly_idx = 0;
}

render.line = function render_line(points, color = Color.white, thickness = 1, pipe = base_pipeline) {
  render._main.line(points, color);
};

/* All draw in screen space */
render.point = function (pos, size, color = Color.blue) {
  render._main.point(pos,color);
};

render.cross = function render_cross(pos, size, color = Color.red, thickness = 1, pipe = base_pipeline) {
  var a = [pos.add([0, size]), pos.add([0, -size])];
  var b = [pos.add([size, 0]), pos.add([-size, 0])];
  render.line(a, color, thickness);
  render.line(b, color, thickness);
};

render.arrow = function render_arrow(start, end, color = Color.red, wingspan = 4, wingangle = 10, pipe = base_pipeline) {
  var dir = end.sub(start).normalized();
  var wing1 = [Vector.rotate(dir, wingangle).scale(wingspan).add(end), end];
  var wing2 = [Vector.rotate(dir, -wingangle).scale(wingspan).add(end), end];
  render.line([start, end], color);
  render.line(wing1, color);
  render.line(wing2, color);
};

render.coordinate = function render_coordinate(pos, size, color) {
  render.text(JSON.stringify(pos.map(p => Math.round(p))), pos, size, color);
  render.point(pos, 2, color);
};

var queued_shader;
var queued_pipe;
render.rectangle = function render_rectangle(rect, color = Color.white, pipe = base_pipeline) {
  render._main.fillrect(rect,color);
};

render.text = function text(str, rect, font = cur_font, size = 0, color = Color.white, wrap = 0, pipe = base_pipeline) {
  if (typeof font === 'string')
    font = render.get_font(font)
  var mesh = os.make_text_buffer(str, rect, 0, color, wrap, font);
  render._main.geometry(font.texture, mesh);
  return;
  
  if (typeof font === 'string')
    font = render.get_font(font);

  if (!font) return;
  var pos = [rect.x,rect.y];
  pos.y -= font.descent;
  if (rect.anchor_y)
    pos.y -= rect.anchor_y*(font.ascent-font.descent);
  os.make_text_buffer(str, pos, size, color, wrap, font); // this puts text into buffer
  cur_font = font;
  check_flush(render.flush_text);
};

var tttsize = render.text_size;
render.text_size = function(str, font, ...args)
{
  if (typeof font === 'string')
    font = render.get_font(font);
  return tttsize(str,font, ...args); 
}

var lasttex = undefined;
var img_cache = [];
var img_idx = 0;

function flush_img() {
  if (img_idx === 0) return;
  render.use_shader(spritessboshader);
  var startidx = render.make_sprite_ssbo(img_cache.slice(0, img_idx), sprite_ssbo);
  render.use_mat({baseinstance:startidx});  
  cur.images = [lasttex];
  render.draw(shape.quad, sprite_ssbo, img_idx);
  lasttex = undefined;
  img_idx = 0;
}

function img_e() {
  img_idx++;
  if (img_idx > img_cache.length) {
    var e = {
      transform: os.make_transform(),
      shade: Color.white,
    };
    img_cache.push(e);
    return e;
  }
  return img_cache[img_idx - 1];
}

var stencil_write = {
  compare: "always",
  fail_op: "replace",
  depth_fail_op: "replace",
  pass_op: "replace"
};

var stencil_writer = function stencil_writer(ref)
{
  var pipe = Object.create(base_pipeline);
  Object.assign(pipe, {
    stencil: {
      enabled: true,
      front: stencil_write,
      back: stencil_write,
      write:true,
      read:true,
      ref:ref
    },
    write_mask: colormask.none
  });
  return pipe;
}.hashify();

render.stencil_writer = stencil_writer;

// objects by default draw where the stencil buffer is 0
render.fillmask = function fillmask(ref)
{
  render.forceflush();
  var pipe = stencil_writer(ref);
  render.use_shader('screenfill.cg', pipe);
  render.draw(shape.quad);
}

var stencil_invert = {
  compare: "always",
  fail_op: "invert",
  depth_fail_op: "invert",
  pass_op: "invert"
};
var stencil_inverter = Object.create(base_pipeline);
/*Object.assign(stencil_inverter, {
  stencil: {
    enabled: true,
    front: stencil_invert,
    back:stencil_invert,
    write:true,
    read:true,
    ref: 0
  },
  write_mask: colormask.none
});*/
render.invertmask = function()
{
  render.forceflush();
  render.use_shader('screenfill.cg', stencil_inverter);
  render.draw(shape.quad);
}

render.mask = function mask(image, pos, scale, rotation = 0, ref = 1)
{
  if (typeof image === 'string')
    image = game.texture(image);

  var tex = image.texture;

  if (scale) scale = sacle.div([tex.width,tex.height]);
  else scale = vector.v3one;
    
  var pipe = stencil_writer(ref);
  render.use_shader('sprite.cg', pipe);
  var t = os.make_transform();
  t.trs(pos, undefined,scale);
  set_model(t);
  render.use_mat({
    diffuse:image.texture,
    rect: image.rect,
    shade: Color.white
  });
  render.draw(shape.quad);
}

function calc_image_size(img)
{
  return [img.texture.width*img.rect.width, img.texture.height*img.rect.height];
}

render.tile = function tile(image, rect = [0,0], color = Color.white)
{
  if (!image) throw Error ('Need an image to render.')
  if (typeof image === "string")
    image = game.texture(image);

  render._main.tile(image, rect, undefined, 1);
  return;

  var tex = image.texture;
  if (!tex) return;

  var image_size = calc_image_size(image); //image.size;
  
  var size = [rect.width ? rect.width : image_size.x, rect.height ? rect.height : image_size.y];

  if (!lasttex) {
    check_flush(flush_img);
    lasttex = tex;
  }

  if (lasttex !== tex) {
    flush_img();
    lasttex = tex;
  }

  render._main.tile(image.texture, rect, image.rect, 1);
  return;
}

render.geometry = function geometry(material, geometry)
{
  render._main.geometry(material.diffuse.texture, geometry);
}

render.image = function image(image, rect = [0,0], rotation = 0, color = Color.white) {
  if (!image) throw Error ('Need an image to render.')
  if (typeof image === "string")
    image = game.texture(image);

  rect.width ??= image.texture.width;
  rect.height ??= image.texture.height;
  var T = os.make_transform();
  T.rect(rect);
  sprite_stack.push({
    transform: T,
    color: color,
    image:image
  });
//  render._main.texture(image.texture, rect, image.rect, color);
};

render.images = function images(image, rects)
{
  if (!image) throw Error ('Need an image to render.');
  if (typeof image === "string") image = game.texture(image);
  for (var rect of rects) render.image(image,rect);
  return;
  var tex = image.texture;
  if (!tex) return;

  var image_size = calc_image_size(image);
  
  if (!lasttex) {
    check_flush(flush_img);
    lasttex = tex;
  }

  if (lasttex !== tex) {
    flush_img();
    lasttex = tex;
  }

//  rects = rects.flat();
  var rect = rects[0];
  var size = [rect.width ? rect.width : image_size.x, rect.height ? rect.height : image_size.y];
  var offset = size.scale([rect.anchor_x, rect.anchor_y]);
  for (var rect of rects) {
    var e = img_e();
    var pos = [rect.x,rect.y].sub(offset);
    e.transform.trs(pos, undefined, size);
    e.image = image;
    e.shade = Color.white;
  }

  return;
}

// slice is given in pixels
render.slice9 = function slice9(image, rect = [0,0], slice = 0, color = Color.white) {
  if (typeof image === 'string')
    image = game.texture(image);

  rect.width ??= image.texture.width;
  rect.height ??= image.texture.height;
  slice = clay.normalizeSpacing(slice);  

  render._main.slice9(image.texture, rect, slice);
};

var textssbos = [];
var tdraw = 0;
var cur_font = undefined;

render.flush_text = function flush_text() {
  if (!render.textshader) return;
  tdraw++;
  if (textssbos.length < tdraw) textssbos.push(render.make_textssbo());

  var textssbo = textssbos[tdraw - 1];
  var amt = render.flushtext(textssbo); // load from buffer into ssbo

  if (amt === 0) {
    tdraw--;
    return;
  }

  render.use_shader(render.textshader);
  render.use_mat({ text: cur_font.texture });
  render.draw(shape.quad, textssbo, amt);
};

var fontcache = {};
var datas= [];
render.get_font = function get_font(path,size)
{
  var parts = path.split('.');
  if (!isNaN(parts[1])) {
    path = parts[0];
    size = Number(parts[1]);
  }
  path = Resources.find_font(path);  
  var fontstr = `${path}.${size}`;
  if (fontcache[fontstr]) return fontcache[fontstr];
  
  var data = io.slurpbytes(path);
  fontcache[fontstr] = os.make_font(data,size);
  fontcache[fontstr].texture = render._main.load_texture(fontcache[fontstr].surface);
  return fontcache[fontstr];
}

render.doc = "Draw shapes in screen space.";
render.cross.doc = "Draw a cross centered at pos, with arm length size.";
render.arrow.doc = "Draw an arrow from start to end, with wings of length wingspan at angle wingangle.";
render.rectangle.doc = "Draw a rectangle, with its corners at lowerleft and upperright.";

render.draw = function render_draw(mesh, ssbo, inst = 1, e_start = 0) {
  sg_bind(mesh, ssbo);
  render.spdraw(e_start, cur.bind.count, inst);
};

render.viewport = function(rect)
{
  render._main.viewport(rect);
}

render.scissor = function(rect)
{
  render.viewport(rect)
}

// Camera viewport is a rectangle with the bottom left corner defined as x,y. Units are pixels on the window.
function camviewport() {

  var aspect = (((this.viewport[2] - this.viewport[0]) / (this.viewport[3] - this.viewport[1])) * prosperon.size.x) / prosperon.size.y;
  var raspect = this.size.x / this.size.y;

  var left = this.viewport[0] * prosperon.size.x;
  var bottom = this.viewport[1] * prosperon.size.y;

  var usemode = this.mode;

  if (this.break && this.size.x > prosperon.size.x && this.size.y > prosperon.size.y) usemode = this.break;

  if (usemode === "fit")
    if (raspect < aspect) usemode = "height";
    else usemode = "width";

  switch (usemode) {
    case "stretch":
    case "expand":
      return {
        x: 0,
        y: 0,
        width: prosperon.size.x,
        height: prosperon.size.y
      };
    case "keep":
      return {
        x: left,
        y: bottom,
        width:left+this.size.x,
        height:bottom+this.size.y
      }
    case "height":
      var ret = {
        x:left,
        y:0,
        width:this.size.x*(prosperon.size.y/this.size.y),
        height:prosperon.size.y
      };
      ret.x = (prosperon.size.x - (ret.width-ret.x))/2;
      return ret;
    case "width":
      var ret = {
        x:0,
        y:bottom,
        width:prosperon.size.x,
        height:this.size.y*(prosperon.size.x/this.size.x)
      };
      ret.y = (prosperon.size.y - (ret.height-ret.y))/2;
      return ret;
  }

  return {
    x:0,
    y:0,
    width:prosperon.size.x,
    height:prosperon.size.y
  };
}

// pos is screen coordinates
function camscreen2world(pos) {
  var view = this.screen2cam(pos);
  var viewport = render._main.get_viewport();
  view.x *= viewport.width;
  view.y *= viewport.height;
  view = view.add(this.pos.xy);
  view = view.sub([viewport.width,viewport.height].scale(0.5))
//  view = view.scale(this.transform.scale);
  return view;
}

// world coordinates, the "actual" view relative to the game's universe
// camera coordinates, normalized from 0 to 1 inside of a camera's viewport, bottom left is 0,0, top right is 1,1
// screen coordinates, pixels, 0,0 at the top left of the window and [w,h] at the bottom right of the window
// hud coordinates, same as screen coordinates but the top left is 0,0

camscreen2world.doc = "Convert a view position for a camera to world.";

// return camera coordinates given a screen position
function screen2cam(pos) {
  var tpos = render._main.coords(pos);
  var viewport = render._main.get_viewport();
  var viewpos = tpos.div([viewport.width,viewport.height]);
  viewpos.y *= -1;
  viewpos.y += 1;
  return viewpos;
}

screen2cam.doc = "Convert a screen space position in pixels to a normalized viewport position in a camera.";

prosperon.gizmos = function gizmos() {
  game.all_objects(o => {
    if (o.gizmo) render.image(game.texture(o.gizmo), o.pos);
  });
};

function screen2hud(pos)
{
  var campos = this.screen2cam(pos);
  var viewport = render._main.get_viewport();  
  campos = campos.scale([viewport.width,viewport.height]);
  return campos;
}

/* cameras
 * Cameras have a position and rotation. They are not affected by scale.
*/

prosperon.make_camera = function make_camera() {
  var cam = world.spawn();
  cam.near = 1;
  cam.far = -1000;
  cam.ortho = true; // True if this is a 2d camera
  cam.viewport = [0, 0, 1, 1]; // normalized screen coordinates of where to draw
  cam.size = prosperon.size.slice() // The render size of this camera in pixels
  // In ortho mode, this determines how many pixels it will see
  cam.mode = "stretch";
  cam.screen2world = camscreen2world;
  cam.screen2cam = screen2cam;
  cam.screen2hud = screen2hud;
  cam.view = camviewport;
  cam.zoom = 1; // the "scale factor" this camera demonstrates
  // camera renders draw calls, and then hud
  cam.render = function() {
    prosperon.draw();
    draw_sprites();
    render._main.camera(this.transform,true);
    render._main.scale([this.zoom, this.zoom]);
    prosperon.hud();
    render._main.scale([1,1]);
    render._main.camera(unit_transform,false);

  }
  return cam;
};

var screencolor;

globalThis.imtoggle = function (name, obj, field) {
  var changed = false;
  var old = obj[field];
  obj[field] = imgui.checkbox(name, obj[field]);
  if (old !== obj[field]) return true;
  return false;
};
var replstr = "";

var imdebug = function imdebug() {
  imtoggle("Physics", debug, "draw_phys");
  imtoggle("Bouning boxes", debug, "draw_bb");
  imtoggle("Gizmos", debug, "draw_gizmos");
  imtoggle("Names", debug, "draw_names");
  imtoggle("Sprite nums", debug, "sprite_nums");
  imtoggle("Debug overlay", debug, "show");
  imtoggle("Show ur names", debug, "urnames");
};

var observed_tex = undefined;

var imgui_fn = function imgui_fn() {
  return;
  imgui.newframe(prosperon.size.x, prosperon.size.y, 0.01);
  if (debug.console)
    debug.console = imgui.window("console", _ => {
      imgui.text(console.transcript);
      replstr = imgui.textinput(undefined, replstr);
      imgui.button("submit", _ => {
        eval(replstr);
        replstr = "";
      });
    });

  imgui.mainmenubar(_ => {
    imgui.menu("File", _ => {
      imgui.menu("Game settings", _ => {
        prosperon.title = imgui.textinput("Title", prosperon.title);
        prosperon.icon = imgui.textinput("Icon", prosperon.icon);
        imgui.button("Refresh window", _ => {
          prosperon.set_icon(game.texture(prosperon.icon));
        });
      });
      imgui.button("quit", os.exit);
    });
    imgui.menu("Debug", imdebug);
    imgui.menu("View", _ => {
      imtoggle("Profiler", debug, "showprofiler");
      imtoggle("Terminal out", debug, "termout");
      imtoggle("Meta [f7]", debug, "meta");
      imtoggle("Cheats [f8]", debug, "cheat");
      imtoggle("Console [f9]", debug, "console");
      if (profile.tracing)
        imgui.button("stop trace", profile.trace_stop);
      else
        imgui.button('start trace', profile.trace_start);
    });

    imgui.sokol_gfx();

    imgui.menu("Graphics", _ => {
      imtoggle("Draw sprites", render, "draw_sprites");
      imtoggle("Draw particles", render, "draw_particles");
      imtoggle("Draw HUD", render, "draw_hud");
      imtoggle("Draw GUI", render, "draw_gui");
      imtoggle("Draw gizmos", render, "draw_gizmos");

      imgui.menu("Window", _ => {
        prosperon.fullscreen = imgui.checkbox("fullscreen", prosperon.fullscreen);
        //      prosperon.vsync = imgui.checkbox("vsync", prosperon.vsync);
        imgui.menu("MSAA", _ => {
          for (var msaa of gamestate.msaa) imgui.button(msaa + "x", _ => (prosperon.sample_count = msaa));
        });
      });
    });

    prosperon.menu_hook?.();
  });

/*
  if (observed_tex) {
    imgui.window("texture", _ => {
      imgui.image(observed_tex);
    });
  }

  var texs = {};
  for (var path in game.texture.cache) {
    var image = game.texture.cache[path];
    if (image.texture && !texs[image.texture])
      texs[image.texture] = image.texture;
  }
  imgui.window("textures", _ => {
    for (var img in texs) {
      imgui.button(img, _ => {
        observed_tex = texs[img];
      });
    }
  });
*/
  prosperon.imgui();
  imgui.endframe(render._main);
};

  // figure out the highest resolution we can render at that's an integer
/*  var basesize = prosperon.camera.size.slice();
  var baseview = prosperon.camera.view();
  var wh = [baseview[2]-baseview[0], baseview[3]-baseview[1]];
  var mult = 1;
  var trysize = basesize.scale(mult);
  while (trysize.x <= wh.x && trysize.y <= wh.y) {
    mult++;
    trysize = basesize.scale(mult);
  }
  if (Math.abs(wh.x - basesize.scale(mult-1).x) < Math.abs(wh.x - trysize.x))
    mult--;
    
  prosperon.window_render(basesize.scale(mult));
*/

var clearcolor = [100,149,237,255].scale(1/255);
prosperon.render = function prosperon_render() {
try{
  render._main.newframe(prosperon.window, clearcolor);
//  render._main.draw_color(clearcolor);
//  render._main.clear();
  
try { prosperon.camera.render(); } catch(e) { console.error(e) }
try { prosperon.app(); } catch(e) { console.error(e) }
if (debug.show) try { imgui_fn(); } catch(e) { console.error(e) }

} catch(e) {
  console.error(e)
} finally {
  render._main.present();
  tracy.end_frame();  
}
};

if (dmon) dmon.watch('.');

function dmon_cb(e)
{
try {
  io.invalidate();
  if (e.file.startsWith('.')) return;
  if (e.file.endsWith('.js'))
    actor.hotreload(e.file);
  else if (Resources.is_image(e.file))
    game.tex_hotreload(e.file);
} catch(e) { console.error(e); }
}

// Ran once per frame
prosperon.process = function process() {
try {
  layout.newframe();
  // check for hot reloading
  if (dmon) dmon.poll(dmon_cb);
  var dt = profile.now() - frame_t;
  frame_t = profile.now();

  game.engine_input(e => {
    prosperon[e.type]?.(e);
  });

try { prosperon.appupdate(dt); } catch(e) { console.error(e) }

  input.procdown();

try {
  update_emitters(dt * game.timescale);
  os.update_timers(dt * game.timescale);
  prosperon.update(dt*game.timescale);
} catch(e) { console.error(e) }

  if (sim.mode === "step") sim.pause();
  if (sim.mode === "play" || sim.mode === "step") {
/* 
    physlag += dt;

    while (physlag > physics.delta) {
      physlag -= physics.delta;
      prosperon.phys2d_step(physics.delta * game.timescale);
      prosperon.physupdate(physics.delta * game.timescale);
    }
  */  
  }

  prosperon.render();
//  tracy.gpu_zone(prosperon.render);
} catch(e) {
  console.error(e)
}
};

return { render };
