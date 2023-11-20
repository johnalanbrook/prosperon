# GUI & Drawing

Register functions with built-in drawing callbacks to have code execute at particular points in time. Some drawing functions take world coordinates, while others take screen coordinates.

register_gui
Called before nk_gui. Screen space.

register_nk_gui
Called during the Nuklear debug pass. Use Nuklear functions only.

register_draw
Called every frame. World space.

register_debug
Called if drawing physics. World space.

# Mum

Mum is the GUI compositing system here. When drawing a mum, the bottom left corner is [0,0]. Change the anchor property of any Mum to change where it is drawn.