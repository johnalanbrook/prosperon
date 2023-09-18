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
  pos_query(pos) {
    return cmd(44, pos);
  },
  
  /* Returns a list of body ids that a box collides with */
  box_query(box) {
    var pts = cmd(52,box.pos,box.wh);
    return cmd(52, box.pos, box.wh);
  },
  
  box_point_query(box, points) {
    if (!box || !points)
      return [];

    return cmd(86, box.pos, box.wh, points, points.length);
  },
};

var Collision = {
  types: {},
  num: 10,
  set_collide(a, b, x) {
    this.types[a][b] = x;
    this.types[b][a] = x;
  },
  sync() {
    for (var i = 0; i < this.num; i++)
      cmd(76,i,this.types[i]);
  },
  types_nuke() {
    Nuke.newline(this.num+1);
    Nuke.label("");
    for (var i = 0; i < this.num; i++) Nuke.label(i);
    
    for (var i = 0; i < this.num; i++) {
      Nuke.label(i);
      for (var j = 0; j < this.num; j++) {
        if (j < i)
	  Nuke.label("");
	else {
          this.types[i][j] = Nuke.checkbox(this.types[i][j]);
  	  this.types[j][i] = this.types[i][j];
	}
      }
    }
  },
};

for (var i = 0; i < Collision.num; i++) {
  Collision.types[i] = [];
  for (var j = 0; j < Collision.num; j++)
    Collision.types[i][j] = false;
};

