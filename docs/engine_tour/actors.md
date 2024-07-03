# Actors


The fundamental tool for building in Prosperon is the actor system. Actors run independently from each other. Actors are defined by a combination of code and data. All actors have a *master* which controls certain properties of the actor.

The most masterful actor is the *Empyrean*. The first actor you create will have the Empyrean as its master. Subsequent actors can use any other actor as its master.

| fn                  | description                                              |
|---------------------|----------------------------------------------------------|
| spawn(text, config) | Creates an actor as the padawan of this one, using text  |
| kill()              | Kills an actor                                           |
| delay(fn, seconds)  | Calls 'fn' after 'seconds' with the context of the actor |

### Actor Lifetime
When an actor dies, all of the actors that have it as their master will die as well.

### Turns
Actors get fragments of time called a *turn*. Actors which belong to different systems can have different lengths of turns.

### Actor files
Actor files end with the extension *.jso*. They list a series of functions to call on a newly formed actor. Actors have a number of useful functions which are called as defined.

| function | call time                                                |
|----------|----------------------------------------------------------|
| start    | The first function called when the actor is in the world |
| update   | Called once per turn                                     |
| gui      | Called on GUI draw                                       |
| stop     | Called when the actor is killed                          |
| gizmo    | Called by the editor when the entity is selected         |

!!! scholium
    Create a new actor, then kill it.
    ```
    var act_die_call = function() {
      console.log(`Actor ${this.id} has died.`);
    }
    var act1 = Empyrean.spawn();
    var act2 = actor1.spawn();
    act1.stop = act_die_call;
    act2.stop = act_die_call;
    Empyrean.kill(); /* Error: The Empyrean cannot be killed */
    act1.kill();
    act2.kill(); /* Error: act2 has been killed because act1 was */
    ```

!!! scholium
    Now simplify by putting the code into a file named *hello.jso*.
    ```
    this.stop = function() {
      console.log(`Actor ${this.id} has died.`);
    }
    ```
    Now spawn two actors using it.
    ```
    var act1 = Empyrean.spawn("hello.jso");
    var act2 = act1.spawn("hello.jso");
    ```


### Actor configuration
Actors can be created using an optional configuration file. A configuration file is one of any accepted data types. Currently, JSON or [[https://www.crockford.com/nota.html][Nota]]. Configuration files are loaded after an actor's script file, overwriting any defined values on it.

!!! scholium
    Add a name for the actor to take on using a configuration file named *hello.json*.
    ```
    {
     "name": "Actor 1"
    }
    ```
    Now create *hello.jso* to use it.
    ```
    this.start = function() { console.log(`I, ${this.name}, have been created.`); }
    ```


## Entities
Game worlds are made of entities. Entities are a type of actor with a number of useful properties. Entities can only be created on the actor named *Primum*. The Primum is the outermost actor with a physical space. While Actors are more abstract, Entities exist in a definite space, with a position, rotation, and so on. Entities can respond to physics and play sounds. Anything which can be thought of as having a position in space should be an entitiy.

!!! scholium
    The first and most masterful entity is the Primum. The Primum has no components, and its rotation and position are zero. It defines the center of the game.

In editor mode, when an entity moves, all of its *padawans* also move.

When the game is actively simulating, this only holds if there are physical constraints between them.

Prosperon automatically generates physical pin constraints between objects with the appropriate physical properties.

### Adding Components
Entities can have *components*. Components are essentially javascript wrappers over C code into the engine. Scripting is done to set the components up on entities, after which most of the work is done by the C plugin.

!!! scholium
    For example, to render an image, set up a *sprite* component on an entity and point its path to an image on your harddrive.
    ```
    var ent = Empyrean.spawn();
    var spr = ent.add_component(component.sprite);
    spr.path = "image.png";
    ```
    Put that into your config file and run `prosperon`. You should see the contents of "image.png" on the screen.
    
    Try using an animated gif. Prosperon has native support for gif animations!


Components only work in the context of an entity. They have no meaning outside of a physical object in the world. They have no inherent scripting capabilities.

While components can be added via scripting, it is easier to add them via the editor, as we will later see.

### Ur system
When prosperon starts, it searches for urs by name. Any file ending in ".jso" or ".json" will be interpereted as an ur, with same named jso and json being applied as (text, config) for an ur. A jso or json alone also constitute an ur.

An ur can also be defined by a json file. If an ur is found, it takes predecent over auto generated urs. The json of an ur looks like this:

| field | description |
|----|----|
| text | Path to a script file, or array of script files, to apply to the object |
| data | Path to a json file, or array of json files, to apply to the object |

Any ur file with this sort of json creates an ur which can be created in the game. A file named "box.ur" will be ingested and be available as "ur.box". When saving differences, it creates a json file with the same name as an ur (in this case, "box.json").

!!! scholium
    Create an ur from the *hello* files above, and then spawn it.
    ```
    ur.create("hello", "hello.jso", "hello.json");
    Primum.spawn(ur.hello);
    ```
    When creating an actor from source files, all of its setup must take place. In this example, the setup happens during *ur.create*, and spawning is simply a matter of prototyping it.

This method allows high composability of game objects.

If an entity is created without an ur, is ur is defined as its given text and data. It cannot be saved. It must be given a new ur name.

Objects can be composed on the fly by stringing together urs. For example, a "2x.json" might define scale as 2x. Then, you can create a goblin with `ur.goblin`, or a large goblin with `ur.goblin.2x`. This creates a goblin object, and then applies the 2x scripts and jsons onto the object.

### Urs in game

Each ur has the following fields.

| field     | description                                                 |
|-----------|-------------------------------------------------------------|
| instances | An array of instances of this ur                            |
| name      | Name of the ur                                              |
| text      | Path to the script file                                     |
| data      | Object to write to a newly generated actor                  |
| proto     | An object that looks like a freshly made entity from the ur |

An *ur* has a full path given like `ur.goblin.big`. `goblin` and `big` can both possibly have a *.jso* script as well as a *data* file.

When `goblin.big` is created, the new object has the `goblin` script run on it, followed by the `big` script. The `data` fields consist of objects prototyped from each other, so that the `__proto__` of `big.data` is `goblin.data`. All fields of this objects are assigned to the `big goblin`.

The unaltered form of every ur-based-entity is saved in the ur's `proto` field. As you edit objects, the differences between how your object is now, compared to its `ur.proto` is a list of differences. These differences can be rolled into the `ur`, or saved as a subtype.

### Prototyping Entities
Ur types are the prototype of created entities. This makes it trivial to change huge swathes of the game, or make tiny adjustments to single objects, in a natural and intuitive way. When a value is changed on an entity, it is private. When a value is changed on an ur, it propogates to all entities. Values cannot be added or removed in subtypes.

Entities all have the following functions to assist with this:

| function      | use                                         |
|---------------|---------------------------------------------|
| clone(parent) | Create an entity prototyped from the parent |
| dup(parent)   | Create an exact duplicate of the parent     |
| revert()      | Removes all local changes on the entity     |

Speaking of practical experience, is best for ur prototype chains to be shallow.

### Spawning
Actor data and ur types can remember which entities were contained in it when saving. They are stored in the *objects* field. When an entity with an *objects* field is spawned, it spawns all of the objects listed in turn.

When an entity is spawned, it is addressable directly through its master entity. Its name is generated from its file or ur type name.

!!! scholium
    Let's take a simple RPG game.
    ```
    Primum
     level1
       orc
       goblin
       human
        sword
     ui
    ```
    The orc, for example, is addressable by `Primum.level1.orc`. The `human` has a `sword` spawned underneath it. When he is killed, his sword also disappears.
