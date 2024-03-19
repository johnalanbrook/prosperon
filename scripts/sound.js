var audio = {};

var sound_pref = ['wav', 'flac', 'mp3', 'qoa'];

audio.sound = {
  bus: {},
  samplerate: dspsound.samplerate,
  
  sounds: [], /* array of loaded sound files */
  play(file, bus) {
    file = Resources.find_sound(file);
    if (!file) {
      console.error(`Cannot play sound ${file}: does not exist.`);
      return;
    }
    var src = audio.dsp.source(file);
    bus ??= audio.sound.bus.master;
    src.plugin(bus);
    return src;
  },

  doc: {
    play: "Play the given file once.",
    volume: "Set the volume. 0 is no sound and 100 is loudest."
  },
};

audio.dsp = {};
Object.assign(audio.dsp, dspsound);

audio.dsp.mix = function(to) {
  var n = audio.dsp.mix();
  if (to) n.plugin(to);
  return n;
};

audio.dsp.allpass = function(secs, decay) {
  var composite = {};
  var fwd = audio.dsp.fwd_delay(secs,-decay);
  var fbk = audio.dsp.delay(secs,decay);
  composite.id = fwd.id;
  composite.plugin = composite.plugin.bind(fbk);
  composite.unplug = dsp_node.unplug.bind(fbk);
  fwd.plugin(fbk);
  return composite;
}

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

Object.mixin(dspsound.master().__proto__, {
  get db() { return 20*Math.log10(Math.abs(this.volume)); },
  set db(x) { x = Math.clamp(x,-100,0); this.volume = Math.pow(10, x/20); },
  get volume() { return this.gain; },
  set volume(x) { this.gain = x; },
});

audio.sound.bus.master = dspsound.master();

/*Object.mixin(audio.dsp.source().__proto__, {
  length() { return this.frames()/sound.samplerate(); },
  time() { return this.frame/sound.samplerate(); },
  pct() { return this.time()/this.length(); },
});
*/

return {audio};
