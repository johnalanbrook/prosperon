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

## Playing, editing, debugging

Playing is playing your game. Controls are what are specified for your game.

In debug builds, additional debug controls are available. For example, F12 brings up GUI debug boxes. C-M-f puts you into a flying camera mode, without pausing your game.

The game can be paused to edit it. Most editor controls are available here, all of them essentially except for loading new levels, clearing the level, etc. An object can be clicked on and edited, objects can be moved, etc.

A prefab can be opened up to edit on its own, without breaking the currently played level.

In edit mode, there are no running scripts; only editing them.

## The ECS system

There are two distinct items in the Primum Machina: the Entity, and the Component. Components give qualities to Entities. An Entity is any real, tangible thing in the universe, and so every entity has a position. Components do not necessarily have a position; they can be things like the image that draws where the entity is located, and colliders that allow the entity to respond with the world.

### Components
The most "bare metal" are the components. These are essentially hooks into the engine that tell it how to do particular things. For example, to render a sprite, Javascript does no rendering, but rather tells the engine to create an image and render it in a particular spot.

Components are rendered in an "ECS" style. To work, components must be installed on an entity. They have no meaning outside of a physical object in the world.

AI would be components. You could have a "sensor" AI component that detects the world around it, and a locomotion AI component, etc, and reserve scripting for hooking them up, etc. Or do it all in scripting.

Components cannot be scripted; they are essentially a hardwired thing that you set different flags and values on, and then can query it for information.

### Entity
Entities are holders of components. Anything that needs a component will be an entity. Components rely on entites to render correctly. For example, the engine knows where to draw a sprite wherever its associated entity is.

Entities can be composed of other entities. When that is the case, an entity "under" a different entity will move when the above entity moves, as if it were attached. 

The outermost entity that all other entities must exist in is the Primum. It always exists and cannot be removed.

## Prototyping model
All objects follow the prototyping model of inheritence. This makes it trivial to change huge swathes of the game, or make tiny adjustments to single objects, in a natural and intuitive way.

Components cannot be prototyped. They are fundamentally tied to the entity they are bound to.

Entities can be prototyped out. What this means is that, when you select an object in the game, you can either make a "subtype" of it, where changes to the object trickle down to the created one, or a "sidetype" of it, which is a total duplicate of the object.

entity.clone(parent) -> create a subtyped version of the entity
entity.dup(parent) -> create a copy of the entity.
entity.promote() -> promote the entity to a new Ur-type, as it currently exists.
entity.revert() -> remove all changes of this entity so it again matches its Ur-type.
entity.push() -> push changes to this entity to its Ur-type to it matches.

### Ur-types
An Ur-type is a thing which cannot be seen but which can stamp out copies of itself. Objects can be promoted to an ur-type, so if it is deleted, another one can later be made.

Ur-types have a lineage going back to the original gameobject. The ur-type lineage can be as deep as you want it to be; but it should probably be shallow.

Only first Ur-types can have components. Every inherited thing after it can only edit the original's components, not add or subtract. Original Ur-types must currently be defined in code.

Ur-types also remember the list of entities that compose the given Ur.

### Loading traits
Traits are defined by code and a data file. When an Ur-type is extended with a trait, the code is run, and then the data file contains modifications and

### Creating entities
Entities are like real world representations of an Ur-type. Ur-types exist only theoretically, but can then be spawned into a true entity in the game world.

An entity can diverge from its ur-type. When this happens, you can either revert the entity, copy how it's changed to its ur-type, or promote it to its own ur-type.

### Efficiency of all this
It is extremely cheap and fast to create entities. Ur-types work as a defined way for the engine to make an object, and can even cache deleted copies of them.

## Resources
Assets can generally be used just with their filename. It will be loaded with default values. However, how the engine interprets it can be altered with a sidecar file, named "filename.asset", so "ball.png" will be modified via "ball.png.asset". These are typical JSON files. For images, specify gamma, if it's a sprite or texture, etc, for sound, specify its gain, etc.

Ur-types are directly related to your file directory hierarchy. In a pinball game where you have a flipper, and then a flipper that is left ...

@/
  /bumper
    hit.wav
    bumper.js
  /ball
    hit.wav
    ball.js
  /flipper
    flipper.js
    flipper.json <-- Modifications applied to the flipper ur-type
    t1.json <-- A variant of flipper.js. Cannot be subtyped.
    flip.wav
    flipper.img
    left/
      flip.wav
      left.js <-- This will load as an extension to flipper.js

This is how resources are loaded in any given ur-type. Relative file paths work. So, in 'ball.js', it can reference 'hit.wav', and will play that file when it does; when bumper.js loads 'hit.wav', it will load the one located in its folder.

The left flipper can use the root flipper flip sound by loading "../flip.wav".

Absolute paths can be specified using a leading slash. The absolute path the bumper's hit sound is "/bumper/hit.wav".

When you attempt to load the "flipper.left" ur-type, if flipper is not loaded, the engine will load it first, and then load left.

Unloading an ur-type unloads everything below it, necessarily. flipper.left means nothing without flipper.

Computer systems have a user configuration folder specified, where you are allowed to write data to and from. This is good for save games and configurations. It is specified with a leading "@" sign. So "@1.save" will load the file "1.save" from the folder allotted to your game by the platform.

Links can be specified using the "#" sign. This is simply defined as, for example, with "trees:/world/assets/nature/trees" specified, you can easily make the ur-type "fern" with "Primum.spawn("#trees/fern")", instead of "Primum.spawn('#trees.fern')"

Primum will attempt to solve most resolution ambiguities automatically. There are numerous valid directory layouts you can have. Examining flipper.left ...

@/
  flipper.js
  flipper/
    left.js

@/
  flipper/
    _.js
    left.js

@/
  flipper/
    flipper.js
    left/
      left.js

In sum, a file that is a single underscore _.js is assumed to be the given folder's ur-type. When populating the ur-type hierarchy, the _ is replaced with the name of the containing folder. if there is a folder with the same name as *.js present, the items in the folder are assumed to be ur-types of the *.js.

Asset links always follow the directory hierarchy, however, so if you want to reference an asset with a relative path, the .js file must actually be present in the same path as the asset.

prototypes.generate_ur(path) will generate all ur-types for a given path. You can preload specific levels this way, or the entire game if it is small enough.

### Spawning
The outmost sphere of the game is the Primum, the first entity. Your first entity  must be spawned in the Primum. Subsequent entities can be spawned in any entity in the game.

Ur-types can remember configurations of entities spawned inside of them.

Once entities are created in the world, they are mostly interested now in addressing actual other objects in the world. Let's look at an example.

Primum
  Level 1
    Orc
    Goblin
    Human
      Sword
  UI

When a script is running, it is completely contained. If "Human" has a "health" parameter, it can only be access through "self.health", not directly through "health". This makes it easy to run a script without fear of overwriting any parameters on accident.

The "$" is populated with an object's children. $.sword.damage will properly get the damage of the human's sword, and will be undefined for Goblin and Orc.

To access the entity's owner, it is through _. For example, the human can access the orc via _.Orc.

## Entities
Entities are things that exist in the game world.

|name| description|
|---|---|
|level| the level this object belongs to|
|pos| the global position|
|relpos| the position relative to its level|
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
