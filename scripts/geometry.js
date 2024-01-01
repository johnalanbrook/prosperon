var Sphere = {};
Sphere.volume = function(r) { return Math.pi*r*r*r*4/3; };
Sphere.random = function(r,theta,phi)
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

var Circle = {};
Circle.area = function(r) { return Math.pi*r*r; };
Circle.random = function(r,theta)
{
  return Sphere.random(r,theta).xz;
}
