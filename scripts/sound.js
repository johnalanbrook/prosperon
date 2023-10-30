var Audio = {
    
};

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
      Log.error(`Cannot play sound ${file}: does not exist.`);
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
  get volume() { 0; },

  killall() {
    Music.stop();
    this.musicstop();
    /* TODO: Kill all sound effects that may still be running */
  },
};

Sound.play.doc = "Play the given file once.";
Sound.music.doc = "Play a midi track with a given soundfont.";
Sound.musicstop.doc = "Stop playing music.";
Sound.doc = {};
Sound.doc.volume = "Set the master volume. 0 is no sound and 100 is loudest.";
Sound.killall.doc = "Kill any playing sounds and music in the game world.";
