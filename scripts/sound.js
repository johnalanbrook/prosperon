var audio = {};
audio.sound = {
  bus: {},
  samplerate() { return cmd(198); },
  sounds: [], /* array of loaded sound files */
  play(file, bus) {
    if (!io.exists(file)) {
      console.error(`Cannot play sound ${file}: does not exist.`);
      return;
    }
    var src = audio.dsp.source(file);
    bus ??= sound.bus.master;
//    src.plugin(bus);
    return src;
  },

  doc: {
    play: "Play the given file once.",
    volume: "Set the volume. 0 is no sound and 100 is loudest."
  },
};

audio.dsp = {
  mix(to) {
    var n = cmd(181);
    if (to) n.plugin(to);
    return n;
  },
  source(path) {
    return cmd(182,path);
  },
  delay(secs,decay) {
    return cmd(185, secs, decay);
  },
  fwd_delay(secs, decay) {
    return cmd(207,secs,decay);
  },
  allpass(secs, decay) {
    var composite = {};
    var fwd = audio.dsp.fwd_delay(secs,-decay);
    var fbk = audio.dsp.delay(secs,decay);
    composite.id = fwd.id;
    composite.plugin = composite.plugin.bind(fbk);
    composite.unplug = dsp_node.unplug.bind(fbk);
    fwd.plugin(fbk);
    return composite;
  },
  lpf(f) {
    return cmd(186,f);
  },
  hpf(f) {
    return cmd(187,f);
  },
  mod(path) {
    return cmd(188,path);
  },
  midi(midi,sf) {
    return cmd(206,midi,sf);
  },
  crush(rate, depth) {
    return cmd(189,rate,depth);
  },
  compressor() {
    return cmd(190);
  },
  limiter(ceil) {
    return cmd(191,ceil);
  },
  noise_gate(floor) {
    return cmd(192,floor);
  },
  pitchshift(octaves) {
    return cmd(200,octaves);
  },
  noise() {
    return cmd(203);
  },
  pink() {
    return cmd(204);
  },
  red() {
    return cmd(205);
  },
};

audio.dsp.doc = {
  delay: "Delays the input by secs, multiplied by decay",
  fwd_delay: "Forward feedback delays the input by secs, multiplied by decay",
  allpass: "Composite node of a delay and fwd_delay",
  lpf: "Low pass filter at a given frequency",
  hpf: "High pass filter at a given frequency",
  midi: "A source node for a midi file with a given soundfont file",
  crush: "Bitcrush the input to a given rate and bit depth",
  limiter: "Limit audio to ceil with pleasent rolloff",
  noise_gate: "Do not pass audio below the given floor",
  pitchshift: "Shift sound by octaves",
  noise: "Plain randon noise",
  pink: "Pink noise",
  red: "Red noise"
};

Object.mixin(cmd(180).__proto__, {
  get db() { return 20*Math.log10(Math.abs(this.volume)); },
  set db(x) { x = Math.clamp(x,-100,0); this.volume = Math.pow(10, x/20); },
  get volume() { return this.gain; },
  set volume(x) { this.gain = x; },
});

/*Object.mixin(audio.dsp.source().__proto__, {
  frames() { return cmd(197,this); },
  length() { return this.frames()/sound.samplerate(); },
  time() { return this.frame/sound.samplerate(); },
  pct() { return this.time()/this.length(); },
});
*/

return {audio};
