/* On collisions, entities are sent a 'hit' object, which looks like this: */
var HIT = {
  normal: "The normal of the collision point.",
  hit: "The gameobject of the object that collided.",
  sensor: "Boolean for if the colliding object was a sensor.",
  velocity: "Velocity of the contact.",
  pos: "Position in world space of the contact.",
  depth: "Depth of the contact.",
};

var Physics = {
  dynamic: 0,
  kinematic: 1,
  static: 2,
};

var physics = {
  set gravity(x) { cmd(8, x); },
  get gravity() { return cmd(72); },
  set damping(x) { cmd(73,Math.clamp(x,0,1)); },
  get damping() { return cmd(74); },
  pos_query(pos, give) {
    give ??= 25;
    return cmd(44, pos, give);
  },
  
  /* Returns a list of body ids that a box collides with */
  box_query(box) { return cmd(52, box.pos, box.wh); },
  
  box_point_query(box, points) {
    if (!box || !points)
      return [];

    return cmd(86, box.pos, box.wh, points, points.length);
  },

  shape_query(shape) { return cmd(80,shape); },

  com(pos) {
    if (!Array.isArray(pos)) return [0,0];
    return pos.reduce((a,i) => a.add(i)).map(g => g/pos.length);
  },
};

physics.doc = {};
physics.doc.gravity = "Gravity expressed in units per second.";
physics.doc.damping = "Damping applied to all physics bodies. Bound between 0 and 1.";
physics.doc.pos_query = "Returns any object colliding with the given point.";
physics.doc.box_query = "Returns an array of body ids that collide with a given box.";
physics.doc.box_point_query = "Returns the subset of points from a given list that are inside a given box.";

var Collision = {
  types: {},
  num: 32,
  set_collide(a, b, x) {
    this.types[a][b] = x;
    this.types[b][a] = x;
    this.sync();
  },
  sync() {
    for (var i = 0; i < this.num; i++)
      cmd(76,i,this.types[i]);
  },
};

for (var i = 0; i < Collision.num; i++) {
  Collision.types[i] = [];
  for (var j = 0; j < Collision.num; j++)
    Collision.types[i][j] = false;
};

Collision.sync();

