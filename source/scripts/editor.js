/*
  Editor-only variables on objects
  selectable
*/

var required_files = ["proto.json"];

required_files.forEach(x => {
  if (!IO.exists(x)) slurpwrite("", x);
});

var editor_level = Level.create();
var editor_camera = editor_level.spawn(camera2d);
editor_camera.save = false;
set_cam(editor_camera.body);

var editor_config = {
  grid_size: 100,
};

var configs = {
  toString() { return "configs"; },
  editor: editor_config,
  physics: physics,
  local: local_conf,
  collision: Collision,
};

var editor = {
  dbgdraw: false,
  selected: null,
  selectlist: [],
  moveoffset: [0,0],
  startrot: 0,
  rotoffset: 0,
  obj_panel: false,
  camera: editor_camera,
  edit_level: {}, /* The current level that is being edited */
  working_layer: -1,
  cursor: null,
  edit_mode: "basic",

  input_f1_pressed() {
    if (Keys.ctrl()) {
      this.edit_mode = "basic";
      return;
    }

    Debug.draw_phys(!Debug.phys_drawing);
  },

  input_f2_pressed() {
    if (Keys.ctrl()) {
      this.edit_mode = "brush";
      return;
    }
    
    objectexplorer.on_close = save_configs;
    objectexplorer.obj = configs;
    this.openpanel(objectexplorer);
  },

  input_j_pressed() {
    if (Keys.alt()) {
      var varmakes = this.selectlist.filter(function(x) { return x.hasOwn('varname'); });
      varmakes.forEach(function(x) { delete x.varname; });
    } else if (Keys.ctrl()) {
      var varmakes = this.selectlist.filter(function(x) { return !x.hasOwn('varname'); });
      varmakes.forEach(function(x) {
        var allvnames = [];
        this.edit_level.objects.forEach(function(x) {
          if (x.varname)
            allvnames.push(x.varname);
        });
      
        var vname = x.from.replace(/ /g, '_').replace(/_object/g, '').replace(/\..*$/g, '');
	var tnum = 1;
        var testname = vname + "_" + tnum;
        while (allvnames.includes(testname)) {
          tnum++;
          testname = vname + "_" + tnum;
        }
        x.varname = testname;
     },this);
    }
  },
  
  input_i_pressed() {
    if (!Keys.ctrl()) return;
    
    this.openpanel(levellistpanel, true);
  },
  
  input_y_pressed() {
    if (Keys.ctrl()) {
      if (this.selectlist.length !== 1) return;
      objectexplorer.obj = this.selectlist[0];
      objectexplorer.on_close = this.save_prototypes;
      this.openpanel(objectexplorer);
    } else if (Keys.alt()) {
      this.openpanel(protoexplorer);
    }
  },

  try_select() { /* nullify true if it should set selected to null if it doesn't find an object */
    var go = physics.pos_query(screen2world(Mouse.pos));
    return this.do_select(go);
  },
    
  /* Tries to select id */
  do_select(go) {
    var obj = go >= 0 ? Game.object(go) : null;
    if (!obj || !obj.selectable) return null;

    if (this.working_layer > -1 && obj.draw_layer !== this.working_layer) return null;

    if (obj.level !== this.edit_level) {
      var testlevel = obj.level;
      while (testlevel && testlevel.level !== this.edit_level) {
         testlevel = testlevel.level;
      }
      
      return testlevel;
    }

    return obj;
  },
  
  curpanel: null,

  input_o_pressed() {
    if (this.sel_comp) return;
    if (Keys.ctrl() && Keys.alt()) {
      if (this.selectlist.length === 1 && this.selectlist[0].file) {
        if (this.edit_level.dirty) return;
	this.load(this.selectlist[0].file);
      }

      return;
    }

    if (Keys.ctrl() && Keys.shift()) {
      if (!this.edit_level.dirty)
        this.load_prev();

      return;
    }
    
    if (Keys.ctrl()) {
      if (this.check_level_nested())
        return;

      if (this.edit_level.dirty) {
	this.openpanel(gen_notify("Level is changed. Are you sure you want to close it?", function() {
	  this.clear_level();
	  this.openpanel(openlevelpanel);
	}.bind(this)));
	return;
      }

      this.openpanel(openlevelpanel);
      return;
    }

    if (Keys.alt()) {
      this.openpanel(addlevelpanel);
      return;
    }
    
    this.obj_panel = !this.obj_panel;
  },

  check_level_nested() {
    if (this.edit_level.level) {
      this.openpanel(gen_notify("Can't close a nested level. Save up to the root before continuing."));
      return true;
    }

    return false;
  },

  levellist: false,
  programmode: false,

  input_l_pressed() {
    if (this.sel_comp) return;
    if (Keys.ctrl()) {
      texteditor.on_close = function() { editor.edit_level.script = texteditor.value;};
      texteditor.input_s_pressed = function() {
        if (!Keys.ctrl()) return;
        editor.edit_level.script = texteditor.value;
	editor.save_current();
	texteditor.startbuffer = texteditor.value.slice();
      };
      
      this.openpanel(texteditor);
      if (!this.edit_level.script)
        this.edit_level.script = "";
      texteditor.value = this.edit_level.script;
      texteditor.start();
    } else if (Keys.alt()) {
      this.programmode = !this.programmode;
    }
  },

  delete_empty_reviver(key, val) {
    if (typeof val === 'object' && val.empty)
      return undefined;

    return val;
  },


  input_minus_pressed() {
    if (this.sel_comp) return;
    
    if (!this.selectlist.empty) {
      this.selectlist.forEach(function(x) { x.draw_layer--; });
      return;
    }
    
    if (this.working_layer > -1)
      this.working_layer--;
  },

  input_plus_pressed() {
    if (this.sel_comp) return;
    
    if (!this.selectlist.empty) {
      this.selectlist.forEach(x => x.draw_layer++);
      return;
    }
    
    if (this.working_layer < 4)
      this.working_layer++;
  },

  input_p_pressed() {
    if (Keys.ctrl()) {
      if (Keys.shift()) {
        /* Save prototype as */
	if (this.selectlist.length !== 1) return;
	this.openpanel(saveprototypeas);
	return;
      }
      else {
        this.save_proto();
      }
    } else if (Keys.alt())
      this.openpanel(prefabpanel);
  },

  save_proto() {
    if (this.selectlist.length !== 1) return;
    Log.warn(`Saving prototype ${this.selectlist[0].toString()}`);
      var protos = JSON.parse(IO.slurp("proto.json"));
      
      var tobj = this.selectlist[0].prop_obj();
      var pobj = this.selectlist[0].__proto__.prop_obj();

      Log.warn("Going to deep merge.");
      deep_merge(pobj, tobj);
      Log.warn("Finished deep merge.");

      pobj.from = this.selectlist[0].__proto__.from;


      protos[this.selectlist[0].__proto__.name] = pobj;
      Log.warn(JSON.stringify(protos));
      slurpwrite(JSON.stringify(protos, null, 2), "proto.json");
      
      /* Save object changes to parent */
      dainty_assign(this.selectlist[0].__proto__, tobj);

      /* Remove the local from this object */
      unmerge(this.selectlist[0], tobj);

      /* Now sync all objects */
      Game.objects.forEach(x => x.sync());
      
  },

  save_prototypes() {
    save_gameobjects_as_prototypes();
  },
  
  /* Save the selected object as a new prototype, extending the chain */
  save_proto_as(name) {
    if (name in gameobjects) {
      Log.info("Already an object with name '" + name + "'. Choose another one.");
      return;
    }
    var newp = this.selectlist[0].__proto__.clone(name);

    for (var key in newp)
      if (typeof newp[key] === 'object' && 'clone' in newp[key])
        newp[key] = newp[key].clone();

    dainty_assign(newp, this.selectlist[0].prop_obj());
    this.selectlist[0].kill();
    var gopos = this.selectlist[0].pos;
    this.unselect();
    var proto = this.edit_level.spawn(gameobjects[name]);
    this.selectlist.push(proto);
    this.save_proto();
    proto.pos = gopos;
  },

  /* Save selected object as a new prototype, replacing the current prototype */
  save_type_as(name) {
    if (name in gameobjects) {
      Log.info("Already an object with name '" + name + "'. Choose another one.");
      return;
    }

    var newp = this.selectlist[0].__proto__.__proto__.clone(name);
    
    for (var key in newp)
      if (typeof newp[key] === 'object' && 'clone' in newp[key])
        newp[key] = newp[key].clone();

    var tobj = this.selectlist[0].prop_obj();
    var pobj = this.selectlist[0].__proto__.prop_obj();
    deep_merge(pobj, tobj);
    

    dainty_assign(newp, pobj);
    this.selectlist[0].kill();
    this.unselect();
    var proto = this.edit_level.spawn(gameobjects[name]);
    this.selectlist.push(proto);
    this.save_proto();
  },

  /* Makes it so only these objects are editable */
  focus_objects(objs) {

  },

  dup_objects(x) {
    var objs = x.slice();
    var duped = [];

    objs.forEach(function(x,i) {
      if (x.file) {
        var newlevel = this.edit_level.addfile(x.file);
	newlevel.pos = x.pos;
	newlevel.angle = x.angle;
        duped.push(newlevel);
      } else {
      var newobj = this.edit_level.spawn(x.__proto__);
      duped.push(newobj);
      newobj.pos = x.pos;
      newobj.angle = x.angle;
      dainty_assign(newobj, x.prop_obj());
    }
    },this);

    duped.forEach(function(x) { delete duped.varname; });

    return duped.flat();
  },

  input_d_pressed() {
    if (!Keys.ctrl() || this.selectlist.length === 0) return;
    var duped = this.dup_objects(this.selectlist);
    this.unselect();
    this.selectlist = duped;
  },
  
  sel_start: [],

  input_lmouse_pressed() {
    if (this.sel_comp) return;

    if (this.edit_mode === "brush") {
      this.paste();
      return;
    }
    
    if (this.selectlist.length === 1 && Keys.ctrl() && Keys.shift() && Keys.alt()) {
      this.selectlist[0].set_center(screen2world(Mouse.pos));
      return;
    }
    
    this.sel_start = screen2world(Mouse.pos);
  },
  
  points2cwh(start, end) {
    var c = [];
    c[0] = (end[0] - start[0]) / 2;
    c[0] += start[0];
    c[1] = (end[1] - start[1]) / 2; 
    c[1] += start[1];
    var wh = [];
    wh[0] = Math.abs(end[0] - start[0]);
    wh[1] = Math.abs(end[1] - start[1]);
    return {c: c, wh: wh};
  },
  
  input_lmouse_released() {
    Mouse.normal();
    if (!this.sel_start) return; 
    
    if (this.sel_comp) {
      this.sel_start = null;
      return;
    }

    var selects = [];
    
    /* TODO: selects somehow gets undefined objects in here */
    if (screen2world(Mouse.pos).equal(this.sel_start)) {
      var sel = this.try_select();
      if (sel) selects.push(sel);
    } else {
      var box = this.points2cwh(this.sel_start, screen2world(Mouse.pos));
    
      box.pos = box.c;
    
      var hits = physics.box_query(box);

      hits.forEach(function(x, i) {
        var obj = this.do_select(x);
	if (obj)
	  selects.push(obj);
      },this);
      
      var levels = this.edit_level.objects.filter(function(x) { return x.file; });
      var lvlpos = [];
      levels.forEach(function(x) { lvlpos.push(x.pos); });
      var lvlhits = physics.box_point_query(box, lvlpos);
      
      lvlhits.forEach(function(x) { selects.push(levels[x]); });
    }

    this.sel_start = null;
    selects = selects.flat();
    selects = selects.unique();
    
    if (selects.empty) return;

    if (Keys.shift()) {
      selects.forEach(function(x) {
        this.selectlist.push_unique(x);
      }, this);

      return;
    }
      
    if (Keys.ctrl()) {
      selects.forEach(function(x) {
        this.selectlist.remove(x);
      }, this);

      return;
    }
    
    this.selectlist = [];
    selects.forEach(function(x) {
      if (x !== null)
        this.selectlist.push(x);
    }, this);
  },

  mover(amt, snap) {
    return function(go) { go.pos = go.pos.add(amt)};
  },

  input_any_released() {
    this.check_snapshot();
    this.edit_level.check_dirty();
  },


  step_amt() { return Keys.shift() ? 10 : 1; },

  on_grid(pos) {
    return pos.every(function(x) { return x % editor_config.grid_size === 0; });
  },

  snapper(dir, grid) {
    return function(go) {
        go.pos = go.pos.add(dir.scale(grid/2));
        go.pos = go.pos.map(function(x) { return Math.snap(x, grid) }, this);
    }
  },

  input_up_pressed() {
    this.key_move([0,1]);
  },

  input_up_rep() {
    this.input_up_pressed();
  },

  input_left_pressed() {
    this.key_move([-1,0]);
  },

  input_left_rep() {
    this.input_left_pressed();
  },

  key_move(dir) {
    if (Keys.ctrl())
      this.selectlist.forEach(this.snapper(dir.scale(1.01), editor_config.grid_size));
    else
      this.selectlist.forEach(this.mover(dir.scale(this.step_amt())));
  },
  
  input_right_pressed() {
    this.key_move([1,0]);
  },

  input_right_rep() {
    this.input_right_pressed();
  },

  input_down_pressed() {
    this.key_move([0,-1]);
  },

  input_down_rep() {
    this.input_down_pressed();
  },

  /* Snapmode
     0 No snap
     1 Pixel snap
     2 Grid snap
  */
  snapmode: 0,

  snapped_pos(pos) {
    switch (this.snapmode) {
      case 0:
        return pos;

      case 1:
        return pos.map(function(x) { return Math.round(x); });

      case 2:
        return pos.map
    }
  },
    
  input_delete_pressed() {
    this.selectlist.forEach(x => x.kill());
    this.unselect();
  },

  unselect() {
    this.selectlist = [];
    this.grabselect = null;
    this.sel_comp = null;
  },

  sel_comp: null,

  input_m_pressed() {
    if (this.sel_comp) return;
    if (Keys.ctrl()) {
      this.selectlist.forEach(function(x) {
        x.flipy = !x.flipy;
      });
    } else
      this.selectlist.forEach(function(x) {
        x.flipx = !x.flipx;
      });
    
  },

  input_u_pressed() {
    if (Keys.ctrl()) {
      this.selectlist.forEach(function(x) {
        x.revert();
      });
    }

    if (Keys.alt()) {
      this.selectlist.forEach(function(x) {
        x.unique = true;
      });
    }
  },

  comp_info: false,
  
  input_q_pressed() {
    this.comp_info = !this.comp_info;
  },

  brush_obj: null,

  input_b_pressed() {
    if (this.sel_comp) return;
    if (this.brush_obj) {
      this.brush_obj = null;
      return;
    }

    if (this.selectlist.length !== 1) return;
    this.brush_obj = this.selectlist[0];
    this.unselect();
  },
  
  camera_recalls: {},
  
  input_num_pressed(num) {
    if (Keys.ctrl()) {
      this.camera_recalls[num] = {
        pos:this.camera.pos,
	zoom:this.camera.zoom
      };
      return;
    }
    
    if (Keys.alt()) {
      switch(num) {
        case 0:
	  Render.normal();
	  break;
	  
	case 2:
	  Render.wireframe();
	  break;
      }
    
      return;
    }
    
    if (num === 0) {
      this.camera.pos = [0,0];
      this.camera.zoom = 1;
      return;
    }

    if (num in this.camera_recalls)
      Object.assign(this.camera, this.camera_recalls[num]);
  },

  zoom_to_bb(bb) {
    var cwh = bb2cwh(bb);
    
    var xscale = cwh.wh.x / Window.width;
    var yscale = cwh.wh.y / Window.height;
    
    var zoom = yscale > xscale ? yscale : xscale;
    
    this.camera.pos = cwh.c;
    this.camera.zoom = zoom*1.3;    
  },

  input_f_pressed() {
    if (!Keys.ctrl()) {
      if (this.selectlist.length === 0) return;
      var bb = this.selectlist[0].boundingbox;
      this.selectlist.forEach(function(obj) { bb = bb_expand(bb, obj.boundingbox); });
      this.zoom_to_bb(bb);
      
      return;
    }

    if (Keys.shift()) {
      if (!this.edit_level.level) return;

      this.edit_level = this.edit_level.level;
      this.unselect();
      this.reset_undos();
      this.curlvl = this.edit_level.save();
      this.edit_level.filejson = this.edit_level.save();
      this.edit_level.check_dirty();
    } else {
      if (this.selectlist.length === 1) {
        if (!this.selectlist[0].file) return;
        this.edit_level = this.selectlist[0];
	this.unselect();
	this.reset_undos();
	this.curlvl = this.edit_level.save();
      }
    }
  },
  
  input_rmouse_down() {
    if (Keys.ctrl() && Keys.alt())
      this.camera.zoom = this.z_start * (1 + (Mouse.pos[1] - this.mousejoy[1])/500);
  },
  
  z_start: 1,
  
  input_rmouse_released() {
    Mouse.normal();
  },

  input_rmouse_pressed() {
    if (Keys.shift()) {
      this.cursor = null;
      return;
    }
    
    if (Keys.ctrl() && Keys.alt()) {
      this.mousejoy = Mouse.pos;
      this.z_start = this.camera.zoom;
      Mouse.disabled();
      return;
    }

    if (this.brush_obj)
      this.brush_obj = null;
  
    if (this.sel_comp) {
      this.sel_comp = null;
      return;
    }
    
    this.unselect();
  },

  grabselect: null,
  mousejoy: [0,0],
  joystart: [0,0],
  
  input_mmouse_pressed() {
    if (Keys.ctrl() && Keys.alt()) {
      this.mousejoy = Mouse.pos;
      this.joystart = this.camera.pos;
      return;
    }

    if (Keys.shift() && Keys.ctrl()) {
      this.cursor = find_com(this.selectlist);
      return;
    }

    if (this.brush_obj) {
      this.selectlist = this.dup_objects([this.brush_obj]);
      this.selectlist[0].pos = screen2world(Mouse.pos);
      this.grabselect = this.selectlist[0];
      return;
    }
    
    if (this.sel_comp && 'pick' in this.sel_comp) {
      this.grabselect = this.sel_comp.pick(screen2world(Mouse.pos));
      if (!this.grabselect) return;
      
      this.moveoffset = this.sel_comp.gameobject.this2world(this.grabselect).sub(screen2world(Mouse.pos));
      return;
    }
  
    var grabobj = this.try_select();
    if (Array.isArray(grabobj)) {
      this.selectlist = grabobj;
      return;
    }
    this.grabselect = null;
    if (!grabobj) return;
    
    if (Keys.ctrl()) {
      grabobj = this.dup_objects([grabobj])[0];
    }

    this.grabselect = grabobj;

    if (!this.selectlist.includes(grabobj)) {
      this.selectlist = [];
      this.selectlist.push(grabobj);
    }

    this.moveoffset = this.grabselect.pos.sub(screen2world(Mouse.pos));
  },
  
  input_mmouse_released() {
    Mouse.normal();
    this.grabselect = null;
  },

  input_mmouse_down() {
    if (Keys.shift() && !Keys.ctrl()) {
      this.cursor = Mouse.worldpos;
      return;
    }

    if (Keys.alt() && Keys.ctrl()) {
      this.camera.pos = this.joystart.add(Mouse.pos.sub(this.mousejoy).mapc(mult, [-1,1]).scale(editor_camera.zoom));
      return;
    }
  
    if (!this.grabselect) return;

    if ('pos' in this.grabselect)
      this.grabselect.pos = this.moveoffset.add(screen2world(Mouse.pos));
    else
      this.grabselect.set(this.selectlist[0].world2this(this.moveoffset.add(screen2world(Mouse.pos))));
  },
  

  stash: "",

  start_play_ed() {
      this.stash = this.edit_level.save();
      this.edit_level.kill();
      load_configs("game.config");
      game.start();
      unset_pawn(this);
      set_pawn(limited_editor);
      Register.unregister_obj(this);
  },
   
  input_f5_pressed() {
    /* Start debug.lvl */
    if (!sim_playing()) {
      this.start_play_ed();
      Level.loadlevel("debug_start.lvl");
    }
  },
  
  input_f6_pressed() {
    /* Start game.lvl */
    if (!sim_playing()) {
      this.start_play_ed();
//      Level.loadlevel(
      /* Load level of what was being edited */
    }
  },
  
  input_f7_pressed() {
    /* Start current level */
    if (!sim_playing()) {
      this.start_play_ed();
      Level.loadlevel("game.lvl");
    }
  },
  
  input_escape_pressed() {
    this.openpanel(quitpanel);
  },
  
  moveoffsets: [],

  input_g_pressed() {
    if (Keys.ctrl() && Keys.shift()) {
      Log.info("Saving as level...");
      this.openpanel(groupsaveaspanel);
      return;
    }

    if (Keys.alt() && this.cursor) {
      var com = find_com(this.selectlist);
      this.selectlist.forEach(function(x) {
        x.pos = x.pos.sub(com).add(this.cursor);
      },this);
    }
    
    if (this.sel_comp) {
      if ('pos' in this.sel_comp)
        this.moveoffset = this.sel_comp.pos.sub(screen2world(Mouse.pos));
	
      return;
    }
  
    if (Keys.ctrl()) {
      this.selectlist = this.dup_objects(this.selectlist);
    }
    
    this.selectlist.forEach(function(x,i) {
      this.moveoffsets[i] = x.pos.sub(screen2world(Mouse.pos));
    }, this);
  },
  
  input_g_down() {
    if (Keys.alt()) return;
    if (this.sel_comp) {
      this.sel_comp.pos = this.moveoffset.add(screen2world(Mouse.pos));
      return;
    }

    if (this.moveoffsets.length === 0) return;

    this.selectlist.forEach(function(x,i) {
      x.pos = this.moveoffsets[i].add(screen2world(Mouse.pos));
    }, this);
  },

  input_g_released() {
    this.moveoffsets = [];
  },
  
  scaleoffset: 0,
  startscales: [],
  selected_com: [0,0],
  
  openpanel(panel, dontsteal) {
    if (this.curpanel)
      this.curpanel.close();
      
    this.curpanel = panel;
    
    var stolen = this;
    if (dontsteal)
      stolen = null;

    this.curpanel.open(stolen);
  },

  curpanels: [],
  addpanel(panel) {
    this.curpanels.push(panel);
    panel.open();
  },

  cleanpanels(panel) {
    this.curpanels = this.curpanels.filter(function(x) { return x.on; });
  },
  
  input_s_pressed() {
    if (Keys.ctrl()) {
      if (Keys.shift()) {
        this.openpanel(saveaspanel);
	return;
      }

      if (this.edit_level.level) {
        if (!this.edit_level.unique)
	  this.save_current();

        this.selectlist = [];
	this.selectlist.push(this.edit_level);
        this.edit_level = this.edit_level.level;

	return;
      }
    
      this.save_current();
      return;
    }

    var offf = this.cursor ? this.cursor : this.selected_com;
    this.scaleoffset = Vector.length(Mouse.worldpos.sub(offf));

    if (this.sel_comp) {
      if (!('scale' in this.sel_comp)) return;
      this.startscales = [];
      this.startscales.push(this.sel_comp.scale);
      return;
    }
    
    this.selectlist.forEach(function(x, i) {
      this.startscales[i] = x.scale;
      if (this.cursor)
        this.startoffs[i] = x.pos.sub(this.cursor);
    }, this);
  },
  
  input_s_down() {
    if (!this.scaleoffset) return;
    var offf = this.cursor ? this.cursor : this.selected_com;
    var dist = Vector.length(screen2world(Mouse.pos).sub(offf));
    var scalediff = dist/this.scaleoffset;
    
    if (this.sel_comp) {
      if (!('scale' in this.sel_comp)) return;
      this.sel_comp.scale = this.startscales[0] * scalediff;
      return;
    }
    
    this.selectlist.forEach(function(x, i) {
      x.scale = this.startscales[i] * scalediff;
      if (this.cursor)
        x.pos = this.cursor.add(this.startoffs[i].scale(scalediff));
    }, this);
  },

  input_s_released() {
    this.scaleoffset = null;
  },
  
  startrots: [],
  startpos: [],
  startoffs: [],
  
  input_r_pressed() {
    this.startrots = [];
    
    if (Keys.ctrl()) {
      this.selectlist.forEach(function(x) {
        x.angle = -x.angle;
      });
      return;
    }
  
    if (this.sel_comp && 'angle' in this.sel_comp) {
      var relpos = screen2world(Mouse.pos).sub(this.sel_comp.gameobject.pos);
      this.startoffset = Math.atan2(relpos.y, relpos.x);
      this.startrot = this.sel_comp.angle;
      
      return;
    }

    var offf = this.cursor ? this.cursor : this.selected_com;
    var relpos = screen2world(Mouse.pos).sub(offf);
    this.startoffset = Math.atan2(relpos[1], relpos[0]);
    
    this.selectlist.forEach(function(x, i) {
      this.startrots[i] = x.angle;
      this.startpos[i] = x.pos;
      this.startoffs[i] = x.pos.sub(offf);
    }, this);
  },
  
  input_r_down() {
    if (this.sel_comp && 'angle' in this.sel_comp) {
      if (!('angle' in this.sel_comp)) return;
      var relpos = screen2world(Mouse.pos).sub(this.sel_comp.gameobject.pos);
      var anglediff = Math.rad2deg(Math.atan2(relpos.y, relpos.x)) - this.startoffset;
      this.sel_comp.angle = this.startrot + anglediff;
      return;
    }
    if (this.startrots.empty) return;

    var offf = this.cursor ? this.cursor : this.selected_com;
    var relpos = screen2world(Mouse.pos).sub(offf);
    var anglediff = Math.rad2deg(Math.atan2(relpos[1], relpos[0]) - this.startoffset);

    if (this.cursor) {
      this.selectlist.forEach(function(x, i) {
        x.angle = this.startrots[i] + anglediff;
        x.pos = offf.add(this.startoffs[i].rotate(Math.deg2rad(anglediff)));
      }, this);
    } else {
      this.selectlist.forEach(function(x,i) {
        x.angle = this.startrots[i]+anglediff;
      }, this);
    }
  },

  snapshots: [],
  curlvl: {}, /* What the level currently looks like on file */

  reset_undos() {
    this.snapshots = [];
    this.backshots = [];
  },

  check_snapshot() {
    if (!this.selectlist.empty || this.grabselect) this.snapshot();
  },

  snapshot() {
    var cur = this.edit_level.save();
    var dif = diff(cur, this.curlvl);
    
    if (dif.empty) return;

    this.snapshots.push(this.curlvl);
    this.backshots = [];
    this.curlvl = cur;
    return;
    
    this.snapshots.push(dif);

    this.backshots = [];
    this.backshots.push(diff(this.curlvl, cur));
    this.curlvl = cur;
    this.edit_level.check_dirty();
  },

  backshots: [], /* Redo snapshots */

  restore_lvl(lvl) {
    this.unselect();
    this.edit_level.clear();
    this.edit_level.load(lvl);
    this.edit_level.check_dirty();
  },

  diff_lvl(d) {
    this.unselect();
    for (var key in d) {
      if (d[key] === "DELETE")
        Game.objects[key].kill();
    }
    diffassign(Game.objects, d);
    Game.objects.forEach(function(x) { x.sync(); });
    this.curlvl = this.edit_level.save();
  },
  
  redo() {
    if (this.backshots.empty) {
      Log.info("Nothing to redo.");
      return;
    }

    this.snapshots.push(this.edit_level.save());
    var dd = this.backshots.pop();
    this.edit_level.clear();
    this.edit_level.load(dd);
    this.edit_level.check_dirty();
    this.curlvl = dd;
    return;

    var dd = this.backshots.pop();
      this.snapshots.push(dd);        
    if (this.was_undoing) {
      dd = this.backshots.pop();
      this.snapshots.push(dd);
      this.was_undoing = false;
    }

    this.diff_lvl(dd);
  },

  undo() {
    if (this.snapshots.empty) {
      Log.info("Nothing to undo.");
      return;
    }
    this.unselect();
    this.backshots.push(this.edit_level.save());
    var dd = this.snapshots.pop();
    this.edit_level.clear();
    this.edit_level.load(dd);
    this.edit_level.check_dirty();
    this.curlvl = dd;
    return;
    
    this.backshots.push(dd);
    
    if (!this.was_undoing) {
      dd = this.snapshots.pop();
      this.backshots.push(dd);
      this.was_undoing = true;  
    }

    this.diff_lvl(dd);  
  },
  
  restore_buffer() {
    this.restore_level(this.filesnap);
  },

  input_z_pressed() {
    if (Keys.ctrl()) {
      if (Keys.shift())
        this.redo();
      else
        this.undo();
    }
  },

  save_current() {
    if (!this.edit_level.file) {
      this.openpanel(saveaspanel);
      return;
    }

    var lvl = this.edit_level.save();

    Log.info("Saving level of size " + lvl.length + " bytes.");

    slurpwrite(lvl, this.edit_level.file);
    this.edit_level.filejson = lvl;
    this.edit_level.check_dirty();

    if (this.edit_level.script) {
      var scriptfile = this.edit_level.file.replace('.lvl', '.js');
      slurpwrite(this.edit_level.script, scriptfile);
    }

    Level.sync_file(this.edit_level.file);
  },
  
  input_t_pressed() {
    if (Keys.ctrl()) {
      if (Keys.shift()) {
        if (this.selectlist.length !== 1) return;
        this.openpanel(savetypeas);
        return;
      }
    }

    this.selectlist.forEach(function(x) { x.selectable = false; });

    if (Keys.alt())
      this.edit_level.objects.forEach(function(x) { x.selectable = true; });
  },

  clear_level() {
    Log.info("Closed level.");

    if (this.edit_level.level) {
      this.openpanel(gen_notify("Can't close a nested level. Save up to the root before continuing."));
      return;
    }
    
    this.unselect();
    this.edit_level.kill();
    this.edit_level = Level.create();
  },

  input_n_pressed() {
    if (!Keys.ctrl()) return;
    
    if (this.edit_level.dirty) {
      Log.info("Level has changed; save before starting a new one.");
      this.openpanel(gen_notify("Level is changed. Are you sure you want to close it?", _ => this.clear_level()));
      return;
    }
    
    this.clear_level();    
  },

  set timescale(x) { cmd(3, x); },
  set updateMS(x) { cmd(6, x); },
  set physMS(x) { cmd(7, x); },
  set renderMS(x) { cmd(5, x); },
  set dbg_draw_phys(x) { cmd(4, x); },
  set dbg_color(x) { cmd(16, x); },
  set trigger_color(x) { cmd(17, x); },
  get fps() { return sys_cmd(8); },
  input_f10_pressed() { this.timescale = 0.1; },
  input_f10_released() { this.timescale = 1.0; },

  input_1_pressed() {
    if (!Keys.alt()) return;
    Render.normal();
  },

  input_2_pressed() {
    if (!Keys.alt()) return;
    Render.wireframe();
  },

  _sel_comp: null,
  get sel_comp() { return this._sel_comp; },
  set sel_comp(x) {
    if (this._sel_comp)
      unset_pawn(this._sel_comp);
    
    this._sel_comp = x;

    if (this._sel_comp) {
      Log.info("sel comp is now " + this._sel_comp);
      set_pawn(this._sel_comp);
    }
  },

  input_tab_pressed() {
    if (!this.selectlist.length === 1) return;
    if (!this.selectlist[0].components) return;

    var sel = this.selectlist[0].components;

    if (!this.sel_comp)
      this.sel_comp = sel.nth(0);
    else {
      var idx = sel.findIndex(this.sel_comp) + 1;
      if (idx >= Object.keys(sel).length)
        this.sel_comp = null;
      else
        this.sel_comp = sel.nth(idx);
    }
    
  },

  draw_gizmos: true,

  input_f4_pressed() { this.draw_gizmos = !this.draw_gizmos; },
  time: 0,

  ed_gui() {

    this.time = Date.now();
    /* Clean out killed objects */
    this.selectlist = this.selectlist.filter(function(x) { return x.alive; });

    GUI.text("WORKING LAYER: " + this.working_layer, [0,520], 1);
    GUI.text("MODE: " + this.edit_mode, [0,500],1);

    Debug.point(world2screen(this.edit_level.pos), 5, Color.yellow);
    if (this.cursor) {
      Debug.point(world2screen(this.cursor), 5, Color.green);

      this.selectlist.forEach(function(x) {
        var p = [];
	p.push(world2screen(this.cursor));
	p.push(world2screen(x.pos));
        Debug.line(p, Color.green);
      },this);
    }

    if (this.programmode) {
      this.edit_level.objects.forEach(function(x) {
        if (x.hasOwn('varname')) GUI.text(x.varname, world2screen(x.pos).add([0,32]), 1, [84,110,255]);
      });
    }

    if (this.comp_info && this.sel_comp && 'help' in this.sel_comp) {
      GUI.text(this.sel_comp.help, [100,700],1);
    }

    gui_text("0,0", world2screen([0,0]), 1);
    
    if (this.draw_gizmos)
      Game.objects.forEach(function(x) {
        if (!x.icon) return;
        gui_img(x.icon, world2screen(x.pos));
      });

    gui_text(sim_playing() ? "PLAYING"
                         : sim_paused() ?
			 "PAUSED" :
			 "STOPPED", [0, 0], 1);

    var clvl = this.edit_level;
    var ypos = 200;
    var lvlcolor = Color.white;
    while (clvl) {
      var lvlstr = clvl.file ? clvl.file : "NEW LEVEL";
      if (clvl.unique)
        lvlstr += "#";
      else if (clvl.dirty)
        lvlstr += "*";
      GUI.text(lvlstr, [0, ypos], 1, lvlcolor);

      lvlcolor = Color.gray;

      clvl = clvl.level;
      if (clvl) {
        GUI.text("^^^^^^", [0,ypos+5],1);
	ypos += 5;
      }
      ypos += 15;
    }
    
    this.edit_level.objects.forEach(function(x) {
      if ('ed_gizmo' in x)
        x.ed_gizmo();
    });
    
    if (Debug.phys_drawing)
      this.edit_level.objects.forEach(function(x) {
        Debug.point(world2screen(x.pos), 2, Color.teal);
      });

    this.selectlist.forEach(function(x) {
      var color = x.color ? x.color : [255,255,255];
      GUI.text(x.toString(), world2screen(x.pos).add([0, 16]), 1, color);
      GUI.text(x.pos.map(function(x) { return Math.round(x); }), world2screen(x.pos), 1, color);
      Debug.arrow(world2screen(x.pos), world2screen(x.pos.add(x.up.scale(40))), Color.yellow, 1);
      if (x.hasOwn('varname')) GUI.text(x.varname, world2screen(x.pos).add([0,32]), 1, [84,110,255]);
      if ('gizmo' in x && typeof x['gizmo'] === 'function' )
        x.gizmo();
      
      if (x.hasOwn('file')) 
        x.objects.forEach(function(x) {
	  GUI.text(x.toString(), world2screen(x.pos).add([0,16]),1,Color.blue);
	});
    });


    if (this.selectlist.length === 1) {
      var i = 1;
      for (var key in this.selectlist[0].components) {
        var selected = this.sel_comp === this.selectlist[0].components[key];
        var str = (selected ? ">" : " ") + key + " [" + this.selectlist[0].components[key].name + "]";
        gui_text(str, world2screen(this.selectlist[0].pos).add([0,-16*(i++)]), 1);
      }

      if (this.sel_comp) {
        if ('gizmo' in this.sel_comp) this.sel_comp.gizmo();
      }
    }

    Game.objects.forEach(function(obj) {
      if (!obj.selectable)
        gui_img("icons/icons8-lock-16.png", world2screen(obj.pos));
    });

    Debug.draw_grid(1, editor_config.grid_size/editor_camera.zoom);
    
    var startgrid = screen2world([-20,Window.height]).map(function(x) { return Math.snap(x, editor_config.grid_size); }, this);
    var endgrid = screen2world([Window.width, 0]);
    
    while(startgrid[0] <= endgrid[0]) {
      gui_text(startgrid[0], [world2screen([startgrid[0], 0])[0], 1], 1);
      startgrid[0] += editor_config.grid_size;
    }

    while(startgrid[1] <= endgrid[1]) {
      gui_text(startgrid[1], [0, world2screen([0, startgrid[1]])[1]], 1);
      startgrid[1] += editor_config.grid_size;
    }
    
    Debug.point(world2screen(this.selected_com), 3);
    this.selected_com = find_com(this.selectlist);
    
    if (this.draw_bb) {
      Game.objects.forEach(function(x) { bb_draw(x.boundingbox); });
    }

    /* Draw selection box */
    if (this.sel_start) {
      var endpos = screen2world(Mouse.pos);
      var c = [];
      c[0] = (endpos[0] - this.sel_start[0]) / 2;
      c[0] += this.sel_start[0];
      c[1] = (endpos[1] - this.sel_start[1]) / 2;
      c[1] += this.sel_start[1];
      var wh = [];
      wh[0] = Math.abs(endpos[0] - this.sel_start[0]);
      wh[1] = Math.abs(endpos[1] - this.sel_start[1]);
      wh[0] /= editor.camera.zoom;
      wh[1] /= editor.camera.zoom;
      var bb = cwh2bb(world2screen(c),wh);
      Debug.boundingbox(bb, [255,255,255,10]);
      Debug.line(bb2points(bb).wrapped(1), Color.white);
    }
    
    if (this.curpanel && this.curpanel.on)
      this.curpanel.gui();

    this.curpanels.forEach(function(x) {
      if (x.on) x.gui();
    });

    if (this.repl) {
      Nuke.window("repl");
      Nuke.newrow(500);
      var log = cmd(84);
      var f = log.prev('\n', 0, 10);
      Nuke.scrolltext(log.slice(f));

      Nuke.end();
    }
    
  },

  input_lbracket_pressed() {
    if (!Keys.ctrl()) return;
    editor_config.grid_size -= Keys.shift() ? 10 : 1;
    if (editor_config.grid_size <= 0) editor_config.grid_size = 1;
  },
  input_lbracket_rep() { this.input_lbracket_pressed(); },

  input_rbracket_pressed() {
    if (!Keys.ctrl()) return;
    editor_config.grid_size += Keys.shift() ? 10 : 1;
  },

  input_rbracket_rep() { this.input_rbracket_pressed(); },

  grid_size: 100,

  ed_debug() {
    if (!Debug.phys_drawing)
      this.selectlist.forEach(function(x) { Debug.draw_obj_phys(x); });
  },
  
  viewasset(path) {
    Log.info(path);
    var fn = function(x) { return path.endsWith(x); };
    if (images.any(fn)) {
      var newtex = copy(texgui, { path: path });
      this.addpanel(newtex);
    }
    else if (sounds.any(fn))
      Log.info("selected a sound");
  },

  killring: [],
  killcom: [],

  input_c_pressed() {
    if (this.sel_comp) return;
    if (!Keys.ctrl()) return;
    this.killring = [];
    this.killcom = [];

    this.selectlist.forEach(function(x) {
      this.killring.push(x);
    },this);

    this.killcom = find_com(this.killring);
  },

  input_x_pressed() {
    if (!Keys.ctrl()) return;

    this.input_c_pressed();
    this.killring.forEach(function(x) { x.kill(); });
  },

  paste() {
    this.selectlist = this.dup_objects(this.killring);

    var setpos = this.cursor ? this.cursor : Mouse.worldpos;
    this.selectlist.forEach(function(x) {
      x.pos = x.pos.sub(this.killcom).add(setpos);
    },this);
  },

  input_v_pressed() {
    if (!Keys.ctrl()) return;
    this.paste();
  },
  
  input_e_pressed() {
    if (Keys.ctrl()) {
      this.openpanel(assetexplorer);
      return;
    }
  },

  lvl_history: [],

  load(file) {
    if (this.edit_level) this.lvl_history.push(this.edit_level.file);
    this.edit_level.clear();
    this.edit_level = Level.loadfile(file);
    this.curlvl = this.edit_level.save();
    this.unselect();
  },

  load_prev() {
    if (this.lvl_history.length === 0) return;

    var file = this.lvl_history.pop();
    this.edit_level = Level.loadfile(file);
    this.unselect();
  },

  addlevel(file, pos) {
    Log.info("adding file " + file + " at pos " + pos);
    var newlvl = this.edit_level.addfile(file);
    newlvl.pos = pos;
    this.selectlist = [];
    this.selectlist.push(newlvl);
    return;
  },

  load_json(json) {
    this.edit_level.load(json);
    this.curlvl = this.edit_level.save();
    this.unselect();
  },
  
  groupsaveas(group, file) {
    if (!file) return;
    
    file = file+".lvl";
    if (IO.exists(file)) {
      this.openpanel(gen_notify("Level already exists with that name. Overwrite?", dosave.bind(this,file)));
      return;
    } else
      dosave(file);
      
    function dosave(file) {
      var com = find_com(group);
      Level.saveas(group, file);
      editor.addlevel(file, com);

      group.forEach(function(x) { x.kill(); });
    }
  },
  
  saveas_check(file) {
    if (!file) return;
    
    if (!file.endsWith(".lvl"))
      file = file + ".lvl";
    
    if (IO.exists(file)) {
      notifypanel.action = editor.saveas;
      this.openpanel(gen_notify("Level already exists with that name. Overwrite?", this.saveas.bind(this, file)));
    } else
      this.saveas(file);
  },

  saveas(file) {
    if (!file) return;
    
    Log.info("made it");
    
    this.edit_level.file = file;
    this.save_current();
  },
  
  input_h_pressed() {
    if (this.sel_comp) return;
    if (Keys.ctrl()) {
      Game.objects.forEach(function(x) { x.visible = true; });
    } else {
      var visible = true;
      this.selectlist.forEach(function(x) { if (x.visible) visible = false; });
      this.selectlist.forEach(function(x) { x.visible = visible; });
    }
  },

  input_a_pressed() {
    if (Keys.alt()) {
      this.openpanel(prefabpanel);
      return;
    }
    if (!Keys.ctrl()) return;
    if (!this.selectlist.empty) {
      this.unselect();
      return;
    }
    this.unselect();

    this.selectlist = this.edit_level.objects.slice();
  },

  repl: false,
  input_backtick_pressed() { this.repl = !this.repl; },
  
  draw_bb: false,
  input_f3_pressed() {
    this.draw_bb = !this.draw_bb;
  },
}

var inputpanel = {
  title: "untitled",
  value: "",
  on: false,
  stolen: {},
  
  gui() {
    Nuke.window(this.title);
    Nuke.newline();
    this.guibody();

    if (Nuke.button("close"))
      this.close();
      
    Nuke.end();
    return false;
  },
  
  guibody() {
    this.value = Nuke.textbox(this.value);
    
    Nuke.newline(2);
    if (Nuke.button("submit")) {
      this.submit();
      return true;
    }
  },
  
  open(steal) {
    this.on = true;
    this.value = "";
    if (steal) {
      this.stolen = steal;
      unset_pawn(this.stolen);
      set_pawn(this);
    }
    this.start();
    this.keycb();
  },
  
  start() {},

  
  close() {
    unset_pawn(this);
    if (this.stolen) {
      set_pawn(this.stolen);
      this.stolen = null;
    }

    this.on = false;
    if ('on_close' in this)
      this.on_close();
  },

  action() {

  },

  closeonsubmit: true,
  submit() {
    if (!this.submit_check()) return;
    this.action();
    if (this.closeonsubmit)
      this.close();
  },

  submit_check() { return true; },
  
  input_enter_pressed() {
    this.submit();
  },

  input_text(char) {
    this.value += char;
    this.keycb();
  },
  
  keycb() {},
  
  input_backspace_pressrep() {
    this.value = this.value.slice(0,-1);
    this.keycb();
  },
  
  input_escape_pressed() {
    this.close();
  },
};

function proto_count_lvls(name)
{
  if (!this.occs) this.occs = {};
  if (name in this.occs) return this.occs[name];
  var lvls = IO.extensions("lvl");
  var occs = {};
  var total = 0;
  lvls.forEach(function(lvl) {
    var json = JSON.parse(IO.slurp(lvl));
    var count = 0;
    json.forEach(function(x) { if (x.from === name) count++; });
    occs[lvl] = count;
    total += count;
  });

  this.occs[name] = occs;
  this.occs[name].total = total;

  return this.occs[name];
}
proto_count_lvls = proto_count_lvls.bind(proto_count_lvls);

function proto_used(name) {
  var occs = proto_count_lvls(name);
  var used = false;
  occs.forEach(function(x) { if (x > 0) used = true; });

  Log.info(used);
}

function proto_total_use(name) {
  return proto_count_lvls(name).total;
}

function proto_children(name) {
  var children = [];

  for (var key in gameobjects)
    if (gameobjects[key].from === name) children.push(gameobjects[key]);

  return children;
}

var texteditor = clone(inputpanel, {
  title: "text editor",
  _cursor:0, /* Text cursor: [char,line] */
  get cursor() { return this._cursor; },
  set cursor(x) {
    if (x > this.value.length)
      x = this.value.length;
    if (x < 0)
      x = 0;
      
    this._cursor = x;
    this.line = this.get_line();
  },

  submit() {},

  line: 0,
  killring: [],
  undos: [],
  startbuffer: "",

  savestate() {
    this.undos.push(this.value.slice());
  },

  popstate() {
    if (this.undos.length === 0) return;
    this.value = this.undos.pop();
    this.cursor = this.cursor;
  },

  input_s_pressed() {
    if (!Keys.ctrl()) return;
    Log.info("SAVING");
    editor.save_current();
  },

  input_u_pressrep() {
    if (!Keys.ctrl()) return;
    this.popstate();
  },

  copy(start, end) {
    return this.value.slice(start,end);
  },

  input_q_pressed() {
    if (!Keys.ctrl()) return;

    var ws = this.prev_word(this.cursor);
    var we = this.end_of_word(this.cursor)+1;
    var find = this.copy(ws, we);
    var obj = editor.edit_level.varname2obj(find);

    if (obj) {
      editor.unselect();
      editor.selectlist.push(obj);
    }
  },

  delete_line(p) {
    var ls = this.line_start(p);
    var le = this.line_end(p)+1;
    this.cut_span(ls,le);
    this.to_line_start();
  },

  line_blank(p) {
    var ls = this.line_start(p);
    var le = this.line_end(p);
    var line =  this.value.slice(ls, le);
    if (line.search(/[^\s]/g) === -1)
      return true;
    else
      return false;
  },

  input_o_pressed() {
    if (Keys.alt()) {
      /* Delete all surrounding blank lines */
      while (this.line_blank(this.next_line(this.cursor)))
        this.delete_line(this.next_line(this.cursor));

      while (this.line_blank(this.prev_line(this.cursor)))
        this.delete_line(this.prev_line(this.cursor));
    } else if (Keys.ctrl()) {
      this.insert_char('\n');
      this.cursor--;
    }
  },

  get_line() {
    var line = 0;
    for (var i = 0; i < this.cursor; i++)
      if (this.value[i] === "\n")
        line++;

    return line;
  },

  start() {
    this.cursor = 0;
    this.startbuffer = this.value.slice();
  },

  get dirty() {
    return this.startbuffer !== this.value;
  },

  gui() {
    GUI.text_cursor(this.value, [100,700],1,this.cursor+1);
    GUI.text("C" + this.cursor + ":::L" + this.line + ":::" + (this.dirty ? "DIRTY" : "CLEAN"), [100,100], 1);
  },

  insert_char(char) {
    this.value = this.value.slice(0,this.cursor) + char + this.value.slice(this.cursor);
    this.cursor++;
  },

  input_enter_pressrep() {
    var white = this.line_starting_whitespace(this.cursor);
    this.insert_char('\n');

    for (var i = 0; i < white; i++)
      this.insert_char(" ");
      
  },

  input_text(char) {
    if (Keys.ctrl() || Keys.alt()) return;
    this.insert_char(char);
    this.keycb();
  },

  input_d_pressrep() {
    if (Keys.ctrl())
      this.value = this.value.slice(0,this.cursor) + this.value.slice(this.cursor+1);
    else if (Keys.alt())
      this.cut_span(this.cursor, this.end_of_word(this.cursor)+1);
  },

  input_backspace_pressrep() {
    this.value = this.value.slice(0,this.cursor-1) + this.value.slice(this.cursor);
    this.cursor--;
  },

  input_a_pressed() {
    if (Keys.ctrl()) {
      this.to_line_start();
      this.desired_inset = this.inset;
    }
  },

  input_y_pressed() {
    if (!Keys.ctrl()) return;
    if (this.killring.length === 0) return;
    this.insert_char(this.killring.pop());
  },

  line_starting_whitespace(p) {
    var white = 0;
    var l = this.line_start(p);

    while (this.value[l] === " ") {
      white++;
      l++;
    }

    return white;
  },

  input_e_pressed() {
    if (Keys.ctrl()) {
      this.to_line_end();
      this.desired_inset = this.inset;
    }
  },

  input_k_pressrep() {
    if (Keys.ctrl()) {
      if (this.cursor === this.value.length-1) return;
      var killamt = this.value.next('\n', this.cursor) - this.cursor;
      var killed = this.cut_span(this.cursor-1, this.cursor+killamt);
      this.killring.push(killed);
    } else if (Keys.alt()) {
      var prevn = this.value.prev('\n', this.cursor);
      var killamt = this.cursor - prevn;
      var killed = this.cut_span(prevn+1, prevn+killamt);
      this.killring.push(killed);
      this.to_line_start();
    }
  },

  input_b_pressrep() {
    if (Keys.ctrl()) {
      this.cursor--;
    } else if (Keys.alt()) {
      this.cursor = this.prev_word(this.cursor-2);
    }

    this.desired_inset = this.inset;
  },

  input_f_pressrep() {
    if (Keys.ctrl()) {
      this.cursor++;
    } else if (Keys.alt()) {
      this.cursor = this.next_word(this.cursor);
    }

    this.desired_inset = this.inset;
  },

  cut_span(start, end) {
    if (end < start) return;
    this.savestate();
    var ret = this.value.slice(start,end);
    this.value = this.value.slice(0,start) + this.value.slice(end);
    if (start > this.cursor)
      return ret;
      
    this.cursor -= ret.length;
    return ret;
  },

  next_word(pos) {
    var v = this.value.slice(pos+1).search(/[^\w]\w/g);
    if (v === -1) return pos;
    return pos + v + 2;
  },

  prev_word(pos) {
    while (this.value.slice(pos,pos+2).search(/[^\w]\w/g) === -1 && pos > 0)
      pos--;

    return pos+1;
  },

  end_of_word(pos) {
    var l = this.value.slice(pos).search(/\w[^\w]/g);
    return l+pos;
  },

  get inset() {
    return this.cursor - this.value.prev('\n', this.cursor) - 1;
  },

  line_start(p) {
    return this.value.prev('\n', p)+1;
  },

  line_end(p) {
    return this.value.next('\n', p);
  },

  next_line(p) {
    return this.value.next('\n',p)+1;
  },

  prev_line(p) {
    return this.line_start(this.value.prev('\n', p));
  },

  to_line_start() {
    this.cursor = this.value.prev('\n', this.cursor)+1;
  },

  to_line_end() {
    var p = this.value.next('\n', this.cursor);
    if (p === -1)
      this.to_file_end();
    else
      this.cursor = p;
  },

  line_width(pos) {
    var start = this.line_start(pos);
    var end = this.line_end(pos);
    if (end === -1)
      end = this.value.length;

    return end-start;
  },

  to_file_end() { this.cursor = this.value.length; },

  to_file_start() { this.cursor = 0; },

  desired_inset: 0,

  input_p_pressrep() {
    if (Keys.ctrl()) {
      if (this.cursor === 0) return;
      this.desired_inset = Math.max(this.desired_inset, this.inset);
      this.cursor = this.prev_line(this.cursor);
      var newlinew = this.line_width(this.cursor);
      this.cursor += Math.min(this.desired_inset, newlinew);
    } else if (Keys.alt()) {
      while (this.line_blank(this.cursor))
        this.cursor = this.prev_line(this.cursor);

      while (!this.line_blank(this.cursor))
        this.cursor = this.prev_line(this.cursor);
    }
  },

  input_n_pressrep() {
    if (Keys.ctrl()) {
      if (this.cursor === this.value.length-1) return;
      if (this.value.next('\n', this.cursor) === -1) {
        this.to_file_end();
        return;
      }

      this.desired_inset = Math.max(this.desired_inset, this.inset);
      this.cursor = this.next_line(this.cursor);
      var newlinew = this.line_width(this.cursor);
      this.cursor += Math.min(this.desired_inset, newlinew);
    } else if (Keys.alt()) {
      while (this.line_blank(this.cursor))
        this.cursor = this.next_line(this.cursor);

      while (!this.line_blank(this.cursor))
        this.cursor = this.next_line(this.cursor);
    }
  },

});

var protoexplorer = copy(inputpanel, {
  title: "prototype explorer",
  waitclose:false,
  
  guibody() {
    var treeleaf = function(name) {
      for (var key in gameobjects) {
        if (gameobjects[key].from === name) {
	  var goname = gameobjects[key].name;
          if (Nuke.tree(goname)) {
	    var lvluses = proto_total_use(goname);
	    var childcount = proto_children(goname).length;
	    if (lvluses === 0 && childcount === 0 && gameobjects[key].from !== 'gameobject') {
              if (Nuke.button("delete")) {
	        Log.info("Deleting the prototype " + goname);
		delete gameobjects[goname];
		editor.save_prototypes();
	      }
	    } else {
	      if (Nuke.tree("Occurs " + lvluses + " times in levels.")) {
	        var occs = proto_count_lvls(goname);
                for (var lvl in occs) {
		  if (occs[lvl] === 0) continue;
		  Nuke.label(lvl + ": " + occs[lvl]);
		}

		Nuke.tree_pop();
	      }
   	      Nuke.label("Has " + proto_children(goname).length + " descendents.");

	      if (Nuke.button("spawn")) {
	        var proto = gameobjects[goname];
	        var obj = this.edit_level.spawn(proto);
                obj.pos = Mouse.worldpos;
                editor.unselect();
                editor.selectlist.push(obj);
		this.waitclose = true;
	      }

	      if (Nuke.button("view")) {
	        objectexplorer.obj = gameobjects[goname];
                editor.openpanel(objectexplorer);
	      }
            }
	    
            treeleaf(goname);
	    Nuke.tree_pop();
	  }
	}
      }
    }

    treeleaf = treeleaf.bind(this);
    
    if (Nuke.tree("gameobject")) {
      treeleaf("gameobject");
      Nuke.tree_pop();
    }

    if (this.waitclose) {
      this.waitclose = false;
      this.close();
    }
  },
});

var objectexplorer = copy(inputpanel, {
  title: "object explorer",
  obj: null,
  previous: [],
  start() {
    this.previous = [];
    Input.setnuke();
  },

  on_close() { Input.setgame(); },

  input_enter_pressed() {},

  goto_obj(obj) {
    if (obj === this.obj) return;
    this.previous.push(this.obj);
    this.obj = obj;
  },

  input_lmouse_pressed() {
    Mouse.disabled();
  },

  input_lmouse_released() {
    Mouse.normal();
  },
  
  guibody() {
    Nuke.label("Examining " + this.obj);
      
    var n = 0;
    var curobj = this.obj;
    while (curobj) {
      n++;
      curobj = curobj.__proto__;
    }

    n--;
    Nuke.newline(n);
    curobj = this.obj.__proto__;
    while (curobj) {
      if (Nuke.button(curobj.toString()))
        this.goto_obj(curobj);

      curobj = curobj.__proto__;
    }

    Nuke.newline(2);

    if (this.previous.empty)
      Nuke.label("");
    else {
      if (Nuke.button("prev: " + this.previous.last))
        this.obj = this.previous.pop();
    }

    Object.getOwnPropertyNames(this.obj).forEach(key => {
      var descriptor = Object.getOwnPropertyDescriptor(this.obj, key);
      if (!descriptor) return;
      var hidden = !descriptor.enumerable;
      var writable = descriptor.writable;
      var configurable = descriptor.configurable;

      if (!descriptor.configurable) return;
      if (hidden) return;
      
      var name = (hidden ? "[hidden] " : "") + key;
      var val = this.obj[key];

      var nuke_str = key + "_nuke";
      if (nuke_str in this.obj) {
        this.obj[nuke_str]();
	if (key in this.obj.__proto__) {
	  if (Nuke.button("delete " + key)) {
	     if (("_" + key) in this.obj)
	       delete this.obj["_"+key];
	     else
	       delete this.obj[key];
	  }
	}
      } else
      switch (typeof val) {
        case 'object':
          if (val) {
	    if (Array.isArray(val)) {
	      this.obj[key] = Nuke.pprop(key,val);
	      break;
	    }
	      
	    Nuke.newline(2);
	    Nuke.label(name);
            if (Nuke.button(val.toString())) this.goto_obj(val);
	  } else {
    	      this.obj[key] = Nuke.pprop(key,val);
	  }
	  break;

	case 'function':
	  Nuke.newline(2);
	  Nuke.label(name);
	  Nuke.label("function");
	  break;

	default:
	  if (!hidden) {// && Object.getOwnPropertyDescriptor(this.obj, key).writable) {
	    if (key.startsWith('_')) key = key.slice(1);

	    this.obj[key] = Nuke.pprop(key, this.obj[key]);

	    if (key in this.obj.__proto__) {
	      if (Nuke.button("delete " + key)) {
	        if ("_"+key in this.obj)
		  delete this.obj["_"+key];
		else
             	  delete this.obj[key];
              }
	    }
	  }
	  else {
	    Nuke.newline(2);
	    Nuke.label(name);
	    Nuke.label(val.toString());
	  }
	  break;
      }
    });

    Nuke.newline();
    Nuke.label("Properties that can be pulled in ...");
    Nuke.newline(3);
    var pullprops = [];
    for (var key in this.obj.__proto__) {
      if (key.startsWith('_')) key = key.slice(1);
      if (!this.obj.hasOwn(key)) {
        if (typeof this.obj[key] === 'object' || typeof this.obj[key] === 'function') continue;
	pullprops.push(key);
      }
    }

    pullprops = pullprops.sort();

    pullprops.forEach(function(key) {
      if (Nuke.button(key))
        this.obj[key] = this.obj[key];
    }, this);

    Game.objects.forEach(function(x) { x.sync(); });

    Nuke.newline();
  },


});

var helppanel = copy(inputpanel, {
  title: "help",
  
  start() {
    this.helptext = slurp("editor.adoc");
  },
  
  guibody() {
    Nuke.label(this.helptext);
  },
});

var openlevelpanel = copy(inputpanel,  {
  title: "open level",
  action() {
    editor.load(this.value);
  },
  
  assets: [],
  allassets: [],
  extensions: ["lvl"],

  submit_check() {
    if (this.assets.length === 1) {
      this.value = this.assets[0];
      return true;
    } else {
      return this.assets.includes(this.value);
    }
  },
  
  start() {
    this.allassets = [];
    
    if (this.allassets.empty) {
      this.extensions.forEach(function(x) {
        this.allassets.push(cmd(66, "." + x));
      }, this);
    }
    this.allassets = this.allassets.flat().sort();
    this.assets = this.allassets.slice();
  },
  
  keycb() {
    this.assets = this.allassets.filter(x => x.search(this.value) !== -1);
  },
  
  input_tab_pressed() {
    this.value = tab_complete(this.value, this.assets);
  },
  
  guibody() {
    this.value = Nuke.textbox(this.value);
    
    this.assets.forEach(function(x) {
      if (Nuke.button(x)) {
        this.value = x;
	this.submit();
      }
    }, this);

    Nuke.newline(2);
    
    if (Nuke.button("submit")) {
      this.submit();
    }
  },
});

var addlevelpanel = copy(openlevelpanel, {
  title: "add level",
  action() { editor.addlevel(this.value, Mouse.worldpos); },
});

var saveaspanel = copy(inputpanel, {
  title: "save level as",
  action() {
    editor.saveas_check(this.value);
  },
});

var groupsaveaspanel = copy(inputpanel, {
  title: "group save as",
  action() { editor.groupsaveas(editor.selectlist, this.value); }
});

var saveprototypeas = copy(inputpanel, {
  title: "save prototype as",
  action() {
    editor.save_proto_as(this.value);
  },
});

var savetypeas = copy(inputpanel, {
  title: "save type as",
  action() {
    editor.save_type_as(this.value);
  },
});

var quitpanel = copy(inputpanel, {
  title: "really quit?",
  action() {
    quit();
  },
  
  guibody () {
    Nuke.label("Really quit?");
    Nuke.newline(2);
    if (Nuke.button("yes"))
      this.submit();
  },
});

var notifypanel = copy(inputpanel, {
  title: "notification",
  msg: "Refusing to save. File already exists.",
  action() {
    this.close();
  },
  
  guibody() {
    Nuke.label(this.msg);
    Nuke.newline(2);
    if (Nuke.button("OK")) {
      if ('yes' in this)
        this.yes();
      this.close();
    }
  },

  input_n_pressed() {
    this.close();
  },
});

var gen_notify = function(val, fn) {
  var panel = Object.create(notifypanel);
  panel.msg = val;
  panel.yes = fn;
  panel.input_y_pressed = function() { panel.yes(); panel.close(); };
  panel.input_enter_pressed = function() { panel.close(); };
  return panel;
};

var scripts = ["js"];
var images = ["png", "jpg", "jpeg"];
var sounds = ["wav", "mp3"];
var allfiles = [];
allfiles.push(scripts, images, sounds);
allfiles = allfiles.flat();

var assetexplorer = copy(openlevelpanel, {
  title: "asset explorer",
  extensions: allfiles,
  closeonsubmit: false,
  allassets:[],
  action() {
    if (editor.sel_comp && 'asset' in editor.sel_comp)
      editor.sel_comp.asset = this.value;
    else
      editor.viewasset(this.value);
  },
});

function tab_complete(val, list) {
    var check = list.filter(function(x) { return x.startsWith(val); }, this);
    if (check.length === 1) {
      list = check;
      return check[0];
    }
    
    var ret = null;
    var i = val.length;
    while (!ret && !check.empty) {

      var char = check[0][i];
      if (!check.every(function(x) { return x[i] === char; }))
        ret = check[0].slice(0, i);
      else {
        i++;
	check = check.filter(function(x) { return x.length-1 > i; });
      }
    }

    if (!ret) return val;

    list = check;
    return ret;
}

var texgui = clone(inputpanel, {
  get path() { return this._path; },
  set path(x) {
    this._path = x;
    this.title = "texture " + x;
  },
  
  guibody() {
    Nuke.label("texture");
    Nuke.img(this.path);
  },
});

var levellistpanel = copy(inputpanel, {
  title: "Level list",
  level: {},
  start() {
    this.level = editor.edit_level;
  },
  
  guibody() {
    Nuke.newline(4);
    Nuke.label("Object");
    Nuke.label("Visible");
    Nuke.label("Selectable");
    Nuke.label("Selected?");
    this.level.objects.forEach(function(x) {
      if (Nuke.button(x.toString())) {
        editor.selectlist = [];
	editor.selectlist.push(x);
      }
      
      x.visible = Nuke.checkbox(x.visible);
      x.selectable = Nuke.checkbox(x.selectable);
      
      if (editor.selectlist.includes(x)) Nuke.label("T"); else Nuke.label("F");
    });
  },
});

var prefabpanel = copy(openlevelpanel, {
  title: "prefabs",
  allassets: Object.keys(gameobjects),
  start() {
    this.allassets = Object.keys(gameobjects).sort();
    this.assets = this.allassets.slice();
  },
  action() {
    var obj = editor.edit_level.spawn(gameobjects[this.value]);
    obj.pos = Mouse.worldpos;
    editor.unselect();
    editor.selectlist.push(obj);
  },
});

var limited_editor = {
  input_f1_pressed() { editor.input_f1_pressed(); },

  input_f5_pressed() {
    /* Pause, and resume editor */
  },
  input_f7_pressed() {
    if (sim_playing()) {
      sim_stop();
      game.stop();
      Sound.killall();
      unset_pawn(limited_editor);
      set_pawn(editor);
      register_gui(editor.ed_gui, editor);
      Debug.register_call(editor.ed_debug, editor);
      Level.kill();
      Level.clear_all();
      editor.load_json(editor.stash);
      set_cam(editor_camera.body);
    }
  },
  input_f8_pressed() { sim_step(); },  
  input_f10_pressed() { editor.input_f10_pressed(); },
  input_f10_released() { editor.input_f10_released(); },
};

set_pawn(editor);
Log.warn(`Total pawn count is ${Player.players[0].pawns.length}`);
register_gui(editor.ed_gui, editor);
Debug.register_call(editor.ed_debug, editor);

if (IO.exists("editor.config"))
  load_configs("editor.config");
editor.edit_level = Level.create();
