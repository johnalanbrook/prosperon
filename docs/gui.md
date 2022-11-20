# Yugine GUI

The GUI is a 2D layer which is always on top of everything and does not move with the camera. The GUI acts like a sub level, each element can have a layer value from -127 to 127 to indicate its draw position. If two GUI elements on the same layer overlap, it is undefined behavior.

  A "Subgui" has its own layer order from -127 to 127. It itself is on one layer of the base GUI.

  ### GUI Scripting
  Each GUI element has a "Draw" boolean. If it is true, the GUI draws. If a Subgui's 'Draw' is set to 'True', it and all of its subelements draw.

  Each GUI element has a script to allow it to hook into a component of the game. When it draws, its script is executed. Any component of the GUI element can be set in its script. A simple thing to do would be for a text script's instruction to be to set the value of the text to the player's health.

  The GUI has read only access to the game state. Each gameobject has a boolean for "GUI". If set to 'True', that gameobject becomes readable by the GUI. It is accessed as 'gameobjectname->...'. The GUI can read all elements of the struct.

### GUI animations
  Each GUI element has a set number of animation slots which can be utilized.

  - Start: Animation that is played once when the GUI element is set to Draw.
  - Repeat: Animation that is always played on a loop when the GUI element is Drawing, after the Start animation is finished.
  - Hover: Looped animation played when the GUI element is selected.
  - Quit: SIngle play animation when Draw is set to False.

  In addition, there are 'Seconds' fields for Start-Repeat, Repeat-Hover, Hover-Repeat, and Hover-Quit. This field represents the time it takes to transition from one animation to the next. If set to 0, the animation immediately jumps to the beginning of the next animation. If set to a number, the GUI element will smoothly interpolate from its ending state to the beginning of the first frame of the next animation. For example, if a HUD element is bouncing up and down on hover, when you go to the next element the bouncing element jumps to its static state. It can instead smoothly return to where it started using this field.