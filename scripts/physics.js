/* On collisions, entities are sent a 'hit' object, which looks like this: 
var HIT = {
  normal: "The normal of the collision point.",
  hit: "The gameobject of the object that collided.",
  sensor: "Boolean for if the colliding object was a sensor.",
  velocity: "Velocity of the contact.",
  pos: "Position in world space of the contact.",
  depth: "Depth of the contact.",
};

*/

var pq = physics.pos_query;
physics.pos_query = function(pos,give) {
  give ??= 25;
  pq(pos,give);
}

var bpq = physics.box_point_query;
physics.box_point_query = function(box,points) {
  if (!box || !points) return [];
  return bpq(box.pos,box.wh,points,points.length)
}

Object.assign(physics, {
  dynamic: 0,
  kinematic: 1,
  static: 2,

  com(pos) {
    if (!Array.isArray(pos)) return [0,0];
    return pos.reduce((a,i) => a.add(i)).map(g => g/pos.length);
  },
});

physics.doc = {};
physics.doc.pos_query = "Returns any object colliding with the given point.";
physics.doc.box_query = "Returns an array of body ids that collide with a given box.";
physics.doc.box_point_query = "Returns the subset of points from a given list that are inside a given box.";

physics.collision = {
  types: {},
  num: 32,
  set_collide(a, b, x) {
    this.types[a][b] = x;
    this.types[b][a] = x;
    this.sync();
  },
  sync() {
    for (var i = 0; i < this.num; i++)
      physics.set_cat_mask(i,this.types[i]);
  },
};

for (var i = 0; i < physics.collision.num; i++) {
  physics.collision.types[i] = [];
  for (var j = 0; j < physics.collision.num; j++)
    physics.collision.types[i][j] = false;
};

physics.collision.sync();

physics.gravity = physics.make_gravity();
physics.gravity.mask = [true];
physics.gravity.strength = 500;
physics.damp = physics.make_damp();
physics.damp.mask = [true];

return {
  physics
}
