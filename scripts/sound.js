soloud.init();

var audio = {};
var pcms = {};

audio.pcm = function pcm(file)
{
  file = Resources.find_sound(file);
  if (!file) throw new Error(`Could not findfile ${file}`);
  if (pcms[file]) return pcms[file];
  var newpcm = soloud.load_wav_mem(io.slurpbytes(file));
  pcms[file] = newpcm;
  return newpcm;
}

audio.play = function play(file) {
  var pcm = audio.pcm(file);
  if (!pcm) return;
  return soloud.play(pcm);
};

audio.cry = function cry(file) {
  var voice = audio.play(file);
  if (!voice) return;
  return function() {
    voice.stop();
    voice = undefined;
  }
};

var song;

// Play 'file' for new song, cross fade for seconds
audio.music = function music(file, fade = 0.5) {
  if (!file) {
    if (song) song.volume = 0;
    return;
  }

  if (!fade) {
    song = audio.play(file);
    song.loop = true;
    return;
  }

  if (!song) {
    song = audio.play(file);
    song.volume = 1;
    //    tween(song,'volume', 1, fade);
    return;
  }

  var temp = audio.play(file);
  if (!temp) return;

  temp.volume = 1;
  var temp2 = song;
  //  tween(temp, 'volume', 1, fade);
  //  tween(temp2, 'volume', 0, fade);
  song = temp;
  song.loop = true;
};

return audio;
