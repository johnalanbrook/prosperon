# Yugine Editor

The main editor view is made up of objects. Each object can have a
number of components attached to it. When an object is selected, its
name, position, and list of components are listed.

## Basic controls
|Ctrl-Z|Undo|
|Ctrl-Shift-Z|Redo|
|Ctrl-A|Select all|
|Ctrl-S|Save|
|Ctrl-N|New|
|Ctrl-O|Open level|
|Ctrl-X|Cut|
|Ctrl-C|Copy|
|Ctrl-V|Paste|
|Alt-O|Add level to current level|
|Alt-A|or Alt-P Add a prefab|
|Ctrl-I|Objects on the level|
|Ctrl-E|Asset viewer. When on a component like a sprite, serves to select that sprite's texture|
|Ctrl-[|Downsize grid|
|Ctrl-]|Upsize grid|
|Backtick|REPL|
|Ctrl-[1-9]|to set camera positions|
|[1-9]|to recall camera positions|
|0|Set camera to home view|
|ESC|quit|
|Alt-1|Normal view|
|Alt-2|Wireframe view|
|Shift-Middle|Set editor cursor to mouse position (Cursor affects how objects rotate)|
|Shift-Ctrl-Middle|Set cursor to object selection|
|Shift-Right|Remove cursor|

## Editor Mode select

|Alt-F1|Basic mode|
|Alt-F2|Brush mode|
  - Clicking will place what is on clipboard

## Object controls

|G|Translate|
|Alt-G|Snap objects to cursor|
|S|Scale|
|R|Rotate|
|Ctrl-P|Save object changes to prefab|
|Ctrl-shift-P|Save object changes as a unique prefab ("Parent")|
|Ctrl-shift-T|Save object changes to a side prefab ("Type")|
|Ctrl-J|Bake name to expose to level script|
|Alt-J|Remove baked name|
|Ctrl-Y|Show obj chain|
|Alt-Y|Start prototype explorer|
|Ctrl-U|Revert object or component to match prototype|
|Alt-U|Make object unique. If a level, allows setting of internal object position and rotation.|
|Ctrl-shift-G|Save group as a level|
|Arrows|Translate 1 px|
|Shift-Arrows|Translate 10 px|
|Tab|Select component|
|F|Zoom to object(s)|
|Ctrl-F|Focus on a selected sublevel. Edit and save it in place.|
|Ctrl-Shift-F|Go up one level in the editing chain.|
|M|Flip horizontally|
|Ctrl-M|Flip vertically|
|Ctrl-D|Duplicate|
|H|Hide|
|Ctrl-H|Unhide all|
|T|Lock|
|Alt-T|Unlock all|
|Q|Toggle component help|
|Ctrl-Shift-Alt-Click|Set object center|

## Mouse controls

|Left|Select|
|Middle|Quick grab|
|Right|Unselect|

## Level controls
|Ctrl-L|Open level script|

## Game controls
|F1|Debug draw|
|F2|Config menu|
|F3|Show bounding boxes|
|F4|Show gizmos|
|F5|Start|
|F6|Pause|
|F7|Stop|
|F10|Toggle slow motion|

== Components
Components all have their own set of controls. Many act similar to
objects. If a component has a position attribute, it will react as
expected to object grabbing; same with scaling, rotation, and so on.

If a component uses an asset, the asset viewer will serve to pick new
assets for it.

## Spline controls
|Ctrl-click|Add a point|
|Shift-click|remove a point|
|+,-|Increase or decrease spline segments|
|Ctrl-+,-|Increase or decrease spline degrees. Put this to 1 for the spline to go point to point|
|Alt-B,V|Increase or decrease spline thickness|

.Collider controls
|Alt-S|Toggle sensor|

## Yugine Programming

### Object functions

* start(): Called when the object is created, before the first update is ran
* update(dt): Called once per frame
* physupdate(dt): Called once per physics calculation
* stop(): Called when the object is killed
* collide(hit): Called when this object collides with another. If on a collider, specific to that collider
  - hit.hit: Gameobject ID of what's being hit
  - hit.velocity: Velocity of impact
  - hit.normal: Normal of impact

### Colliders
Colliders visually look different based on their status. Objects can
be in one of three states

- Dynamic: respond to physics
- Kinematic: modeled with infinite momentum. An "unstoppable force"
controlled by a user.
- Static: modeled with infinite mass. An "immovable object" that
shouldn't move.

Objects can then have colliders on them, each collider being a sensor,
or solid. Sensors respond to collision signals, while solid ones also
do not let objects through.



### Input
Input works by adding functions to an object, and then "controlling"
them. The format for a function is "input_[key]_[action]". [Action]
can be any of

- down: Called once per frame the key is down
- pressed: Called when the key is pressed
- released: called when the key is released

For example, "input_p_pressed()" will be called when p is pressed, and not again
until it is released and pressed again.

### Your game

When the engine runs, it executes config.js, and then game.js. A
window should be created in config.js, and custom code for prototypes
should be executed.

game.js is the place to open your first level.

### Levels

A level is a collection of objects. A level has a script associated
with it. The script is ran when the level is loaded.

Levels can be added to other levels. Each is independent and unique.
In this way, complicated behavior can easily be added up. For example,
a world might have a door that opens with a lever. The door and lever
can be saved off as their own level, and the level script can contain
the code that causes the door to open when the lever is thrown. Then,
as many door-lever levels can be added to your game as you want.

The two primary ways to add objects to the game are World.spawn, and
Level.add. World.spawn creates a single object in the world, Level.add
adds an entire level, along with its script.

Levels also can be checked for "completion". A level can be loaded
over many frames, and only have all of its contents appear once it's
finished loading. World.spawn is immediate.

Level.clear removes the level from the game world.

.Level scripting
Each level has a script which is ran when the level is loaded, or the
game is played. A single object declared in it called "scene" can be
used to expose itself to the rest of the game.