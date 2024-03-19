var Spline = {};
Spline.sample_angle = function(type, points, angle) {
  if (type === 0) return spline.catmull(points, angle);
  else if (type === 1) return spline.bezier(points,angle);
  return undefined;
}

Spline.bezier_loop = function(cp)
{
  cp.push(Vector.reflect_point(cp.at(-2),cp.at(-1)));
  cp.push(Vector.reflect_point(cp[1],cp[0]));
  cp.push(cp[0].slice());
  return cp;
}

Spline.bezier_node_count = function(cp)
{
  if (cp.length === 4) return 2;
  return 2 + (cp.length-4)/3;
}

Spline.is_bezier = function(t) { return t === Spline.type.bezier; }
Spline.is_catmull = function(t) { return t === Spline.type.catmull; }

Spline.bezier2catmull = function(b)
{
  var c = [];
  for (var i = 0; i < b.length; i += 3)
    c.push(b[i]);
  return c;
}

Spline.catmull2bezier = function(c)
{
  var b = [];
  for (var i = 1; i < c.length-2; i++) {
    b.push(c[i].slice());
    b.push(c[i+1].sub(c[i-1]).scale(0.25).add(c[i]));
    b.push(c[i].sub(c[i+2]).scale(0.25).add(c[i+1]));
  }
  b.push(c[c.length-2]);
  return b;
}

Spline.catmull_loop = function(cp)
{
  cp = cp.slice();
  cp.unshift(cp.last());
  cp.push(cp[1]);
  cp.push(cp[2]);
  return cp;
}

Spline.catmull_caps = function(cp)
{
  if (cp.length < 2) return;
  cp = cp.slice();
  cp.unshift(cp[0].sub(cp[1]).add(cp[0]));
  cp.push(cp.last().sub(cp.at(-2).add(cp.last())));
  return cp;
}

Spline.catmull_caps.doc = "Given a set of control points cp, return the necessary caps added to the spline.";

Spline.catmull2bezier.doc = "Given a set of control points C for a camtull-rom type curve, return a set of cubic bezier points to give the same curve."

Spline.type = {
  catmull: 0,
  bezier: 1,
  bspline: 2,
  cubichermite: 3
};

Spline.bezier_tan_partner = function(points, i)
{
  if (i%3 === 0) return undefined;
  var partner_i = (i%3) === 2 ? i-1 : i+1;
  return points[i];
}

Spline.bezier_cp_mirror = function(points, i)
{
  if (i%3 === 0) return undefined;
  var partner_i = (i%3) === 2 ? i+2 : i-2;
  var node_i = (i%3) === 2 ? i+1 : i-1;
  if (partner_i >= points.length || node_i >= points.length) return;
  points[partner_i] = points[node_i].sub(points[i]).add(points[node_i]);
}

Spline.bezier_point_handles = function(points, i)
{
  if (!Spline.bezier_is_node(points,i)) return [];
  var a = i-1;
  var b = i+1;
  var c = []
  if (a > 0)
    c.push(a);

  if (b < points.length)
    c.push(b);

  return c;
}

Spline.bezier_nodes = function(points)
{
  var c = [];
  for (var i = 0; i < points.length; i+=3)
    c.push(points[i].slice());

  return c;
}

Spline.bezier_is_node = function(points, i) { return i%3 === 0; }
Spline.bezier_is_handle = function(points, i) { return !Spline.bezier_is_node(points,i); }

return {Spline};
