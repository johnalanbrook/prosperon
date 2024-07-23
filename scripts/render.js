render.doc = {
  doc: "Functions for rendering modes.",
  normal: "Final render with all lighting.",
  wireframe: "Show only wireframes of models."
};

var cur = {};

render.use_shader = function(shader)
{
  if (typeof shader === 'string')
    shader = render.make_shader(shader);
  if (cur.shader === shader) return;
  cur.shader = shader;
  cur.globals = {};
  cur.bind = undefined;
  cur.mesh = undefined;
  render.setpipeline(shader.pipe);
}

render.use_mat = function(mat)
{
  if (!cur.shader) return;
  if (cur.mat === mat) return;

  render.shader_apply_material(cur.shader, mat, cur.mat);
  
  cur.mat = mat;

  cur.images = [];
  if (!cur.shader.fs.images) return;
  for (var img of cur.shader.fs.images)
    if (mat[img.name])
      cur.images.push(mat[img.name]);
    else
      cur.images.push(game.texture("icons/no_tex.gif"));
}

var models_array = [];

render.set_model = function(t)
{
  if (cur.shader.vs.unimap.model)
    render.setunim4(0, cur.shader.vs.unimap.model.slot, t);
}

var shaderlang = {
 macos: "metal_macos",
 windows: "hlsl5",
 linux: "glsl430",
 web: "wgsl",
 ios: "metal_ios",
}

var attr_map = {
  a_pos: 0,
  a_uv: 1,  
  a_norm: 2,
  a_bone: 3,
  a_weight: 4,
  a_color: 5,
  a_tan: 6,
  a_angle: 7,
  a_wh: 8,
  a_st: 9,
  a_ppos: 10,
  a_scale: 11
}

var blend_map = {
  mix: true,
  none: false 
}

var primitive_map = {
  point: 1,
  line: 2,
  linestrip: 3,
  triangle: 4,
  trianglestrip: 5
}

var cull_map = {
 none: 1,
 front: 2,
 back: 3 
}

var depth_map = {
  off: false,
  on: true
}

var face_map = {
  cw: 2,
  ccw: 1
}

render.poly_prim = function(verts)
{
  var index = [];
  if (verts.length < 1) return undefined;
  
  for (var i = 0; i < verts.length; i++)
    verts[i][2] = 0;
  
  for (var i = 2; i < verts.length; i++) {
    index.push(0);
    index.push(i-1);
    index.push(i);
  }
  
  return {
    pos: os.make_buffer(verts.flat()),
    verts: verts.length,
    index: os.make_buffer(index, 1),
    count: index.length
  };
}

function shader_directive(shader, name, map)
{
  var reg = new RegExp(`#${name}.*`, 'g');
  var mat = shader.match(reg);
  if (!mat) return undefined;
  
  reg = new RegExp(`#${name}\s*`, 'g');
  var ff = mat.map(d=>d.replace(reg,''))[0].trim();
  
  if (map) return map[ff];
  return ff;
}

function global_uni(uni, stage)
{
  cur.globals[stage] ??= {};
  if (cur.globals[stage][uni.name]) return true;
  switch(uni.name) {
    case "time":
      cur.globals[stage][uni.name]
      render.setuniv(stage, uni.slot, profile.secs(profile.now()));
      cur.globals[stage][uni.name] = true;
      return true;
    case "projection":
      render.setuniproj(stage, uni.slot);
      cur.globals[stage][uni.name] = true;
      return true;
    case "view":
      render.setuniview(stage, uni.slot);
      cur.globals[stage][uni.name] = true;
      return true;
    case "vp":
      render.setunivp(stage, uni.slot);
      cur.globals[stage][uni.name] = true;
      return true;
  }
  
  return false;
}

var setcam = render.set_camera;
render.set_camera = function(cam)
{
  if (nextflush) {
    nextflush();
    nextflush = undefined;
  }
  delete cur.shader;
  setcam(cam);
}

var shader_cache = {};
 
render.make_shader = function(shader)
{
  if (shader_cache[shader]) return shader_cache[shader];
  
  var file = shader;
  shader = io.slurp(shader);
  if (!shader) {
    console.info(`not found! slurping shaders/${file}`);
    shader = io.slurp(`shaders/${file}`);
  }
  var writejson = `.prosperon/${file.name()}.shader.json`;
  var st = profile.now();
  
  breakme: if (io.exists(writejson)) {
    var data = json.decode(io.slurp(writejson));
    var filemod = io.mod(writejson);
    if (!data.files) break breakme;
    for (var i of data.files) {
      if (io.mod(i) > filemod) {
        break breakme;
      }
    }
  
    profile.report(st, `CACHE make shader from ${file}`);
    var shaderobj = json.decode(io.slurp(writejson));
    var obj = shaderobj[os.sys()];
    obj.pipe = render.pipeline(obj);
    shader_cache[shader] = obj;
    return obj;
  }
  
  var out = `.prosperon/${file.name()}.shader`;
  
  var files = [file];

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
  
  var blend = shader_directive(shader, 'blend', blend_map);
  var primitive = shader_directive(shader, 'primitive', primitive_map);
  var cull = shader_directive(shader, 'cull', cull_map);
  var depth = shader_directive(shader, 'depth', depth_map);
  var face = shader_directive(shader, 'face', face_map);
  var indexed = shader_directive(shader, 'indexed');
  
  if (typeof indexed == 'undefined') indexed = true;
  if (indexed === 'false') indexed = false;
  
  shader = shader.replace(/uniform\s+(\w+)\s+(\w+);/g, "uniform _$2 { $1 $2; };");
  shader = shader.replace(/(texture2D|sampler) /g, "uniform $1 ");
//  shader = shader.replace(/uniform texture2D ?(.*);/g, "uniform _$1_size { vec2 $1_size; };\nuniform texture2D $1;");

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
  if (!obj.fs)
   if (obj.vs.fs) {
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

  if (obj.vs.inputs)
    for (var i of obj.vs.inputs) {
      if (!(i.name in attr_map))
        i.mat = -1;
      else
        i.mat = attr_map[i.name];
    }
    
  function make_unimap(stage) {
    if (!stage.uniform_blocks) return {};
    var unimap = {};
    for (var uni of stage.uniform_blocks) {
      var uniname = uni.struct_name[0] == "_" ? uni.struct_name.slice(1) : uni.struct_name;
        
      unimap[uniname] = {
        name: uniname,
        slot: Number(uni.slot),
        size: Number(uni.size)
      };
    }
    
    return unimap;
  }
  
  obj.vs.unimap = make_unimap(obj.vs);
  obj.fs.unimap = make_unimap(obj.fs);
  
  obj.name = file;

  compiled[platform] = obj;
  }

  compiled.files = files;
  
  io.slurpwrite(writejson, json.encode(compiled));
  profile.report(st, `make shader from ${file}`);
  
  var obj = compiled[os.sys()];
  obj.pipe = render.pipeline(obj);

  shader_cache[shader] = obj;
  return obj;
}

var shader_unisize = {
  4: render.setuniv,
  8: render.setuniv2,
  12: render.setuniv3,
  16: render.setuniv4
};
  
render.shader_apply_material = function(shader, material = {}, old = {})
{
  for (var p in shader.vs.unimap) {
    if (global_uni(shader.vs.unimap[p], 0)) continue;
    if (material[p] === old[p]) continue;
    assert(p in material, `shader ${shader.name} has no uniform for ${p}`);
    var s = shader.vs.unimap[p];
    shader_unisize[s.size](0, s.slot, material[p]);
  }
  
  for (var p in shader.fs.unimap) {
    if (global_uni(shader.fs.unimap[p], 1)) continue;
    if (material[p] === old[p]) continue;
    assert(p in material, `shader ${shader.name} has no uniform for ${p}`);    
    var s = shader.fs.unimap[p];
    shader_unisize[s.size](1, s.slot, material[p]);
  }
  
  if (!material.diffuse) return;
  if (material.diffuse === old.diffuse) return;
  
  if ("diffuse_size" in shader.fs.unimap)
    render.setuniv2(1, shader.fs.unimap.diffuse_size.slot, [material.diffuse.width, material.diffuse.height]);

  if ("diffuse_size" in shader.vs.unimap)
    render.setuniv2(0, shader.vs.unimap.diffuse_size.slot, [material.diffuse.width, material.diffuse.height]);
}

render.sg_bind = function(mesh, ssbo)
{
  if (cur.mesh === mesh && cur.bind) {
    cur.bind.inst = 1;
    cur.bind.images = cur.images;
    render.setbind(cur.bind);
    return cur.bind;
  }
  
  cur.mesh = mesh;
  
  var bind = {};
  bind.attrib = [];
  if (cur.shader.vs.inputs)
  for (var a of cur.shader.vs.inputs) {
    if (!(a.name in mesh)) {
      if (!(a.name.slice(2) in mesh)) {
        console.error(`cannot draw shader ${cur.shader.name}; there is no attrib ${a.name} in the given mesh.`);
        return undefined;
      } else
      bind.attrib.push(mesh[a.name.slice(2)]);
    } else
    bind.attrib.push(mesh[a.name]);
  }
  
  if (cur.shader.indexed) {
    bind.index = mesh.index;
    bind.count = mesh.count;
  } else
    bind.count = mesh.verts;
    
  bind.ssbo = [];
  if (cur.shader.vs.storage_buffers)
  for (var b of cur.shader.vs.storage_buffers)
    bind.ssbo.push(ssbo);

  bind.inst = 1;
  bind.images = cur.images;

  cur.bind = bind;

  render.setbind(cur.bind);
  
  return bind;
}

render.device = {
  pc: [1920,1080],
  macbook_m2: [2560,1664, 13.6],
  ds_top: [400,240, 3.53],
  ds_bottom: [320,240, 3.02],
  playdate: [400,240,2.7],
  switch: [1280,720, 6.2],
  switch_lite: [1280,720,5.5],
  switch_oled: [1280,720,7],
  dsi: [256,192,3.268],
  ds: [256,192, 3],
  dsixl: [256,192,4.2],
  ipad_air_m2: [2360,1640, 11.97],
  iphone_se: [1334, 750, 4.7],
  iphone_12_pro: [2532,1170,6.06],
  iphone_15: [2556,1179,6.1],
  gba: [240,160,2.9],
  gameboy: [160,144,2.48],
  gbc: [160,144,2.28],
  steamdeck: [1280,800,7],
  vita: [960,544,5],
  psp: [480,272,4.3],
  imac_m3: [4480,2520,23.5],
  macbook_pro_m3: [3024,1964, 14.2],
  ps1: [320,240,5],
  ps2: [640,480],
  snes: [256,224],
  gamecube: [640,480],
  n64: [320,240],
  c64: [320,200],
  macintosh: [512,342,9],
  gamegear: [160,144,3.2],
};

render.device.doc = `Device resolutions given as [x,y,inches diagonal].`;

var textshader;
var circleshader;
var polyshader;
var slice9shader;
var parshader;
var spritessboshader;
var polyssboshader;

render.init = function() {
  textshader = render.make_shader("shaders/text_base.cg");
  render.spriteshader = render.make_shader("shaders/sprite.cg");
  spritessboshader = render.make_shader("shaders/sprite_ssbo.cg");
  render.postshader = render.make_shader("shaders/simplepost.cg");
  slice9shader = render.make_shader("shaders/9slice.cg");
  circleshader = render.make_shader("shaders/circle.cg");
  polyshader = render.make_shader("shaders/poly.cg");
  parshader = render.make_shader("shaders/baseparticle.cg");
  polyssboshader = render.make_shader("shaders/poly_ssbo.cg");
  textssbo = render.make_textssbo();
  poly_ssbo = render.make_textssbo();
  
  render.textshader = textshader;
  
  os.make_circle2d().draw = function() {
    render.circle(this.body().transform().pos, this.radius, [1,1,0,1]);
  }
  
  var disabled = [148/255,148/255, 148/255, 1];
  var sleep = [1, 140/255, 228/255, 1];
  var dynamic = [1, 70/255, 46/255, 1];
  var kinematic = [1, 194/255, 64/255, 1];
  var static_color = [73/255, 209/255, 80/255, 1];
  
  os.make_poly2d().draw = function() {
    var body = this.body();
    var color = body.sleeping() ? [0,0.3,0,0.4] : [0,1,0,0.4];
    var t = body.transform();
    render.poly(this.points, color, body.transform());
    color.a = 1;
    render.line(this.points.wrapped(1), color, 1, body.transform());
  }
  
  os.make_seg2d().draw = function() {
    render.line([this.a(), this.b()], [1,0,1,1], Math.max(this.radius/2, 1), this.body().transform());
  }
  
  joint.pin().draw = function() {
    var a = this.bodyA();
    var b = this.bodyB();
    render.line([a.transform().pos.xy, b.transform().pos.xy], [0,1,1,1], 1);
  }
}

render.circle = function(pos, radius, color) {
  check_flush();
  var mat = {
    radius: radius,
    coord: pos,
    shade: color
  };
  render.use_shader(circleshader);
  render.use_mat(mat);
  render.draw(shape.quad);
}

render.poly = function(points, color, transform) {
  var buffer = render.poly_prim(points);
  var mat = { shade: color};
  render.use_shader(polyshader);
  render.set_model(transform);
  render.use_mat(mat);
  render.draw(buffer);
}

var nextflush = undefined;
var check_flush = function(flush_fn)
{
  if (!flush_fn) {
    if (!nextflush) return;
    nextflush();
    nextflush = undefined;
  }
  if (!nextflush)
    nextflush = flush_fn;
  else if (nextflush !== flush_fn) {
      nextflush();
      nextflush = flush_fn;
    }
}

var poly_cache = [];
var poly_ssbo;

render.flush_poly = function()
{
  if (poly_cache.length === 0) return;
  render.use_shader(polyssboshader);
  render.use_mat({});
  render.make_particle_ssbo(poly_cache, poly_ssbo);
  render.draw(shape.centered_quad, poly_ssbo, poly_cache.length);
  poly_cache = [];
}

render.line = function(points, color = Color.white, thickness = 1) {
  var transform = os.make_transform();
  var dist = Vector.distance(points[0],points[1]);
  transform.move(Vector.midpoint(points[0],points[1]));
  transform.rotate([0,0,-1], Vector.angle([points[1].x-points[0].x, points[1].y-points[0].y]));
  transform.scale = [dist, thickness, 1];
  poly_cache.push({
    transform:transform,
    color:color
  });
  
  check_flush(render.flush_poly);
}

/* All draw in screen space */
render.point =  function(pos,size,color = Color.blue) {
  render.circle(pos,size,size,color);
};
  
render.cross = function(pos, size, color = Color.red, thickness = 1) {
  var a = [
    pos.add([0,size]),
    pos.add([0,-size])
  ];
  var b = [
    pos.add([size,0]),
    pos.add([-size,0])
  ];
  render.line(a,color,thickness);
  render.line(b,color,thickness);
};
  
render.arrow = function(start, end, color = Color.red, wingspan = 4, wingangle = 10) {
  var dir = end.sub(start).normalized();
  var wing1 = [
    Vector.rotate(dir, wingangle).scale(wingspan).add(end),
    end
  ];
  var wing2 = [
    Vector.rotate(dir,-wingangle).scale(wingspan).add(end),
    end
  ];
  render.line([start,end],color);
  render.line(wing1,color);
  render.line(wing2,color);
};

render.coordinate = function(pos, size, color) {
  render.text(JSON.stringify(pos.map(p=>Math.round(p))), pos, size, color);
  render.point(pos, 2, color);
}

render.boundingbox = function(bb, color = Color.white) {
  render.line(bbox.topoints(bb).wrapped(1), color);
}

render.rectangle = function(lowerleft, upperright, color) {
  var transform = os.make_transform();
  var wh = [upperright.x-lowerleft.x, upperright.y-lowerleft.y];
  transform.move(Vector.midpoint(lowerleft,upperright));
  transform.scale = [wh.x,wh.y,1];
  poly_cache.push({
    transform:transform,
    color:color
  });
  check_flush(render.flush_poly);
};
  
render.box = function(pos, wh, color = Color.white) {
  var transform = os.make_transform();
  transform.move(pos);
  transform.scale = [wh.x,wh.y,1];
  poly_cache.push({
    transform:transform,
    color:color
  });
  check_flush(render.flush_poly);  
};

render.window = function(pos, wh, color) {
  var p = pos.slice();
  p = p.add(wh.scale(0.5));
  render.box(p,wh,color);
};

render.text_bb = function(str, size = 1, wrap = -1, pos = [0,0])
{
  var bb = render.text_size(str,size,wrap);
  var w = (bb.r - bb.l);
  var h = (bb.t - bb.b);

  bb.r += pos.x;
  bb.l += pos.x;
  bb.t += pos.y;
  bb.b += pos.y;
  return bb;
}

render.text = function(str, pos, size = 1, color = Color.white, wrap = -1, anchor = [0,1], cursor = -1) {
  var bb = render.text_bb(str, size, wrap, pos);
  gui.text(str, pos, size, color, wrap, cursor);
  check_flush(render.flush_text);
  return bb;
  
  p.x -= w * anchor.x;
  bb.r += (w*anchor.x);
  bb.l += (w*anchor.x);
  p.y += h * (1 - anchor.y);
  bb.t += h*(1-anchor.y);
  bb.b += h*(1-anchor.y);

  return bb;
};

render.image = function(tex, pos, scale = [tex.width, tex.height], rotation = 0, color = Color.white) {
  check_flush();
  var t = os.make_transform();
  t.pos = pos;
  t.scale = [scale.x/tex.width,scale.y/tex.height,1];
  render.use_shader(render.spriteshader);
  render.set_model(t);
  render.use_mat({
    shade: color,
    diffuse: tex,
    rect:[0,0,1,1]
  });
  
  render.draw(shape.quad);
  
  var bb = {};
  bb.b = pos.y;
  bb.l = pos.x;
  bb.t = pos.y + tex.height*scale;
  bb.r = pos.x + tex.width*scale;
  return bb;
}


// pos is the lower left corner, scale is the width and height
render.slice9 = function(tex, pos, bb, scale = [tex.width,tex.height], color = Color.white)
{
  var t = os.make_transform();
  t.pos = pos;
  t.scale = [scale.x/tex.width,scale.y/tex.height,1];
  var border;
  if (typeof bb === 'number')
    border = [bb/tex.width,bb/tex.height,bb/tex.width,bb/tex.height];
  else
    border = [bb.l/tex.width, bb.b/tex.height, bb.r/tex.width, bb.t/tex.height];

  render.use_shader(slice9shader);
  render.set_model(t);
  render.use_mat({
    shade: color,
    diffuse:tex,
    rect:[0,0,1,1],
    border: border,
    scale: [scale.x/tex.width,scale.y/tex.height]
  });

  render.draw(shape.quad);
}

render.emitter = function(emit)
{
  var amt = Object.values(emit.particles).length;
  if (amt === 0) return;
  render.use_shader(parshader);
  render.use_mat(emit);
  var ts = [];
  for (var p of Object.values(emit.particles)) ts.push([p.transform,p.color]);
  render.make_particle_ssbo(ts, emit.ssbo);
  render.draw(shape.quad, emit.ssbo, amt);
}

var textssbo;

render.flush_text = function()
{
  if (!render.textshader) return;
  var amt = render.flushtext(textssbo);
  
  render.use_shader(render.textshader);
  render.use_mat({text:render.font.texture});

  if (amt === 0) return;
  
  render.draw(shape.quad, textssbo, amt);
}

render.fontcache = {};
render.set_font = function(path, size) {
  var fontstr = `${path}-${size}`;
  if (render.font && render.fontcache[fontstr] === render.font) return;
  if (!render.fontcache[fontstr]) render.fontcache[fontstr] = os.make_font(path, size);
  
  render.flush_text();

  gui.font_set(render.fontcache[fontstr]);
  render.font = render.fontcache[fontstr];
}

render.doc = "Draw shapes in screen space.";
//render.circle.doc = "Draw a circle at pos, with a given radius and color.";
render.cross.doc = "Draw a cross centered at pos, with arm length size.";
render.arrow.doc = "Draw an arrow from start to end, with wings of length wingspan at angle wingangle.";
render.rectangle.doc = "Draw a rectangle, with its corners at lowerleft and upperright.";

render.draw = function(mesh, ssbo, inst = 1)
{
  render.sg_bind(mesh, ssbo);
  render.spdraw(cur.bind.count, inst);
}

return {render};
