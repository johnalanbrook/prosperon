/* This file runs after the audio system is initiated */

var cries = {};

Object.readonly(audio, 'samplerate');
Object.readonly(audio, 'channels');
Object.readonly(audio, 'buffer_frames');

audio.play = function(file,bus = audio.bus.master) {
  file = Resources.find_sound(file);
  if (!file) {
    console.error(`Cannot play sound ${file}: does not exist.`);
    return;
  }
  var src = audio.dsp.source(file);
  src.plugin(bus);
  src.guid = prosperon.guid();
  return src;
}
audio.bus = {};
audio.bus.master = dspsound.master();
audio.dsp = {};
audio.dsp = dspsound;

audio.dsp.mix().__proto__.imgui = function()
{
  imgui.pushid(this.memid());
  this.volume = imgui.slider("Volume", this.volume);
  this.off = imgui.checkbox("Mute", this.off);
  imgui.popid();
}

audio.cry = function(file, bus = audio.bus.sfx)
{
  file = Resources.find_sound(file);
  var player = audio.play(file, bus);
  if (!player) return;
  
  player.guid = prosperon.guid();
  cries[player.guid] = player;
  player.ended = function() { delete cries[player.guid]; player = undefined; }
  return player.ended;
}

var killer = Register.appupdate.register(function() {
  for (var i in cries) {
    var cry = cries[i];
    if (!cry.ended) continue;
    if (cry.frame < cry.lastframe || cry.frame === cry.frames())
      cry.ended();
    cry.lastframe = cry.frame;
  }
});

var song;

// Play 'file' for new song, cross fade for seconds
audio.music = function(file, fade = 0.5) {
  if (!file) {
    if (song)
      song.volume = 0;
  }
  file = Resources.find_sound(file);
  if (!fade) {
    song = audio.play(file, audio.bus.music);
    song.loop = true;
    return;
  }

  if (!song) {
    song = audio.play(file, audio.bus.music);
    song.volume = 1;
//    tween(song,'volume', 1, fade);
    return;
  }
  
  var temp = audio.play(file, audio.bus.music);
  if (!temp) return;
  
  temp.volume = 1;
  var temp2 = song;
//  tween(temp, 'volume', 1, fade);
//  tween(temp2, 'volume', 0, fade);
  song = temp;
  song.loop = true;
}

audio.bus.music = audio.dsp.mix();
audio.bus.music.plugin(audio.bus.master);

audio.bus.sfx = audio.dsp.mix();
audio.bus.sfx.plugin(audio.bus.master);

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

audio.dsp.obscure('doc');

Object.mixin(audio.bus.master.__proto__, {
  get db() { return 20*Math.log10(Math.abs(this.volume)); },
  set db(x) { x = Math.clamp(x,-100,0); this.volume = Math.pow(10, x/20); },
  get volume() { return this.gain; },
  set volume(x) { this.gain = x; },
});

audio.bus.master.__proto__.toJSON = function()
{
  return {
    volume: this.volume,
    off: this.off,
    pan: this.pan,
    pass: this.pass
  };
}

/*Object.mixin(audio.dsp.source().__proto__, {
  length() { return this.frames()/audio.samplerate(); },
  time() { return this.frame/sound.samplerate(); },
  pct() { return this.time()/this.length(); },
});
*/

return {audio};
