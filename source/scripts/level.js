var Level = {
  levels: [],
  objects: [],
  alive: true,
  selectable: true,
  toString() {
    if (this.file)
      return this.file;

    return "Loose level";
  },

  fullpath() {
    //return `${this.level.fullpath()}.${this.name}`;
  },
  
  get boundingbox() {
    return bb_from_objects(this.objects);
  },

  varname2obj(varname) {
    for (var i = 0; i < this.objects.length; i++)
      if (this.objects[i].varname === varname)
        return this.objects[i];

    return null;
  },

  run() {
    // TODO: If an object does not have a varname, give it one based on its parent 
    this.objects.forEach(function(x) {
      if (x.hasOwn('varname')) {
        scene[x.varname] = x;
	this[x.varname] = x;
      }
    },this);

    var script = IO.slurp(this.scriptfile);
    compile_env(`var self = this;${script}`, this, this.scriptfile);

    if (typeof this.update === 'function')
      Register.update.register(this.update, this);

    if (typeof this.gui === 'function')
      Register.gui.register(this.gui, this);

    if (typeof this.nk_gui === 'function')
      register_nk_gui(this.nk_gui, this);

    if (typeof this.inputs === 'object') {
      Player.players[0].control(this);
    }
  },

  revert() {
    delete this.unique;
    this.load(this.filelvl);
  },

  /* Returns how many objects this level created are still alive */
  object_count() {
    return objects.length();
  },

  /* Save a list of objects into file, with pos acting as the relative placement */
  saveas(objects, file, pos) {
    if (!pos) pos = find_com(objects);

    objects.forEach(function(obj) {
      obj.pos = obj.pos.sub(pos);
    });

    var newlvl = Level.create();

    objects.forEach(function(x) { newlvl.register(x); });

    var save = newlvl.save();
    slurpwrite(save, file);
  },

  clean() {
    for (var key in this.objects)
      clean_object(this.objects[key]);

    for (var key in gameobjects)
      clean_object(gameobjects[key]);
  },
  
  sync_file(file) {
    var openlvls = this.levels.filter(function(x) { return x.file === file && x !== editor.edit_level; });

    openlvls.forEach(function(x) {
      x.clear();
      x.load(IO.slurp(x.file));
      x.flipdirty = true;
      x.sync();
      x.flipdirty = false;
      x.check_dirty();
    });    
  },

  save() {
    this.clean();
    var pos = this.pos;
    var angle = this.angle;

    this.pos = [0,0];
    this.angle = 0;
    if (this.flipx) {
      this.objects.forEach(function(obj) {
        this.mirror_x_obj(obj);
      }, this);
    }

    if (this.flipy) {
      this.objects.forEach(function(obj) {
        this.mirror_y_obj(obj);
      }, this);
    }
    
    var savereturn = JSON.stringify(this.objects, replacer_empty_nil, 1);

    if (this.flipx) {
      this.objects.forEach(function(obj) {
        this.mirror_x_obj(obj);
      }, this);
    }

    if (this.flipy) {
      this.objects.forEach(function(obj) {
        this.mirror_y_obj(obj);
      }, this);
    }

    this.pos = pos;
    this.angle = angle;
    return savereturn;
  },

  mirror_x_obj(obj) {
    obj.flipx = !obj.flipx;
    var rp = obj.relpos;
    obj.pos = [-rp.x, rp.y].add(this.pos);
    obj.angle = -obj.angle;
  },

  mirror_y_obj(obj) {
    var rp = obj.relpos;
    obj.pos = [rp.x, -rp.y].add(this.pos);
    obj.angle = -obj.angle;
  },

  /* TODO: Remove this; make it work without */
  toJSON() {
    var obj = {};
    obj.file = this.file;
    obj.pos = this._pos;
    obj.angle = this._angle;
    obj.from = "group";
    obj.flipx = this.flipx;
    obj.flipy = this.flipy;
    obj.scale = this.scale;

    if (this.varname)
      obj.varname = this.varname;

    if (!this.unique)
      return obj;

    obj.objects = {};

    this.objects.forEach(function(x,i) {
      obj.objects[i] = {};
      var adiff = Math.abs(x.relangle - this.filelvl[i]._angle) > 1e-5;
      if (adiff)
        obj.objects[i].angle = x.relangle;

      var pdiff = Vector.equal(x.relpos, this.filelvl[i]._pos, 1e-5);
      if (!pdiff)
        obj.objects[i].pos = x._pos.sub(this.pos);

      if (obj.objects[i].empty)
        delete obj.objects[i];
    }, this);

    return obj;
  },

  register(obj) {
    if (obj.level)
      obj.level.unregister(obj);
      
    this.objects.push(obj);
  },

  make() {
    return Level.loadfile(this.file, this.pos);
  },

  spawn(prefab) {
    if (typeof prefab === 'string') {
      var newobj = this.addfile(prefab);
      return newobj;
    }
      
    var newobj = prefab.make();
    newobj.defn('level', this);
    this.objects.push(newobj);
    Game.register_obj(newobj);
    newobj.setup?.();
    newobj.start?.();
    if (newobj.update)
      Register.update.register(newobj.update, newobj);
    return newobj;
  },

  dup(level) {
    level ??= this.level;
    var n = level.spawn(this.from);
    /* TODO: Assign this's properties to the dup */
    return ;n
  },

  create() {
    var newlevel = Object.create(this);
    newlevel.objects = [];
    newlevel._pos = [0,0];
    newlevel._angle = 0;
    newlevel.color = Color.green;
/*    newlevel.toString = function() {
      return (newlevel.unique ? "#" : "") + newlevel.file;
    };
 */
    newlevel.filejson = newlevel.save();
    return newlevel;
  },

  addfile(file) {
    /* TODO: Register this as a prefab for caching */
    var lvl = this.loadfile(file);
    this.objects.push(lvl);
    lvl.level = this;
    return lvl;
  },

  check_dirty() {
    this.dirty = this.save() !== this.filejson;
  },

  add_child(obj) {
    obj.unparent();
    this.objects.push(obj);
    obj.level = this;
  },

  start() {
    this.objects.forEach(function(x) { if ('start' in x) x.start(); });
  },

  loadlevel(file) {
    var lvl = Level.loadfile(file);
    if (lvl && Game.playing())
      lvl.start();

    return lvl;
  },

  loadfile(file) {
    if (!file.endsWith(".lvl")) file = file + ".lvl";
    var newlevel = Level.create();

    if (IO.exists(file)) {
      newlevel.filejson = IO.slurp(file);
      
      try {
	newlevel.filelvl = JSON.parse(newlevel.filejson);
	newlevel.load(newlevel.filelvl);
      } catch (e) {
        newlevel.ed_gizmo = function() { GUI.text("Invalid level file: " + newlevel.file, world2screen(newlevel.pos), 1, Color.red); };
	newlevel.selectable = false;
	throw e;
      }
      newlevel.file = file;
      newlevel.dirty = false;
    }
    
    var scriptfile = file.replace('.lvl', '.js');
    if (IO.exists(scriptfile)) {
      newlevel.script = IO.slurp(scriptfile);
      newlevel.scriptfile = scriptfile;
    }

    newlevel.from = scriptfile.replace('.js','');
    newlevel.file = newlevel.from;
    newlevel.run();

    return newlevel;
  },

  /* Spawns all objects specified in the lvl json object */
  load(lvl) {
    this.clear();
    this.levels.push_unique(this);
    
    if (!lvl) {
      Log.warn("Level is " + lvl + ". Need a better formed one.");

      return;
    }

    var opos = this.pos;
    var oangle = this.angle;
    this.pos = [0,0];
    this.angle = 0;

    var objs;
    var created = [];

    if (typeof lvl === 'string')
      objs = JSON.parse(lvl);
    else
      objs = lvl;

   if (typeof objs === 'object')
     objs = objs.array();

    objs.forEach(x => {
      if (x.from === 'group') {
        var loadedlevel = Level.loadfile(x.file);
	if (!loadedlevel) {
          Log.error("Error loading level: file " + x.file + " not found.");
	  return;
	}
	if (!IO.exists(x.file)) {
	  loadedlevel.ed_gizmo = function() { GUI.text("MISSING LEVEL " + x.file, world2screen(loadedlevel.pos) ,1, Color.red) };
	}
	var objs = x.objects;
	delete x.objects;
	Object.assign(loadedlevel, x);

	if (objs) {
   	  objs.array().forEach(function(x, i) {
	    if (x.pos)
  	      loadedlevel.objects[i].pos = x.pos.add(loadedlevel.pos);

	    if (x.angle)
	      loadedlevel.objects[i].angle = x.angle + loadedlevel.angle;
	  });

	  loadedlevel.unique = true;
	}
	loadedlevel.level = this;
	loadedlevel.sync();
	created.push(loadedlevel);
	this.objects.push(loadedlevel);
	return;
      }
      var prototype = gameobjects[x.from];
      if (!prototype) {
        Log.error(`Prototype for ${x.from} does not exist.`);
	return;
      }

      var newobj = this.spawn(gameobjects[x.from]);

      delete x.from;

      dainty_assign(newobj, x);
      
      if (x._pos)
        newobj.pos = x._pos;
	

      if (x._angle)
        newobj.angle = x._angle;
      for (var key in newobj.components)
        if ('sync' in newobj.components[key]) newobj.components[key].sync();

      newobj.sync();

      created.push(newobj);
    });

    created.forEach(function(x) {
      if (x.varname)
        this[x.varname] = x;
    },this);

    this.pos = opos;
    this.angle = oangle;

    return created;
  },

  clear() {
    for (var i = this.objects.length-1; i >= 0; i--)
      if (this.objects[i].alive)
        this.objects[i].kill();

    this.levels.remove(this);
  },

  clear_all() {
    this.levels.forEach(function(x) { x.kill(); });
  },

  kill() {
    if (this.level)
      this.level.unregister(this);
    
    Register.unregister_obj(this);
      
    this.clear();
  },
  
  unregister(obj) {
    var removed = this.objects.remove(obj);

    if (removed && obj.varname)
      delete this[obj.varname];
  },

  remove_child(child) {
    this.objects.remove(child);
  },

  get pos() { return this._pos; },
  set pos(x) {
    var diff = x.sub(this._pos);
    this.objects.forEach(function(x) { x.pos = x.pos.add(diff); });
    this._pos = x;
  },

  get angle() { return this._angle; },
  set angle(x) {
    var diff = x - this._angle;
    this.objects.forEach(function(x) {
      x.angle = x.angle + diff;
      var pos = x.pos.sub(this.pos);
      var r = Vector.length(pos);
      var p = Math.rad2deg(Math.atan2(pos.y, pos.x));
      p += diff;
      p = Math.deg2rad(p);
      x.pos = this.pos.add([r*Math.cos(p), r*Math.sin(p)]);
    },this);
    this._angle = x;
  },

  flipdirty: false,
  
  sync() {
    this.flipx = this.flipx;
    this.flipy = this.flipy;
  },

  _flipx: false,
  get flipx() { return this._flipx; },
  set flipx(x) {
    if (this._flipx === x && (!x || !this.flipdirty)) return;
    this._flipx = x;
    
    this.objects.forEach(function(obj) {
      obj.flipx = !obj.flipx;
      var rp = obj.relpos;
      obj.pos = [-rp.x, rp.y].add(this.pos);
      obj.angle = -obj.angle;
    },this);
  },
  
  _flipy: false,
  get flipy() { return this._flipy; },
  set flipy(x) {
    if (this._flipy === x && (!x || !this.flipdirty)) return;
    this._flipy = x;
    
    this.objects.forEach(function(obj) {
      var rp = obj.relpos;
      obj.pos = [rp.x, -rp.y].add(this.pos);
      obj.angle = -obj.angle;
    },this);
  },

  _scale: 1.0,
  get scale() { return this._scale; },
  set scale(x) {
    var diff = (x - this._scale) + 1;
    this._scale = x;

    this.objects.forEach(function(obj) {
      obj.scale *= diff;
      obj.relpos = obj.relpos.scale(diff);
    }, this);
  },
  
  get up() {
    return [0,1].rotate(Math.deg2rad(this.angle));
  },
  
  get down() {
    return [0,-1].rotate(Math.deg2rad(this.angle));
  },
  
  get right() {
    return [1,0].rotate(Math.deg2rad(this.angle));
  },
  
  get left() {
    return [-1,0].rotate(Math.deg2rad(this.angle));
  },
};
