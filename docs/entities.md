# Entities

Entities are defined by creating a .jso script in your game directory. Variants of the original entity can be created by defining a .json file, typically done via the provided editor. The original entity is known as its ur-type. If you create a player.jso file in your game directory, a player can be spawned by saying Primum.spawn(ur.player).

An entity which differs from its ur will have an asterisk * next to its name.

Entities have components. The components all do a thing. Components are placed relative to the entity.

All entities are child to Primum, once the simulation is running.

Entites can be static, kinematic, or dynamic. Static entities never move. Kinematic entities move via explicit command. Dynamic ones move under the auspices of the game world.

Components, if they have a defined transform, are relative to the entity they reside on. Components cannot have components, ensuring that components can be processed and rendered in any order.

Entities are defined via a typical scene graph system. This allows for easy reuse of components when designing levels.

When the game is run, the movement relationships are broken, and the physics system takes over. In the physics engine, each entity is a direct child of the world. Objects can still be constrained to other objects via the physics system.

The transform properties have local, global, and screen space accessors. For example, for position ..

- pos: the relative position of the object to its master
- worldpos: the position of the object in the world
- screenpos: the position of the object in screen space

The core engine only remembers each object's world position. When its position is requested, it figures its position based on its parent.

## Inheritence? Composition?

The idea behind how the engine is oragnized is to encourage composition. Compisition creates cleaner objects than does inheritence, and is easier to understand. And yet, inheritence is useful for specific circumstances.

- Creating a specific unique object in a level

There are only two cases where inheritence can be utilized in this engine:
1. Whole entities can be subtyped. Only values can be substituted. An ur-type must define all functions and behaviors of an object. but then that can be subtyped to change only a handful of properties.

2. Entities can be subtyped when thrall to another entity. This can only be done once. Thrall objects, like components, can be defined only once on an ur-type. Sub types can only change parameters, and not of the sub objects.