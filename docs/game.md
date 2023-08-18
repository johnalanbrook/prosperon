# Yugine

The yugine essentially is made of a sequence of levels. Levels can be
nested, they can be streamed in, or loaded one at a time. Levels are
made of levels.

Different "modes" of using the engine has unique sequences of level
loading orders. Each level has an associated script file. Level
loading functions call the script file, as well as the level file. The
level file can be considered to be a container of objects that the
associated script file can access.

* Game start
  1. Engine scripts
  2. config.js
  3. game.lvl & game.js

* Editor
  1. Engine scripts
  2. config.js
  3. editor.js

* Editor play
  * F5 debug.lvl. Used for custom debug level testing. If doesn't exist, game.lvl is loaded.
  * F6 game.lvl
  * F7 Currently edited level

While playing ...

* F7 Stop

## Level model
The game world is made up of objects. Levels are collections of
objects. The topmost level is called "World". Objects are spawned into
a specific level. If none are explicitly given, objects are spawned
into World. Objects in turn are made up of components - sprites,
colliders, and so on. Accessing an object might go like this:

World.level1.enemy1.sprite.path = "brick.png";

To set the image of enemy1 in level 1's sprite.

### Level functions
|name| description|
|---|---|
|levels| a list of all levels loaded in this one|
|objects| a list of all objects belonging to this level (objects + levels)|
|object_count| objects.length()|
|spawn(type)| creates an object from the supplied type in the level|
|create()| creates an empty level inside of this one|
|loadfile(file)| loads file as a level inside of this one; returns it. Mainly for editor|
|loadlevel(file)| loads file as a level and does running required for gameplay|
|load(level_json)| clears this level and spawns all objects specified in the level_json|
|clear()| kills all objects in this level|
|kill()| cleans up the level and kills it|

## Objects
Objects are things that exist in the game world.

|name| description|
|---|---|
|level| the level this object belongs to
|pos| the global position|
|relpos| the position relative to its level
angle| the global angle|
|relangle| angle relative to its level|
|velocity| velocity of the object|
|angularvelocity| angular velocity of the object|
|alive| true if the object is valid|
|varname| the variable name of the object (used for friendly naming)|
|friction| physics attribnute|
|elasticity| physics attribute|
|flipx| true if the object is flipped on its x axis|
|flipy| true if the object is mirrored on its y axis|
|body| the internal gameobject id of the object|
|controlled| true if the object is controlled by something|
|phys| set to dynamic; kinematic; or static; explained below|
|moi| the moment of inertia of the object|
|mass| mass of the object|
|visible| true if the object is visible. Set to false to disable all visible features belonging to it|
|in_air()| true if the object is not on the ground|
|on_ground()| !in_air()|
|layer| collision layer for the physics engine|
|draw_layer| draw order. higher numbers draw on top of lower ones|
|scale| change to make the object larger or smaller|
|from| the object this object was created from|
|boundingbox| the boundingbox of this object in world dimensions|
|push(vec)| push this object each frame with vec velocity|
|width| the boundingbox defined width|
|height| the boundingbox defined height|
|kill| destroy the object|
|stop| ???|
|world2this(pos)| return the pos in world coordinates to relative this object|
|this2world(pos)| return the pos in this coordinates relative to the world|
|make(props, level)| instantiate an object based on this, with additional props, in level|

## Editor related object features

|gizmo| path to an image to draw in editor mode|

## Functions for object control

|clone(name; ext)| create a copy of this object and extend it with ext; does not spawn|
|instadup()| create an exact duplicate of this object in the World|
|revert()| remove everything that makes the object unique; make it exactly like what it was spawned from|

## Physics
All objects belong to the physics engine, but may be totally ignored by it.

|static| does not and will not move|
|dynamic| moves under the auspices of the physics engine|
|kinematic| moves under the auspices of the player or other control mechanism|

Physics properties work as such

|mass| affects how much a given force will move an object|
|elasticity| affects momentum loss on collisions; multiplicative between two objects for each collision; 1 for no loss; 0 for total stoppage; >1 for a chaotic increasing entropy simulation|
|friction| affects momentum loss when rubbing against another surface; multiplicative between the two objects|

## Textures & images
A sprite is a display of a specific texture in the game world. The
underlying texture has values associated with it, like how it should
be rendered: is it a sprite, is it a texture, does it have mipmaps,
etc. Textures are all file based, and are only accessed through the
explicit path to their associated image file.

## Finding & Addressing Objects


## Editor & Debugging
Although intertwined, debugging functions and the editor are separate entities.

### Debugging
Debugging functions are mapped to the F buttons, and are always available during gameplay in a debug build. Pressing the F button toggles the feature; pressing it with ALT shows a legend for that feature; pressing it with CTRL shows additional info

|F1| Draw physics info|
|F3| Draw bounding boxes|
|F12| Drawing gui debugging info|
