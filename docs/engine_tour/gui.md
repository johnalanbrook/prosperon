# GUI
Game GUIs are written by registering an entity's `gui` property to a function, or its `hud` property.

`gui` draws in window space, where the bottom left corner is `[0,0]`. `hud` draws in screen space. In either of these, you can call "render" functions directly.

`draw` draws in world space, and mum functions can equally be used there.

## MUM
The GUI system which ships with Prosperon is called *MUM*. MUM is a declarative, immediate mode HUD system. While Imgui is designed to make it easy to make editor-like controls, mum is designed to be easy to make video game huds.
