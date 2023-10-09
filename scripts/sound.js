var Music = {
  play(path) {
    Log.info("Playing " + path);
    cmd(87,path);
  },

  stop() {
    cmd(89);
  },

  pause() {
    cmd(88);
  },

  set volume(x) {
  },
};

var Sound = {
  sounds: [], /* array of loaded sound files */
  play(file) {
    if (!IO.exists(file)) {
      Log.warn(`Cannot play sound ${file}: does not exist.`);
      return;
    }
     this.id = cmd(14,file);
  },
  
  music(midi, sf) {
    cmd(13, midi, sf);
  },

  musicstop() {
    cmd(15);
  },

  /* Between 0 and 100 */
  set volume(x) { cmd(19, x); },

  killall() {
    Music.stop();
    this.musicstop();
    /* TODO: Kill all sound effects that may still be running */
  },
};
