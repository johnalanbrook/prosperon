var dsp_node = {
  make(id) {
    var n = Object.create(this);
    n.id = id;
    return n;
  },
  
  id: undefined,
  get volume() { return cmd(175, this.id); },
  set volume(x) { x = Math.clamp(x,0,1); cmd(176, this.id,x); },
  get db() { return 20*Math.log10(Math.abs(this.volume)); },
  set db(x) { x = Math.clamp(x,-100,0); this.volume = Math.pow(10, x/20); },
  get pan() { return cmd(178,this.id); },
  set pan(x) { x = Math.clamp(x,-1,1); cmd(179,this.id,x); },
  off(t) { cmd(183, this.id, t); }, /* True to turn off */
  bypass(t) { cmd(184, this.id, t); }, /* True to bypass filter effect */
  unplug() { cmd(164, this.id); },
  plugin(to) { cmd(177, this.id, to.id); },
  kill() {
    if (this._dead) return;
    this._dead = true;
    cmd(193, this.id);
  },
  end() {},
};

var dsp_source = Object.copy(dsp_node,{
  end(){},
  get loop() { return cmd(194,this.id); },
  set loop(x) { cmd(195,this.id, x);},
  get timescale() { return cmd(201,this.id); },
  set timescale(x) { cmd(202,this.id,x); },
  get frame() { return cmd(196,this.id); },
  set frame(x) { cmd(199, this.id, x); },
  frames() { return cmd(197,this.id); },
  length() { return this.frames()/Sound.samplerate(); },
  time() { return this.frame/Sound.samplerate(); },
  pct() { return this.time()/this.length(); },
});

var Sound = {
  bus: {},
  samplerate() { return cmd(198); },
  sounds: [], /* array of loaded sound files */
  play(file, bus) {
    if (!IO.exists(file)) {
      Log.error(`Cannot play sound ${file}: does not exist.`);
      return;
    }
    var src = DSP.source(file);
    bus ??= Sound.bus.master;
    src.plugin(bus);
    return src;
  },

  music(midi, sf) {
    cmd(13, midi, sf);
  },
};

var DSP = {
  mix(to) {
    var n = dsp_node.make(cmd(181));
    if (to) n.plugin(to);
    return n;
  },
  source(path) {
    var src = Object.create(dsp_source);
    src.id = cmd(182,path,src);
    return src;
  },
  delay(secs,decay) {
    return dsp_node.make(cmd(185, secs, decay));
  },
  fwd_delay(secs, decay) {
    return dsp_node.make(cmd(207,secs,decay));
  },
  allpass(secs, decay) {
    var fwd = DSP.fwd_delay(secs,-decay);
    var fbk = DSP.delay(secs,decay);
    fwd.plugin(fbk);
  },
  lpf(f) {
    return dsp_node.make(cmd(186,f));
  },
  hpf(f) {
    return dsp_node.make(cmd(187,f));
  },
  mod(path) {
    return dsp_node.make(cmd(188,path));
  },
  midi(midi,sf) {
    return dsp_node.make(cmd(206,midi,sf));
  },
  crush(rate, depth) {
    return dsp_node.make(cmd(189,rate,depth));
  },
  compressor() {
    return dsp_node.make(cmd(190));
  },
  limiter(ceil) {
    return dsp_node.make(cmd(191,ceil));
  },
  noise_gate(floor) {
    return dsp_node.make(cmd(192,floor));
  },
  pitchshift(octaves) {
    return dsp_node.make(cmd(200,octaves));
  },
  noise() {
    return dsp_node.make(cmd(203));
  },
  pink() {
    return dsp_node.make(cmd(204));
  },
  red() {
    return dsp_node.make(cmd(205));
  },
};

Sound.bus.master = dsp_node.make(cmd(180));
Sound.master = Sound.bus.master;

Sound.play.doc = "Play the given file once.";
Sound.doc = {};
Sound.doc.volume = "Set the master volume. 0 is no sound and 100 is loudest.";
