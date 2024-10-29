var unit_transform = os.make_transform();

/*
  Anatomy of rendering an image
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

render.doc = {
  doc: "Functions for rendering modes.",
  normal: "Final render with all lighting.",
  wireframe: "Show only wireframes of models.",
};

var cur = {};
cur.images = [];
cur.samplers = [];

/* A pipeline defines the characteristics of the incoming draw.
  {
    shader <--- the shader determines the vertex layout, 
    depth
      compare always (never/less/equal/less_equal/greater/not_equal/greater_equal/always)
      write false
      bias 0
      bias_slope_scale 0
      bias_clamp 0
    stencil
      enabled false
      front/back
        compare always
        fail_op keep (zero/replace/incr_clamp/decr_clamp/invert/incr_wrap/decr_wrap)
        depth_fail_op keep
        pass_op keep
      read true
      write false
      ref 0 (0-255)
    color
      write_mask rgba
      blend
        enabled false
        src_factor_rgb one
        dst_factor_rgb zero
        op_rgb add
        src_factor_alpha one
        dst_factor_alpha zero
        op_alpha add
    cull none
    face_winding cw
    alpha_to_coverage false
    label ""
*/

var blendop = {
  add: 1,
  subtract: 2,
  reverse_subtract: 3
};

var blendfactor = {
  zero: 1,
  one: 2,
  src_color: 3,
  one_minus_src_color: 4,
  src_alpha: 5,
  one_minus_src_alpha: 6,
  dst_color: 7,
  one_minus_dst_color: 8,
  dst_alpha: 9,
  one_minus_dst_alpha: 10,
  src_alpha_saturated: 11,
  blend_color: 12,
  one_minus_blend_color: 13,
  blend_alpha: 14,
  one_minus_blend_alpha: 15
};

var colormask = {
  none: 0x10,
  r: 0x1,
  g: 0x2,
  rg: 0x3,
  b: 0x4,
  rb: 0x5,
  gb: 0x6,
  rgb: 0x7,
  a: 0x8,
  ra: 0x9,
  ga: 0xA,
  rga: 0xB,
  ba: 0xC,
  rba: 0xD,
  gba: 0xE,
  rgba: 0xF
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

var stencilop = {
  keep: 1,
  zero: 2,
  replace: 3,
  incr_clamp: 4,
  decr_clamp: 5,
  invert: 6,
  incr_wrap: 7,
  decr_wrap: 8
};

var face_map = {
  ccw: 1,
  cw: 2,
};

var compare = {
  never: 1,
  less: 2,
  equal: 3,
  less_equal: 4,
  greater: 5,
  not_equal: 6,
  greater_equal: 7,
  always: 8
};

var base_pipeline = {
  primitive: primitive_map.triangle,
  depth: {
    compare: compare.always,
    write: false,
    bias: 0,
    bias_slope_scale: 0,
    bias_clamp: 0
  },
  stencil: {
    enabled: true,
    front: {
      compare: compare.equal,
      fail_op: stencilop.keep,
      depth_fail_op: stencilop.keep,
      pass_op: stencilop.keep
    },
    back: {
      compare: compare.equal,
      fail_op: stencilop.keep,
      depth_fail_op: stencilop.keep,
      pass_op: stencilop.keep
    },
    read: true,
    write: false,
    ref: 0
  },
  write_mask: colormask.rgba,
  blend: {
    enabled: false,
    src_factor_rgb: blendfactor.one,
    dst_factor_rgb: blendfactor.zero,
    op_rgb: blendop.add,
    src_factor_alpha: blendfactor.one,
    dst_factor_alpha: blendfactor.zero,
    op_alpha: blendop.add,
  },
  cull: cull_map.none,
  face: face_map.cw,
  alpha_to_coverage: false,
  label: "scripted pipeline"
}

render.base_pipeline = base_pipeline;
render.colormask = colormask;
render.primitive_map = primitive_map;
render.cull_map = cull_map;
render.stencilop = stencilop;
render.face_map = face_map;
render.compare = compare;
render.blendfactor = blendfactor;

var pipe_shaders = new WeakMap();

// Uses the shader with the specified pipeline. If none specified, uses the base pipeline
render.use_shader = function use_shader(shader, pipeline = base_pipeline) {
  if (typeof shader === "string") shader = make_shader(shader);
  if (cur.shader === shader) return;
  
  if (!pipe_shaders.has(shader)) pipe_shaders.set(shader, new WeakMap());
  var shader_pipelines = pipe_shaders.get(shader);
  if (!shader_pipelines.has(pipeline)) {
    var new_pipeline = render.make_pipeline(shader,pipeline);
    shader_pipelines.set(pipeline, new_pipeline);
  }
  
  var use_pipeline = shader_pipelines.get(pipeline);
  if (cur.shader === shader && cur.pipeline === use_pipeline) return;
  
  cur.shader = shader;
  cur.bind = undefined;
  cur.mesh = undefined;
  cur.ssbo = undefined;
  cur.images = [];
  cur.pipeline = use_pipeline;
  
  // Grab or create a pipeline obj that utilizes the specific shader and pipeline
  render.setpipeline(use_pipeline);
  shader_globals(cur.shader);
};

render.use_pipeline = function use_pipeline(pipeline) {

}

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

    obj.indexed = true;

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

function make_shader(shader, pipe) {
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
    
    shader_cache[file] = obj;
    shader_times[file] = io.mod(file);
    return obj;
  }
  
  profile.report(`shader_${file}`);  

  var compiled = create_shader_obj(file);
  io.slurpwrite(writejson, json.encode(compiled));
  var obj = compiled[os.sys()];

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

render.device.doc = `Device resolutions given as [x,y,inches diagonal].`;

var textshader;
var circleshader;
var polyshader;
var slice9shader;
var parshader;
var spritessboshader;
var polyssboshader;
var sprite_ssbo;

render.init = function () {
  textshader = make_shader("shaders/text_base.cg");
  render.spriteshader = make_shader("shaders/sprite.cg");
  spritessboshader = make_shader("shaders/sprite_ssbo.cg");
  var postpipe = Object.create(base_pipeline);
  postpipe.cull = cull_map.none;
  postpipe.primitive = primitive_map.triangle;
  render.postshader = make_shader("shaders/simplepost.cg", postpipe);
  slice9shader = make_shader("shaders/9slice.cg");
  circleshader = make_shader("shaders/circle.cg");
  polyshader = make_shader("shaders/poly.cg");
  parshader = make_shader("shaders/baseparticle.cg");
  polyssboshader = make_shader("shaders/poly_ssbo.cg");
  poly_ssbo = render.make_textssbo();
  sprite_ssbo = render.make_textssbo();

  render.textshader = textshader;

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
  profile.report("sprites");
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
  profile.endreport("sprites");
};

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
render.forceflush = function()
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
    poly.transform.rotate([0, 0, 1], vector.angle([b.x - a.x, b.y - a.y]));
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

var queued_shader;
var queued_pipe;
render.rectangle = function render_rectangle(rect, color = Color.white, shader = polyssboshader, pipe = base_pipeline) {
  var transform = os.make_transform();
  var wh = [rect.width, rect.height];
  var poly = poly_e();
  var pos = [rect.x,rect.y].add([rect.width,rect.height].scale(0.5));
  pos = pos.sub([rect.width,rect.height].scale([rect.anchor_x,rect.anchor_y]));
  poly.transform.move(pos);
  poly.transform.scale = [wh.x, wh.y, 1];
  poly.color = color;
  
  queued_shader = shader;
  queued_pipe = pipe;
  check_flush(flush_poly);
};

render.window = function render_window(pos, wh, color) {
  render.box(pos.add(wh.scale(0.5)), wh, color);
};

render.text = function (str, rect, font = cur_font, size = 0, color = Color.white, wrap = -1, ) {
  if (typeof font === 'string')
    font = render.get_font(font);

  if (!font) return;
  var pos = [rect.x,rect.y];
  pos.y -= font.descent;
  if (rect.anchor_y)
    pos.y -= rect.anchor_y*(font.ascent-font.descent);
  gui.text(str, pos, size, color, wrap, font); // this puts text into buffer
  cur_font = font;
  check_flush(render.flush_text);
};

var tttsize = render.text_size;
render.text_size = function(str, font)
{
  if (typeof font === 'string')
    font = render.get_font(font);
  return tttsize(str,font); 
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
  compare: compare.always,
  fail_op: stencilop.replace,
  depth_fail_op:stencilop.replace,
  pass_op: stencilop.replace
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
render.fillmask = function(ref)
{
  render.forceflush();
  var pipe = stencil_writer(ref);
  render.use_shader('shaders/screenfill.cg', pipe);
  render.draw(shape.quad);
}

var stencil_invert = {
  compare: compare.always,
  fail_op: stencilop.invert,
  depth_fail_op: stencilop.invert,
  pass_op: stencilop.invert
};
var stencil_inverter = Object.create(base_pipeline);
Object.assign(stencil_inverter, {
  stencil: {
    enabled: true,
    front: stencil_invert,
    back:stencil_invert,
    write:true,
    read:true,
    ref: 0
  },
  write_mask: colormask.none
});
render.invertmask = function()
{
  render.forceflush();
  render.use_shader('shaders/screenfill.cg', stencil_inverter);
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
  render.use_shader('shaders/sprite.cg', pipe);
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

render.image = function image(image, rect = [0,0], rotation = 0, color = Color.white) {
  if (typeof image === "string")
    image = game.texture(image);

  var tex = image.texture;
  if (!tex) return;

  var image_size = calc_image_size(image);
  
  var size = [rect.width ? rect.width : image_size.x, rect.height ? rect.height : image_size.y];

  if (!lasttex) {
    check_flush(flush_img);
    lasttex = tex;
  }

  if (lasttex !== tex) {
    flush_img();
    lasttex = tex;
  }

  var e = img_e();
  var pos = [rect.x,rect.y].sub(size.scale([rect.anchor_x, rect.anchor_y]));
  e.transform.trs(pos, undefined, size);
  e.image = image;
  e.shade = color;

  return;
};

var slice9_t = os.make_transform();
// pos is the lower left corner, scale is the width and height
// slice is given in pixels
render.slice9 = function (image, rect = [0,0], slice = 0, color = Color.white) {
  if (typeof image === 'string')
    image = game.texture(image);

  var tex = image.texture;
  var image_size = calc_image_size(image);
  var size = [rect.width ? rect.width : image_size.x, rect.height ? rect.height : image_size.y];
  
  slice9_t.trs([rect.x,rect.y].sub(size.scale([rect.anchor_x, rect.anchor_y])), undefined, size);
  slice = clay.normalizeSpacing(slice);
  var border = [slice.l/image_size.x, slice.b/image_size.y, slice.r/image_size.x, slice.t/image_size.y];
  render.use_shader(slice9shader);
  set_model(slice9_t);
  render.use_mat({
    shade: color,
    diffuse: tex,
    win_tex_scale: size.div(image_size),
    rect: [image.rect.x, image.rect.y,image.rect.width,image.rect.height],
    frag_rect: [image.rect.x, image.rect.y,image.rect.width,image.rect.height],    
    border: border,
  });

  render.draw(shape.quad);
};

function endframe() {
  tdraw = 0;
}

var textssbos = [];
var tdraw = 0;
var cur_font = undefined;

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
  render.use_mat({ text: cur_font.texture });
  render.draw(shape.quad, textssbo, amt);
};

var fontcache = {};

render.get_font = function(path,size)
{
  var parts = path.split('.');
  if (!isNaN(parts[1])) {
    path = parts[0];
    size = Number(parts[1]);
  }
  path = Resources.find_font(path);
  var fontstr = `${path}.${size}`;
  if (!fontcache[fontstr]) fontcache[fontstr] = os.make_font(path,size);
  return fontcache[fontstr];
}

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

// Camera viewport is a rectangle with the bottom left corner defined as x,y. Units are pixels on the window.
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
      return {
        x: 0,
        y: 0,
        width: window.size.x,
        height: window.size.y
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
        width:this.size.x*(window.size.y/this.size.y),
        height:window.size.y
      };
      ret.x = (window.size.x - (ret.width-ret.x))/2;
      return ret;
    case "width":
      var ret = {
        x:0,
        y:bottom,
        width:window.size.x,
        height:this.size.y*(window.size.x/this.size.x)
      };
      ret.y = (window.size.y - (ret.height-ret.y))/2;
      return ret;
  }

  return {
    x:0,
    y:0,
    width:window.size.x,
    height:window.size.y
  };
}

// pos is pixels on the screen, lower left[0,0]
function camscreen2world(pos) {
  var view = this.screen2cam(pos);
  view.x *= this.size.x;
  view.y *= this.size.y;
  view = view.add(this.pos.xy);
  view = view.scale(this.transform.scale);
  return view;
}

// world coordinates, the "actual" view relative to the game's universe
// camera coordinates, normalized from 0 to 1 inside of a camera's viewport, bottom left is 0,0, top right is 1,1
// screen coordinates, pixels, 0,0 at the top left of the window and [w,h] at the top right of the screen
// hud coordinates, same as screen coordinates but the top left is 0,0

camscreen2world.doc = "Convert a view position for a camera to world.";

// return camera coordinates given a screen position
function screen2cam(pos) {
  var winsize = window.size.slice();
  var viewport = this.view();
  var viewpos = pos.sub([viewport.x,viewport.y]);
  viewpos = viewpos.div([viewport.width,viewport.height]);
  viewpos.y += 1;
  return viewpos;
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

function screen2hud(pos)
{
  var campos = this.screen2cam(pos);
  campos = campos.scale(this.size);
  campos.y -= this.size.y;
  return campos;
}

prosperon.make_camera = function () {
  var cam = world.spawn();
  cam.near = 1;
  cam.far = -1000;
  cam.ortho = true;
  cam.viewport = [0, 0, 1, 1]; // normalized screen coordinates of where to draw
  cam.size = window.size.slice(); // The render size of this camera in pixels
  // In ortho mode, this determines how many pixels it will see
  cam.mode = "stretch";
  cam.screen2world = camscreen2world;
  cam.screen2cam = screen2cam;
  cam.screen2hud = screen2hud;
  cam.extents = camextents;
  cam.view = camviewport;
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
  imgui.init();
  imgui.newframe(window.size.x, window.size.y, 0.01);
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
  imgui.endframe();
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

prosperon.render = function () {
  render.glue_pass();
  render.set_view(prosperon.camera.transform);
  render.set_projection_ortho({
    l:-prosperon.camera.size.x/2,
    r:prosperon.camera.size.x/2,
    b:-prosperon.camera.size.y/2,
    t:prosperon.camera.size.y/2
  }, prosperon.camera.near,prosperon.camera.far);
  render.viewport(prosperon.camera.view(), false);
  
  if (render.draw_sprites) render.sprites();
  if (render.draw_particles) draw_emitters();
  prosperon.draw();
  render.fillmask(0);
  render.forceflush();
  
  render.set_projection_ortho({
    l:0,
    r:prosperon.camera.size.x,
    b:-prosperon.camera.size.y,
    t:0
  },-1,1);
  
  render.set_view(unit_transform);
  if (render.draw_hud) prosperon.hud();
  render.forceflush();

  render.set_projection_ortho({
    l:0,
    r:window.size.x,
    b:-window.size.y,
    t:0
  },-1,1);
  render.viewport({
    t:0,
    height:window.size.y,
    width:window.size.x,
    l:0
  }, false);
  prosperon.app();
  render.forceflush();

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
/*    profile.report("physics");
    physlag += dt;

    while (physlag > physics.delta) {
      physlag -= physics.delta;
      prosperon.phys2d_step(physics.delta * game.timescale);
      prosperon.physupdate(physics.delta * game.timescale);
    }
  
    profile.endreport("physics");    
    profile.pushdata(profile.data.cpu.physics, profile.now() - sst);
    sst = profile.now();
  */  
  }

  profile.report("render");
  prosperon.render();
  profile.endreport("render");  
  profile.pushdata(profile.data.cpu.render, profile.now() - sst);
  profile.endreport('frame');

  profile.capture_data();
};

return { render };
