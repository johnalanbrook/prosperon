/* On collisions, entities are sent a 'hit' object, which looks like this: 
var HIT = {
  normal: "The normal of the collision point.",
  obj: "The gameobject of the object that collided.",
  sensor: "Boolean for if the colliding object was a sensor.",
  velocity: "Velocity of the contact.",
  pos: "Position in world space of the contact.",
  depth: "Depth of the contact.",
};
*/

export function pos_query(pos, start = world, give = 10) {
  var ret;
  ret = physics.point_query_nearest(pos, 0);

  if (ret) return ret.entity;

  return game.all_objects(function (o) {
    var dist = Vector.length(o.pos.sub(pos));
    if (dist <= give) return o;
  });
};

export function box_point_query(box, points) {
  if (!box || !points) return [];
  var bbox = bbox.fromcwh(box.pos, box.wh);
  var inside = [];
  for (var i in points) if (bbox.pointin(bbox, points[i])) inside.push[i];
  return inside;
};

Object.assign(physics, {
  dynamic: 0,
  kinematic: 1,
  static: 2,

  com(pos) {
    if (!Array.isArray(pos)) return [0, 0];
    return pos.reduce((a, i) => a.add(i)).map(g => g / pos.length);
  },
});

physics.doc = {};
physics.doc.pos_query = "Returns any object colliding with the given point.";
physics.doc.box_query = "Calls a given function on every shape object in the given bbox.";
physics.doc.box_point_query = "Returns the subset of points from a given list that are inside a given box.";

physics.gravity = physics.make_gravity();
physics.gravity.mask = ~1;
physics.gravity.strength = 500;
physics.damp = physics.make_damp();
physics.damp.mask = ~1;

physics.delta = 1 / 240;

return {
  physics,
};
