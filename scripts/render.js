render.doc = {
  doc: "Functions for rendering modes.",
  normal: "Final render with all lighting.",
  wireframe: "Show only wireframes of models.",
};

var cur = {};
cur.images = [];
cur.samplers = [];

// When changing a shader, everything must wipe
render.use_shader = function use_shader(shader) {
  if (typeof shader === "string") shader = make_shader(shader);
  if (cur.shader === shader) return;
  cur.shader = shader;
  cur.bind = undefined;
  cur.mesh = undefined;
  cur.ssbo = undefined;
  render.setpipeline(shader.pipe);
  shader_globals(cur.shader);
};

render.use_mat = function use_mat(mat) {
  if (!cur.shader) return;
  if (cur.mat === mat) return;

  shader_apply_material(cur.shader, mat, cur.mat);

  cur.mat = mat;

  cur.images.length = 0;
  cur.samplers.length = 0;
  if (!cur.shader.fs.images) return;
  for (var img of cur.shader.fs.images) {
    if (mat[img.name]) cur.images.push(mat[img.name]);
    else cur.images.push(game.texture("icons/no_tex.gif"));
  }
  for (var smp of cur.shader.fs.samplers) {
    var std = smp.sampler_type === "nonfiltering";
    cur.samplers.push(std);
  }
};

var models_array = [];

function set_model(t) {
  if (cur.shader.vs.unimap.model) render.setunim4(0, cur.shader.vs.unimap.model.slot, t);
}

render.set_model = set_model;

var shaderlang = {
  macos: "metal_macos",
  windows: "hlsl5",
  linux: "glsl430",
  web: "wgsl",
  ios: "metal_ios",
};

var attr_map = {
  a_pos: 0,
  a_uv: 1,
  a_norm: 2,
  a_joint: 3,
  a_weight: 4,
  a_color: 5,
  a_tan: 6,
  a_angle: 7,
  a_wh: 8,
  a_st: 9,
  a_ppos: 10,
  a_scale: 11,
};

var blend_map = {
  mix: true,
  none: false,
};

var primitive_map = {
  point: 1,
  line: 2,
  linestrip: 3,
  triangle: 4,
  trianglestrip: 5,
};

var cull_map = {
  none: 1,
  front: 2,
  back: 3,
};

var depth_map = {
  off: false,
  on: true,
};

var face_map = {
  cw: 2,
  ccw: 1,
};

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

function shader_directive(shader, name, map) {
  var reg = new RegExp(`#${name}.*`, "g");
  var mat = shader.match(reg);
  if (!mat) return undefined;

  reg = new RegExp(`#${name}\s*`, "g");
  var ff = mat.map(d => d.replace(reg, ""))[0].trim();

  if (map) return map[ff];
  return ff;
}

var uni_globals = {
  time(stage, slot) {
    render.setuniv(stage, slot, profile.secs(profile.now()));
  },
  projection(stage, slot) {
    render.setuniproj(stage, slot);
  },
  view(stage, slot) {
    render.setuniview(stage, slot);
  },
  vp(stage, slot) {
    render.setunivp(stage, slot);
  },
};

function set_global_uni(uni, stage) {
  uni_globals[uni.name]?.(stage, uni.slot);
}

var setcam = render.set_camera;
render.set_camera = function (cam) {
  if (nextflush) {
    nextflush();
    nextflush = undefined;
  }
  delete cur.shader;
  setcam(cam);
};

var shader_cache = {};
var shader_times = {};

function strip_shader_inputs(shader) {
  for (var a of shader.vs.inputs) a.name = a.name.slice(2);
}

render.hotreload = function () {
  for (var i in shader_times) {
    if (io.mod(i) <= shader_times[i]) continue;
    say(`HOT RELOADING SHADER ${i}`);
    shader_times[i] = io.mod(i);
    var obj = create_shader_obj(i);
    obj = obj[os.sys()];
    obj.pipe = render.pipeline(obj);
    var old = shader_cache[i];
    Object.assign(shader_cache[i], obj);
    cur.bind = undefined;
    cur.mesh = undefined;
  }
};

function create_shader_obj(file) {
  var files = [file];
  var out = ".prosperon/tmp.shader";
  var shader = io.slurp(file);

  var incs = shader.match(/#include <.*>/g);
  if (incs)
    for (var inc of incs) {
      var filez = inc.match(/#include <(.*)>/)[1];
      var macro = io.slurp(filez);
      if (!macro) {
        filez = `shaders/${filez}`;
        macro = io.slurp(filez);
      }
      shader = shader.replace(inc, macro);
      files.push(filez);
    }

  var blend = shader_directive(shader, "blend", blend_map);
  var primitive = shader_directive(shader, "primitive", primitive_map);
  var cull = shader_directive(shader, "cull", cull_map);
  var depth = shader_directive(shader, "depth", depth_map);
  var face = shader_directive(shader, "face", face_map);
  var indexed = shader_directive(shader, "indexed");
  var stencil = shader_directive(shader, "stencil");

  if (typeof indexed == "undefined") indexed = true;
  if (indexed === "false") indexed = false;

  shader = shader.replace(/uniform\s+(\w+)\s+(\w+);/g, "uniform _$2 { $1 $2; };");
  shader = shader.replace(/(texture2D|sampler) /g, "uniform $1 ");

  io.slurpwrite(out, shader);

  var compiled = {};

  // shader file is created, now cross compile to all targets
  for (var platform in shaderlang) {
    var backend = shaderlang[platform];
    var ret = os.system(`sokol-shdc -f bare_yaml --slang=${backend} -i ${out} -o ${out}`);
    if (ret) {
      console.error(`error compiling shader ${file}. No compilation found for ${platform}:${backend}, and no cross compiler available.`);
      return;
    }

    /* Take YAML and create the shader object */
    var yamlfile = `${out}_reflection.yaml`;
    var jjson = yaml.tojson(io.slurp(yamlfile));
    var obj = json.decode(jjson);
    io.rm(yamlfile);

    obj = obj.shaders[0].programs[0];
    function add_code(stage) {
      stage.code = io.slurp(stage.path);

      io.rm(stage.path);
      delete stage.path;
    }

    add_code(obj.vs);
    if (!obj.fs && obj.vs.fs) {
      obj.fs = obj.vs.fs;
      delete obj.vs.fs;
    }

    add_code(obj.fs);

    obj.blend = blend;
    obj.cull = cull;
    obj.primitive = primitive;
    obj.depth = depth;
    obj.face = face;
    obj.indexed = indexed;
    obj.stencil = stencil === 'write';

    if (obj.vs.inputs)
      for (var i of obj.vs.inputs) {
        if (!(i.name in attr_map)) i.mat = -1;
        else i.mat = attr_map[i.name];
      }

    function make_unimap(stage) {
      if (!stage.uniform_blocks) return {};
      var unimap = {};
      for (var uni of stage.uniform_blocks) {
        var uniname = uni.struct_name[0] == "_" ? uni.struct_name.slice(1) : uni.struct_name;

        unimap[uniname] = {
          name: uniname,
          slot: Number(uni.slot),
          size: Number(uni.size),
        };
      }

      return unimap;
    }

    obj.vs.unimap = make_unimap(obj.vs);
    obj.fs.unimap = make_unimap(obj.fs);

    obj.name = file;

    strip_shader_inputs(obj);

    compiled[platform] = obj;
  }

  compiled.files = files;
  compiled.source = shader;

  return compiled;
}

function make_shader(shader) {
  if (shader_cache[shader]) return shader_cache[shader];

  var file = shader;
  shader = io.slurp(file);
  if (!shader) {
    console.info(`not found! slurping shaders/${file}`);
    shader = io.slurp(`shaders/${file}`);
  }
  var writejson = `.prosperon/${file.name()}.shader.json`;

  breakme: if (io.exists(writejson)) {
    var data = json.decode(io.slurp(writejson));
    var filemod = io.mod(writejson);
    if (!data.files) break breakme;
    for (var i of data.files) {
      if (io.mod(i) > filemod) {
        break breakme;
      }
    }
    
    var shaderobj = json.decode(io.slurp(writejson));
    var obj = shaderobj[os.sys()];
    obj.pipe = render.pipeline(obj);
    shader_cache[file] = obj;
    shader_times[file] = io.mod(file);
    return obj;
  }
  
  profile.report(`shader_${file}`);  

  var compiled = create_shader_obj(file);
  io.slurpwrite(writejson, json.encode(compiled));
  var obj = compiled[os.sys()];
  obj.pipe = render.pipeline(obj);

  shader_cache[file] = obj;
  shader_times[file] = io.mod(file);
  
  profile.endreport(`shader_${file}`);    

  return obj;
}

var shader_unisize = {
  4: render.setuniv,
  8: render.setuniv2,
  12: render.setuniv3,
  16: render.setuniv4,
};

function shader_globals(shader) {
  for (var p in shader.vs.unimap) set_global_uni(shader.vs.unimap[p], 0);

  for (var p in shader.fs.unimap) set_global_uni(shader.fs.unimap[p], 1);
}

function shader_apply_material(shader, material = {}, old = {}) {
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
  if (cur.mesh === mesh && cur.ssbo === ssbo) {
    cur.bind.ssbo = [ssbo];
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

render.device.doc = `Device resolutions given as [x,y,inches diagonal].`;

var textshader;
var circleshader;
var polyshader;
var slice9shader;
var parshader;
var spritessboshader;
var polyssboshader;
var maskshader;
var sprite_ssbo;

render.init = function () {
  textshader = make_shader("shaders/text_base.cg");
  render.spriteshader = make_shader("shaders/sprite.cg");
  spritessboshader = make_shader("shaders/sprite_ssbo.cg");
  render.postshader = make_shader("shaders/simplepost.cg");
  slice9shader = make_shader("shaders/9slice.cg");
  circleshader = make_shader("shaders/circle.cg");
  polyshader = make_shader("shaders/poly.cg");
  parshader = make_shader("shaders/baseparticle.cg");
  polyssboshader = make_shader("shaders/poly_ssbo.cg");
  maskshader = make_shader('shaders/mask.cg');
  poly_ssbo = render.make_textssbo();
  sprite_ssbo = render.make_textssbo();

  render.textshader = textshader;

  os.make_circle2d().draw = function () {
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
};

render.draw_sprites = true;
render.draw_particles = true;
render.draw_hud = true;
render.draw_gui = true;
render.draw_gizmos = true;

render.buckets = [];
render.sprites = function render_sprites() {
  profile.report("drawing");
  render.use_shader(spritessboshader);
  var buckets = component.sprite_buckets();
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
  profile.endreport("drawing");
};

render.circle = function render_circle(pos, radius, color, inner_radius = 1) {
  check_flush(undefined);

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
render.forceflush = function()
{
  if (nextflush) nextflush();
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
  render.use_shader(polyssboshader);
  var base = render.make_particle_ssbo(poly_cache.slice(0, poly_idx), poly_ssbo);
  render.use_mat({baseinstance:base});  
  render.draw(shape.centered_quad, poly_ssbo, poly_idx);
  poly_idx = 0;
}

render.line = function render_line(points, color = Color.white, thickness = 1) {
  for (var i = 0; i < points.length - 1; i++) {
    var a = points[i];
    var b = points[i + 1];
    var poly = poly_e();
    var dist = vector.distance(a, b);
    poly.transform.move(vector.midpoint(a, b));
    poly.transform.rotate([0, 0, -1], vector.angle([b.x - a.x, b.y - a.y]));
    poly.transform.scale = [dist, thickness, 1];
    poly.color = color;
  }
  check_flush(flush_poly);
};

/* All draw in screen space */
render.point = function (pos, size, color = Color.blue) {
  render.circle(pos, size, size, color);
};

render.cross = function render_cross(pos, size, color = Color.red, thickness = 1) {
  var a = [pos.add([0, size]), pos.add([0, -size])];
  var b = [pos.add([size, 0]), pos.add([-size, 0])];
  render.line(a, color, thickness);
  render.line(b, color, thickness);
};

render.arrow = function render_arrow(start, end, color = Color.red, wingspan = 4, wingangle = 10) {
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

render.boundingbox = function render_boundingbox(bb, color = Color.white) {
  render.line(bbox.topoints(bb).wrapped(1), color);
};

render.rectangle = function render_rectangle(lowerleft, upperright, color) {
  var transform = os.make_transform();
  var wh = [upperright.x - lowerleft.x, upperright.y - lowerleft.y];
  var poly = poly_e();
  poly.transform.move(vector.midpoint(lowerleft, upperright));
  poly.transform.scale = [wh.x, wh.y, 1];
  poly.color = color;
  check_flush(flush_poly);
};

render.box = function render_box(pos, wh, color = Color.white) {
  var poly = poly_e();
  poly.transform.move(pos);
  poly.transform.scale = [wh.x, wh.y, 1];
  poly.color = color;
  check_flush(flush_poly);
};

render.window = function render_window(pos, wh, color) {
  render.box(pos.add(wh.scale(0.5)), wh, color);
};

render.text_bb = function (str, size = 1, wrap = -1, pos = [0, 0]) {
  var bb = render.text_size(str, size, wrap);
  var w = bb.r - bb.l;
  var h = bb.t - bb.b;

  bb.r += pos.x;
  bb.l += pos.x;
  bb.t += pos.y;
  bb.b += pos.y;
  return bb;
};

render.text = function (str, pos, size = 1, color = Color.white, wrap = -1, anchor = [0, 1], cursor = -1) {
  var bb = render.text_bb(str, size, wrap, pos);
  gui.text(str, pos, size, color, wrap, cursor); // this puts text into buffer
  check_flush(render.flush_text);
  return bb;

  p.x -= w * anchor.x;
  bb.r += w * anchor.x;
  bb.l += w * anchor.x;
  p.y += h * (1 - anchor.y);
  bb.t += h * (1 - anchor.y);
  bb.b += h * (1 - anchor.y);

  return bb;
};

var lasttex = undefined;
var img_cache = [];
var img_idx = 0;

function flush_img() {
  if (img_idx === 0) return;
  render.use_shader(spritessboshader);
  var startidx = render.make_sprite_ssbo(img_cache.slice(0, img_idx), sprite_ssbo);
  render.use_mat({ baseinstance:startidx});  
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
      rect: [0, 0, 1, 1],
    };
    img_cache.push(e);
    return e;
  }
  return img_cache[img_idx - 1];
}

render.floodmask = function(val)
{
  render.use_shader(polyshader);
  render.use_mat({});
  render.draw(
}

render.mask = function mask(tex, pos, scale, rotation = 0)
{
  if (typeof tex === 'string') tex = game.texture(tex);

  render.use_shader(maskshader);
  var t = os.make_transform();
  t.pos = pos;
  t.scale = scale;
  set_model(t);
  render.use_mat({
    diffuse:tex,
    rect: [0,0,1,1]
  });
  render.draw(shape.quad);
}

render.image = function image(tex, pos, scale, rotation = 0, color = Color.white) {
  if (typeof tex === "string")
    tex = game.texture(tex);

  if (scale)
    scale = scale.div([tex.width, tex.height]);
  else
    scale = vector.v3one;

  if (!tex) return;

  if (!lasttex) {
    check_flush(flush_img);
    lasttex = tex;
  }

  if (lasttex !== tex) {
    flush_img();
    lasttex = tex;
  }

  var e = img_e();
  e.transform.trs(pos, undefined, scale);
  e.shade = color;
  e.texture = tex;
  
  return;
  var bb = {};
  bb.b = pos.y;
  bb.l = pos.x;
  bb.t = pos.y + tex.height * scale;
  bb.r = pos.x + tex.width * scale;
  return bb;
};

// pos is the lower left corner, scale is the width and height
render.slice9 = function (tex, pos, bb, scale = [tex.width, tex.height], color = Color.white) {
  var t = os.make_transform();
  t.pos = pos;
  t.scale = [scale.x / tex.width, scale.y / tex.height, 1];
  var border;
  if (typeof bb === "number") border = [bb / tex.width, bb / tex.height, bb / tex.width, bb / tex.height];
  else border = [bb.l / tex.width, bb.b / tex.height, bb.r / tex.width, bb.t / tex.height];

  render.use_shader(slice9shader);
  set_model(t);
  render.use_mat({
    shade: color,
    diffuse: tex,
    rect: [0, 0, 1, 1],
    border: border,
    scale: [scale.x / tex.width, scale.y / tex.height],
  });

  render.draw(shape.quad);
};

function endframe() {
  tdraw = 0;
}

var textssbos = [];
var tdraw = 0;

render.flush_text = function () {
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
  render.use_mat({ text: render.font.texture });

  render.draw(shape.quad, textssbo, amt);
};

var fontcache = {};
render.set_font = function (path, size) {
  var fontstr = `${path}-${size}`;
  if (render.font && fontcache[fontstr] === render.font) return;
  if (!fontcache[fontstr]) fontcache[fontstr] = os.make_font(path, size);

  render.flush_text();

  gui.font_set(fontcache[fontstr]);
  render.font = fontcache[fontstr];
};

render.doc = "Draw shapes in screen space.";
render.cross.doc = "Draw a cross centered at pos, with arm length size.";
render.arrow.doc = "Draw an arrow from start to end, with wings of length wingspan at angle wingangle.";
render.rectangle.doc = "Draw a rectangle, with its corners at lowerleft and upperright.";

render.draw = function render_draw(mesh, ssbo, inst = 1, e_start = 0) {
  sg_bind(mesh, ssbo);
  profile.report("gpu_draw");
  render.spdraw(e_start, cur.bind.count, inst);
  profile.endreport("gpu_draw");
};

// Returns an array in the form of [left, bottom, right, top] in pixels of the camera to render to
// Camera viewport is [left,bottom,width,height] in relative values
function camviewport() {
  var aspect = (((this.viewport[2] - this.viewport[0]) / (this.viewport[3] - this.viewport[1])) * window.size.x) / window.size.y;
  var raspect = this.size.x / this.size.y;

  var left = this.viewport[0] * window.size.x;
  var bottom = this.viewport[1] * window.size.y;

  var usemode = this.mode;

  if (this.break && this.size.x > window.size.x && this.size.y > window.size.y) usemode = this.break;

  if (usemode === "fit")
    if (raspect < aspect) usemode = "height";
    else usemode = "width";

  switch (usemode) {
    case "stretch":
    case "expand":
      return [0, 0, window.size.x, window.size.y];
    case "keep":
      return [left, bottom, left + this.size.x, bottom + this.size.y];
    case "height":
      var ret = [left, 0, this.size.x * (window.size.y / this.size.y), window.size.y];
      ret[0] = (window.size.x - (ret[2] - ret[0])) / 2;
      return ret;
    case "width":
      var ret = [0, bottom, window.size.x, this.size.y * (window.size.x / this.size.x)];
      ret[1] = (window.size.y - (ret[3] - ret[1])) / 2;
      return ret;
  }

  return [0, 0, window.size.x, window.size.y];
}

// pos is pixels on the screen, lower left[0,0]
function camscreen2world(pos) {
  var view = this.screen2cam(pos);
  view.x *= this.size.x;
  view.y *= this.size.y;
  view = view.sub([this.size.x / 2, this.size.y / 2]);
  view = view.add(this.pos.xy);
  return view;
}

// three coordinatse
// world coordinates, the "actual" view relative to the game's universe
// camera coordinates, normalized from 0 to 1 inside of a camera's viewport, bottom left is 0,0, top right is 1,1
// screen coordinates, pixels, 0,0 at the bottom left of the window and [w,h] at the top right of the screen

camscreen2world.doc = "Convert a view position for a camera to world.";

// return camera coordinates given a screen position
function screen2cam(pos) {
  var viewport = this.view();
  var width = viewport[2];
  var height = viewport[3];
  var viewpos = pos.sub([viewport[0], viewport[1]]);
  return viewpos.div([width, height]);
}

function camextents() {
  var half = this.size; //.scale(0.5);
  return {
    l: this.pos.x - half.x,
    r: this.pos.x + half.x,
    t: this.pos.y + half.y,
    b: this.pos.y - half.y,
  };
}

screen2cam.doc = "Convert a screen space position in pixels to a normalized viewport position in a camera.";

prosperon.gizmos = function () {
  game.all_objects(o => {
    if (o.gizmo) render.image(game.texture(o.gizmo), o.pos);
  });
};

prosperon.make_camera = function () {
  var cam = world.spawn();
  cam.near = 0.1;
  cam.far = 1000;
  cam.ortho = true;
  cam.viewport = [0, 0, 1, 1];
  cam.size = window.size.slice(); // The render size of this camera in pixels
  // In ortho mode, this determines how many pixels it will see
  cam.mode = "stretch";
  cam.screen2world = camscreen2world;
  cam.screen2cam = screen2cam;
  cam.extents = camextents;
  cam.mousepos = function () {
    return this.screen2world(input.mouse.screenpos());
  };
  cam.view = camviewport;
  cam.offscreen = false;
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

var imdebug = function () {
  imtoggle("Physics", debug, "draw_phys");
  imtoggle("Bouning boxes", debug, "draw_bb");
  imtoggle("Gizmos", debug, "draw_gizmos");
  imtoggle("Names", debug, "draw_names");
  imtoggle("Sprite nums", debug, "sprite_nums");
  imtoggle("Debug overlay", debug, "show");
  imtoggle("Show ur names", debug, "urnames");
};

var imgui_fn = function () {
  render.imgui_new(window.size.x, window.size.y, 0.01);
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
        window.title = imgui.textinput("Title", window.title);
        window.icon = imgui.textinput("Icon", window.icon);
        imgui.button("Refresh window", _ => {
          window.set_icon(game.texture(window.icon));
        });
      });
      imgui.button("quit", os.quit);
    });
    imgui.menu("Debug", imdebug);
    imgui.menu("View", _ => {
      imtoggle("Profiler", debug, "showprofiler");
      imtoggle("Terminal out", debug, "termout");
      imtoggle("Meta [f7]", debug, "meta");
      imtoggle("Cheats [f8]", debug, "cheat");
      imtoggle("Console [f9]", debug, "console");
    });

    imgui.sokol_gfx();

    imgui.menu("Graphics", _ => {
      imtoggle("Draw sprites", render, "draw_sprites");
      imtoggle("Draw particles", render, "draw_particles");
      imtoggle("Draw HUD", render, "draw_hud");
      imtoggle("Draw GUI", render, "draw_gui");
      imtoggle("Draw gizmos", render, "draw_gizmos");

      imgui.menu("Window", _ => {
        window.fullscreen = imgui.checkbox("fullscreen", window.fullscreen);
        //      window.vsync = imgui.checkbox("vsync", window.vsync);
        imgui.menu("MSAA", _ => {
          for (var msaa of gamestate.msaa) imgui.button(msaa + "x", _ => (window.sample_count = msaa));
        });
        imgui.menu("Resolution", _ => {
          for (var res of gamestate.resolutions) imgui.button(res, _ => (window.resolution = res));
        });
      });
    });

    prosperon.menu_hook?.();
  });

  prosperon.imgui();
  render.imgui_end();
};

prosperon.postvals = {};
prosperon.postvals.offset_amt = 300;
prosperon.render = function () {
  profile.report("world");
  render.set_camera(prosperon.camera);
  // figure out the highest resolution we can render at that's an integer
  var basesize = prosperon.camera.size.slice();
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
  profile.report("sprites");
  if (render.draw_sprites) render.sprites();
  if (render.draw_particles) draw_emitters();
  profile.endreport("sprites");
  profile.report("draws");
  prosperon.draw();
//  sgl.draw();
  profile.endreport("draws");
  profile.endreport("world");
  prosperon.hudcam.size = prosperon.camera.size;
  prosperon.hudcam.transform.pos = [prosperon.hudcam.size.x / 2, prosperon.hudcam.size.y / 2, -100];
  render.set_camera(prosperon.hudcam);

  profile.report("hud");
  if (render.draw_hud) prosperon.hud();
  render.flush_text();

  render.set_camera(prosperon.camera);
  //if (render.draw_gizmos && prosperon.gizmos) prosperon.gizmos();
  render.flush_text();

  render.end_pass();

  profile.endreport("hud");
  /* draw the image of the game world first */
  render.glue_pass();
  profile.report("frame");
  profile.report("render");
  profile.report("post process");
  render.viewport(...prosperon.camera.view());
  render.use_shader(render.postshader);
  prosperon.postvals.diffuse = prosperon.screencolor;
  render.use_mat(prosperon.postvals);
  render.draw(shape.quad);

  profile.endreport("post process");

  profile.report("app");

  // Flush & render
  prosperon.appcam.transform.pos = [window.size.x / 2, window.size.y / 2, -100];
  prosperon.appcam.size = window.size.slice();
  if (os.sys() !== "macos") prosperon.appcam.size.y *= -1;

  render.set_camera(prosperon.appcam);
  render.viewport(...prosperon.appcam.view());

  // Call gui functions
  mum.style = mum.dbg_style;
  if (render.draw_gui) prosperon.gui();
  if (mum.drawinput) mum.drawinput();
  render.flush_text();
  mum.style = mum.base;

  check_flush();

  profile.endreport("app");

  profile.report("imgui");

  if (debug.show) imgui_fn();

  profile.endreport("imgui");

  render.end_pass();

  render.commit();
  profile.report_frame(profile.secs(profile.now()) - frame_t);  

  endframe();
};

prosperon.process = function process() {
  profile.report("frame");
  var dt = profile.secs(profile.now()) - frame_t;
  frame_t = profile.secs(profile.now());

  var sst = profile.now();

  /* debugging: check for gc */
  profile.print_gc();

  var cycles = os.check_cycles();
  if (cycles) say(cycles);

  profile.report("app update");
  prosperon.appupdate(dt);
  profile.endreport("app update");  

  profile.report("input");
  input.procdown();
  profile.endreport("input");  

  if (sim.mode === "play" || sim.mode === "step") {
    profile.report("update");
    prosperon.update(dt * game.timescale);
    update_emitters(dt * game.timescale);
    os.update_timers(dt * game.timescale);
    profile.endreport("update");    
    if (sim.mode === "step") sim.pause();
  }

  profile.pushdata(profile.data.cpu.scripts, profile.now() - sst);
  sst = profile.now();

  if (sim.mode === "play" || sim.mode === "step") {
    profile.report("physics");
    physlag += dt;

    while (physlag > physics.delta) {
      physlag -= physics.delta;
      prosperon.phys2d_step(physics.delta * game.timescale);
      prosperon.physupdate(physics.delta * game.timescale);
    }
    profile.endreport("physics");    
    profile.pushdata(profile.data.cpu.physics, profile.now() - sst);
    sst = profile.now();
  }

  profile.report("render");
  prosperon.render();
  profile.endreport("render");  
  profile.pushdata(profile.data.cpu.render, profile.now() - sst);
  profile.endreport('frame');

  profile.capture_data();
};

return { render };
