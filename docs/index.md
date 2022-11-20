# The Yugine NAME TBD
The Yugine is an engine for people who can code. If you learn to code, you will be rewarded with the ability to make games way faster than you can with any other tool.

## Compiling
The main component of the Yugine is the library that enables the engine, yugine.a. Use this to create variations on the engine. The standard engine is a single "engine.c" file that enables all systems and initiates a standard game loop. It's compiled with a simple command, "tcc engine.c -o engine -static -lyugine -Iyugine". The editor that ships with it is compiled similarly. The editor and engine files should include preprocessor macros at the top of the files to customize the engine.

The engine is supposed to be extendable with C code easily.

### Included compiler
The yugine includes a C compiler with it. Objects can be scripted with direct C, for extremely fast performance. Each time you press "play", the C is compiled and executed as if it's part of the original engine. The C "scripts" are compiled using the embedded TCC (tiny C compiler).

### Why C instead of C++??? Or anything else that's "easier"???
C is the most simple programming language there is. Simple as.

## It's an operating system
A game engine is like an operating system. You have low level code managing threads, loading files when needed, and so on. Then you have a higher level interpreted language for manipulating it (like a standard OS terminal/command line). That's all this is; multiple little C programs for manipulating stuff that makes a game work, and a scripting language used to string them together at 300 FPS.

There are multiple levels on which the engine can be modified. Like most game engines, you can create games with it; UNLIKE most game engines, it is as easy to make game editors as it is to make the games themselves. This makes making games faster and easier, of any sort.

### Modifying code
The Yugine is the most generic version of the engine. It calls the engine library and runs everything as standard as possible. It is a simple short file that calls the standard basic API to get the engine running. You can hook into the engine API to make your own core engine; call the editor or not, change the rendering schedule, when input is gathered, etc.

### Editing in-engine
In the engine, you make objects. These can be scripted with Scheme.

## Scheme
The crown jewel idea behind this engine is its integration with Scheme. It's the scripting language for the engine, selected to be implemented for its extreme versatility and decent speed. Our Scheme implementation is faster than Lua, but multitudes more expressive, and, eventually, can compile down to C for distribution. If you're going to use a scripting language instead of C, choose one that's as far away from C as possible.

### Game languages
Computer game development would be benefitted greatly by a similar number of languages used by an OS (for example: C, regex, sed, awk, make, bash scripting ...). Some day in the future we have languages for all the things we need them for, but in the mean time, Scheme is extremely good for developing little computer languages, and is the principal reason for its use in the engine. The Zork games and many following Infocom games were developed with a language made using Lisp, for example. It would be doable to recreate SCUMM using Scheme and incorporate it into a game with modern graphics. Wow!

Usually you have some cumbersome UI to input data into complicated structures to get the following things to work. Because, in Scheme. data is code and code is data, it's much more natural to express this stuff as written commands.
- Game scripting. Calling library functions and making things in the game move.
- AI. Lisp is known as an AI language, and here it is used to make writing AI easier than ever. Instead of clunky GUI behavior trees with lines, you can write out what you want your enemies to behave like.
- Cutscenes. One of the first big game softwares I worked on was a cutscene maker. You could tell characters to move here or there, then say this line, then ask a question; I had to implement some crude form of coroutines and the ability to loop back around to previous points in the dialogue depending on what a player did. No more! For directly included in the language of scheme are continuations! And coroutines can be implemented in a mere 4 lines of code. Best of all, instead of little islands of data connected with arrows (don't they always end up like this?) you can write it like it's supposed to be written: a little (movie) script.
- Formatting text. Scheme gives a natural way to write formatted text. No more opening and closing braces for your various text formatting, now it's all (). Plus, since data is code and code is data, you don't even need any code hooks to grab data like a player's name. The entire engine, every object in it, and any data associated with those objects, are available to be rendered as text.

## Kernel side and user side
The "kernel" of the engine and the "user side" operate on totally different planes. Data has to be transformed to work on one side or the other anyway, so we make maximal use of that. Most game engines are at a tug of war with themselves, where on one hand you want to code in such a way to keep the processor happy (long arrays of the same operation on the same data type!), while also coding in a way to keep the designers happy (object model). C++ tried to get the best of both worlds, but instead it sucks. Jon Blow is doing something interesting with Jai, where you can swap between these two "views". I hope this turns out well, but I'm convinced that you should just separate systems level programming and user level programming from each other. Hence, C and Scheme, polar opposite languages.

In this engine, all data is internally represented in CPU optimal formatting. When you are poking around in Scheme, though, the data is put into the view of the objects they compose. You can edit freely in a way that makes the most sense for how games are composed, without sacrificing an ounce of speed.

## Compiling
Various subsystems in the engine, including the editor, can be stripped at compile time. Running make creates the editor with all subsystems in place. If you instead run make with
- EDITOR=0
    Removes all editor functionality.
    - DEBUG=0
Removes writing debug logs, measuring FPS, the ability to draw physics shapes, various debugging viewmodes, etc.

It is intended for a script to be written to invoke make with whichever specifications you need.

The following commands can remove subsystems which a specific game might not need.  In the editor, running the "COMPILE INFO" command will tell you which flags you can set to OFF for the game you are currently editing. These are pulled out of the game to increase speed, rather than to decrease size (so anything which it doesn't matter for performance if it is left in is not presented as an option)
- 3D=0
    Removes 3D rendering
    - AUDIO=0
etc ...

## Basic usage

If the editor is run, you are presented with a list of projects. If there are no projects listed, one can browse to an empty folder to create a project. If a project is selected, it opens. The editor.info file contains a list of paths for all known projects. If the editor is launched while in a game path, the editor starts immediately editing that game.

Each project has a game.project file that specifies info about the game, including which engine version the game was built with, which version of the game it is, and graphical settings. The game starts by loading the level specified in this file (set during editing). If the computer the game is run on cannot use a specified graphics setting, the game automatically applies what it thinks would work best (ie if the target is 1080p and the computer is 720p, it assumes fullscreen and so sets the resolution to 720p).

## Scenes

A "scene" is a list of classes and starting values for those classes.

This is the an improved godot, but with some extra features and focus:
- More tools to actually make the game
- First class localization tools
- Easier to work with

## Controls
### EDITOR
Middle mouse - select object

Space - Toggle global/local transform

1 - Render mode LIT
2 - Render mode UNLIT
3 - Render mode WIRE
4 - Show directional shadow map
5 - Toggle gizmos visible
6 - Toggle physics visible
7 - Toggle detail lighting (only albedo)
8 - Lighting only (lit only with gray material)
9 - POV Depth map
0 - Ambient occlusion

(UE4 viewmodes: https://docs.unrealengine.com/4.26/en-US/BuildingWorlds/LevelEditor/Viewports/ViewModes/)

CTRL + D - Duplicate object

Delete - delete object

F2 - Rename object

G - Translate object
R - Rotate object
T - Scale object

F3 - Toggle game stats display
F4 - Toggle hierarchy menu
F5 - Toggle lighting menu
F6 - Toggle game settings menu
F7 - Viewmode menu

H - Toggle object hidden
K - Toggle grid visible

F - Focus on model

Y - Reload shaders

## Gameobject
A gameobject is anything with a transform.

Only one level deep inheritance. Each object inherits GameObject. Then, that object defines in its constructor how it looks. What mesh does it use, etc (if it uses a mesh).

Gameobjects are completely defined in the source code. A designer can only add gameobjects to the game world; a programmer must add components together to create new gameobjects.

## Object data
Prefabs and import data are sidecar files. They sit right next to the .c and .h files and are named instances you can load. For a prefab of point light, you have point_light.1.prefab

Import data also sits right next to the asset in the file directory. The name for asset.jpg is asset.import

## Resources
On engine startup, links to all assets are placed in a hidden .resources folder at the root of the project. Any content in the assets folder will get a symlink in the .resources folder. Humans shouldn't edit the .resources folder. There is one file in the resource folder for each asset found in the assets folder. They also define import options for the assets.

Filetypes include:
*Images*
.png
.jpg
.tif

*Sounds*
.wav
.aiff
.voc
.mod
.midi
.ogg
.mp3
.flac

*3D Models*
.gltf

*Shaders*
.glsl

*Fonts*
.ttf
.bff (bitmap font)

And yugine generated things:
.yugh (level)
.prefab (An object with some defined characteristics)

# Internals
## Render pipeline

3D rendering goes as the following:
Textures --->
Model----------------> Static actor
Meshes ----->
Raw dataCombination of dataGame world spawn of it

## Resources
Assets you add are used as resources in the game engine. Each asset has a number of "contexts". You decide how each context is handled for each asset. There are also default contexts you can set. As an example, a .png file can be used as a exture, as a sprite, as a font, as an animation, and so on. It can be used for one of these or more than one. When you use an image as a texture, in most game engines you create the texture and link the image to it; in ours, you simply use the image, and the game uses the appropriate context for the situation.

### Resource Views
There is a single resource panel which you can then filter based on what you want to see. You can filter by:
File type: .png, .jpg, ...
Resource type: image, video, model, ...
Resource context: texture, sprite, ... (They're only listed here if they are actually used in the game as one of these)

Plus, you can filter by folder. Most of the time I browse by filter and a search, but you can also filter by grouped objects. So a goblin might have a model, a texture, and some audio files. You can view everything that belongs to the goblin prefab.

### 3D cursor
In this 3d game engine, you're editing in 3d. Makes sense to have a 3d cursor to do so!

SPHERE SELECT
Left click, and then use the mouse wheel to enlarge your selection sphere. Release the left mouse to select everything in the sphere.

CUBE SELECT
Left click and drag to draw a square on any surface. Scroll UP to increase the selection volume out from the surface, and scroll DOWN to decrease it (and even make it negative).

## Real time editing
You can edit levels in real time with work partners. Start a server and send your partner your IP address and server password. They will gain the ability to edit your game. The host can remove editors from the server, ban them, and restrict their editing privledges (edit, play). Each editor in the room has a color assigned to them. When they select an object to move, the object is shaded their color and their name is overlayed so you know who is editing what.

When the edits are done, the host can distribute his copy of the game to everybody else, or they can click the download button to get the edits in full.

## Levels
A level can be either 3D or 2D. All levels feature familiar drag and drop editing, scripting, and so on.


## Device view: specifics on resolution, screen sizes, and color gamuts
You can emulate devices on your screen to see what it will physically look like on the target device. This is particularly useful for devices smaller than your editing screen, like tablets and the play date. You can also emulate device color gamuts. When editing, images will be scaled to fit the target device, based on the project target. For example, on a 4k project, a 200 pixel wide sprite will be rendered as a 100 pixel wide sprite when viewed on a 2k device emulator.

PlayDate: 400x240 pixels, 2.7 inches/68mm, 1 bit color.

This preview window has a free camera you can view the current scene with. It cannot be zoomed like the main editor can be. The window cannot be resized; it is determined by the selected device settings.

### Color scopes
The engine has a standard array of colorscopes for the user.

## Playing the game
When the Play button is selected, the device window becomes what you play the game on.

## Baking
Baking assets involves converting assets for use on a target device or platform. When a game is played, the engine will scale sprites in real time. If the assets have been baked, it will instead use the scaled assets. Baking sprites means to resize them to to not waste space on the target platform's screen, as well as pack them into atlases. Sounds need to be converted for each platform as well. In the bake menu, select a platform and device to bake the assets for. Once the platform and device have been baked, the game can be exported for that platform.

## Exporting
An exported game varies by platform, but generally it is basically a single big blob file of all the game's assets with the platform's executable as a sidecar. All baked assets for a platform are put inside of the blob file, and the executable expects the blob file to be next to it.

Game folder
  - engine.exe
    - content.blob

    ### Blob file
      - struct blob
- - int blobid (identifies this as a blob)
  - - int files (number of assets the blob contains)
    - - int gameversion
      - - int engineverison
- offset array[int files] (array of hashed values of the filepaths containing the byte offset of the data
  - data

  To get a filepath's data from the blob, the filepath is hashed, then it is looked up in the offset array. The data can be grabbed by copying it from the start of it to the start of the next file (ie file[hash+1]), because the data is stored in offset array order.

  ### Patching
  A patch can be created in the engine. To create a patch, specify the last release you want to patch from, and then export a blob. The new blob will contain only modified files compared with the old blob. Place the patch blob next to the old blob. When an asset is searched for, it will be searched for first in the new blob, and if not found will be searched for in the old blob. If an asset is found in neither, it will be searched for in the filepath.

  If another patch is made, all three blobs must be present (ie patch 1.3 requires the 1.2 patch which requires the base game).

  The game code comes with the ability to merge patches as a CLI. It merges everything into a blob for the most recent patch, ie updates the 1.3<-1.2<-base blobs to a 1.3 blob.

  ### Expansions
  Expansions are delivered as their own blobs, with their own patch paths. When an expansion blob is exported, if its blob is placed in the directory with the game runtime, it will be pulled in when the game starts. The main blob should be patched to accomodate "knowing" if the expansion is present or not, so you can for example add a new menu item on the start menu if an expansion is present.

  ### Selecting assets for export
  Starting with the game start level, all assets required for it to run are marked for export. Then scripts are investigated: if they include a reference to a level, or asset, or other script, all of these are looked at and their referenced assets pulled in. In this way, nothing that is not seen in the running game will be pulled into the blob. Any test levels will not be included in the export (unless you reference it somewhere in the game)

  To give somebody a development build, simply zip up the entire project and give it to them.

  ## Physics
  Each gameobject has a value field that represents what it is, from 0 to 15. Objects which move should be checked as "Kinematic", which then reveals two more bitmask fields: a 16 bit bitmask to represent what it should send trigger events to, and a 16 bit bitmask to represent what it should be blocked by. The 16 fields can freely be renamed by the game designer. Gameobjects can only be one thing.

  For example, bit 1 may be assigned to Environment. A tree would have bit 1 on its object field set to True. The player would then assign bit 1 on its Block block bitfield to "true", so that it cannot walk through the environment. Bit 2 you may assign to Triggers. The player would assign its Trigger bit 2 to true so that it recieves "Hit" events without actually being stopped by it.

  Sometimes, two non stationary objects can collide. As long as EITHER object has their Block bit set for the other object, a collision occurs and each object recieves a Block message. A trigger message is only sent to either object if they are set to recieve trigger events.

  ### Physics properties
  Objects have

  ## Scripting
  Each gameobject has a script attached to it. When the gameobject is selected, its script appears in the Script window.

  ## GUI

  ## The gamestate
  The gamestate is a global variable that holds all of the current information for the game. Various elements of the game engine has access to the gamestate in different capacities. The GUI can read the entire game state; a player can read and write his own status onto the gamestate.

  ## AI


  ## Yugh scheme
  Yugh scheme is the style of scheme used for scripting in game events. It is built on top of S7 scheme.

  Scheme hooks into the engine are highly abstracted away from the engine code. For example, the command "move" takes a single value as units/second. No matter if it is called in the update loop, or the physics loop, the given number will be multiplied by the relevant delta to make it work the same either way. In addition, if used on a static object, the object won't move; if used on a kinematic object it will move as expected; and if used on a dynamic object it will add the given number as a velocity, pushing the dynamic object instead of taking complete control.

  Rather than overriding hooks into engine loops, there are distinct scripts for every hook you can want. If you don't write script into them, they won't be used. There are hooks into rendering loops, animation states (for example, footfall moments in an animatio to play sounds), and more.
