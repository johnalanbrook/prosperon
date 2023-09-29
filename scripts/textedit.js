var texteditor = Object.copy(inputpanel, {
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

  copy(start, end) {
    return this.value.slice(start,end);
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

  line_starting_whitespace(p) {
    var white = 0;
    var l = this.line_start(p);

    while (this.value[l] === " ") {
      white++;
      l++;
    }

    return white;
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
});

texteditor.inputs = {};
texteditor.inputs.char = function(char) {
  this.insert_char(char);
  this.keycb();
},

texteditor.inputs.enter = function(){
  var white = this.line_starting_whitespace(this.cursor);
  this.insert_char('\n');

  for (var i = 0; i < white; i++)
    this.insert_char(" ");
};
texteditor.inputs.enter.rep = true;

texteditor.inputs.backspace = function(){
  this.value = this.value.slice(0,this.cursor-1) + this.value.slice(this.cursor);
  this.cursor--;
};
texteditor.inputs.backspace.rep = true;


texteditor.inputs['C-s'] = function() {
  editor.edit_level.script = texteditor.value;
  editor.save_current();
  texteditor.startbuffer = texteditor.value.slice();
};
texteditor.inputs['C-s'].doc = "Save script to file.";

texteditor.inputs['C-u'] = function() { this.popstate(); };
texteditor.inputs['C-u'].doc = "Undo.";

texteditor.inputs['C-q'] = function() {
  var ws = this.prev_word(this.cursor);
  var we = this.end_of_word(this.cursor)+1;
  var find = this.copy(ws, we);
  var obj = editor.edit_level.varname2obj(find);

  if (obj) {
    editor.unselect();
    editor.selectlist.push(obj);
  }
};
texteditor.inputs['C-q'].doc = "Select object of selected word.";

texteditor.inputs['C-o'] = function() {
  this.insert_char('\n');
  this.cursor--;
};
texteditor.inputs['C-o'].doc = "Insert newline.";
texteditor.inputs['C-o'].rep = true;

texteditor.inputs['M-o'] = function() {
  while (this.line_blank(this.next_line(this.cursor)))
    this.delete_line(this.next_line(this.cursor));

  while (this.line_blank(this.prev_line(this.cursor)))
    this.delete_line(this.prev_line(this.cursor));
};
texteditor.inputs['M-o'].doc = "Delete surround blank lines.";

texteditor.inputs['C-d'] = function () { this.value = this.value.slice(0,this.cursor) + this.value.slice(this.cursor+1); };
texteditor.inputs['C-d'].doc = "Delete character.";

texteditor.inputs['M-d'] = function() { this.cut_span(this.cursor, this.end_of_word(this.cursor)+1); };
texteditor.inputs['M-d'].doc = "Delete word.";

texteditor.inputs['C-a'] = function() {
  this.to_line_start();
  this.desired_inset = this.inset;
};
texteditor.inputs['C-a'].doc = "To start of line.";

texteditor.inputs['C-y'] = function() {
  if (this.killring.length === 0) return;
  this.insert_char(this.killring.pop());
};
texteditor.inputs['C-y'].doc = "Insert from killring.";

texteditor.inputs['C-e'] = function() {
  this.to_line_end();
  this.desired_inset = this.inset;
};
texteditor.inputs['C-e'].doc = "To line end.";

texteditor.inputs['C-k'] = function() {
  if (this.cursor === this.value.length-1) return;
  var killamt = this.value.next('\n', this.cursor) - this.cursor;
  var killed = this.cut_span(this.cursor-1, this.cursor+killamt);
  this.killring.push(killed);
};
texteditor.inputs['C-k'].doc = "Kill from cursor to end of line.";

texteditor.inputs['M-k'] = function() {
  var prevn = this.value.prev('\n', this.cursor);
  var killamt = this.cursor - prevn;
  var killed = this.cut_span(prevn+1, prevn+killamt);
  this.killring.push(killed);
  this.to_line_start();
};
texteditor.inputs['M-k'].doc = "Kill entire line the cursor is on.";

texteditor.inputs['C-b'] = function() {
  this.cursor--;
  this.desired_inset = this.inset;
};
texteditor.inputs['C-b'].rep = true;
texteditor.inputs['M-b'] = function() {
  this.cursor = this.prev_word(this.cursor-2);
  this.desired_inset = this.inset;
};
texteditor.inputs['M-b'].rep = true;

texteditor.inputs['C-f'] = function() {
  this.cursor++;
  this.desired_inset = this.inset;
};
texteditor.inputs['C-f'].rep = true;
texteditor.inputs['M-f'] = function() {
  this.cursor = this.next_word(this.cursor);
  this.desired_inset = this.inset;
};
texteditor.inputs['M-f'].rep = true;

texteditor.inputs['C-p'] = function() {
  if (this.cursor === 0) return;
  this.desired_inset = Math.max(this.desired_inset, this.inset);
  this.cursor = this.prev_line(this.cursor);
  var newlinew = this.line_width(this.cursor);
  this.cursor += Math.min(this.desired_inset, newlinew);
};
texteditor.inputs['C-p'].rep = true;

texteditor.inputs['M-p'] = function() {
  while (this.line_blank(this.cursor))
    this.cursor = this.prev_line(this.cursor);

  while (!this.line_blank(this.cursor))
    this.cursor = this.prev_line(this.cursor);
};
texteditor.inputs['M-p'].doc = "Go up to next line with text on it.";
texteditor.inputs['M-p'].rep = true;

texteditor.inputs['C-n'] = function() {
  if (this.cursor === this.value.length-1) return;
  if (this.value.next('\n', this.cursor) === -1) {
    this.to_file_end();
    return;
  }

  this.desired_inset = Math.max(this.desired_inset, this.inset);
  this.cursor = this.next_line(this.cursor);
  var newlinew = this.line_width(this.cursor);
  this.cursor += Math.min(this.desired_inset, newlinew);
};
texteditor.inputs['C-n'].rep = true;

texteditor.inputs['M-n'] = function() {
  while (this.line_blank(this.cursor))
    this.cursor = this.next_line(this.cursor);

  while (!this.line_blank(this.cursor))
    this.cursor = this.next_line(this.cursor);
};
texteditor.inputs['M-n'].doc = "Go down to next line with text on it.";
texteditor.inputs['M-n'].rep = true;
