var shape = {};
shape.sphere = {};
shape.circle = {};
shape.sphere.volume = function(r) { return Math.pi*r*r*r*4/3; };
shape.sphere.random = function(r,theta,phi)
{
  if (typeof r === 'number') r = [r,r];
  theta ??= [0,1];
  phi ??= [-0.5,0.5];
  if (typeof theta === 'number') theta = [theta,theta];
  if (typeof phi === 'number') phi = [phi,phi];
  
  var ra = Math.random_range(r[0],r[1]);
  var ta = Math.turn2rad(Math.random_range(theta[0],theta[1]));
  var pa = Math.turn2rad(Math.random_range(phi[0],phi[1]));
  return [
    ra*Math.sin(ta)*Math.cos(pa),
    ra*Math.sin(ta)*Math.sin(pa),
    ra*Math.cos(ta)
  ];
}

shape.circle.area = function(r) { return Math.pi*r*r; };
shape.circle.random = function(r,theta)
{
  return shape.sphere.random(r,theta).xz;
}

shape.box = function(w,h) {
  w /= 2;
  h /= 2;

  var points = [
    [w,h],
    [-w,h],
    [-w,-h],
    [w,-h]
  ];

  return points;
};

shape.ngon = function(radius, n) {
  return shape.arc(radius,360,n);
};

shape.arc = function(radius, angle, n, start) {
  start ??= 0;
  start = Math.deg2rad(start);
  if (angle >= 360)
    angle = 360;

  if (n <= 1) return [];
  var points = [];

  angle = Math.deg2rad(angle);
  var arclen = angle/n;
  for (var i = 0; i < n; i++)
    points.push(Vector.rotate([radius,0], start + (arclen*i)));

  return points;
};

shape.circle.points = function(radius, n) {
  if (n <= 1) return [];
  return shape.arc(radius, 360, n);
};
