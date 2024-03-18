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
    render.circle(pos,size,color);
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

render.rectangle = function(lowerleft, upperright, color) {
    var pos = lowerleft.add(upperright).map(x=>x/2);
    var wh = [upperright.x-lowerleft.x,upperright.y-lowerleft.y];
    render.box(pos,wh,color);
  };
  
render.box = function(pos, wh, color) {
    color ??= Color.white;
    cmd(53, pos, wh, color);
  };

render.doc = "Draw shapes in screen space.";
render.circle.doc = "Draw a circle at pos, with a given radius and color.";
render.cross.doc = "Draw a cross centered at pos, with arm length size.";
render.arrow.doc = "Draw an arrow from start to end, with wings of length wingspan at angle wingangle.";
render.poly.doc = "Draw a concave polygon from a set of points.";
render.rectangle.doc = "Draw a rectangle, with its corners at lowerleft and upperright.";
render.box.doc = "Draw a box centered at pos, with width and height in the tuple wh.";
render.line.doc = "Draw a line from a set of points, and a given thickness.";

return {render};
