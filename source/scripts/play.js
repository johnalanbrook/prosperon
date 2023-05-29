if (load("game.js") === false) {
  Log.error("No game.js. No game.");
  quit();
}

sim_start();

