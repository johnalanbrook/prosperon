# GUI
Game GUIs are written by registering an entity's *gui* property to a function.

The GUI system which ships with Prosperon is called *MUM*. MUM is a declarative, immediate mode interface system. Immediate to eliminate the issue of data synchronization in the game.

All GUI objects derive from MUM. MUM has a list of properties, used for rendering. Mum also has functions which cause drawing to appear on the screen.
