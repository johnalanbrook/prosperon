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

  hide() { this.components.forEach(function(x) { x.hide(); }); this.objects.forEach(function(x) { x.hide(); }); },
  show() { this.components.forEach(function(x) { x.show(); }); this.objects.forEach(function(x) { x.show(); }); },

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

  get_relangle() {
    if (!this.level) return this.angle;
    return this.angle - this.level.angle;
  },

  width() {
    var bb = this.boundingbox();
    return bb.r - bb.l;
  },

  height() {
    var bb = this.boundingbox();
    return bb.t-bb.b;
  },

  /* Make a unique object the same as its prototype */
  revert() {
//    var t = this.transform();
    Object.totalmerge(this,this.ur);
//    Object.merge(this,t);
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
      get scale() { return cmd(103, this.body); },
      set scale(x) {
	var pct = x/this.scale;
        cmd(36, this.body, x);	

	this.objects.forEach(function(obj) {
	  obj.scale *= pct;
	  obj.pos = obj.pos.scale(pct);
	});      
      },

      get flipx() { return cmd(104,this.body); },
      set flipx(x) {
        cmd(55, this.body, x);
        return;
	this.objects.forEach(function(obj) {
	  obj.flipx = !obj.flipx;
	  var rp = obj.pos;
	  obj.pos = [-rp.x, rp.y].add(this.worldpos());
	  obj.angle = -obj.angle;
	},this);	
      },
      
      get flipy() { return cmd(105,this.body); },
      set flipy(x) {
        cmd(56, this.body, x);
	return;
	this.objects.forEach(function(obj) {
	  var rp = obj.pos;
	  obj.pos = [rp.x, -rp.y].add(this.worldpos());
	  obj.angle = -obj.angle;
	},this);	
      },
      
      set pos(x) { this.set_worldpos(Vector.rotate(x, Math.deg2rad(this.level.angle)).add(this.level.worldpos())); },
      get pos() {
        var offset = this.worldpos().sub(this.level.worldpos());
	return Vector.rotate(offset, -Math.deg2rad(this.level.angle));
      },

      worldpos() { return q_body(1,this.body); },
      set_worldpos(x) {
        var diff = x.sub(this.worldpos());
	this.objects.forEach(function(x) { x.set_worldpos(x.worldpos().add(diff)); });
	set_body(2,this.body,x);
      },

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
      get mass() {
        if (!(this.phys === Physics.dynamic))
	  return this.ur.mass;
	  
        return q_body(5, this.body);
      },

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

      spawn(ur) {
	if (typeof ur === 'string')
	  ur = prototypes.get_ur(ur);
        if (!ur) {
	  Log.warn("Failed to make UR from " + ur);
	  return undefined;
	}
	
	var go = gameobject.make(ur, this);
        return go;
      },

      /* Bounding box of the object in world dimensions */
      boundingbox() {
	var boxes = [];
	boxes.push({t:0, r:0,b:0,l:0});

	for (var key in this.components) {
	  if ('boundingbox' in this.components[key])
	    boxes.push(this.components[key].boundingbox());
	}
	for (var key in this.objects)
	  boxes.push(this.objects[key].boundingbox());

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

      diff(from, to) {
	var ret = {};

	for (var key in from) {
	  if (!Object.hasOwn(from, key) && !Object.isAccessor(from,key)) continue;
	  if (typeof from[key] === 'undefined' || typeof to[key] === 'undefined') continue;
	  if (typeof from[key] === 'function') continue;
//	  if (typeof to === 'object' && !(key in to)) continue; 

	  if (Array.isArray(from[key])) {
	    if (!Array.isArray(to[key]))
	      ret[key] = Object.values(gameobject.diff(from[key], []));

	    if (from[key].length !== to[key].length)
	      ret[key] = Object.values(gameobject.diff(from[key], []));

	    var diff = gameobject.diff(from[key], to[key]);
	    if (diff && !diff.empty)
	      ret[key] = Object.values(diff);

	    continue;
	  }

	  if (typeof from[key] === 'object') {
	    var diff = gameobject.diff(from[key], to[key]);
	    if (diff && !diff.empty)
		ret[key] = diff;	
	    continue;
	  }

	  if (typeof from[key] === 'number') {
	    var a = Number.prec(from[key]);
	    if (a !== to[key])
	      ret[key] = a;
	    continue;
	  }

	  if (from[key] !== to[key])
	    ret[key] = from[key];
	}
	if (ret.empty) return undefined;
	return ret;
      },

      json_obj() {
	var ur = gameobject.diff(this,this.ur);
        return ur ? ur : {};
      },

      transform_obj() {
        var t = this.json_obj();
	Object.assign(t, this.transform());
	return t;
      },

      level_obj() {
        var json = this.json_obj();
	var objects = {};
	this.ur.objects ??= {};
	if (!Object.keys(this.objects).equal(Object.keys(this.ur.objects))) {
	  for (var o in this.objects)
	    objects[o] = this.objects[o].transform_obj();
	} else {
	  for (var o in this.objects) {
	    var obj = this.objects[o].json_obj();
	    Object.assign(obj, gameobject.diff(this.objects[o].transform(), this.ur.objects[o]));
	    if (!obj.empty)
	      objects[o] = obj;
	  }
	}

	if (!objects.empty)
  	  json.objects = objects;
	
	return json;
      },

      ur_obj() {
        var ur = this.json_obj();
	for (var k in ur) 
	  if (ur[k].ur)
	    delete ur[k];

	return ur;
      },
      
      make_ur() {
        var thisur = this.json_obj();
	thisur.pos = this.pos;
	thisur.angle = this.angle;
	return thisur;
      },

      transform() {
        var t = {};
	t.pos = this.pos.map(Number.prec);
	t.angle = Number.prec(this.angle);
	return t;
      },

      phys_obj() {
        var phys = {};
	phys.velocity = this.velocity.map(Number.prec);
	phys.angularvelocity = Number.prec(this.angularvelocity);
	return phys;
     },

      dup(diff) {
        var n = this.level.spawn(this.ur);
	Object.totalmerge(n, this.make_ur());
	return n;
      },
      
      kill() {
	if (this.body === -1) {
	  Log.warn(`Object is already dead!`);
	  return;
	}

	Register.endofloop(() => {
	  cmd(2, this.body);
	  delete Game.objects[this.body];
  	  this.level.remove_obj(this);

	  Player.uncontrol(this);
	  this.instances.remove(this);
	  Register.unregister_obj(this);

	  this.body = -1;
	  for (var key in this.components) {
	    Register.unregister_obj(this.components[key]);
	    this.components[key].kill();
	  }

	  this.objects.forEach(x => x.kill());
	  if (typeof this.stop === 'function')
  	    this.stop();
	});
      },

   remove_obj(obj) {
     delete this[obj.toString()];     
     this.objects.remove(obj);
   },

      up() { return [0,1].rotate(Math.deg2rad(this.angle));},
      down() { return [0,-1].rotate(Math.deg2rad(this.angle));},
      right() { return [1,0].rotate(Math.deg2rad(this.angle));},
      left() { return [-1,0].rotate(Math.deg2rad(this.angle));},
  reparent(parent) {
    if (this.level === parent)
      this.level.remove_obj(this);
    var name = parent.objects.push(this);
    this.toString = function() { return name; };
    
    if (this.level)
      this.level.objects.remove(this);
    this.level = parent;
  },

  make(ur, level) {
    level ??= Primum;
    var obj = Object.create(gameobject);
    obj.body = make_gameobject();
    obj.components = {};
    obj.objects = {};
    Object.hide(obj, 'components');
    Object.hide(obj, 'objects');
    obj.toString = function() { return obj.ur.toString(); };
    
    Game.register_obj(obj);

    cmd(113, obj.body, obj); // set the internal obj reference to this obj

    obj.ur = ur;

    obj.reparent(level);

    for (var prop in ur) {
      var p = ur[prop];
      if (typeof p !== 'object') continue;

      if ('ur' in p) {
        obj[prop] = obj.spawn(prototypes.get_ur(p.ur));
	obj.rename_obj(obj[prop].toString(), prop);
      } else if ('comp' in p) {
        obj[prop] = component[p.comp].make(obj);
	obj.components[prop] = obj[prop];
      }
    };

    if (ur.objects) {
      for (var prop in ur.objects) {
        var o = ur.objects[prop];
        var newobj = obj.spawn(prototypes.get_ur(o.ur));
	obj.rename_obj(newobj.toString(), prop);
      }
    }

    var save_tostr = obj.toString;
    Object.totalmerge(obj,ur);
    obj.toString = save_tostr;
    obj.components.forEach(function(x) { if ('sync' in x) x.sync(); });
    obj.check_registers(obj);

    obj.objects.forEach(function(x) {
      x.ur = prototypes.get_ur(x.ur);
    });
    
    if (typeof obj.start === 'function') obj.start();

    return obj;
  },

  rename_obj(name, newname) {
    if (!this.objects[name]) {
      Log.warn(`No object with name ${name}. Could not rename to ${newname}.`);
      return;
    }
    if (this.objects[newname]) {
      Log.warn(`Already an object with name ${newname}.`);
      return;
    }
    Log.warn(`Renaming from ${name} to ${newname}.`);

    this.objects[newname] = this.objects[name];
    delete this.objects[name];
    return this.objects[newname];
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

gameobject.toJSON = gameobject.level_obj;

gameobject.spawn.doc = `Spawn an entity of type 'ur' on this entity. Returns the spawned entity.`;

gameobject.ur = {
//  pos: [0,0],
  scale:1,
  flipx:false,
  flipy:false,
//  angle:0,
  elasticity:0.5,
  friction:1,
  mass:1,
//  velocity:[0,0],
//  angularvelocity:0,
  layer: 0,
  save: true,
  selectable: true,
  ed_locked: false,
};

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

/* Makes a new ur-type from disk. If the ur doesn't exist, it searches on the disk to create it. */
prototypes.from_file = function(file)
{
  var urpath = file;
  var path = urpath.split('.');
  if (path.length > 1 && (path.at(-1) === path.at(-2))) {
    urpath = path.slice(0,-1).join('.');
    return prototypes.get_ur(urpath);
  }
    
  var upperur = gameobject.ur;
  
  if (path.length > 1) {
    var upperpath = path.slice(0,-1);
    upperur = prototypes.get_ur(upperpath.join('/'));
    if (!upperur) {
      Log.error(`Attempted to create an UR ${urpath}, but ${upperpath} is not a defined UR.`);
      return undefined;
    }
  }

  var newur = Object.create(upperur);
  file = file.replaceAll('.','/');

  var jsfile = prototypes.get_ur_file(urpath, ".js");
  var jsonfile = prototypes.get_ur_file(urpath, ".json");

  var script = undefined;
  var json = undefined;

  if (jsfile) script = IO.slurp(jsfile);
  if (jsonfile) json = JSON.parse(IO.slurp(jsonfile));

  if (!json && !script) {
    Log.warn(`Could not make ur from ${file}`);
    return undefined;
  }

  if (script)
    compile_env(script, newur, file);
  
  json ??= {};
  Object.merge(newur,json);

  prototypes.list.push(urpath);
  newur.toString = function() { return urpath; };
  ur[urpath] = newur;

  return ur[urpath];
}
prototypes.from_file.doc = "Create a new ur-type from a given script file.";
prototypes.list = [];

prototypes.from_obj = function(name, obj)
{
  var newur = Object.copy(gameobject.ur, obj);
  prototypes.ur[name] = newur;
  newur.toString = function() { return name; };
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
      list.concat(list_obj(obj[e], e + "."));
    }

    return list;
  }
  
  return list_obj(ur);
}

prototypes.ur2file = function(urpath)
{
  return urpath.replaceAll('.', '/');
}

prototypes.file2ur = function(file)
{
  file = file.strip_ext();
  file = file.replaceAll('/','.');
  return file;
}

/* Returns an ur, or makes it, for any given type of path
   could be a file on a disk like ball/big.js
   could be an ur path like ball.big
*/
prototypes.get_ur = function(name)
{
  var urpath = name;
  if (urpath.includes('/'))
    urpath = prototypes.file2ur(name);

  if (!prototypes.ur[urpath]) {
    var ur = prototypes.from_file(urpath);
    if (ur)
      return ur;
    else {
      Log.warn(`Could not find prototype using name ${name}.`);
      return undefined;
    }
  } else
    return prototypes.ur[urpath];
}

prototypes.get_ur_file = function(path, ext)
{
  var urpath = prototypes.ur2file(path);
  var file = urpath + ext;
  if (IO.exists(file)) return file;
  file = urpath + "/" + path.split('.').at(-1) + ext;
  if (IO.exists(file)) return file;
  return undefined;
}

prototypes.generate_ur = function(path)
{
  var ob = IO.glob("**.js");
  ob = ob.concat(IO.glob("**.json"));
  ob = ob.filter(function(path) { return path !== "game.js" && path !== "play.js" });
  ob = ob.map(function(path) { return path.set_ext(""); });
  ob.forEach(function(name) { prototypes.get_ur(name); });
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
