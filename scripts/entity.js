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
  impl: {
    clear() {
      for (var k in this.objects) {
        Log.info(`Killing ${k}`);
	this.objects[k].kill();
      };
      this.objects = {};
    },
      gscale() { return cmd(103,this.body); },
      get scale() {
        if (!this.level) return this.gscale();
        return this.gscale()/this.level.gscale();
      },
      set scale(x) {
        if (this.level)
          x *= this.level.gscale();
	var pct = x/this.scale;
        cmd(36, this.body, x);	

	this.objects?.forEach(function(obj) {
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
      
      set pos(x) {
        this.set_worldpos(Vector.rotate(x,Math.deg2rad(this.level.angle)).add(this.level.worldpos()));
      },
      
      get pos() {
        var offset = this.worldpos().sub(this.level.worldpos());
	return Vector.rotate(offset, -Math.deg2rad(this.level.angle));
      },
      get elasticity() { return cmd(107,this.body); },
      set elasticity(x) { cmd(106,this.body,x); },

      get friction() { return cmd(109,this.body); },
      set friction(x) { cmd(108,this.body,x); },

      set mass(x) { set_body(7,this.body,x); },
      get mass() {
        if (!(this.phys === Physics.dynamic))
	  return this.__proto__.mass;
	  
        return q_body(5, this.body);
      },

      set phys(x) { set_body(1, this.body, x); },
      get phys() { return q_body(0,this.body); },
      get velocity() { return q_body(3, this.body); },
      set velocity(x) { set_body(9, this.body, x); },
      get angularvelocity() { return Math.rad2deg(q_body(4, this.body)); },
      set angularvelocity(x) { set_body(8, this.body, Math.deg2rad(x)); },
      worldpos() { return q_body(1,this.body); },
      set_worldpos(x) {
        var diff = x.sub(this.worldpos());
	this.objects.forEach(function(x) { x.set_worldpos(x.worldpos().add(diff)); });
	set_body(2,this.body,x);
      },

      worldangle() { return Math.rad2deg(q_body(2,this.body))%360; },
      get angle() {
        if (!this.level) return this.worldangle();
        return this.worldangle() - this.level.worldangle();
      },
      set angle(x) {
        var diff = x - this.angle;
	var thatpos = this.pos;
	this.objects.forEach(function(x) {
	  x.rotate(diff);
//	  x.angle = x.angle + diff;
	  var opos = x.pos;
	  var r = Vector.length(opos);
	  var p = Math.rad2deg(Math.atan2(opos.y, opos.x));
	  p += diff;
	  p = Math.deg2rad(p);
	  x.pos = [r*Math.cos(p), r*Math.sin(p)];
	}, this);
	  
        set_body(0,this.body, Math.deg2rad(x - this.level.worldangle()));
      },

      rotate(x) {
        this.angle = this.angle + x;
      },

      
      pulse(vec) { set_body(4, this.body, vec);},

      shove(vec) { set_body(12,this.body,vec);},
      world2this(pos) { return cmd(70, this.body, pos); },
      this2world(pos) { return cmd(71, this.body,pos); },
      set layer(x) { cmd(75,this.body,x); },
      get layer() { return 0; },
      alive() { return this.body >= 0; },
      in_air() { return q_body(7, this.body);},
      on_ground() { return !this.in_air(); },
      spawn_from_instance(inst) {
        return this.spawn(inst.ur, inst);
      },
      
      spawn(ur, data) {
	if (typeof ur === 'string')
	  ur = prototypes.get_ur(ur);
        if (!ur) {
	  Log.warn("Failed to make UR from " + ur);
	  return undefined;
	}
	
	var go = ur.make(this, data);
        return go;
      },

  /* Reparent 'this' to be 'parent's child */
  reparent(parent) {
    if (this.level === parent)
      return;

    this.level?.remove_obj(this);
    
    this.level = parent;
      
    function unique_name(list, obj) {
      var str = obj.toString().replaceAll('.', '_');
      var n = 1;
      var t = str;
      while (Object.hasOwn(list, t)) {
        t = str + n;
	n++;
      }
      return t;
    };
    
    var name = unique_name(parent.objects, this.ur);
    parent.objects[name] = this;
    this.toString = function() { return name; };
  },
  
   remove_obj(obj) {
     if (this[obj.toString()] === this.objects[obj.toString()])
       delete this[obj.toString()];
       
     delete this.objects[obj.toString()];
   },
      
  },
  
  draw_layer: 1,
  components: {},
  objects: {},
  level: undefined,

  hide() { this.components.forEach(function(x) { x.hide(); }); this.objects.forEach(function(x) { x.hide(); }); },
  show() { this.components.forEach(function(x) { x.show(); }); this.objects.forEach(function(x) { x.show(); }); },

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

  move(vec) {
    this.pos = this.pos.add(vec);
  },

  rotate(amt) {
    this.angle += amt;
  },

  /* Make a unique object the same as its prototype */
  revert() {
    Object.merge(this,this.__proto__);
    this.sync();
  },

  check_registers(obj) {
    Register.unregister_obj(obj);
  
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
    pos: [0,0],
    angle:0,
    phys:1,
    flipx:false,
    flipy:false,
    scale:1,
    elasticity:0.5,
    friction:1,
    mass:1,
    velocity:[0,0],
    angularvelocity:0,
    layer:0,
    worldpos() { return [0,0]; },
    
    save:true,
    selectable:true,
    ed_locked:false,
    

      disable() { this.components.forEach(function(x) { x.disable(); });},
      enable() { this.components.forEach(function(x) { x.enable(); });},
      sync() {
       this.components.forEach(function(x) { x.sync(); });
       this.objects.forEach(function(x) { x.sync(); });
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

      json_obj() {
        var d = ediff(this,this.__proto__);
	d ??= {};
 
	var objects = {};
	this.__proto__.objects ??= {};
	var curobjs = {};
	for (var o in this.objects)
	  curobjs[o] = this.objects[o].instance_obj();

        var odiff = ediff(curobjs, this.__proto__.objects);
	if (odiff)
	  d.objects = curobjs;

	delete d.pos;
	delete d.angle;
	delete d.velocity;
	delete d.angularvelocity;
        return d;
      },

      instance_obj() {
        var t = this.transform_obj();
	t.ur = this.ur;
	return t;
      },

      transform_obj() {
        var t = this.json_obj();
	Object.assign(t, this.transform());
	return t;
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
        var n = this.level.spawn(this.__proto__);
	Object.totalmerge(n, this.make_ur());
	return n;
      },
      
      kill() {
	if (this.body === -1) {
	  Log.warn(`Object is already dead!`);
	  return;
	}
	

//	Register.endofloop(() => {
//	  cmd(2, this.body);
	  q_body(8,this.body);
	  Game.unregister_obj(this);

	  if (this.level) {
    	    this.level.remove_obj(this);
	    this.level = undefined;
	  }

	  Player.uncontrol(this);
	  Register.unregister_obj(this);
//	  this.instances.remove(this);
	  this.body = -1;

	  for (var key in this.components) {
	    Register.unregister_obj(this.components[key]);
	    Log.info(`Destroying component ${key}`);
	    this.components[key].kill();
	    this.components.gameobject = undefined;
	  }
	  delete this.components;

          this.clear();

	  if (typeof this.stop === 'function')
  	    this.stop();
//	});
      },

      up() { return [0,1].rotate(Math.deg2rad(this.angle));},
      down() { return [0,-1].rotate(Math.deg2rad(this.angle));},
      right() { return [1,0].rotate(Math.deg2rad(this.angle));},
      left() { return [-1,0].rotate(Math.deg2rad(this.angle));},

  make(level, data) {
    level ??= Primum;
    var obj = Object.create(this);
    obj.level = level;
//    this.instances.push(obj);
    obj.body = make_gameobject();
    obj.components = {};
    obj.objects = {};
    assign_impl(obj, gameobject.impl);
    obj._ed = {
      selectable: true,
      dirty: false,
    };
    obj.ur = this.toString();
    
    Game.register_obj(obj);

    cmd(113, obj.body, obj); // set the internal obj reference to this obj

    obj.level = undefined;
    obj.reparent(level);

    Object.hide(obj, 'ur','body', 'components', 'objects', '_ed', 'level');    

    for (var prop in this) {
      var p = this[prop];
      if (typeof p !== 'object') continue;
      if (typeof p.make === 'function') {
        obj[prop] = p.make(obj);
	obj.components[prop] = obj[prop];
      }
    };
    
    if (this.objects)
      obj.make_objs(this.objects);

    Object.dainty_assign(obj, this);
    obj.sync();
    
    gameobject.check_registers(obj);

    if (Game.playing() && typeof obj.start === 'function') obj.start();

    return obj;
  },

  make_objs(objs) {
    for (var prop in objs) {
      var newobj = this.spawn_from_instance(objs[prop]);
      if (!newobj) continue;
      this.rename_obj(newobj.toString(), prop);
    }
  },

  rename_obj(name, newname) {
    if (!this.objects[name]) {
      Log.warn(`No object with name ${name}. Could not rename to ${newname}.`);
      return;
    }
    if (this.objects[newname])
      return;

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

gameobject.impl.spawn.doc = `Spawn an entity of type 'ur' on this entity. Returns the spawned entity.`;

/* Default objects */
var prototypes = {};
prototypes.ur = {};
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
    
  var upperur = gameobject;
  
  if (path.length > 1) {
    var upperpath = path.slice(0,-1);
    upperur = prototypes.get_ur(upperpath.join('/'));
    if (!upperur) {
      Log.error(`Attempted to create an UR ${urpath}, but ${upperpath} is not a defined UR.`);
      return undefined;
    }
  }

  var newur = {};
  
  file = file.replaceAll('.','/');

  var jsfile = prototypes.get_ur_file(urpath, ".js");
  var jsonfile = prototypes.get_ur_file(urpath, ".json");

  var script = undefined;
  var json = undefined;

  if (jsfile) script = IO.slurp(jsfile);
  try {
    if (jsonfile) json = JSON.parse(IO.slurp(jsonfile));
  } catch(e) {
    Log.warn(`Unable to create json from ${jsonfile}`);
  }

  if (!json && !script) {
    Log.warn(`Could not make ur from ${file}`);
    return undefined;
  }

  if (script)
    compile_env(script, newur, file);
  
  json ??= {};
  Object.merge(newur,json);

  Object.entries(newur).forEach(function([k,v]) {
    if (Object.isObject(v) && Object.isObject(upperur[k]))
      v.__proto__ = upperur[k];
  });

  Object.values(newur).forEach(function(v) {
    if (typeof v !== 'object') return;
    if (!v.comp) return;
    v.__proto__ = component[v.comp];
  });
      
  newur.__proto__ = upperur;
  newur.instances = [];
  Object.hide(newur, 'instances');

  prototypes.list.push(urpath);
  newur.toString = function() { return urpath; };
  ur[urpath] = newur;

  return ur[urpath];
}
prototypes.from_file.doc = "Create a new ur-type from a given script file.";
prototypes.list = [];

prototypes.from_obj = function(name, obj)
{
  var newur = Object.copy(gameobject, obj);
  prototypes.ur[name] = newur;
  prototypes.list.push(name);
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
  if (!name) {
    Log.warn(`Can't get ur from an undefined.`);
    return;
  }
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

prototypes.resavi = function(ur, path)
{
  if (!ur) return path;
  if (path[0] === '/') return path;

  var res = ur.replaceAll('.', '/');
  var dir = path.dir();
  if (res.startsWith(dir))
    return path.base();

  return path;
}

prototypes.resani = function(ur, path)
{
  if (!path) return "";
  if (!ur) return path;
  if (path[0] === '/') return path.slice(1);

  var res = ur.replaceAll('.', '/');
  var restry = res + "/" + path;
  while (!IO.exists(restry)) {
    res = res.updir() + "/";
    if (res === "/")
      return path;

    restry = res + path;
  }
  return restry;
}
