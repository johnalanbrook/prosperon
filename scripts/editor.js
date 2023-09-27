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
  dbgdraw: false,
  selected: undefined,
  selectlist: [],
  grablist: [],
  scalelist: [],
  rotlist: [],
  camera: undefined,
  edit_level: undefined, /* The current level that is being edited */
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
    if (!obj || !obj.selectable) return undefined;
    
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
    this.edit_level.toString = function() { return "desktop"; };
    editor.edit_level.selectable = false;
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
    
    var clvl = this.edit_level;
    var lvlcolorsample = 1;
    var ypos = 200;
    while (clvl) {
      var lvlstr = clvl.toString();
      if (clvl.dirty)
        lvlstr += "*";
      GUI.text(lvlstr, [0, ypos], 1, ColorMap.Inferno.sample(lvlcolorsample));
      lvlcolorsample -= 0.1;

      if (!clvl.level) break;
      clvl = clvl.level;
      if (clvl) {
        GUI.text("^^^^^^", [0,ypos-15],1);
	ypos -= 15;
      }
      ypos -= 5;
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
      var color = x.color ? x.color : Color.white;
      var sname = x.ur.toString();
      if (!x.json_obj().empty)
        x.dirty = true;
      else
        x.dirty = false;
      if (x.dirty) sname += "*";
      GUI.text(sname, world2screen(x.pos).add([0, 16]), 1, Color.purple);
      for (var key in x.$) {
        var o = x.$[key];
        GUI.text(o.ur.toString(), world2screen(o.pos).add([0,16]),1,Color.purple);
        GUI.text(key, world2screen(o.pos), 1, Color.green);
      }
      
      GUI.text(x.pos.map(function(x) { return Math.round(x); }), world2screen(x.pos), 1, color);
      Debug.arrow(world2screen(x.pos), world2screen(x.pos.add(x.up().scale(40))), Color.yellow, 1);

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

    editor.edit_level.objects.forEach(function(obj) {
      if (!obj.selectable)
        GUI.image("icons/icons8-lock-16.png", world2screen(obj.pos));
    });

    Debug.draw_grid(1, editor_config.grid_size, Color.Editor.grid.alpha(0.3));
    var startgrid = screen2world([-20,Window.height]).map(function(x) { return Math.snap(x, editor_config.grid_size); });
    var endgrid = screen2world([Window.width, 0]);
    
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
  saveas_check(sub) {
    if (!sub) return;
    var curur = prototypes.get_ur(sub);
    
    if (curur) {
      notifypanel.action = editor.saveas;
      this.openpanel(gen_notify("Entity already exists with that name. Overwrite?", this.saveas.bind(this, sub)));
    } else {
      var path = sub.replaceAll('.', '/') + ".json";
      IO.slurpwrite(JSON.stringify(editor.selectlist[0],null,1), path);
      var t = editor.selectlist[0].transform();
      editor.selectlist[0].kill();
      editor.unselect();
      editor.load(sub);
      editor.selectlist[0].pos = t.pos;
      editor.selectlist[0].angle = t.angle;
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
  if (!editor.edit_level.level) return;

  editor.edit_level = editor.edit_level.level;
  editor.unselect();
  editor.reset_undos();
  editor.curlvl = editor.edit_level.save();
  editor.edit_level.filejson = editor.edit_level.save();
  editor.edit_level.check_dirty();
};
editor.inputs['C-F'].doc = "Tunnel out of the level you are editing, saving it in the process.";

editor.inputs['C-r'] = function() { editor.selectlist.forEach(function(x) { x.angle = -x.angle; }); };
editor.inputs['C-r'].doc = "Negate the selected's angle.";

editor.inputs.r = function() {
  if (editor.sel_comp && 'angle' in editor.sel_comp) {
    var relpos = Mouse.worldpos.sub(editor.sel_comp.gameobject.pos);
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
  if (editor.selectlist.length !== 1 || !editor.selectlist[0].dirty) return;
  Log.warn(JSON.stringify(editor.selectlist[0],null,1));
  Log.warn(JSON.stringify(editor.selectlist[0].ur,null,1));  
  Object.merge(editor.selectlist[0].ur, editor.selectlist[0].json_obj());
  Log.warn(JSON.stringify(editor.selectlist[0].ur,null,1));
  var path = editor.selectlist[0].toString();
  path = path.replaceAll('.','/');
  path = path + "/" + path.name() + ".json";
  IO.slurpwrite(JSON.stringify(editor.selectlist[0].ur,null,1), path);
  return;

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
/*  if (editor.edit_level.dirty) {
    editor.openpanel(gen_notify("Level is changed. Are you sure you want to close it?", function() {
      editor.clear_level();
      editor.openpanel(openlevelpanel);
    }.bind(editor)));
    return;
  }
*/
  editor.openpanel(openlevelpanel);
};
editor.inputs['C-o'].doc = "Open a level.";

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

editor.inputs.lm = function() { editor.sel_start = Mouse.worldpos; };
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

editor.inputs.mm = function() {
    if (editor.brush_obj) {
      editor.selectlist = editor.dup_objects([editor.brush_obj]);
      editor.selectlist[0].pos = Mouse.worldpos;
      editor.grabselect = editor.selectlist[0];
      return;
    }
    
    if (editor.sel_comp && 'pick' in editor.sel_comp) {
      editor.grabselect = [editor.sel_comp.pick(Mouse.worldpos)];
      return;
    }

    var grabobj = editor.try_select();
    editor.grabselect = [];
    if (!grabobj) return;
    
    if (Keys.ctrl()) {
      grabobj = editor.dup_objects([grabobj])[0];
    }

    editor.grabselect = [grabobj];
    editor.selectlist = [grabobj];
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
}

editor.inputs.mouse['C-scroll'] = function(scroll)
{
  editor.camera.pos = editor.camera.pos.sub(scroll.scale(editor.camera.zoom * 3).scale([1,-1]));  
}

editor.inputs.mouse['C-M-scroll'] = function(scroll)
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
  editor.grabselect = [];
  
  if (this.sel_comp) {
    if ('pos' in this.sel_comp)
      editor.grabselect = [this.sel_comp];
  } else     
    editor.grabselect = editor.selectlist.slice();

  if (!editor.grabselect.empty)
    Mouse.disabled();
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

  keycb() {},
  
  input_backspace_pressrep() {
    this.value = this.value.slice(0,-1);
    this.keycb();
  },
};

inputpanel.inputs = {};
inputpanel.inputs.char = function(c) { this.value += c; this.keycb(); }
inputpanel.inputs.tab = function() { this.value = tab_complete(this.value, this.assets); }
inputpanel.inputs.escape = function() { this.close(); }
inputpanel.inputs.backspace = function() { this.value = this.value.slice(0,-1); this.keycb(); };
inputpanel.inputs.backspace.rep = true;
inputpanel.inputs.enter = function() { this.submit(); }

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
  title: "REPL",
  closeonsubmit:false,
  guibody() {
    Nuke.newrow(400);
    var log = cmd(84);
    var f = log.prev('\n', 0,10);
    Nuke.scrolltext(log.slice(f));
    Nuke.newrow(30);
    this.value = Nuke.textbox(this.value);
  },

  action() {
    var ecode = "";  
    if (editor.selectlist.length === 1) 
      for (var key in editor.selectlist[0].$)
        ecode += `var ${key} = editor.selectlist[0].$['${key}'];`;
	
    ecode += this.value;
    Log.say(this.value);
    this.value = "";
    var ret = function() {return eval(ecode);}.call(editor.selectlist[0]);
    if (ret) Log.say(ret);
  },
});

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

  submit_check() {
    if (this.assets.length === 0) return false;

    this.value = this.assets[0];
    return true;
  },
  
  start() {
    this.allassets = prototypes.list.sort();
    this.assets = this.allassets.slice();
  },

  keycb() { this.assets = this.allassets.filter(x => x.search(this.value) !== -1); },
  
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

var saveaspanel = Object.copy(inputpanel, {
  title: "save level as",
  action() {
    editor.saveas_check(this.stem + "." + this.value);
  },
});

var groupsaveaspanel = Object.copy(inputpanel, {
  title: "group save as",
  action() { editor.groupsaveas(editor.selectlist, this.value); }
});

var savetypeas = Object.copy(inputpanel, {
  title: "save type as",
  action() {
    editor.save_type_as(this.value);
  },
});

var quitpanel = Object.copy(inputpanel, {
  title: "really quit?",
  action() {
    Game.quit();
  },
  
  guibody () {
    Nuke.label("Really quit?");
    Nuke.newline(2);
    if (Nuke.button("yes"))
      this.submit();
  },
});

var notifypanel = Object.copy(inputpanel, {
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
  Game.view_camera(editor.camera);
}

/* This is used for editing during a paused game */
var limited_editing = {};
limited_editing.inputs = {};

Player.players[0].control(editor);
Register.gui.register(editor.ed_gui, editor);
Debug.register_call(editor.ed_debug, editor);

if (IO.exists("editor.config"))
  load_configs("editor.config");

/* This is the editor level & camera - NOT the currently edited level, but a level to hold editor things */
editor.clear_level();
editor.camera = Game.camera;
Game.stop();
Game.editor_mode(true);

load("editorconfig.js");
