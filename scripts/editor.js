/*
  Editor-only variables on objects
  selectable
*/
prototypes.generate_ur('.');

var editor_config = {
  grid_size: 100,
  ruler_mark_px: 100,
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
  dbg_ur: "arena.level1",
  selectlist: [],
  grablist: [],
  scalelist: [],
  rotlist: [],
  camera: undefined,
  edit_level: undefined, /* The current level that is being edited */
  desktop: undefined, /* The editor desktop, where all editing objects live */
  working_layer: 0,
  get cursor() {
    if (this.selectlist.length === 0 ) return Mouse.worldpos;
    return find_com(this.selectlist);
  },
  edit_mode: "basic",

  try_select() { /* nullify true if it should set selected to null if it doesn't find an object */
    var go = physics.pos_query(Mouse.worldpos);
    return this.do_select(go);
  },
    
  /* Tries to select id */
  do_select(go) {
    var obj = go >= 0 ? Game.object(go) : undefined;
    if (!obj || !obj._ed.selectable) return undefined;
    
    if (obj.level !== this.edit_level) {
      var testlevel = obj.level;
      while (testlevel && testlevel.level !== this.edit_level) {
         testlevel = testlevel.level;
      }
      
      return testlevel;
    }
    
    return obj;

    if (this.working_layer > -1 && obj.draw_layer !== this.working_layer) return undefined;

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

  dup_objects(x) {
    var objs = x.slice();
    var duped = [];

    objs.forEach(function(x) { duped.push(x.dup()); } );
    return duped;
  },

  sel_start: [],

  mover(amt, snap) {
    return function(go) { go.pos = go.pos.add(amt)};
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
    if (!editor.grabselect) return;
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
    this.grabselect = [];
    this.scalelist = [];
    this.rotlist = [];
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
  
  stash: undefined,

  start_play_ed() {
    this.stash = this.edit_level.json_obj();
    this.edit_level.kill();
//      load_configs("game.config");
    Game.play();
    Player.players[0].uncontrol(this);
    Player.players[0].control(limited_editor);
    Register.unregister_obj(this);
    Primum.spawn(this.dbg_ur);
  },

  enter_editor() {
    Game.pause();
    Player.players[0].control(this);
    Player.players[0].uncontrol(limited_editor);
    Register.gui.register(editor.ed_gui, editor);
    Debug.register_call(editor.ed_debug, editor);
    Register.update.register(gui_controls.update, gui_controls);
    Player.players[0].control(gui_controls);
  },

  end_debug() {
    
  },
   
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
  },
  
  clear_level() {
    if (this.edit_level) {
      if (this.edit_level.level) {
        this.openpanel(gen_notify("Can't close a nested level. Save up to the root before continuing."));
        return;
      }
      this.unselect();
      this.edit_level.kill();
    }
    
    this.edit_level = Primum.spawn(ur.arena);
    this.desktop = this.edit_level;
    if (this.stash)
      
    editor.edit_level._ed.selectable = false;
  },

  load_desktop(d) {
    
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
    /* Clean out killed objects */
    this.selectlist = this.selectlist.filter(function(x) { return x.alive; });

    GUI.text("WORKING LAYER: " + this.working_layer, [0,520]);
    GUI.text("MODE: " + this.edit_mode, [0,500]);

    Debug.point(world2screen(this.cursor), 2, Color.green);

    if (this.comp_info && this.sel_comp) {
      GUI.text(Input.print_pawn_kbm(this.sel_comp,false), [100,700],1);
    }

    GUI.text("0,0", world2screen([0,0]));
    
    var clvl = this.selectlist.length === 1 ? this.selectlist[0] : this.edit_level;
    var lvlchain = [];
    while (clvl !== Primum) {
      lvlchain.push(clvl);
      clvl = clvl.level;
    }
    lvlchain.push(clvl);
    
    var lvlcolorsample = 1;
    var colormap = ColorMap.Bathymetry;
    var lvlcolor = colormap.sample(lvlcolorsample);
    var ypos = 200;
    
    lvlchain.reverse();
    lvlchain.forEach(function(x,i) {
      lvlcolor = colormap.sample(lvlcolorsample);
      var lvlstr = x.toString();
      if (x._ed.dirty)
        lvlstr += "*";
      if (i === lvlchain.length-1) lvlstr += "[this]";
      GUI.text(lvlstr, [0, ypos], 1, lvlcolor);
     
      lvlcolorsample -= 0.1;

      GUI.text("^^^^^^", [0,ypos+=5],1);
      ypos += 15;
    });
    
    /* Color selected objects with the next deeper color */
    lvlcolorsample -= 0.1;
    lvlcolor = colormap.sample(lvlcolorsample);

    GUI.text("$$$$$$", [0,ypos],1,lvlcolor);

    this.selectlist.forEach(function(x) {
      var sname = x.__proto__.toString();
      if (!x.json_obj().empty)
        x._ed.dirty = true;
      else
        x._ed.dirty = false;
	
      if (x._ed.dirty) sname += "*";
      
      GUI.text(sname, world2screen(x.worldpos()).add([0, 16]), 1, lvlcolor);
      GUI.text(x.worldpos().map(function(x) { return Math.round(x); }), world2screen(x.worldpos()), 1, Color.white);
      Debug.arrow(world2screen(x.worldpos()), world2screen(x.worldpos().add(x.up().scale(40))), Color.yellow, 1);

      if ('gizmo' in x && typeof x['gizmo'] === 'function' )
        x.gizmo();
    });

    if (this.selectlist.length === 0)
      for (var key in this.edit_level.objects) {
        var o = this.edit_level.objects[key];
        GUI.text(key, world2screen(o.worldpos()), 1, lvlcolor);
      }
    else
      this.selectlist.forEach(function(x) {
        Object.entries(x.objects).forEach(function(x) {
	  GUI.text(x[0], world2screen(x[1].worldpos()), 1, lvlcolor);
	});
      });
    
    this.edit_level.objects.forEach(function(x) {
      if ('ed_gizmo' in x)
        x.ed_gizmo();
    });

    if (this.selectlist.length === 1) {
      var i = 1;
      for (var key in this.selectlist[0].components) {
        var selected = this.sel_comp === this.selectlist[0].components[key];
        var str = (selected ? ">" : " ") + key + " [" + this.selectlist[0].components[key].toString() + "]";
        GUI.text(str, world2screen(this.selectlist[0].worldpos()).add([0,-16*(i++)]));
      }

      if (this.sel_comp) {
        if ('gizmo' in this.sel_comp) this.sel_comp.gizmo();
      }
    }

    editor.edit_level.objects.forEach(function(obj) {
      if (!obj._ed.selectable)
        GUI.image("icons/icons8-lock-16.png", world2screen(obj.worldpos()));
    });

    Debug.draw_grid(1, editor_config.grid_size, Color.Editor.grid.alpha(0.3));
    var startgrid = screen2world([-20,0]).map(function(x) { return Math.snap(x, editor_config.grid_size); });
    var endgrid = screen2world([Window.width, Window.height]);
    
    var w_step = Math.round(editor_config.ruler_mark_px/Window.width * (endgrid.x-startgrid.x)/editor_config.grid_size)*editor_config.grid_size;
    if (w_step === 0) w_step = editor_config.grid_size;
    
    var h_step = Math.round(editor_config.ruler_mark_px/Window.height * (endgrid.y-startgrid.y)/editor_config.grid_size)*editor_config.grid_size;
    if (h_step === 0) h_step = editor_config.grid_size;
    
    while(startgrid[0] <= endgrid[0]) {
      GUI.text(startgrid[0], [world2screen([startgrid[0], 0])[0],0]);
      startgrid[0] += w_step;
    }

    while(startgrid[1] <= endgrid[1]) {
      GUI.text(startgrid[1], [0, world2screen([0, startgrid[1]])[1]]);
      startgrid[1] += h_step;
    }
    
    /* Draw selection box */
    if (this.sel_start) {
      var endpos = Mouse.worldpos;
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
      Debug.boundingbox(bb, Color.Editor.select.alpha(0.1));
      Debug.line(bb2points(bb).wrapped(1), Color.white);
    }
    
    if (this.curpanel && this.curpanel.on)
      this.curpanel.gui();

    this.curpanels.forEach(function(x) {
      if (x.on) x.gui();
    });
  },
  
  ed_debug() {
    if (!Debug.phys_drawing)
      this.selectlist.forEach(function(x) { Debug.draw_obj_phys(x); });
  },
  
  viewasset(path) {
    Log.info(path);
    var fn = function(x) { return path.endsWith(x); };
    if (images.any(fn)) {
      var newtex = Object.copy(texgui, { path: path });
      this.addpanel(newtex);
    }
    else if (sounds.any(fn))
      Log.info("selected a sound");
  },

  killring: [],
  killcom: [],

  lvl_history: [],

  load(file) {
    Log.warn("LOADING " + file);
    var ur = prototypes.get_ur(file);
    if (!ur) return;
    var obj = editor.edit_level.spawn(ur);
    obj.pos = Mouse.worldpos;
    this.selectlist = [obj];
  },

  load_prev() {
    if (this.lvl_history.length === 0) return;

    var file = this.lvl_history.pop();
    this.edit_level = Level.loadfile(file);
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

  /* Checking to save an entity as a subtype. */
  /* sub is the name of the type; obj is the object to save it as */
  saveas_check(sub, obj) {
    if (!sub) return;
    obj ??= editor.selectlist[0];
    
    var curur = prototypes.get_ur(sub);
    
    if (curur) {
      notifypanel.action = editor.saveas;
      this.openpanel(gen_notify("Entity already exists with that name. Delete first."));
    } else {
      var path = sub.replaceAll('.', '/') + ".json";
      var saveobj = obj.json_obj();
      IO.slurpwrite(JSON.stringify(saveobj,null,1), path);


      if (obj === editor.edit_level) {
        obj.clear();
	var nobj = editor.edit_level.spawn(sub);
	editor.selectlist = [nobj];
	return;
      }
      
      var t = obj.transform();      
      obj.kill();
      editor.unselect();
      obj = editor.load(sub);
      obj.pos = t.pos;
      obj.angle = t.angle;
    }
  },
}

editor.inputs = {};
editor.inputs.release_post = function() {
  editor.check_snapshot();
  editor.edit_level.check_dirty();
};
editor.inputs['C-a'] = function() {
  if (!editor.selectlist.empty) { editor.unselect(); return; }
  editor.unselect();
  editor.selectlist = editor.edit_level.objects.slice();
};
editor.inputs['C-a'].doc = "Select all objects.";

editor.inputs['C-`'] = function() { editor.openpanel(replpanel); }
editor.inputs['C-`'].doc = "Open or close the repl.";

/* Return if selected component. */
editor.inputs['h'] = function() {
  var visible = true;
  editor.selectlist.forEach(function(x) { if (x.visible) visible = false; });
  editor.selectlist.forEach(function(x) { x.visible = visible; });
};
editor.inputs['h'].doc = "Toggle object hidden.";

editor.inputs['C-h'] = function() { Primum.objects.forEach(function(x) { x.visible = true; }); };
editor.inputs['C-h'].doc = "Unhide all objects.";

editor.inputs['C-e'] = function() { editor.openpanel(assetexplorer); };
editor.inputs['C-e'].doc = "Open asset explorer.";

editor.inputs['C-l'] = function() { editor.openpanel(entitylistpanel); };
editor.inputs['C-l'].doc = "Open list of spawned entities.";

/*editor.inputs['C-i'] = function() {
  if (editor.selectlist.length !== 1) return;
  objectexplorer.obj = editor.selectlist[0];
  editor.openpanel(objectexplorer);
};
editor.inputs['C-i'].doc = "Open the object explorer for a selected object.";
*/
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
  var bb = editor.selectlist[0].boundingbox();
  editor.selectlist.forEach(function(obj) { bb = bb_expand(bb, obj.boundingbox()); });
  editor.zoom_to_bb(bb);
};
editor.inputs.f.doc = "Find the selected objects.";

editor.inputs['C-f'] = function() {
  if (editor.selectlist.length !== 1) return;

  editor.edit_level = editor.selectlist[0];
  editor.unselect();
};
editor.inputs['C-f'].doc = "Tunnel into the selected level object to edit it.";

editor.inputs['C-F'] = function() {
  if (editor.edit_level.level === Primum) return;

  editor.edit_level = editor.edit_level.level;
  editor.unselect();
  editor.reset_undos();
//  editor.curlvl = editor.edit_level.save();
//  editor.edit_level.check_dirty();
};
editor.inputs['C-F'].doc = "Tunnel out of the level you are editing, saving it in the process.";

editor.inputs['C-r'] = function() { editor.selectlist.forEach(function(x) { x.angle = -x.angle; }); };
editor.inputs['C-r'].doc = "Negate the selected's angle.";

editor.inputs.r = function() {
  if (editor.sel_comp && 'angle' in editor.sel_comp) {
    var relpos = Mouse.worldpos.sub(editor.sel_comp.gameobject.worldpos());
    editor.startoffset = Math.atan2(relpos.y, relpos.x);
    editor.startrot = editor.sel_comp.angle;

    return;
  }

  editor.rotlist = [];
  editor.selectlist.forEach(function(x) {
    var relpos = Mouse.worldpos.sub(editor.cursor);
    editor.rotlist.push({
      obj: x,
      angle: x.angle,
      pos: x.pos,
      offset: x.pos.sub(editor.cursor),
      rotoffset: Math.atan2(relpos.y, relpos.x),
    });
  });
};
editor.inputs.r.doc = "Rotate selected using the mouse while held down.";

editor.inputs.r.released = function() { editor.rotlist = []; }

editor.inputs.f5 = function()
{
  editor.start_play_ed();
}

editor.inputs.f5.doc = "Start game from 'debug' if it exists; otherwise, from 'game'.";

editor.inputs['M-p'] = function() {
  if (Game.playing())
    Game.pause();

  Game.step();
}
editor.inputs['M-p'].doc = "Do one time step, pausing if necessary.";

editor.inputs['C-M-p'] = function() {
  if (!Game.playing()) {
    editor.start_play_ed();
    Game.editor_mode(false);    
  }
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
  if (editor.selectlist.length === 0) {
    saveaspanel.stem = editor.edit_level.ur.toString();
    saveaspanel.obj = editor.edit_level;
    editor.openpanel(saveaspanel);
    return;
  };

  if (editor.selectlist.length !== 1 || !editor.selectlist[0]._ed.dirty) return;
  var saveobj = editor.selectlist[0].json_obj();
  Object.merge(editor.selectlist[0].__proto__, saveobj);
  editor.selectlist[0].__proto__.objects = saveobj.objects;
  var path = editor.selectlist[0].ur.toString();
  path = path.replaceAll('.','/');
  path = path + "/" + path.name() + ".json";

  IO.slurpwrite(JSON.stringify(editor.selectlist[0].__proto__,null,1), path);
  Log.warn(`Wrote to file ${path}`);
};
editor.inputs['C-s'].doc = "Save selected.";

editor.inputs['C-S'] = function() {
  if (editor.selectlist.length !== 1) return;
  saveaspanel.stem = this.selectlist[0].toString();
  editor.openpanel(saveaspanel);
};
editor.inputs['C-S'].doc = "Save selected as.";

editor.inputs['C-z'] = function() { editor.undo(); };
editor.inputs['C-z'].doc = "Undo the last change made.";

editor.inputs['C-S-z'] = function() { editor.redo(); };
editor.inputs['C-S-z'].doc = "Redo the last undo.";

editor.inputs.t = function() { editor.selectlist.forEach(function(x) { x._ed.selectable = false; }); };
editor.inputs.t.doc = "Lock selected objects to make them non selectable.";

editor.inputs['M-t'] = function() { editor.edit_level.objects.forEach(function(x) { x._ed.selectable = true; }); };
editor.inputs['M-t'].doc = "Unlock all objects in current level.";

editor.inputs['C-n'] = function() {
  if (editor.edit_level._ed.dirty) {
    Log.info("Level has changed; save before starting a new one.");
    editor.openpanel(gen_notify("Level is changed. Are you sure you want to close it?", _ => editor.clear_level()));
    return;
  }

  editor.clear_level();    
};
editor.inputs['C-n'].doc = "Open a new level.";

editor.inputs['C-o'] = function() {
  editor.openpanel(openlevelpanel);
};
editor.inputs['C-o'].doc = "Open a level.";

editor.inputs['C-M-o'] = function() {
  if (editor.selectlist.length === 1 && editor.selectlist[0].file) {
    if (editor.edit_level._ed.dirty) return;
    editor.load(editor.selectlist[0].file);
  }
};
editor.inputs['C-M-o'].doc = "Revert opened level back to its disk form.";

editor.inputs['C-S-o'] = function() {
  if (!editor.edit_level._ed.dirty)
    editor.load_prev();
};
editor.inputs['C-S-o'].doc = "Open previous level.";

editor.inputs['C-y'] = function() {
  texteditor.on_close = function() { editor.edit_level.script = texteditor.value;};

  editor.openpanel(texteditor);
  texteditor.value = "";
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

editor.inputs.lm = function() { editor.sel_start = Mouse.worldpos; };
editor.inputs.lm.doc = "Selection box.";

editor.inputs.lm.released = function() {
    Mouse.normal();
    editor.unselect();

    if (!editor.sel_start) return; 
    
    if (editor.sel_comp) {
      editor.sel_start = undefined;
      return;
    }

    var selects = [];
    
    /* TODO: selects somehow gets undefined objects in here */
    if (Mouse.worldpos.equal(editor.sel_start)) {
      var sel = editor.try_select();
      if (sel) selects.push(sel);
    } else {
      var box = points2cwh(editor.sel_start, Mouse.worldpos);
    
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
    if (editor.brush_obj)
      editor.brush_obj = undefined;
  
    if (editor.sel_comp) {
      editor.sel_comp = undefined;
      return;
    }
    
    editor.unselect();
};

editor.try_pick = function()
{
  editor.grabselect = [];

  if (editor.sel_comp && 'pick' in editor.sel_comp) {
    var o = editor.sel_comp.pick(Mouse.worldpos);
    if (o)
      editor.grabselect = [o];
    return;
  }

  if (editor.selectlist.length > 0) {
    editor.grabselect = editor.selectlist.slice();
    return;
  }

  var grabobj = editor.try_select();

  if (!grabobj) return;
  editor.grabselect = [grabobj];
  editor.selectlist = [grabobj];
}

editor.inputs.mm = function() {
    if (editor.brush_obj) {
      editor.selectlist = editor.dup_objects([editor.brush_obj]);
      editor.selectlist[0].pos = Mouse.worldpos;
      editor.grabselect = editor.selectlist[0];
      return;
    }

    editor.try_pick();
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

editor.inputs.rm.released = function() {
  editor.mousejoy = undefined;
  editor.z_start = undefined;
  Mouse.normal();
};

editor.inputs.mm.released = function () {
  Mouse.normal();
  this.grabselect = [];
  editor.mousejoy = undefined;
  editor.joystart = undefined;
};
editor.inputs.mouse = {};
editor.inputs.mouse.move = function(pos, dpos)
{
  if (editor.mousejoy) {
    if (editor.z_start)
      editor.camera.zoom -= dpos.y/500;
    else if (editor.joystart)
      editor.camera.pos = editor.camera.pos.sub(dpos.scale(editor.camera.zoom));
  }

  editor.grabselect?.forEach(function(x) {
    x.pos = x.pos.add(dpos.scale(editor.camera.zoom));
    if ('sync' in x)
      x.sync();
  });
  
  var relpos = Mouse.worldpos.sub(editor.cursor);
  var dist = Vector.length(relpos);

  editor.scalelist?.forEach(function(x) {
    var scalediff = dist / x.scaleoffset;
    x.obj.scale = x.scale * scalediff;
    if (x.offset)
      x.obj.pos = editor.cursor.add(x.offset.scale(scalediff));
  });

  editor.rotlist?.forEach(function(x) {
    var anglediff = Math.atan2(relpos.y, relpos.x) - x.rotoffset;
    
    x.obj.angle = x.angle + Math.rad2deg(anglediff);
    if (x.pos)
      x.obj.pos = x.pos.sub(x.offset).add(x.offset.rotate(anglediff));
  });
}

editor.inputs.mouse.scroll = function(scroll)
{
  scroll.y *= -1;
  editor.grabselect?.forEach(function(x) {
    x.pos = x.pos.add(scroll.scale(editor.camera.zoom));
  });

  scroll.y *= -1;
  editor.camera.pos = editor.camera.pos.sub(scroll.scale(editor.camera.zoom * 3).scale([1,-1]));  
}

editor.inputs.mouse['C-scroll'] = function(scroll)
{
  editor.camera.zoom += scroll.y/100;
}

editor.inputs['C-M-S-lm'] = function() { editor.selectlist[0].set_center(Mouse.worldpos); };
editor.inputs['C-M-S-lm'].doc = "Set world center to mouse position.";

editor.inputs.delete = function() {
  this.selectlist.forEach(x => x.kill());
  this.unselect();
};
editor.inputs.delete.doc = "Delete selected objects.";
editor.inputs['S-d'] = editor.inputs.delete;
editor.inputs['C-k'] = editor.inputs.delete;

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
  editor.try_pick();
};
editor.inputs.g.doc = "Move selected objects.";
editor.inputs.g.released = function() { editor.grabselect = []; Mouse.normal(); };

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
  this.killcom = find_com(this.selectlist);  

  this.selectlist.forEach(function(x) {
    this.killring.push(x.make_ur());
  },this);
};
editor.inputs['C-c'].doc = "Copy selected objects to killring.";

editor.inputs['C-x'] = function() {
  editor.inputs['C-c'].call(editor);
  this.selectlist.forEach(function(x) { x.kill(); });
  editor.unselect();
};
editor.inputs['C-x'].doc = "Cut objects to killring.";

editor.inputs['C-v'] = function() {
    this.unselect();
    this.killring.forEach(function(x) {
      editor.selectlist.push(editor.edit_level.spawn(x));
    });

    this.selectlist.forEach(function(x) {
      x.pos = x.pos.sub(this.killcom).add(this.cursor);
    },this);
};
editor.inputs['C-v'].doc = "Pull objects from killring to world.";

editor.inputs.char = function(c) {
  
};

var brushmode = {};
brushmode.inputs = {};
brushmode.inputs.lm = function() { editor.inputs['C-v'].call(editor); };
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

editor.scalelist = [];
editor.inputs.s = function() {
  var scaleoffset = Vector.length(Mouse.worldpos.sub(editor.cursor));
  editor.scalelist = [];
  
  if (editor.sel_comp) {
    if (!('scale' in editor.sel_comp)) return;
    Log.warn(`scaling ${editor.sel_comp.toString()}`);
    editor.scalelist.push({
      obj: editor.sel_comp,
      scale: editor.sel_comp.scale,
      scaleoffset: scaleoffset,
    });
    return;
  }

  editor.selectlist.forEach(function(x) {
    editor.scalelist.push({
      obj: x,
      scale: x.scale,
      offset: x.pos.sub(editor.cursor),
      scaleoffset: scaleoffset,
    });
  });
};
editor.inputs.s.doc = "Scale selected.";

editor.inputs.s.released = function() { this.scalelist = []; };

var inputpanel = {
  title: "untitled",
  value: "",
  on: false,
  stolen: {},
  pos:[100,Window.height-50],
  wh:[350,600],
  anchor: [0,1],
  padding:[5,-15],

  gui() {
    this.win ??= Mum.window({width:this.wh.x,height:this.wh.y, color:Color.black.alpha(0.1), anchor:this.anchor, padding:this.padding});
    var itms = this.guibody();
    if (!Array.isArray(itms)) itms = [itms];
    if (this.title)
    this.win.items = [
      Mum.column({items: [
        Mum.text({str:this.title}),
	...itms
       ]})
    ];
    else
      this.win.items = itms;
    this.win.draw(this.pos.slice());
  },
  
  guibody() {
    return [
      Mum.text({str:this.value, color:Color.green}),
      Mum.button({str:"SUBMIT", action:this.submit.bind(this)})
    ];
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

  keycb() {},

  caret: 0,

  reset_value() {
    this.value = "";
    this.caret = 0;
  },
  
  input_backspace_pressrep() {
    this.value = this.value.slice(0,-1);
    this.keycb();
  },
};

inputpanel.inputs = {};

inputpanel.inputs.post = function()
{
  this.keycb();
}

inputpanel.inputs.char = function(c) {
  this.value = this.value.slice(0,this.caret) + c + this.value.slice(this.caret);
  this.caret++;
}
inputpanel.inputs['C-d'] = function() { this.value = this.value.slice(0,this.caret) + this.value.slice(this.caret+1); };
inputpanel.inputs['C-d'].rep = true;
inputpanel.inputs.tab = function() {
  this.value = tab_complete(this.value, this.assets);
  this.caret = this.value.length;
}
inputpanel.inputs.escape = function() { this.close(); }
inputpanel.inputs['C-b'] = function() {
  if (this.caret === 0) return;
  this.caret--;
};
inputpanel.inputs['C-b'].rep = true;
inputpanel.inputs['C-u'] = function()
{
  this.value = this.value.slice(this.caret);
  this.caret = 0;
}
inputpanel.inputs['C-f'] = function() {
  if (this.caret === this.value.length) return;
  this.caret++;
};
inputpanel.inputs['C-f'].rep = true;
inputpanel.inputs['C-a'] = function() { this.caret = 0; };
inputpanel.inputs['C-e'] = function() { this.caret = this.value.length; };
inputpanel.inputs.backspace = function() {
  if (this.caret === 0) return;
  this.value = this.value.slice(0,this.caret-1) + this.value.slice(this.caret);
  this.caret--;
};
inputpanel.inputs.backspace.rep = true;
inputpanel.inputs.enter = function() { this.submit(); }

inputpanel.inputs['C-k'] = function() {
  this.value = this.value.slice(0,this.caret);
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

var replpanel = Object.copy(inputpanel, {
  title: "",
  closeonsubmit:false,
  wh: [700,300],
  pos: [50,50],
  anchor: [0,0],
  padding: [0,0],
  scrolloffset: [0,0],

  guibody() {
    this.win.selectable = true;
    var log = cmd(84);
    log = log.slice(-5000);
    
    return [
      Mum.text({str:log, anchor:[0,0], offset:[0,-300].sub(this.scrolloffset), selectable: true}),
      Mum.text({str:this.value,color:Color.green, offset:[0,-290], caret: this.caret})
    ];
  },
  prevmark:-1,
  prevthis:[],

  action() {
    if (!this.value) return;
    this.prevthis.unshift(this.value);
    this.prevmark = -1;
    var ecode = "";
    var repl_obj = (editor.selectlist.length === 1) ? editor.selectlist[0] : editor.edit_level;
    ecode += `var $ = repl_obj.objects;`;
    for (var key in repl_obj.objects)
      ecode += `var ${key} = editor.edit_level.objects['${key}'];`;
	
    ecode += this.value;
    Log.say(this.value);
    this.value = "";
    this.caret = 0;
    var ret = function() {return eval(ecode);}.call(repl_obj);
    Log.say(ret);
  },

  resetscroll() {
    this.scrolloffset.y = 0;  
  },
});

replpanel.inputs = Object.create(inputpanel.inputs);
replpanel.inputs.tab = function() {
  this.resetscroll();
  var obj = globalThis;
  var keys = [];
  var keyobj = this.value.tolast('.');
  var o = this.value.tolast('.');
  var stub = this.value.fromlast('.');  
  var replobj = (editor.selectlist.length === 1) ? "editor.selectlist[0]" : "editor.edit_level";
  
  if (this.value.startswith("this."))
    keyobj = keyobj.replace("this", replobj);

  if (!this.value.includes('.')) keys.push("this");

  if (eval(`typeof ${keyobj.tofirst('.')}`) === 'object' && eval(`typeof ${keyobj.replace('.', '?.')}`) === 'object')
    obj = eval(keyobj);
  else if (this.value.includes('.')){
    Log.say(`${this.value} is not an object.`);
    return;
  } 
   
  for (var k in obj)
    keys.push(k)

  var comp = "";
  if (stub)
    comp = tab_complete(stub, keys);
  else if (!this.value.includes('.'))
    comp = tab_complete(o, keys);
  else
    comp = tab_complete("",keys);
  
  if (stub)
    this.value = o + '.' + comp;
  else if (this.value.endswith('.'))
    this.value = o + '.' + comp;
  else
    this.value = comp;

  this.caret = this.value.length;

  keys = keys.sort();

  keys = keys.map(function(x) {
    if (typeof obj[x] === 'function')
      return Esc.color(Color.Apple.orange) + x + Esc.reset;
    if (Object.isObject(obj[x]))
      return Esc.color(Color.Apple.purple) + x + Esc.reset;    
    if (Array.isArray(obj[x]))
      return Esc.color(Color.Apple.green) + x + Esc.reset;    

    return x;
  });

  if (keys.length > 1)
    Log.console(keys.join(', '));
};
replpanel.inputs['C-p'] = function()
{
  if (this.prevmark >= this.prevthis.length) return;
  this.prevmark++;
  this.value = this.prevthis[this.prevmark];
  this.inputs['C-e'].call(this);
}

replpanel.inputs['C-n'] = function()
{
  this.prevmark--;
  if (this.prevmark < 0) {
    this.prevmark = -1;
    this.value = "";
  } else
    this.value = this.prevthis[this.prevmark];

  this.inputs['C-e'].call(this);
}

replpanel.inputs.mouse = {};
replpanel.inputs.mouse.scroll = function(scroll)
{
  if (!this.win.selected) return;

  this.scrolloffset.y += scroll.y;
  if (this.scrolloffset.y < 0) this.scrolloffset.y = 0;
}

replpanel.inputs.up = function()
{
  this.scrolloffset.y += 40;
}
replpanel.inputs.up.rep = true;
replpanel.inputs.down = function()
{
  this.scrolloffset.y -= 40;
  if (this.scrolloffset.y < 0) this.scrolloffset.y = 0;  
}
replpanel.inputs.down.rep = true;

replpanel.inputs.pgup = function()
{
  this.scrolloffset.y += 300;
}
replpanel.inputs.pgup.rep = true;

replpanel.inputs.pgdown = function()
{
  this.scrolloffset.y -= 300;
  if (this.scrolloffset.y < 0) this.scrolloffset.y = 0;  
}
replpanel.inputs.pgdown.rep = true;

var objectexplorer = Object.copy(inputpanel, {
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

var helppanel = Object.copy(inputpanel, {
  title: "help",
  
  start() {
    this.helptext = slurp("editor.adoc");
  },
  
  guibody() {
    Nuke.label(this.helptext);
  },
});

var openlevelpanel = Object.copy(inputpanel,  {
  title: "open entity",
  action() {
    editor.load(this.value);
  },
  
  assets: [],
  allassets: [],

  mumlist: {},

  submit_check() {
    if (this.assets.length === 0) return false;

    this.value = this.assets[0];
    return true;
  },

  start() {
    this.allassets = prototypes.list.sort();
    this.assets = this.allassets.slice();
    this.caret = 0;
    var click_ur = function(btn) {
      Log.warn(btn.str);
      this.value = btn.str;
      this.keycb();
      this.submit();
    };
    click_ur = click_ur.bind(this);

    this.mumlist = [];
    this.assets.forEach(function(x) {
      this.mumlist[x] = Mum.button({str:x, action:click_ur});
    }, this);
  },

  keycb() {
    this.assets = this.allassets.filter(x => x.search(this.value) !== -1);
    for (var m in this.mumlist)
      this.mumlist[m].hide = true;
    this.assets.forEach(function(x) {
     this.mumlist[x].hide = false;
    }, this);
  },
  
  guibody() {
    var a = [Mum.text({str:this.value,color:Color.green, caret:this.caret})];
    var b = a.concat(Object.values(this.mumlist));
    return Mum.column({items:b, offset:[0,-10]});
  },
});

var saveaspanel = Object.copy(inputpanel, {
  get title() { return `save level as: ${this.stem}.`; },

  action() {
    var savename = "";
    if (this.stem) savename += this.stem + ".";
    editor.saveas_check(savename + this.value, this.obj);
  },
});

var groupsaveaspanel = Object.copy(inputpanel, {
  title: "group save as",
  action() { editor.groupsaveas(editor.selectlist, this.value); }
});

var quitpanel = Object.copy(inputpanel, {
  title: "really quit?",
  action() {
    Game.quit();
  },
  
  guibody () {
    return Mum.text({str: "Really quit?"});
  },
});

var notifypanel = Object.copy(inputpanel, {
  title: "notification",
  msg: "Refusing to save. File already exists.",
  action() {
    this.close();
  },
  
  guibody() {
    return Mum.column({items: [
      Mum.text({str:this.msg}),
      Mum.button({str:"OK", action:this.close.bind(this)})
    ]});
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

var assetexplorer = Object.copy(openlevelpanel, {
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
    if (!val) return val;
    list.dofilter(function(x) { return x.startsWith(val); });

    if (list.length === 1) {
      return list[0];
    }
    
    var ret = undefined;
    var i = val.length;
    while (!ret && !list.empty) {
      var char = list[0][i];
      if (!list.every(function(x) { return x[i] === char; }))
        ret = list[0].slice(0, i);
      else {
        i++;
	list.dofilter(function(x) { return x.length-1 > i; });
      }
    }

    return ret ? ret : val;
}

var texgui = Object.copy(inputpanel, {
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

var entitylistpanel = Object.copy(inputpanel, {
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
      x._ed.selectable = Nuke.checkbox(x._ed.selectable);
      
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
  Sound.killall();
  editor.enter_editor();
  Primum.clear_all();
  editor.load_json(editor.stash);
  Game.view_camera(editor.camera);
}

limited_editor.inputs['M-q'] = function()
{
  editor.enter_editor();
}

/* This is used for editing during a paused game */
var limited_editing = {};
limited_editing.inputs = {};

if (IO.exists("editor.config"))
  load_configs("editor.config");

/* This is the editor level & camera - NOT the currently edited level, but a level to hold editor things */
editor.clear_level();
editor.camera = Game.camera;
Game.stop();
Game.editor_mode(true);

editor.enter_editor();

load("editorconfig.js");
