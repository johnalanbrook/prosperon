var Color = {
  white: [255, 255, 255],
  black: [0, 0, 0],
  blue: [0, 0, 255],
  green: [0, 255, 0],
  yellow: [255, 255, 0],
  red: [255, 0, 0],
  gray: [181, 181, 181],
  cyan: [0, 255, 255],
  purple: [162, 93, 227],
  orange: [255, 144, 64],
  magenta: [255, 0, 255],
};

Color.editor = {};
Color.editor.ur = Color.green;

Color.tohtml = function (v) {
  var html = v.map(function (n) {
    return Number.hex(n * 255);
  });
  return "#" + html.join("");
};

var esc = {};
esc.reset = "\x1b[0";
esc.color = function (v) {
  var c = v.map(function (n) {
    return Math.floor(n * 255);
  });
  var truecolor = "\x1b[38;2;" + c.join(";") + ";";
  return truecolor;
};

esc.doc = "Functions and constants for ANSI escape sequences.";

Color.Arkanoid = {
  orange: [255, 143, 0],
  teal: [0, 255, 255],
  green: [0, 255, 0],
  red: [255, 0, 0],
  blue: [0, 112, 255],
  purple: [255, 0, 255],
  yellow: [255, 255, 0],
  silver: [157, 157, 157],
  gold: [188, 174, 0],
};

Color.Arkanoid.Powerups = {
  red: [174, 0, 0] /* laser */,
  blue: [0, 0, 174] /* enlarge */,
  green: [0, 174, 0] /* catch */,
  orange: [224, 143, 0] /* slow */,
  purple: [210, 0, 210] /* break */,
  cyan: [0, 174, 255] /* disruption */,
  gray: [143, 143, 143] /* 1up */,
};

Color.Gameboy = {
  darkest: [229, 107, 26],
  dark: [229, 189, 26],
  light: [189, 229, 26],
  lightest: [107, 229, 26],
};

Color.Apple = {
  green: [94, 189, 62],
  yellow: [255, 185, 0],
  orange: [247, 130, 0],
  red: [226, 56, 56],
  purple: [151, 57, 153],
  blue: [0, 156, 223],
};

Color.Debug = {
  boundingbox: Color.white,
  names: [84, 110, 255],
};

Color.Editor = {
  grid: [99, 255, 128],
  select: [255, 255, 55],
  newgroup: [120, 255, 10],
};

/* Detects the format of all colors and munges them into a floating point format */
Color.normalize = function (c) {
  var add_a = function (a) {
    var n = this.slice();
    n.a = a;
    return n;
  };

  for (var p of Object.keys(c)) {
    var fmt = "nrm";
    if (typeof c[p] !== "object") continue;
    if (!Array.isArray(c[p])) {
      Color.normalize(c[p]);
      continue;
    }

    c[p][3] = 255;

    for (var color of c[p]) {
      if (color > 1) {
        fmt = "8b";
        break;
      }
    }

    switch (fmt) {
      case "8b":
        c[p] = c[p].map(function (x) {
          return x / 255;
        });
    }
    c[p].alpha = add_a;
  }
};

Color.normalize(Color);

var ColorMap = {};
ColorMap.makemap = function (map) {
  var newmap = Object.create(ColorMap);
  Object.assign(newmap, map);
  return newmap;
};
ColorMap.Jet = ColorMap.makemap({
  0: [0, 0, 131],
  0.125: [0, 60, 170],
  0.375: [5, 255, 255],
  0.625: [255, 255, 0],
  0.875: [250, 0, 0],
  1: [128, 0, 0],
});

ColorMap.BlueRed = ColorMap.makemap({
  0: [0, 0, 255],
  1: [255, 0, 0],
});

ColorMap.Inferno = ColorMap.makemap({
  0: [0, 0, 4],
  0.13: [31, 12, 72],
  0.25: [85, 15, 109],
  0.38: [136, 34, 106],
  0.5: [186, 54, 85],
  0.63: [227, 89, 51],
  0.75: [249, 140, 10],
  0.88: [249, 201, 50],
  1: [252, 255, 164],
});

ColorMap.Bathymetry = ColorMap.makemap({
  0: [40, 26, 44],
  0.13: [59.49, 90],
  0.25: [64, 76, 139],
  0.38: [63, 110, 151],
  0.5: [72, 142, 158],
  0.63: [85, 174, 163],
  0.75: [120, 206, 163],
  0.88: [187, 230, 172],
  1: [253, 254, 204],
});

ColorMap.Viridis = ColorMap.makemap({
  0: [68, 1, 84],
  0.13: [71, 44, 122],
  0.25: [59, 81, 139],
  0.38: [44, 113, 142],
  0.5: [33, 144, 141],
  0.63: [39, 173, 129],
  0.75: [92, 200, 99],
  0.88: [170, 220, 50],
  1: [253, 231, 37],
});

Color.normalize(ColorMap);

ColorMap.sample = function (t, map = this) {
  if (t < 0) return map[0];
  if (t > 1) return map[1];

  var lastkey = 0;
  for (var key of Object.keys(map).sort()) {
    if (t < key) {
      var b = map[key];
      var a = map[lastkey];
      var tt = (key - lastkey) * t;
      return a.lerp(b, tt);
    }
    lastkey = key;
  }
  return map[1];
};

ColorMap.doc = {
  sample: "Sample a given colormap at the given percentage (0 to 1).",
};

return {
  Color,
  esc,
  ColorMap,
};
