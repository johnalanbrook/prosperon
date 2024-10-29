var layout = use("layout.js");

this.hud = function () {
  layout.draw_commands(clay.draw([], _ => {
    clay.text("No game yet! Make game.js to get started!");
  }));
};
