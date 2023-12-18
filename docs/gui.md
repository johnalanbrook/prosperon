# GUI & Drawing

Register functions with built-in drawing callbacks to have code execute at particular points in time. Some drawing functions take world coordinates, while others take screen coordinates.

gui
Called every frame.

draw
Called every frame.

debug
Called if drawing physics.

gizmo
Called on an object if it is selected in the editor.

# Mum

Mum is the GUI compositing system here. When drawing a mum, the bottom left corner is [0,0]. Change the anchor property of any Mum to change where it is drawn.