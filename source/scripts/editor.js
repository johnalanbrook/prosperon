/*
  Editor-only variables on objects
  selectable
*/
prototypes.generate_ur('.');

/* This is the editor level & camera - NOT the currently edited level, but a level to hold editor things */
var editor_level = Primum.spawn(ur.arena);
var editor_camera = Game.camera;

var editor_config = {
  grid_size: 100,
  grid_color: [99, 255, 128, 100],
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
  selected: undefined,
  selectlist: [],
  moveoffset: [0,0],
  startrot: 0,
  rotoffset: 0,
  camera: editor_camera,
  edit_level: {}, /* The current level that is being edited */
  working_layer: 0,
  cursor: undefined,
  edit_mode: "basic",

  try_select() { /* nullify true if it should set selected to null if it doesn't find an object */
    var go = physics.pos_query(screen2world(Mouse.pos));
    return this.do_select(go);
  },
    
  /* Tries to select id */
  do_select(go) {
    var obj = go >= 0 ? Game.object(go) : undefined;
    if (!obj || !obj.selectable) return undefined;
    return obj;

    if (this.working_layer > -1 && obj.draw_layer !== this.working_layer) return undefined;

    if (obj.level !== this.edit_level) {
      var testlevel = obj.level;
      while (testlevel && testlevel.level !== this.edit_level) {
         testlevel = testlevel.level;
      }
      
      return testlevel;
    }

    return obj;
  },
  
  curpanel: undefined,

  check_level_nested() {
    if (this.edit_level.level) {
      this.openpanel(gen_notify("Can't close a nested level. Save up to the root before continuing."));
      return true;
    }

    return false;
  },

  programmode: false,

  delete_empty_reviver(key, val) {
    if (typeof val === 'object' && val.empty)
      return undefined;

    return val;
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
      slurpwrite(JSON.stringify(protos, undefined, 2), "proto.json");
      
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

  dup_objects(x) {
    var objs = x.slice();
    var duped = [];

    objs.forEach(function(x) {
      var newobj = this.edit_level.spawn(x.ur);
      dainty_assign(newobj, x);
      newobj.pos = x.pos;
      newobj.angle = x.angle;
      duped.push(newobj);
    } ,this);

    return duped.flat();
  },

  sel_start: [],

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

  key_move(dir) {
    if (Keys.ctrl())
      this.selectlist.forEach(this.snapper(dir.scale(1.01), editor_config.grid_size));
    else
      this.selectlist.forEach(this.mover(dir.scale(this.step_amt())));
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
    
  unselect() {
    editor.selectlist = [];
    this.grabselect = undefined;
    this.sel_comp = undefined;
  },

  sel_comp: undefined,
  comp_info: false,
  brush_obj: undefined,
  
  camera_recalls: {},
  camera_recall_stack: [],

  camera_recall_store() {
    this.camera_recall_stack.push({
      pos:this.camera.pos,
      zoom:this.camera.zoom
    });
  },

  camera_recall_pop() {
    Object.assign(this.camera, this.camera_recalls.pop());
  },

  camera_recall_clear() {
    this.camera_recall_stack = [];
  },
  
  input_num_pressed(num) {
    if (Keys.ctrl()) {
      this.camera_recalls[num] = {
        pos:this.camera.pos,
	zoom:this.camera.zoom
      };
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

  z_start: undefined,
  grabselect: undefined,
  mousejoy: undefined,
  joystart: undefined,
  
  stash: "",

  start_play_ed() {
      this.stash = this.edit_level.save();
      this.edit_level.kill();
      load_configs("game.config");
      Game.play();
      Player.players[0].uncontrol(this);
      Player.players[0].control(limited_editor);
      Register.unregister_obj(this);
  },
   
  moveoffsets: [],

  scaleoffset: 0,
  startscales: [],
  selected_com: [0,0],
  
  openpanel(panel, dontsteal) {
    if (this.curpanel)
      this.curpanel.close();
      
    this.curpanel = panel;
    
    var stolen = this;
    if (dontsteal)
      stolen = undefined;

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
  
  
  startrots: [],
  startpos: [],
  startoffs: [],
  
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

  _sel_comp: undefined,
  get sel_comp() { return this._sel_comp; },
  set sel_comp(x) {
    if (this._sel_comp)
      Player.players[0].uncontrol(this._sel_comp);
    
    this._sel_comp = x;

    if (this._sel_comp) {
      Log.info("sel comp is now " + this._sel_comp);
      Player.players[0].control(this._sel_comp);
    }
  },

  time: 0,

  ed_gui() {
    this.time = Date.now();
    /* Clean out killed objects */
    this.selectlist = this.selectlist.filter(function(x) { return x.alive; });

    GUI.text("WORKING LAYER: " + this.working_layer, [0,520]);
    GUI.text("MODE: " + this.edit_mode, [0,500]);

    if (this.cursor) {
      Debug.point(World2screen(this.cursor), 5, Color.green);

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

    if (this.comp_info && this.sel_comp) {
      GUI.text(Input.print_pawn_kbm(this.sel_comp), [100,700],1);
    }

    GUI.text("0,0", world2screen([0,0]));
    Debug.point([0,0],3);
    
    var clvl = this.edit_level;
    var ypos = 200;
    var lvlcolor = Color.white;
    while (clvl) {
      var lvlstr = clvl.file ? clvl.file : "NEW ENTITY";
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
        GUI.text(str, world2screen(this.selectlist[0].pos).add([0,-16*(i++)]));
      }

      if (this.sel_comp) {
        if ('gizmo' in this.sel_comp) this.sel_comp.gizmo();
      }
    }

    Game.objects.forEach(function(obj) {
      if (!obj.selectable)
        gui_img("icons/icons8-lock-16.png", world2screen(obj.pos));
    });

    Debug.draw_grid(1, editor_config.grid_size/editor_camera.zoom, editor_config.grid_color);
    var startgrid = screen2world([-20,Window.height]).map(function(x) { return Math.snap(x, editor_config.grid_size); }, this);
    var endgrid = screen2world([Window.width, 0]);
    
    while(startgrid[0] <= endgrid[0]) {
      GUI.text(startgrid[0], [world2screen([startgrid[0], 0])[0],0]);
      startgrid[0] += editor_config.grid_size;
    }

    while(startgrid[1] <= endgrid[1]) {
      GUI.text(startgrid[1], [0, world2screen([0, startgrid[1]])[1]]);
      startgrid[1] += editor_config.grid_size;
    }
    
    Debug.point(world2screen(this.selected_com), 3);
    this.selected_com = find_com(this.selectlist);

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

  paste() {
    this.selectlist = this.dup_objects(this.killring);

    var setpos = this.cursor ? this.cursor : Mouse.worldpos;
    this.selectlist.forEach(function(x) {
      x.pos = x.pos.sub(this.killcom).add(setpos);
    },this);
  },

  lvl_history: [],

  load(file) {
    if (this.edit_level) this.lvl_history.push(this.edit_level.ur);
//    this.edit_level.kill();
//    this.edit_level = Level.loadfile(file);
//    this.curlvl = this.edit_level.save();
    Primum.spawn(prototypes.get_ur(file));

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
    editor.selectlist = [];
    editor.selectlist.push(newlvl);
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
  
  repl: false,
}

editor.inputs = {};
editor.inputs['C-a'] = function() {
  if (!editor.selectlist.empty) { editor.unselect(); return; }
  editor.unselect();
  editor.selectlist = editor.edit_level.objects.slice();
  Log.warn("C-a pressed.");
};
editor.inputs['C-a'].doc = "Select all objects.";

editor.inputs['`'] = function() { editor.repl = !editor.repl; };
editor.inputs['`'].doc = "Open or close the repl.";

/* Return if selected component. */
editor.inputs['h'] = function() {
  var visible = true;
  editor.selectlist.forEach(function(x) { if (x.visible) visible = false; });
  editor.selectlist.forEach(function(x) { x.visible = visible; });
};
editor.inputs['h'].doc = "Toggle object hidden.";

editor.inputs['C-h'] = function() { Game.objects.forEach(function(x) { x.visible = true; }); };
editor.inputs['C-h'].doc = "Unhide all objects.";

editor.inputs['C-e'] = function() { editor.openpanel(assetexplorer); };
editor.inputs['C-e'].doc = "Open asset explorer.";

editor.inputs['C-l'] = function() { editor.openpanel(entitylistpanel, true); };
editor.inputs['C-l'].doc = "Open list of spawned entities.";

editor.inputs['C-i'] = function() {
  if (editor.selectlist.length !== 1) return;
  objectexplorer.obj = editor.selectlist[0];
  objectexplorer.on_close = editor.save_prototypes;
  editor.openpanel(objectexplorer);
};
editor.inputs['C-i'].doc = "Open the object explorer for a selected object.";

editor.inputs['C-d'] = function() {
  if (editor.selectlist.length === 0) return;
  var duped = editor.dup_objects(editor.selectlist);
  editor.unselect();
  editor.selectlist = duped;
};
editor.inputs['C-d'].doc = "Duplicate all selected objects.";

editor.inputs['C-m'] = function() {
  if (editor.sel_comp) {
    if (editor.sel_comp.flipy)
      editor.sel_comp.flipy = !editor.sel_comp.flipy;

    return;
  }
  
  editor.selectlist.forEach(function(x) { x.flipy = !x.flipy; });
};
editor.inputs['C-m'].doc = "Mirror selected objects on the Y axis.";

editor.inputs.m = function() {
  if (editor.sel_comp) {
    if (editor.sel_comp.flipx)
      editor.sel_comp.flipx = !editor.sel_comp.flipx;

    return;
  }
  
  editor.selectlist.forEach(function(x) { x.flipx = !x.flipx; });
};
editor.inputs.m.doc = "Mirror selected objects on the X axis.";

editor.inputs.q = function() { editor.comp_info = !editor.comp_info; };
editor.inputs.q.doc = "Toggle help for the selected component.";

editor.inputs.f = function() {
  if (editor.selectlist.length === 0) return;
  var bb = editor.selectlist[0].boundingbox;
  editor.selectlist.forEach(function(obj) { bb = bb_expand(bb, obj.boundingbox); });
  editor.zoom_to_bb(bb);
};
editor.inputs.f.doc = "Find the selected objects.";

editor.inputs['C-f'] = function() {
  if (editor.selectlist.length !== 1) return;
  if (!editor.selectlist[0].file) return;
  editor.edit_level = editor.selectlist[0];
  editor.unselect();
  editor.reset_undos();
  editor.curlvl = editor.edit_level.save();
};
editor.inputs['C-f'].doc = "Tunnel into the selected level object to edit it.";

editor.inputs['C-S-f'] = function() {
  if (!editor.edit_level.level) return;

  editor.edit_level = editor.edit_level.level;
  editor.unselect();
  editor.reset_undos();
  editor.curlvl = editor.edit_level.save();
  editor.edit_level.filejson = editor.edit_level.save();
  editor.edit_level.check_dirty();
};
editor.inputs['C-S-f'].doc = "Tunnel out of the level you are editing, saving it in the process.";

editor.inputs['C-r'] = function() { editor.selectlist.forEach(function(x) { x.angle = -x.angle; }); };
editor.inputs['C-r'].doc = "Negate the selected's angle.";

editor.inputs.r = function() {
  editor.startrots = [];

  if (editor.sel_comp && 'angle' in editor.sel_comp) {
    var relpos = screen2world(Mouse.pos).sub(editor.sel_comp.gameobject.pos);
    editor.startoffset = Math.atan2(relpos.y, relpos.x);
    editor.startrot = editor.sel_comp.angle;

    return;
  }

  var offf = editor.cursor ? editor.cursor : editor.selected_com;
  var relpos = screen2world(Mouse.pos).sub(offf);
  editor.startoffset = Math.atan2(relpos[1], relpos[0]);

  editor.selectlist.forEach(function(x, i) {
    editor.startrots[i] = x.angle;
    editor.startpos[i] = x.pos;
    editor.startoffs[i] = x.pos.sub(offf);
  }, editor);
};
editor.inputs.r.doc = "Rotate selected using the mouse while held down.";

editor.inputs.r.down = function() {
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
};

editor.inputs['C-p'] = function() {
  if (!Game.playing()) {
    editor.start_play_ed();
//    if (!Level.loadlevel("debug"))
      World.loadlevel("game");
  } else {
    Game.pause();
  }
};
editor.inputs['C-p'].doc = "Start game from 'debug' if it exists; otherwise, from 'game'.";

editor.inputs['M-p'] = function() {
  if (Game.playing())
    Game.pause();

  Game.step();
}
editor.inputs['M-p'].doc = "Do one time step, pausing if necessary.";

editor.inputs['C-M-p'] = function() {
  Log.warn(`Starting edited level ...`);
};
editor.inputs['C-M-p'].doc = "Start game from currently edited level.";

editor.inputs['C-q'] = function() {
  
};
editor.inputs['C-q'].doc = "Quit simulation and return to editor.";

var rebinder = {};
rebinder.inputs = {};
rebinder.inputs.any = function(cmd) {
  
};

editor.inputs['C-space'] = function() {
  
};
editor.inputs['C-space'].doc = "Search to execute a specific command.";

editor.inputs['M-m'] = function() {
//  Player.players[0].control(rebinder);
};
editor.inputs['M-m'].doc = "Rebind a shortcut. Usage: M-m SHORTCUT TARGET";

editor.inputs['M-S-8'] = function() {
  editor.camera_recall_pop();
};
editor.inputs['M-S-8'].doc = "Jump to last location.";

editor.inputs.escape = function() { editor.openpanel(quitpanel); }
editor.inputs.escape.doc = "Quit editor.";

editor.inputs['C-s'] = function() {
  if (editor.edit_level.level) {
    if (!editor.edit_level.unique)
      editor.save_current();

    editor.selectlist = [];
    editor.selectlist.push(editor.edit_level);
    editor.edit_level = editor.edit_level.level;

    return;
  }

  editor.save_current();
};
editor.inputs['C-s'].doc = "Save selected.";

editor.inputs['C-S-s'] = function() {
  editor.openpanel(saveaspanel);
};
editor.inputs['C-S-s'].doc = "Save selected as.";

editor.inputs['C-z'] = function() { editor.undo(); };
editor.inputs['C-z'].doc = "Undo the last change made.";

editor.inputs['C-S-z'] = function() { editor.redo(); };
editor.inputs['C-S-z'].doc = "Redo the last undo.";

editor.inputs.t = function() { editor.selectlist.forEach(function(x) { x.selectable = false; }); };
editor.inputs.t.doc = "Lock selected objects to make them non selectable.";

editor.inputs['M-t'] = function() { editor.edit_level.objects.forEach(function(x) { x.selectable = true; }); };
editor.inputs['M-t'].doc = "Unlock all objects in current level.";

editor.inputs['C-n'] = function() {
  if (editor.edit_level.dirty) {
    Log.info("Level has changed; save before starting a new one.");
    editor.openpanel(gen_notify("Level is changed. Are you sure you want to close it?", _ => editor.clear_level()));
    return;
  }

  editor.clear_level();    
};
editor.inputs['C-n'].doc = "Open a new level.";

editor.inputs['C-o'] = function() {
  if (editor.check_level_nested()) {
    Log.warn("Nested level ...");
    return;
  }

  if (editor.edit_level.dirty) {
    editor.openpanel(gen_notify("Level is changed. Are you sure you want to close it?", function() {
      editor.clear_level();
      editor.openpanel(openlevelpanel);
    }.bind(editor)));
    return;
  }

  editor.openpanel(openlevelpanel);
};
editor.inputs['C-o'].doc = "Open a level.";

editor.inputs['M-o'] = function() {
   editor.openpanel(addlevelpanel);
};
editor.inputs['M-o'].doc = "Select a level to add to the current level.";

editor.inputs['C-M-o'] = function() {
  if (editor.selectlist.length === 1 && editor.selectlist[0].file) {
    if (editor.edit_level.dirty) return;
    editor.load(editor.selectlist[0].file);
  }
};
editor.inputs['C-M-o'].doc = "Revert opened level back to its disk form.";

editor.inputs['C-S-o'] = function() {
  if (!editor.edit_level.dirty)
    editor.load_prev();
};
editor.inputs['C-S-o'].doc = "Open previous level.";

editor.inputs['C-y'] = function() {
  texteditor.on_close = function() { editor.edit_level.script = texteditor.value;};

  editor.openpanel(texteditor);
  if (!editor.edit_level.script)
    editor.edit_level.script = "";
  texteditor.value = editor.edit_level.script;
  texteditor.start();
};
editor.inputs['C-y'].doc = "Open script editor for the level.";

editor.inputs['M-y'] = function() { editor.programmode = !editor.programmode; };
editor.inputs['M-y'].doc = "Toggle program mode.";

editor.inputs.minus = function() {
  if (!editor.selectlist.empty) {
    editor.selectlist.forEach(function(x) { x.draw_layer--; });
    return;
  }

  if (editor.working_layer > -1)
    editor.working_layer--;
};
editor.inputs.minus.doc = "Go down one working layer, or, move selected objects down one layer.";

editor.inputs.plus = function() {
  if (!editor.selectlist.empty) {
    editor.selectlist.forEach(x => x.draw_layer++);
    return;
  }

  if (editor.working_layer < 4)
    editor.working_layer++;
};

editor.inputs.plus.doc = "Go up one working layer, or, move selected objects down one layer.";

editor.inputs['C-f1'] = function() { editor.edit_mode = "basic"; };
editor.inputs['C-f1'].doc = "Enter basic edit mode.";
editor.inputs['C-f2'] = function() { editor.edit_mode = "brush"; };
editor.inputs['C-f2'].doc = "Enter brush mode.";

editor.inputs.f2 = function() {
  objectexplorer.on_close = save_configs;
  objectexplorer.obj = configs;
  this.openpanel(objectexplorer);
};
editor.inputs.f2.doc = "Open configurations object.";

editor.inputs['C-j'] = function() {
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
};
editor.inputs['C-j'].doc = "Give selected objects a variable name.";

editor.inputs['M-j'] = function() {
  var varmakes = this.selectlist.filter(function(x) { return x.hasOwn('varname'); });
  varmakes.forEach(function(x) { delete x.varname; });
};
editor.inputs['M-j'].doc = "Remove variable names from selected objects.";

editor.inputs.lm = function() { editor.sel_start = screen2world(Mouse.pos); };
editor.inputs.lm.doc = "Selection box.";

editor.inputs.lm.released = function() {
    Mouse.normal();

    if (!editor.sel_start) return; 
    
    if (editor.sel_comp) {
      editor.sel_start = undefined;
      return;
    }

    var selects = [];
    
    /* TODO: selects somehow gets undefined objects in here */
    if (screen2world(Mouse.pos).equal(editor.sel_start)) {
      var sel = editor.try_select();
      if (sel) selects.push(sel);
    } else {
      var box = editor.points2cwh(editor.sel_start, screen2world(Mouse.pos));
    
      box.pos = box.c;
    
      var hits = physics.box_query(box);

      hits.forEach(function(x, i) {
        var obj = editor.do_select(x);
	if (obj)
	  selects.push(obj);
      },editor);
      
      var levels = editor.edit_level.objects.filter(function(x) { return x.file; });
      var lvlpos = [];
      levels.forEach(function(x) { lvlpos.push(x.pos); });
      var lvlhits = physics.box_point_query(box, lvlpos);
      
      lvlhits.forEach(function(x) { selects.push(levels[x]); });
    }

    this.sel_start = undefined;
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
    
    editor.selectlist = [];
    selects.forEach(function(x) {
      if (x !== undefined)
        this.selectlist.push(x);
    }, this);
};

editor.inputs.rm = function() {
    if (Keys.shift()) {
      editor.cursor = undefined;
      return;
    }
    
    if (editor.brush_obj)
      editor.brush_obj = undefined;
  
    if (editor.sel_comp) {
      editor.sel_comp = undefined;
      return;
    }
    
    editor.unselect();
};

editor.inputs.mm = function() {
    if (editor.brush_obj) {
      editor.selectlist = editor.dup_objects([editor.brush_obj]);
      editor.selectlist[0].pos = screen2world(Mouse.pos);
      editor.grabselect = editor.selectlist[0];
      return;
    }
    
    if (editor.sel_comp && 'pick' in editor.sel_comp) {
      editor.grabselect = editor.sel_comp.pick(screen2world(Mouse.pos));
      if (!editor.grabselect) return;
      
      editor.moveoffset = editor.sel_comp.gameobject.editor2world(editor.grabselect).sub(screen2world(Mouse.pos));
      return;
    }
  
    var grabobj = editor.try_select();
/*    if (Array.isArray(grabobj)) {
      editor.selectlist = grabobj;
      return;
    }
*/
    editor.grabselect = undefined;
    if (!grabobj) return;
    
    if (Keys.ctrl()) {
      grabobj = editor.dup_objects([grabobj])[0];
    }

    editor.grabselect = grabobj;

    if (!editor.selectlist.includes(grabobj)) {
      editor.selectlist = [];
      editor.selectlist.push(grabobj);
    }

    editor.moveoffset = editor.grabselect.pos.sub(screen2world(Mouse.pos));
};
editor.inputs['C-mm'] = editor.inputs.mm;

editor.inputs['C-M-mm'] = function() {
  editor.mousejoy = Mouse.pos;
  editor.joystart = editor.camera.pos;
};

editor.inputs['C-M-rm'] = function() {
  editor.mousejoy = Mouse.pos;
  editor.z_start = editor.camera.zoom;
  Mouse.disabled();
};

editor.inputs['C-S-mm'] = function() { editor.cursor = find_com(editor.selectlist); };

editor.inputs.rm.released = function() {
  editor.mousejoy = undefined;
  editor.z_start = undefined;
  Mouse.normal();
};

editor.inputs['S-mm'] = function() { this.cursor = Mouse.worldpos; };

editor.inputs.mm.released = function () {
  Mouse.normal();
  this.grabselect = undefined;
  editor.mousejoy = undefined;
  editor.joystart = undefined;
};
editor.inputs.mouse = {};
editor.inputs.mouse.move = function(pos, dpos)
{
  if (editor.mousejoy) {
    if (editor.z_start)
      editor.camera.zoom += dpos.y/500;
    else if (editor.joystart)
      editor.camera.pos = editor.camera.pos.add(dpos.scale(editor.camera.zoom).mapc(mult,[-1,1]));
  }
  
  if (this.grabselect) {
    if ('pos' in this.grabselect)
      this.grabselect.pos = this.moveoffset.add(screen2world(Mouse.pos));
    else
      this.grabselect.set(this.selectlist[0].world2this(this.moveoffset.add(screen2world(Mouse.pos))));
  }
}

editor.inputs['C-M-S-lm'] = function() { editor.selectlist[0].set_center(screen2world(Mouse.pos)); };
editor.inputs['C-M-S-lm'].doc = "Set world center to mouse position.";

editor.inputs.delete = function() {
  this.selectlist.forEach(x => x.kill());
  this.unselect();
};
editor.inputs.delete.doc = "Delete selected objects.";

editor.inputs['C-u'] = function() {
  this.selectlist.forEach(function(x) {
    x.revert();
  });
};
editor.inputs['C-u'].doc = "Revert selected objects back to their prefab form.";

editor.inputs['M-u'] = function() {
  this.selectlist.forEach(function(x) {
    x.unique = true;
  });
};
editor.inputs['M-u'].doc = "Make selected objects unique.";

editor.inputs['C-S-g'] = function() { editor.openpanel(groupsaveaspanel); };
editor.inputs['C-S-g'].doc = "Save selected objects as a new level.";

editor.inputs.g = function() {
    if (this.sel_comp) {
      if ('pos' in this.sel_comp)
        this.moveoffset = this.sel_comp.pos.sub(screen2world(Mouse.pos));
      return;
    }
  
  editor.selectlist.forEach(function(x,i) { editor.moveoffsets[i] = x.pos.sub(screen2world(Mouse.pos)); } );
};
editor.inputs.g.doc = "Move selected objects.";
editor.inputs.g.released = function() { editor.moveoffsets = []; };

editor.inputs.g.down = function() {
    if (this.sel_comp) {
      this.sel_comp.pos = this.moveoffset.add(screen2world(Mouse.pos));
      return;
    }

    if (this.moveoffsets.length === 0) return;

    this.selectlist.forEach(function(x,i) {
      x.pos = this.moveoffsets[i].add(screen2world(Mouse.pos));
    }, this);
};

editor.inputs.up = function() { this.key_move([0,1]); };
editor.inputs.up.rep = true;

editor.inputs.left = function() { this.key_move([-1,0]); };
editor.inputs.left.rep = true;

editor.inputs.right = function() { this.key_move([1,0]); };
editor.inputs.right.rep = true;

editor.inputs.down = function() { this.key_move([0,-1]); };
editor.inputs.down.rep = true;

editor.inputs.tab = function() {
  if (!this.selectlist.length === 1) return;
  if (!this.selectlist[0].components) return;

  var sel = this.selectlist[0].components;

  if (!this.sel_comp)
    this.sel_comp = sel.nth(0);
  else {
    var idx = sel.findIndex(this.sel_comp) + 1;
    if (idx >= Object.keys(sel).length)
      this.sel_comp = undefined;
    else
      this.sel_comp = sel.nth(idx);
  }
};
editor.inputs.tab.doc = "Cycle through selected object's components.";

editor.inputs['C-g'] = function() {
  if (!this.selectlist) return;
  this.selectlist = this.dup_objects(this.selectlist);
  editor.inputs.g();
};
editor.inputs['C-g'].doc = "Duplicate selected objects, then move them.";

editor.inputs['C-lb'] = function() {
  editor_config.grid_size -= Keys.shift() ? 10 : 1;
  if (editor_config.grid_size <= 0) editor_config.grid_size = 1;
};
editor.inputs['C-lb'].doc = "Decrease grid size. Hold shift to decrease it more.";
editor.inputs['C-lb'].rep = true;

editor.inputs['C-rb'] = function() { editor_config.grid_size += Keys.shift() ? 10 : 1; };
editor.inputs['C-rb'].doc = "Increase grid size. Hold shift to increase it more.";
editor.inputs['C-rb'].rep = true;

editor.inputs['C-c'] = function() {
  this.killring = [];
  this.killcom = [];

  this.selectlist.forEach(function(x) {
    this.killring.push(x);
  },this);

  this.killcom = find_com(this.killring);
};
editor.inputs['C-c'].doc = "Copy selected objects to killring.";

editor.inputs['C-x'] = function() {
  editor.inputs['C-c']();
  this.killring.forEach(function(x) { x.kill(); });
};
editor.inputs['C-x'].doc = "Cut objects to killring.";

editor.inputs['C-v'] = function() { editor.paste(); };
editor.inputs['C-v'].doc = "Pull objects from killring to world.";

editor.inputs['M-g'] = function() {
  if (this.cursor) return;
  var com = find_com(this.selectlist);
  this.selectlist.forEach(function(x) {
    x.pos = x.pos.sub(com).add(this.cursor);
  },this);
};
editor.inputs['M-g'].doc = "Set cursor to the center of selected objects.";

var brushmode = {};
brushmode.inputs = {};
brushmode.inputs.lm = function() { editor.paste(); };
brushmode.inputs.lm.doc = "Paste selected brush.";

brushmode.inputs.b = function() {
  if (editor.brush_obj) {
    editor.brush_obj = undefined;
    return;
  }

  if (editor.selectlist.length !== 1) return;
  editor.brush_obj = editor.seliectlist[0];
  editor.unselect();
};
brushmode.inputs.b.doc = "Clear brush, or set a new one.";

var compmode = {};
compmode.inputs = {};
compmode.inputs['C-c'] = function() {}; /* Simply a blocker */
compmode.inputs['C-x'] = function() {};

editor.inputs.s = function() {
  var offf = editor.cursor ? editor.cursor : editor.selected_com;
  editor.scaleoffset = Vector.length(Mouse.worldpos.sub(offf));

  if (editor.sel_comp) {
    if (!('scale' in editor.sel_comp)) return;
    editor.startscales = [];
    editor.startscales.push(editor.sel_comp.scale);
    return;
  }

  editor.selectlist.forEach(function(x, i) {
    editor.startscales[i] = x.scale;
    if (editor.cursor)
      editor.startoffs[i] = x.pos.sub(editor.cursor);
  }, editor);

};
editor.inputs.s.doc = "Scale selected.";

editor.inputs.s.down = function() {
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
};

editor.inputs.s.released = function() { this.scaleoffset = undefined; };

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
      Player.players[0].uncontrol(this.stolen);
      Player.players[0].control(this);
    }
    this.start();
    this.keycb();
  },
  
  start() {},

  
  close() {
    Player.players[0].uncontrol(this);
    if (this.stolen) {
      Player.players[0].control(this.stolen);
      this.stolen = undefined;
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

load("scripts/textedit.js");

var objectexplorer = copy(inputpanel, {
  title: "object explorer",
  obj: undefined,
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
  title: "open entity",
  action() {
    editor.load(this.value);
  },
  
  assets: [],
  allassets: [],

  submit_check() {
    if (this.assets.length === 1) {
      this.value = this.assets[0];
      return true;
    } else {
      return this.assets.includes(this.value);
    }
  },
  
  start() {
    this.allassets = prototypes.list.sort();
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
  panel.inputs = {};
  panel.inputs.y = function() { panel.yes(); panel.close(); };
  panel.inputs.y.doc = "Confirm yes.";
  panel.inputs.enter = function() { panel.close(); };
  panel.inputs.enter.doc = "Close.";
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
    
    var ret = undefined;
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

var entitylistpanel = copy(inputpanel, {
  title: "Level object list",
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

var limited_editor = {};

limited_editor.inputs = {};

limited_editor.inputs['C-p'] = function()
{
  if (Game.playing())
    Game.pause();
  else
    Game.play();
}

limited_editor.inputs['M-p'] = function()
{
  Game.pause();
  Game.step();
}

limited_editor.inputs['C-q'] = function()
{
  Game.stop();
  game.stop();
  Sound.killall();
  Player.players[0].uncontrol(limited_editor);
  Player.players[0].control(editor);
  Register.gui.register(editor.ed_gui, editor);
  Debug.register_call(editor.ed_debug, editor);
//  World.kill();
  World.clear_all();
  editor.load_json(editor.stash);
  Game.view_camera(editor_camera);
}

/* This is used for editing during a paused game */
var limited_editing = {};
limited_editing.inputs = {};

Player.players[0].control(editor);
Register.gui.register(editor.ed_gui, editor);
Debug.register_call(editor.ed_debug, editor);

if (IO.exists("editor.config"))
  load_configs("editor.config");

editor.edit_level = editor_level;

Game.stop();
