var inputpanel = {
  title: "untitled",
  toString() { return this.title; },  
  value: "",
  on: false,
  pos:[20,window.size.y-20],
  wh:[100,100],
  anchor: [0,1],
  padding:[5,-15],

  gui() {
    this.win ??= Mum.window({width:this.wh.x,height:this.wh.y, color:Color.black.alpha(0.1), anchor:this.anchor, padding:this.padding});
    var itms = this.guibody();
    if (!Array.isArray(itms)) itms = [itms];
    if (this.title)
      this.win.items = [
        Mum.column({items: [Mum.text({str:this.title}), ...itms ]})
      ];
    else
      this.win.items = itms;
      
    this.win.draw([100, window.size.y-50]);
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
    player[0].uncontrol(this);
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
inputpanel.inputs.block = true;

inputpanel.inputs.post = function() { this.keycb(); }

inputpanel.inputs.char = function(c) {
  this.value = this.value.slice(0,this.caret) + c + this.value.slice(this.caret);
  this.caret++;
}
inputpanel.inputs['C-d'] = function() { this.value = this.value.slice(0,this.caret) + this.value.slice(this.caret+1); };
inputpanel.inputs['C-d'].rep = true;
inputpanel.inputs.tab = function() {
  this.value = input.tabcomplete(this.value, this.assets);
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

inputpanel.inputs.lm = function() { gui.controls.check_submit(); }

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

var listpanel = Object.copy(inputpanel,  {
  assets: [],
  allassets: [],
  mumlist: {},

  submit_check() {
    if (this.assets.length === 0) return false;

    this.value = this.assets[0];
    return true;
  },

  start() {
    this.assets = this.allassets.slice();
    this.caret = 0;
    this.mumlist = [];
    this.assets.forEach(function(x) {
      this.mumlist[x] = Mum.text({str:x, action:this.action, color: Color.blue, hovered: {color:Color.red}, selectable:true});
    }, this);
  },

  keycb() {
    if(this.value)
      this.assets = this.allassets.filter(x => x.startsWith(this.value));
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

return {inputpanel, gen_notify, notifypanel, listpanel}