var Audio = {
    
};

var Sound = {
  sounds: [], /* array of loaded sound files */
  play(file) {
    if (!IO.exists(file)) {
      Log.error(`Cannot play sound ${file}: does not exist.`);
      return;
    }
    var p = cmd(14,file);
    return p;
  },

  finished(sound) {
    return cmd(165, sound);
  },

  stop(sound) {
    cmd(164, sound);
  },
  
  music(midi, sf) {
    cmd(13, midi, sf);
  },

  musicstop() {
    cmd(15);
  },

  /* Between 0 and 100 */
  set volume(x) { cmd(19, x); },
  get volume() { 0; },
};

Sound.play.doc = "Play the given file once.";
Sound.doc = {};
Sound.doc.volume = "Set the master volume. 0 is no sound and 100 is loudest.";
