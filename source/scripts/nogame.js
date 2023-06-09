function nogamegui()
{
  GUI.column({
    items: [
      GUI.text_fn("NO GAME LOADED", {font_size: 6}),
      GUI.text_fn("No game.js available.")
    ],
    anchor: [0.5,0.5],
    
  }).draw(Window.dimensions.scale(0.5));
}

register_gui(nogamegui);

