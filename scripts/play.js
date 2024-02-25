Game.play();

if (!io.exists("game.js"))
  load("scripts/nogame.js");
else
  load("game.js");
