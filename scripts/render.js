render.doc = {
  doc: "Functions for rendering modes.",
  normal: "Final render with all lighting.",
  wireframe: "Show only wireframes of models."
};

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

/* All draw in screen space */
render.point =  function(pos,size,color) {
    color ??= Color.blue;
    render.circle(pos,size,size,color);
};
  
var tmpline = render.line;
render.line = function(points, color, thickness) {
    thickness ??= 1;
    color ??= Color.white;
    tmpline(points,color,thickness);
  };

render.cross = function(pos, size, color) {
    color ??= Color.red;
    var a = [
      pos.add([0,size]),
      pos.add([0,-size])
    ];
    var b = [
      pos.add([size,0]),
      pos.add([-size,0])
    ];
    
    render.line(a,color);
    render.line(b,color);
  };
  
render.arrow = function(start, end, color, wingspan, wingangle) {
  color ??= Color.red;
  wingspan ??= 4;
  wingangle ??=10;
  
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
    color ??= Color.white;
    render.text(JSON.stringify(pos.map(p=>Math.round(p))), pos, size, color);
    render.point(pos, 2, color);
}

render.boundingbox = function(bb, color) {
  color ??= Color.white;
  cmd_points(0, bbox.topoints(bb), color);
}

render.rectangle = function(lowerleft, upperright, color) {
  var points = [lowerleft, lowerleft.add([upperright.x-lowerleft.x,0]), upperright, lowerleft.add([0,upperright.y-lowerleft.y])];
  render.poly(points, color);
};
  
render.box = function(pos, wh, color) {
  color ??= Color.white;
  var lower = pos.sub(wh.scale(0.5));
  var upper = pos.add(wh.scale(0.5));
  render.rectangle(lower,upper,color);
};

render.text = function(str, pos, size, color, wrap, anchor, cursor) {
  size ??= 1;
  color ??= Color.white;
  wrap ??= -1;
  anchor ??= [0,1];
  
  cursor ??= -1;
  
  var bb = render.text_size(str, size, wrap);
  var w = bb.r*2;
  var h = bb.t*2;
  
  //gui.text draws with an anchor on top left corner
  var p = pos.slice();
  p.x -= w * anchor.x;
  bb.r += (w*anchor.x);
  bb.l += (w*anchor.x);
  p.y += h * (1 - anchor.y);
  bb.t += h*(1-anchor.y);
  bb.b += h*(1-anchor.y);
  gui.text(str, p, size, color, wrap, cursor);
  
  return bb;
};

render.image = function(tex, pos, rotation, color) {
  color ??= Color.white;
  rotation ??= 0;
  gui.img(tex,pos, [1.0,1.0], 0.0, false, [0.0,0.0], color); 
  return bbox.fromcwh([0,0], [tex.width,tex.height]);
}

render.doc = "Draw shapes in screen space.";
render.circle.doc = "Draw a circle at pos, with a given radius and color.";
render.cross.doc = "Draw a cross centered at pos, with arm length size.";
render.arrow.doc = "Draw an arrow from start to end, with wings of length wingspan at angle wingangle.";
render.poly.doc = "Draw a concave polygon from a set of points.";
render.rectangle.doc = "Draw a rectangle, with its corners at lowerleft and upperright.";
render.box.doc = "Draw a box centered at pos, with width and height in the tuple wh.";
render.line.doc = "Draw a line from a set of points, and a given thickness.";

return {render};
