this.phys = physics.kinematic;
this.dir_view2world = function (dir) {
  return dir.scale(this.zoom);
};
this.view2world = function (pos) {
  var useren = window.rendersize.scale(this.zoom);
  if (window.mode === window.modetypes.stretch) {
    pos = pos.scale([window.rendersize.x / window.size.x, window.rendersize.y / window.size.y]);
    pos = pos.sub(window.rendersize.scale(0.5));
    pos = pos.scale(this.zoom);
    pos = pos.add(this.pos);
  }
  if (window.mode === window.modetypes.full) {
    pos = pos.sub(window.size.scale(0.5));
    pos = pos.scale(this.zoom);
    pos = pos.add(this.pos);
  }
  if (window.mode === window.modetypes.expand) {
    pos = pos.sub(window.size.scale(0.5));
    pos = pos.scale([window.rendersize.x / window.size.x, window.rendersize.y / window.size.y]);
  }
  return pos;
};
this.world2view = function (pos) {
  if (window.mode === window.modetypes.stretch) {
    pos = pos.sub(this.pos);
    pos = pos.scale(1.0 / this.zoom);
    pos = pos.add(window.rendersize.scale(0.5));
  }
  if (window.mode === window.modetypes.full) {
    pos = pos.sub(this.pos);
    pos = pos.scale(1 / this.zoom);
    pos = pos.add(window.size.scale(0.5));
  }
  if (window.mode === window.modetypes.expand) {
  }
  return pos;
};

this.screenright = function () {
  return this.view2world(window.size).x;
};
this.screenleft = function () {
  return this.view2world([0, 0]).x;
};

var zoom = 1;

Object.mixin(self, {
  set zoom(x) {
    zoom = x;
    if (zoom <= 0.1) zoom = 0.1;
  },
  get zoom() {
    return zoom;
  },
});
