/*
  Editor-only variables on objects
  selectable
*/
prototypes.generate_ur('.');

var editor = {
  toString() { return "editor"; },
  grid_size: 100,
  ruler_mark_px: 100,
  grid_color: Color.green.alpha(0.3),

  dbg_ur: "arena.level1",
  machine: undefined,
  device_test: undefined,
  selectlist: [],
  scalelist: [],
  rotlist: [],
  camera: undefined,
  edit_level: undefined, /* The current level that is being edited */
  desktop: undefined, /* The editor desktop, where all editing objects live */
  working_layer: 0,
  get cursor() {
    if (this.selectlist.length === 0 ) return Mouse.worldpos;
    return physics.com(this.selectlist.map(x => x.pos));
  },
  edit_mode: "basic",

  get_this() { return this.edit_level; },

  get_that() { return this.selectlist.length === 1 ? this.selectlist[0] : this.get_this(); },

  try_select() { /* nullify true if it should set selected to null if it doesn't find an object */
    var go = physics.pos_query(Mouse.worldpos);
    return this.do_select(go);
  },
    
  /* Tries to select id */
  do_select(obj) {
    if (!obj) return;
    if (!obj._ed.selectable) return undefined;
    
    if (obj.level !== this.edit_level) {
      var testlevel = obj.level;
      while (testlevel && testlevel.level !== Primum && testlevel.level !== this.edit_level && testlevel !== testlevel.level)
         testlevel = testlevel.level;
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

    objs.forEach(x => duped.push(x.dup()));
    return duped;
  },

  sel_start: [],

  mover(amt, snap) {
    return function(go) { go.pos = go.pos.add(amt)};
  },

  step_amt() { return Keys.shift() ? 10 : 1; },

  on_grid(pos) {
    return pos.every(function(x) { return x % editor.grid_size === 0; });
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
      this.selectlist.forEach(this.snapper(dir.scale(1.01), editor.grid_size));
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
    this.stash = this.desktop.instance_obj();
    Primum.clear();
    load("config.js");
    Game.play();
    Player.players[0].uncontrol(this);
    Player.players[0].control(limited_editor);
    Register.unregister_obj(this);
    load("predbg.js");
    console.warn(`starting game with ${this.dbg_ur}`);
    editor.dbg_play = Primum.spawn(this.dbg_ur);
    editor.dbg_play.pos = [0,0];
    load("debug.js");
  },

  start_play() {
    Primum.clear();
    load("config.js");
    Game.play();
    Player.players[0].uncontrol(this);
    Player.players[0].control(limited_editor);
    Register.unregister_obj(this);
    load("game.js");
  },

  enter_editor() {
    Game.pause();
    Player.players[0].control(this);
    Player.players[0].uncontrol(limited_editor);
    Register.gui.register(editor.gui, editor);
    Register.draw.register(editor.draw, editor);
    Debug.register_call(editor.ed_debug, editor);
    Register.update.register(gui_controls.update, gui_controls);
    this.desktop = Primum.spawn(ur.arena);
    Primum.rename_obj(this.desktop.toString(), "desktop");
    this.edit_level = this.desktop;
    editor.edit_level._ed.selectable = false;
    if (this.stash) {
      this.desktop.make_objs(this.stash.objects);
      Object.dainty_assign(this.desktop, this.stash);
    }
    this.selectlist = [];
    editor.camera = Primum.spawn(ur.camera2d);
    editor.camera._ed.selectable = false;
    Game.view_camera(editor.camera);
  },

  end_debug() {
    
  },
   
  openpanel(panel) {
    if (this.curpanel) {
      this.curpanel.close();
      Player.players[0].uncontrol(this.curpanel);
    }
      
    this.curpanel = panel;
    Player.players[0].control(this.curpanel);
    this.curpanel.open();
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

  snapshot() {
    var dif = this.edit_level.json_obj();
    if (!dif) return;
  
    if (this.snapshots.length !== 0) {
      var ddif = ediff(dif, this.snapshots.last());
      if (!ddif) return;
      dif = ddif;
    }

    this.snapshots.push(dif);
    this.backshots = [];
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
  },

  undo() {
    if (this.snapshots.empty) {
      Log.info("Nothing to undo.");
      return;
    }
    this.unselect();
//    this.backshots.push(this.edit_level.save());
    var dd = this.snapshots.pop();
    Object.dainty_assign(this.edit_level, dd);
    this.edit_level.check_dirty();
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
  
  load_desktop(d) {
    
  },

  draw_objects_names(obj,root,depth){
      if (!obj) return;
      if (!obj.objects) return;
      depth ??= 0;
      root = root ? root + "." : root;
      Object.entries(obj.objects).forEach(function(x) {
        var p = root + x[0];
        GUI.text(p, x[1].screenpos(), 1, editor.color_depths[depth]);
	editor.draw_objects_names(x[1], p, depth+1);
      });
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

  color_depths: [],
  draw() {
    this.selectlist.forEach(x => {
      if ('gizmo' in x && typeof x['gizmo'] === 'function' )
        x.gizmo();
    });
  
    Shape.line(bb2points(cwh2bb([0,0],[Game.native.x,Game.native.y])).wrapped(1), Color.yellow);

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
      var bb = cwh2bb(c,wh);
      Debug.boundingbox(bb, Color.Editor.select.alpha(0.1));
      Shape.line(bb2points(bb).wrapped(1), Color.white);
    }
  },

  gui() { 
    /* Clean out killed objects */
    this.selectlist = this.selectlist.filter(function(x) { return x.alive; });
    Debug.coordinate(world2screen([0,0]));
    Shape.cross(Mouse.pos, 5);

    GUI.text("WORKING LAYER: " + this.working_layer, [0,520]);
    GUI.text("MODE: " + this.edit_mode, [0,500]);

    if (this.comp_info && this.sel_comp)
      GUI.text(Input.print_pawn_kbm(this.sel_comp,false), [100,700],1);

    Shape.cross(editor.edit_level.screenpos(),3,Color.blue);

    var thiso = editor.get_this();
    var clvl = thiso;
    
    var lvlchain = [];
    while (clvl !== Primum) {
      lvlchain.push(clvl);
      clvl = clvl.level;
    }
    lvlchain.push(clvl);
    
    var colormap = ColorMap.Inferno;
    editor.color_depths = [];
    for (var i = 1; i > 0 ; i -= 0.1)
      editor.color_depths.push(colormap.sample(i));
    
    var ypos = 200;
    var depth = 0;
    var alldirty = false;
    for (var lvl of lvlchain) {
      if (alldirty)
        lvl._ed.dirty = true;
      else {
        lvl.check_dirty();
        if (lvl._ed.dirty) alldirty = true;
      }
    }
    lvlchain.reverse();
    lvlchain.forEach(function(x,i) {
      depth = i;
      var lvlstr = x.namestr();
      if (i === lvlchain.length-1) lvlstr += "[this]";
      GUI.text(lvlstr, [0, ypos], 1, editor.color_depths[depth]);
     
      GUI.text("^^^^^^", [0,ypos+=5],1);
      ypos += 15;
    });

    depth++;
    GUI.text("$$$$$$", [0,ypos],1,editor.color_depths[depth]);
    
    this.selectlist.forEach(function(x) {
      var sname = x.__proto__.toString();
      x.check_dirty();
      if (x._ed.dirty) sname += "*";
      
      GUI.text(sname, x.screenpos().add([0, 32]), 1, Color.editor.ur);
      GUI.text(x.worldpos().map(function(x) { return Math.round(x); }), x.screenpos(), 1, Color.white);
      Shape.cross(x.screenpos(), 10, Color.blue);
    });

    Object.entries(thiso.objects).forEach(function(x) {
      var p = x[1].namestr();
      GUI.text(p, x[1].screenpos().add([0,16]),1,editor.color_depths[depth]);
      Shape.circle(x[1].screenpos(),10,Color.blue.alpha(0.3));
    });

    var mg = physics.pos_query(Mouse.worldpos,10);
    
    if (mg) {
      var p = mg.path_from(thiso);
      GUI.text(p, Mouse.screenpos(),1,Color.teal);
    }

    if (this.rotlist.length === 1)
      GUI.text(Math.trunc(this.rotlist[0].obj.angle), Mouse.screenpos(), 1, Color.teal);

    if (this.selectlist.length === 1) {
      var i = 1;
      for (var key in this.selectlist[0].components) {
        var selected = this.sel_comp === this.selectlist[0].components[key];
        var str = (selected ? ">" : " ") + key + " [" + this.selectlist[0].components[key].toString() + "]";
        GUI.text(str, this.selectlist[0].screenpos().add([0,-16*(i++)]));
      }

      if (this.sel_comp) {
        if ('gizmo' in this.sel_comp) this.sel_comp.gizmo();
      }
    }

    editor.edit_level.objects.forEach(function(obj) {
      if (!obj._ed.selectable)
        GUI.image("icons/icons8-lock-16.png", obj.screenpos());
    });

    Debug.draw_grid(1, editor.grid_size, Color.Editor.grid.alpha(0.3));
    var startgrid = screen2world([-20,0]).map(function(x) { return Math.snap(x, editor.grid_size); });
    var endgrid = screen2world([Window.width, Window.height]);
    
    var w_step = Math.round(editor.ruler_mark_px/Window.width * (endgrid.x-startgrid.x)/editor.grid_size)*editor.grid_size;
    if (w_step === 0) w_step = editor.grid_size;
    
    var h_step = Math.round(editor.ruler_mark_px/Window.height * (endgrid.y-startgrid.y)/editor.grid_size)*editor.grid_size;
    if (h_step === 0) h_step = editor.grid_size;
    
    while(startgrid[0] <= endgrid[0]) {
      GUI.text(startgrid[0], [world2screen([startgrid[0], 0])[0],0]);
      startgrid[0] += w_step;
    }

    while(startgrid[1] <= endgrid[1]) {
      GUI.text(startgrid[1], [0, world2screen([0, startgrid[1]])[1]]);
      startgrid[1] += h_step;
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
  
  killring: [],
  killcom: [],

  lvl_history: [],

  load(file) {
    var ur = prototypes.get_ur(file);
    if (!ur) return;
    var obj = editor.edit_level.spawn(ur);
    obj.set_worldpos(Mouse.worldpos);
    this.selectlist = [obj];
  },

  load_prev() {
    if (this.lvl_history.length === 0) return;

    var file = this.lvl_history.pop();
    this.edit_level = Level.loadfile(file);
    this.unselect();
  },
  
  /* Checking to save an entity as a subtype. */
  /* sub is the name of the (sub)type; obj is the object to save it as */
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
        if (obj === editor.desktop) {
	  obj.clear();
    	  var nobj = editor.edit_level.spawn(sub);
	  editor.selectlist = [nobj];
	  return;
	}
	editor.edit_level = editor.edit_level.level;
      }

      var t = obj.transform();
      editor.unselect();
      obj.kill();
      obj = editor.edit_level.spawn(sub);
      obj.pos = t.pos;
      obj.angle = t.angle;
    }
  },
}

editor.inputs = {};
editor.inputs.drop = function(str) {
  if (!Resources.is_image(str)) {
    console.warn("NOT AN IMAGE");
    return;
  }

  if (this.selectlist.length === 0) {
    
  }
  
  if (this.sel_comp?.comp === 'sprite') {
    this.sel_comp.path = str;
    return;
  }
  
  var mg = physics.pos_query(Mouse.worldpos,10);
  if (!mg) return;
  var img = mg.get_comp_by_name('sprite');
  if (!img) return;
  img[0].path = str;
}

editor.inputs.f9 = function() {
  Log.warn("CAPTURING");
  cmd(173, "capture.bmp", 0, 0, 500, 500);
}

editor.inputs.release_post = function() {
  editor.snapshot();
  editor.edit_level.check_dirty();

  /* snap all objects to be pixel perfect */
  editor.edit_level.obj_descend(o => o.pos = o.pos.map(x => Math.round(x)));
};
editor.inputs['C-a'] = function() {
  if (!editor.selectlist.empty) { editor.unselect(); return; }
  editor.unselect();
  editor.selectlist = editor.edit_level.objects.slice();
};
editor.inputs['C-a'].doc = "Select all objects.";

editor.inputs['C-`'] = function() { editor.openpanel(replpanel); }
editor.inputs['C-`'].doc = "Open or close the repl.";

editor.inputs.n = function() {
  if (editor.selectlist.length !== 1) return;
  var o = editor.try_select();
  if (!o) return;
  if (o === editor.selectlist[0]) return;
  if (o.level !== editor.selectlist[0].level) return;

  var tpos = editor.selectlist[0].pos;
  tpos.x *= -1;
  o.pos = tpos;
};
editor.inputs.n.doc = "Set the hovered object's position to mirror the selected object's position on the X axis."
editor.inputs['M-n'] = function()
{
  if (editor.selectlist.length !== 1) return;
  var o = editor.try_select();
  if (!o) return;
  if (o === editor.selectlist[0]) return;
  if (o.level !== editor.selectlist[0].level) return;

  var tpos = editor.selectlist[0].pos;
  tpos.y *= -1;
  o.pos = tpos;
};
editor.inputs.n.doc = "Set the hovered object's position to mirror the selected object's position on the Y axis."

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

editor.inputs['C-i'] = function() {
  if (editor.selectlist.length !== 1) return;
  objectexplorer.obj = editor.selectlist[0];
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
    if ('flipy' in editor.sel_comp)
      editor.sel_comp.flipy = !editor.sel_comp.flipy;

    return;
  }
  
  editor.selectlist.forEach(function(x) { x.mirror([0,1]);});
};
editor.inputs['C-m'].doc = "Mirror selected objects on the Y axis.";

editor.inputs.m = function() {
  if (editor.sel_comp) {
    if ('flipx' in editor.sel_comp)
      editor.sel_comp.flipx = !editor.sel_comp.flipx;

    return;
  }

  editor.selectlist.forEach(obj => obj.scale = [-obj.scale[0], obj.scale[1]]);
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
  editor.reset_undos();  
};
editor.inputs['C-f'].doc = "Tunnel into the selected level object to edit it.";

editor.inputs['C-F'] = function() {
  if (editor.edit_level.level === Primum) return;

  editor.edit_level = editor.edit_level.level;
  editor.unselect();
  editor.reset_undos();
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

editor.inputs.f5 = function() { editor.start_play_ed(); }
editor.inputs.f5.doc = "Start game from 'debug' if it exists; otherwise, from 'game'.";

editor.inputs.f6 = function() { editor.start_play(); }
editor.inputs.f6.doc = "Start game as if the player started it.";

editor.inputs['M-p'] = function() {
  if (Game.playing())
    Game.pause();

  Game.step();
}
editor.inputs['M-p'].doc = "Do one time step, pausing if necessary.";

editor.inputs['C-M-p'] = function() {
  if (!Game.playing()) {
    editor.start_play_ed();
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
  var saveobj = undefined;
  if (editor.selectlist.length === 0) {
    if (editor.edit_level === editor.desktop) {
      saveaspanel.stem = editor.edit_level.ur.toString();
      saveaspanel.obj = editor.edit_level;
      editor.openpanel(saveaspanel);
      return;
    } else
      saveobj = editor.edit_level;
  } else if (editor.selectlist.length === 1)
    saveobj = editor.selectlist[0];

//  saveobj.check_dirty();
//  if (!saveobj._ed.dirty) return;

  var savejs = saveobj.json_obj();
  Object.merge(saveobj.__proto__, savejs);
  if (savejs.objects) saveobj.__proto__.objects = savejs.objects;
  var path = prototypes.ur_stem(saveobj.ur.toString()) + ".json";

  IO.slurpwrite(JSON.stringify(saveobj.__proto__,null,1), path);
  Log.warn(`Wrote to file ${path}`);

  Object.values(saveobj.objects).forEach(function(x) { x.check_dirty(); });

  Game.all_objects(function(x) {
    if (typeof x !== 'object') return;
    if (!('_ed' in x)) return;
    if (x._ed.dirty) return;
    x.revert();
    x.check_dirty();
  });
};
editor.inputs['C-s'].doc = "Save selected.";

editor.inputs['C-S'] = function() {
  if (editor.selectlist.length !== 1) return;
  saveaspanel.stem = this.selectlist[0].ur;
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
  
  return;
};
editor.inputs['C-n'].doc = "Create an empty object.";

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
  if (this.selectlist.length !== 1) return;
  objectexplorer.obj = this.selectlist[0];
  this.openpanel(objectexplorer);
};
editor.inputs.f2.doc = "Open configurations object.";

editor.inputs.f3 = function() {
  if (this.selectlist.length !== 1) return;
  this.openpanel(componentexplorer);
};

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
    if (Vector.equal(Mouse.worldpos, editor.sel_start, 5)) {
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
      });
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

  if (editor.sel_comp && 'pick' in editor.sel_comp)
    return editor.sel_comp.pick(Mouse.worldpos);

  return editor.try_select();
}

editor.inputs.mm = function() {
    if (editor.brush_obj) {
      editor.selectlist = editor.dup_objects([editor.brush_obj]);
      editor.selectlist[0].pos = Mouse.worldpos;
      editor.grabselect = editor.selectlist[0];
      return;
    }

  var o = editor.try_pick();
  if (!o) return;
//  editor.selectlist = [o];
  editor.grabselect = [o];
};
editor.inputs['C-mm'] = editor.inputs.mm;

editor.inputs['C-M-lm'] = function()
{
  var go = physics.pos_query(Mouse.worldpos);
  if (!go) return;
  editor.edit_level = go.level;
}

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
      editor.camera.pos = editor.camera.pos.sub(Game.camera.dir_view2world(dpos));
  }

  editor.grabselect?.forEach(function(x) {
    if (!x) return;
    x.move(Game.camera.dir_view2world(dpos));
    x.sync();
  });
  
  var relpos = Mouse.worldpos.sub(editor.cursor);
  var dist = Vector.length(relpos);

  editor.scalelist?.forEach(function(x) {
    var scalediff = dist / x.scaleoffset;
    if (typeof x.obj.scale === 'number')
      x.obj.scale = x.scale * scalediff;
    else {
      x.obj.scale = x.scale.map(x=> x * scalediff);
      if (x.offset)
        x.obj.pos = editor.cursor.add(x.offset.scale(scalediff));
    }
  });

  editor.rotlist?.forEach(function(x) {
    var anglediff = Math.atan2(relpos.y, relpos.x) - x.rotoffset;
    x.obj.angle = x.angle + Math.rad2turn(anglediff);
    if (Keys.shift())
      x.obj.angle = Math.nearest(x.obj.angle, (1/24));
    if (x.pos)
      x.obj.pos = x.pos.sub(x.offset).add(x.offset.rotate(anglediff));
  });
}

editor.inputs.mouse.scroll = function(scroll)
{
  scroll.y *= -1;
  editor.camera.move(Game.camera.dir_view2world(scroll.scale(-3)));
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
  if (editor.selectlist.length === 0) {
    var o = editor.try_pick();
    if (!o) return;
    editor.selectlist = [o];
  }
  
  if (editor.sel_comp) {
    if ('pick' in editor.sel_comp) {
      editor.grabselect = [editor.sel_comp.pick(Mouse.worldpos)];
      return;
    }

    if ('pos' in editor.sel_comp) {
      var comp = editor.sel_comp;
      var o = {
        pos: editor.sel_comp.pos,
	move(d) { comp.pos = comp.pos.add(comp.gameobject.dir_world2this(d)); },
	sync: comp.sync.bind(comp),
      };
      editor.grabselect = [o];
      return;
    }
  }

  if (editor.sel_comp && 'pick' in editor.sel_comp) {
    var o = editor.sel_comp.pick(Mouse.worldpos);
    if (o) editor.grabselect = [o];
    return;
  }
    
  editor.grabselect = editor.selectlist.slice();
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
  if (!(this.selectlist.length === 1)) return;
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

editor.inputs['M-g'] = function()
{
  if (this.sel_comp && 'pick_all' in this.sel_comp)
    this.grabselect = this.sel_comp.pick_all();
}
editor.inputs['M-g'].doc = "Move all.";

editor.inputs['C-lb'] = function() {
  editor.grid_size -= Keys.shift() ? 10 : 1;
  if (editor.grid_size <= 0) editor.grid_size = 1;
};
editor.inputs['C-lb'].doc = "Decrease grid size. Hold shift to decrease it more.";
editor.inputs['C-lb'].rep = true;

editor.inputs['C-rb'] = function() { editor.grid_size += Keys.shift() ? 10 : 1; };
editor.inputs['C-rb'].doc = "Increase grid size. Hold shift to increase it more.";
editor.inputs['C-rb'].rep = true;

editor.inputs['C-c'] = function() {
  this.killring = [];
  this.killcom = [];
  this.killcom = physics.com(this.selectlist.map(x=>x.pos));  

  this.selectlist.forEach(function(x) {
    this.killring.push(x.instance_obj());
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
  if (c === '0') {
    this.camera.pos = [0,0];
    this.camera.zoom = 1;
  }
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
  toString() { return this.title; },  
  value: "",
  on: false,
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
  
  open() {
    this.on = true;
    this.value = "";
    this.start();
    this.keycb();
  },
  
  start() {},
  
  close() {
    Player.players[0].uncontrol(this);
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

inputpanel.inputs.lm = function()
{
  gui_controls.check_submit();
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
    var repl_obj = editor.get_this();
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
replpanel.inputs.block = true;
replpanel.inputs.lm = function()
{
  var mg = physics.pos_query(Mouse.worldpos);
  if (!mg) return;
  var p = mg.path_from(editor.get_this());
  this.value = p;
  this.caret = this.value.length;
}
replpanel.inputs.tab = function() {
  this.resetscroll();
  if (!this.value) return;
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

  for (var k in editor.get_this())
    keys.push(k);

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
    Log.repl(keys.join(', '));
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

replpanel.inputs.mm = function() { this.mm = true; };
replpanel.inputs.mm.released = function() { this.mm = false; };

replpanel.inputs.mouse.move = function(pos,dpos)
{
  if (this.mm)
    this.scrolloffset.y -= dpos.y;
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
  },

  goto_obj(obj) {
    if (obj === this.obj) return;
    this.previous.push(this.obj);
    this.obj = obj;
  },

  prev_obj() {
    this.obj = this.previous.pop();
  },

  guibody() {
    var items = [];
    items.push(Mum.text({str:"Examining " + this.obj.toString() + " entity"}));
    items.push(Mum.text({str: JSON.stringify(this.obj,undefined,1)}));
    return items;
      
    var n = 0;
    var curobj = this.obj;
    while (curobj) {
      n++;
      curobj = curobj.__proto__;
    }

    n--;
    curobj = this.obj.__proto__;
    while (curobj) {
      items.push(Mum.text({str:curobj.toString(), action:this.goto_obj(curobj)}));
      curobj = curobj.__proto__;
    }

    if (!this.previous.empty)
      items.push(Mum.text({str:"prev: " + this.previous.last(), action: this.prev_obj}));

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

      switch (typeof val) {
        case 'object':
          if (val) {
	    if (Array.isArray(val)) {
//	      this.obj[key] = Nuke.pprop(key,val);
	      break;
	    }
	      
//	    Nuke.newline(2);
	    items.push(Mum.text({str:name}));
	    items.push(Mum.text({str:val.toString(), action: this.goto_obj.bind(val)}));
	  } else {
//    	      this.obj[key] = Nuke.pprop(key,val);
	  }
	  break;

	case 'function':
	  items.push(Mum.text({str:name}));
	  items.push(Mum.text({str:"function"}));
	  break;

	default:
	  if (!hidden) {// && Object.getOwnPropertyDescriptor(this.obj, key).writable) {
//	    this.obj[key] = Nuke.pprop(key, this.obj[key]);

/*	    if (key in this.obj.__proto__) {
	      if (Nuke.button("delete " + key)) {
	        if ("_"+key in this.obj)
		  delete this.obj["_"+key];
		else
             	  delete this.obj[key];
              }
	    }*/
	  }
	  else {
	    items.push(Mum.text({str:name}));
	    items.push(Mum.text({str:val.toString()}));
	  }
	  break;
      }
    });

    items.push(Mum.text({str:"Properties that can be pulled in ..."}));
//    Nuke.newline(3);
    var pullprops = [];
    for (var key in this.obj.__proto__) {
      if (!this.obj.hasOwn(key)) {
        if (typeof this.obj[key] === 'object' || typeof this.obj[key] === 'function') continue;
	pullprops.push(key);
      }
    }

    pullprops = pullprops.sort();

    pullprops.forEach(function(key) {
      items.push(Mum.text({str:key}));
//        this.obj[key] = this.obj[key];
    });

    return items;
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
      this.value = btn.str;
      this.keycb();
      this.submit();
    };
    click_ur = click_ur.bind(this);

    this.mumlist = [];
    this.assets.forEach(function(x) {
      this.mumlist[x] = Mum.text({str:x, action:click_ur, color: Color.blue, hovered: {color:Color.red}, selectable:true});
    }, this);
  },

  keycb() {
    if(this.value)
      this.assets = this.allassets.filter(x => x.startswith(this.value));
    else
      this.assets = this.allassets.slice();
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

var allfiles = [];
allfiles.push(Resources.scripts, Resources.images, Resources.sounds);
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

var componentexplorer = Object.copy(inputpanel, {
  title: "component menu",
  assets: ['sprite', 'model', 'edge2d', 'polygon2d', 'circle2d'],
  click(name) {
    if (editor.selectlist.length !== 1) return;
    editor.selectlist[0].add_component(component[name]);
  },
  guibody() {
    return componentexplorer.assets.map(x => Mum.text({str:x, action:this.click, color: Color.blue, hovered:{Color:Color.red}, selectable:true}));
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
  Primum.clear();
  load("editorconfig.js");
  load("dbgret.js");
  
  editor.enter_editor();
}

/* This is used for editing during a paused game */
var limited_editing = {};
limited_editing.inputs = {};

if (IO.exists("editor.config"))
  load_configs("editor.config");

/* This is the editor level & camera - NOT the currently edited level, but a level to hold editor things */
Game.stop();
Game.editor_mode(true);
Debug.draw_phys(true);
