# Editor
Prosperon's visual editor is an assistant for the creation and editing of your game entities and actors. In the editor, all ur types are loaded, and assets are constantly monitored for changes for hot reloading.

To initiate it, execute `prosperon`.

## Editing entities
The desktop is the topmost entity that exists in the editor. Instead of editing specific files, you simply load them into your desktop, and go from there. This makes it easier to see two different entities simultaneously so you can ensure changes to one are congruous with the vision for the others.

The main editor view is made up of entities. Each entity can have a number of components attached to it. When an entity is selected, its name, position, and list of components are listed.

Basic use of the editor involves spawning new entities, or ones from already made ur types, editing them, and then saving them as new ur types or overwriting the ones they spawned from. Specific tools have been written to make editing components and levels easier than with a text editor, and the editor is easily extendable for your own purposes.

Assign the entity's *gizmo* property to a function to have that function called each gui rendering frame.

## The REPL
The REPL lets you poke around in the game. It makes iteration and experimentation fast, fun, and easy.

The symbol `$` references the current REPL entity. If no entity is selected, the REPL entity is the currently edited one. Otherwise, it is the selected entity, or group of entities, as an array.

!!! scholium
    Easily run commands on multiple entities using Javascript functions like for each.
    ```
    $.forEach(e => console.log(e.pos));
    ```

The REPL is a powerful tool for editing your game. Arbitrary code can be ran in it, meaning any esoteric activity you need done for your game can be done easily. Commonly used functions should be copied into your /editorconfig.js/ to be called and used at will.

## Playing the game
Playing the game involves running the game from a special /debug.js/ file, or from the beginning, as if the game were packaged and shipped.

| key   | action                                              |
|-------|-----------------------------------------------------|
| f5    | Play the game, starting with entry point /debug.js/ |
| f6    | Play the game from the beginning                    |

While playing the game, a limited editor is available that allows for simple debugging tasks.

| key | action                      |
|-----|-----------------------------|
| C-p | Pause                       |
| M-p | One time step               |
| C-q | Quit play, return to editor |

## Script Editor
Prosperon comes with an in-engine script editor. It implements a subset of emacs, and adds a few engine specific features.

### Syntax coloring? ... nope!
The editor that ships with Prosperon has *context coloring*, which is a good deal more useful than syntax coloring.

## Debugging
Debugging functions are mapped to the F buttons, and are available in any debug build of the game. Pressing the specified key toggles the feature; pressing it with /alt/ shows a legend for that feature.

| key | description                |
|-----|----------------------------|
| F1  | Draw physics info          |
| F3  | Draw bounding boxes        |
| F12 | Draw gui info              |
