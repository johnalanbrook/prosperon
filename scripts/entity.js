function grab_from_points(pos, points, slop) {
  var shortest = slop;
  var idx = -1;
  points.forEach(function(x,i) {
    if (Vector.length(pos.sub(x)) < shortest) {
      shortest = Vector.length(pos.sub(x));
      idx = i;
    }
  });
  return idx;
};

var gameobject = {
  save: true,
  selectable: true,

  layer_nuke() {
    Nuke.label("Collision layer");
    Nuke.newline(Collision.num);
    for (var i = 0; i < Collision.num; i++)
      this.layer = Nuke.radio(i, this.layer, i);
  },

  draw_layer: 1,
  draw_layer_nuke() {
    Nuke.label("Draw layer");
    Nuke.newline(5);
    for (var i = 0; i < 5; i++)
      this.draw_layer = Nuke.radio(i, this.draw_layer, i);
  },

  ed_locked: false,
  
  _visible:  true,
  get visible(){ return this._visible; },
  set visible(x) {
    this._visible = x; 
    for (var key in this.components) {
      if ('visible' in this.components[key]) {
        this.components[key].visible = x;
      }
    }
  },

  phys_nuke() {
    Nuke.newline(1);
    Nuke.label("phys");
    Nuke.newline(3);
    this.phys = Nuke.radio("dynamic", this.phys, 0);
    this.phys = Nuke.radio("kinematic", this.phys, 1);
    this.phys = Nuke.radio("static", this.phys, 2);
  },
  
  set_center(pos) {
    var change = pos.sub(this.pos);
    this.pos = pos;
    
    for (var key in this.components) {
      this.components[key].finish_center(change);
    }
  },

  set_relpos(x) {
    if (!this.level) {
      this.pos = x;
      return;
    }

    this.pos = Vector.rotate(x, Math.deg2rad(this.level.angle)).add(this.level.pos);
  },

  get_relpos() {
    if (!this.level) return this.pos;

    var offset = this.pos.sub(this.level.pos);
    return Vector.rotate(offset, -Math.deg2rad(this.level.angle));
  },
  
  get_relangle() {
    if (!this.level) return this.angle;
    return this.angle - this.level.angle;
  },

  gizmo: "", /* Path to an image to draw for this gameobject */

  width() {
    var bb = this.boundingbox();
    return bb.r - bb.l;
  },

  height() {
    var bb = this.boundingbox();
    return bb.t-bb.b;
  },

  stop() {},

  /* Make a unique object the same as its prototype */
  revert() {
//    unmerge(this, this.prop_obj());
  },

  gui() {
    var go_guis = walk_up_get_prop(this, 'go_gui');
    Nuke.newline();

    go_guis.forEach(function(x) { x.call(this); }, this);

    for (var key in this) {
      if (typeof this[key] === 'object' && 'gui' in this[key]) this[key].gui();
    }
  },


  check_registers(obj) {
    Register.unregister_obj(this);
  
    if (typeof obj.update === 'function')
      Register.update.register(obj.update, obj);

    if (typeof obj.physupdate === 'function')
      Register.physupdate.register(obj.physupdate, obj);

    if (typeof obj.collide === 'function')
      obj.register_hit(obj.collide, obj);

    if (typeof obj.separate === 'function')
      obj.register_separate(obj.separate, obj);

    if (typeof obj.draw === 'function')
      Register.draw.register(obj.draw,obj);

    if (typeof obj.debug === 'function')
      Register.debug.register(obj.debug, obj);

    obj.components.forEach(function(x) {
      if (typeof x.collide === 'function')
        register_collide(1, x.collide, x, obj.body, x.shape);
    });
  },
  instances: [],

      get scale() { return cmd(103, this.body); },
      set scale(x) {
	var pct = x/this.scale;
        cmd(36, this.body, x);	

	this.objects.forEach(function(obj) {
	  obj.scale *= pct;
	  obj.set_relpos(obj.get_relpos().scale(pct));
	}, this);      
      },

      get flipx() { return cmd(104,this.body); },
      set flipx(x) {
        cmd(55, this.body, x);
        return;
	this.objects.forEach(function(obj) {
	  obj.flipx = !obj.flipx;
	  var rp = obj.get_relpos();
	  obj.pos = [-rp.x, rp.y].add(this.pos);
	  obj.angle = -obj.angle;
	},this);	
      },
      
      get flipy() { return cmd(105,this.body); },
      set flipy(x) {
        cmd(56, this.body, x);
	return;
	this.objects.forEach(function(obj) {
	  var rp = obj.get_relpos();
	  obj.pos = [rp.x, -rp.y].add(this.pos);
	  obj.angle = -obj.angle;
	},this);	
      },
      
      set pos(x) {
        var diff = x.sub(this.pos);
	this.objects.forEach(function(x) { x.pos = x.pos.add(diff); });
        set_body(2,this.body,x); },
      get pos() { return q_body(1,this.body); },

      get angle() { return Math.rad2deg(q_body(2,this.body))%360; },
      set angle(x) {
        var diff = x - this.angle;
	this.objects.forEach(function(x) {
	  x.angle = x.angle + diff;
	  var pos = x.pos.sub(this.pos);
	  var r = Vector.length(pos);
	  var p = Math.rad2deg(Math.atan2(pos.y, pos.x));
	  p += diff;
	  p = Math.deg2rad(p);
	  x.pos = this.pos.add([r*Math.cos(p), r*Math.sin(p)]);
	}, this);
	  
        set_body(0,this.body, Math.deg2rad(x));
      },

      get elasticity() { return cmd(107,this.body); },
      set elasticity(x) { cmd(106,this.body,x); },

      get friction() { return cmd(109,this.body); },
      set friction(x) { cmd(108,this.body,x); },

      set mass(x) { set_body(7,this.body,x); },
      get mass() { return q_body(5, this.body); },

      set phys(x) { set_body(1, this.body, x); },
      get phys() { return q_body(0,this.body); },
      get velocity() { return q_body(3, this.body); },
      set velocity(x) { set_body(9, this.body, x); },
      get angularvelocity() { return Math.rad2deg(q_body(4, this.body)); },
      set angularvelocity(x) { set_body(8, this.body, Math.deg2rad(x)); },
      pulse(vec) { set_body(4, this.body, vec);},

      push(vec) { set_body(12,this.body,vec);},
      world2this(pos) { return cmd(70, this.body, pos); },
      this2world(pos) { return cmd(71, this.body,pos); },
      set layer(x) { cmd(75,this.body,x); },
      get layer() { return 0; },
      alive() { return this.body >= 0; },
      in_air() { return q_body(7, this.body);},
      on_ground() { return !this.in_air(); },

      disable() { this.components.forEach(function(x) { x.disable(); });},
      enable() { this.components.forEach(function(x) { x.enable(); });},
      sync() { },
      dirty() { return false; },

      spawn(ur) {
	if (typeof ur === 'string')
	  ur = prototypes.get_ur(ur);
        if (!ur) Log.warn("Failed to make UR from " + ur);

        return gameobject.make(ur, this);
      },


      /* Bounding box of the object in world dimensions */
      boundingbox() {
	var boxes = [];
	boxes.push({t:0, r:0,b:0,l:0});

	for (var key in this.components) {
	  if ('boundingbox' in this.components[key])
	    boxes.push(this.components[key].boundingbox());
	}

	if (boxes.empty) return cwh2bb([0,0], [0,0]);

	var bb = boxes[0];

	boxes.forEach(function(x) {
	  bb = bb_expand(bb, x);
	});

	var cwh = bb2cwh(bb);

	if (!bb) return;

	if (this.flipx) cwh.c.x *= -1;
	if (this.flipy) cwh.c.y *= -1;

	cwh.c = cwh.c.add(this.pos);
	bb = cwh2bb(cwh.c, cwh.wh);

	return bb ? bb : cwh2bb([0,0], [0,0]);
      },

      dup(diff) {
	var dup = Primum.spawn(this.__proto__);
	Object.assign(dup, this);
	return dup;
      },
      
      kill() {
	if (this.body === -1) {
	  Log.warn(`Object is already dead!`);
	  return;
	}

	Register.endofloop(() => {
	  cmd(2, this.body);
	  delete Game.objects[this.body];

//	  if (this.level)
//	    this.level.unregister(this);

	  Player.uncontrol(this);
	  this.instances.remove(this);
	  Register.unregister_obj(this);
    //      Signal.clear_obj(this);

	  this.body = -1;
	  for (var key in this.components) {
	    Register.unregister_obj(this.components[key]);
	    this.components[key].kill();
	  }

	  this.objects.forEach(x => x.kill());

	  this.stop();
	});
      },

      up() { return [0,1].rotate(Math.deg2rad(this.angle));},
//      down() { return [0,-1].rotate(Math.deg2rad(this.angle));},
//      right() { return [1,0].rotate(Math.deg2rad(this.angle));},
//      left() { return [-1,0].rotate(Math.deg2rad(this.angle));},

  make(ur, level) {
    level ??= Primum;
    var obj = Object.create(gameobject);
    obj.defn('body', make_gameobject());
    obj.defn('components', {});
    
    Game.register_obj(obj);
    gameobject.make_parentable(obj);    

    cmd(113, obj.body, obj); // set the internal obj reference to this obj

    Object.totalassign(obj, ur);
    obj.ur = ur;

/*    for (var prop in obj) {
       if (typeof obj[prop] === 'object' && 'comp' in obj[prop]) {
         var newcomp = component[obj[prop].comp].make(obj.body);
	 Object.assign(newcomp, obj[prop]);
	 newcomp.sync?.();
         obj[prop] = newcomp;
	 obj[prop].defn('gameobject', obj);
	 obj.components[prop] = obj[prop];
       }
    };
*/
    level.add_child(obj);
    for (var prop in obj) {
      var p = obj[prop];
      if (typeof p !== 'object') continue;

      if ('ur' in p) {
        Log.warn(`spawning a ${prop} on this obj`);
        var newobj = obj.spawn(prototypes.get_ur(p.ur));
	Object.assign(newobj, p);
	obj.$[prop] = newobj;
      } else if ('make' in p) {
        obj[prop] = obj[prop].make(obj.body);
        obj.components[prop] = obj[prop];
      }
    };

    obj.check_registers(obj);    
    
    if (typeof obj.start === 'function') obj.start();



    return obj;
  },

  register_hit(fn, obj) {
    if (!obj)
      obj = this;

    Signal.obj_begin(fn, obj, this);
  },

  register_separate(fn, obj) {
    if (!obj)
      obj = this;

    Signal.obj_separate(fn,obj,this);
  },
}

gameobject.toJSON = ur_json;

gameobject.ur = {
  pos: [0,0],
  scale:1,
  flipx:false,
  flipy:false,
  angle:0,
  elasticity:0.5,
  friction:1,
  mass:1,
  velocity:[0,0],
  angularvelocity:0,
  layer: 0,
};

gameobject.make_parentable = function(obj) {
  var objects = [];
  Object.defHidden(obj, 'level');

  obj.remove_child = function(child) {
    objects.remove(child);
  }

  obj.add_child = function(child) {
    child.unparent();
    objects.push(child);
    child.level = obj;
  }

  /* Reparent this object to a new one */
  obj.reparent = function(parent) {
    if (parent === obj.level)
      return;

    parent.add_child(obj);
    obj.level = parent;
  }

  obj.unparent = function() {
    if (!obj.level) return;
    obj.level.remove_child(obj);
    obj.parent = undefined;
  }
  obj.objects = objects;
}

gameobject.entity = {};

/* Default objects */
var prototypes = {};
prototypes.ur = {};
prototypes.load_all = function()
{
if (IO.exists("proto.json"))
  prototypes = JSON.parse(IO.slurp("proto.json"));

for (var key in prototypes) {
  if (key in gameobjects)
    Object.dainty_assign(gameobjects[key], prototypes[key]);
  else {
    /* Create this gameobject fresh */
    Log.info("Making new prototype: " + key + " from " + prototypes[key].from);
    var newproto = gameobjects[prototypes[key].from].clone(key);
    gameobjects[key] = newproto;

    for (var pkey in newproto)
      if (typeof newproto[pkey] === 'object' && newproto[pkey] && 'clone' in newproto[pkey])
        newproto[pkey] = newproto[pkey].clone();

    Object.dainty_assign(gameobjects[key], prototypes[key]);
  }
}
}

prototypes.save_gameobjects = function() { slurpwrite(JSON.stringify(gameobjects,null,2), "proto.json"); };

/* Makes a new ur-type from a file. The file can define components. */
prototypes.from_file = function(file)
{
  if (!IO.exists(file)) {
    Log.error(`File ${file} does not exist.`);
    return;
  }

  var newur = Object.create(gameobject.ur);
  newur.$ = {};
  var script = IO.slurp(file);

  var json = {};
  if (IO.exists(file.name() + ".json")) {
    var json = JSON.parse(IO.slurp(file.name() + ".json"));
    Object.assign(newur.$, json.$);
    delete json.$;
  }
  
  compile_env(script, newur, file);  
  Object.dainty_assign(newur, json);    

  file = file.replaceAll('/', '.');
  var path = file.name().split('.');
  var nested_access = function(base, names) {
    for (var i = 0; i < names.length; i++)
      base = base[names[i]] = base[names[i]] || {};

    return base;
  };

  var instances = [];
  var tag = file.name();
  prototypes.list.push(tag);
  
  newur.toString = function() { return tag; };
  Object.assign(nested_access(ur,path), newur);
  nested_access(ur,path).__proto__ = newur.__proto__;

  return nested_access(ur,path);
}
prototypes.from_file.doc = "Create a new ur-type from a given script file.";
prototypes.list = [];

prototypes.from_obj = function(name, obj)
{
  var newobj = Object.copy(gameobject.ur, obj);
  prototypes.ur[name] = newobj;
  Log.say(Object.keys(newobj));
  newobj.toString = function() { return name; };
  return prototypes.ur[name];
}

prototypes.load_config = function(name)
{
  if (!prototypes.ur[name])
    prototypes.ur[name] = gameobject.clone(name);

  Log.warn(`Made new ur of name ${name}`);

  return prototypes.ur[name];
}


prototypes.list_ur = function()
{
  var list = [];
  function list_obj(obj, prefix)
  {
    prefix ??= "";
    var list = [];
    for (var e in obj) {
      list.push(prefix + e);
      Log.warn("Descending into " + e);
      list.concat(list_obj(obj[e], e + "."));
    }

    return list;
  }
  
  return list_obj(ur);
}

prototypes.get_ur = function(name)
{
  if (!prototypes.ur[name]) {
    if (IO.exists(name + ".js"))
      prototypes.from_file(name + ".js");

    prototypes.load_config(name);
    return prototypes.ur[name];
  } else
    return prototypes.ur[name];
}

prototypes.generate_ur = function(path)
{
  var ob = IO.glob("**.js");
  ob = ob.filter(function(str) { return !str.startsWith("scripts"); });

  ob.forEach(function(name) {
    if (name === "game.js") return;
    if (name === "play.js") return;

    prototypes.from_file(name);
  });
}

var ur = prototypes.ur;

prototypes.from_obj("camera2d", {
    phys: Physics.kinematic,
    speed: 300,
    
    get zoom() { return cmd(135); },
    set zoom(x) {
      x = Math.clamp(x,0.1,10);
      cmd(62, x);
    },
    
    speedmult: 1.0,
    
    selectable: false,

    world2this(pos) { return cmd(70, this.body, pos); },
    this2world(pos) { return cmd(71, this.body,pos); },
    
    view2world(pos) {
      return cmd(137,pos);
    },
    
    world2view(pos) {
      return cmd(136,pos);
    },
});

prototypes.from_obj("arena", {});
