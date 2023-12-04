# Entities

Entities are defined by creating a .jso script in your game directory. Variants of the original entity can be created by defining a .json file, typically done via the provided editor. The original entity is known as its ur-type. If you create a player.jso file in your game directory, a player can be spawned by saying Primum.spawn(ur.player).

An entity which differs from its ur will have an asterisk * next to its name.

Entities have components. The components all do a thing. Components are placed relative to the entity.

All entities are child to Primum, once the simulation is running.

Entites can be static, kinematic, or dynamic. Static entities never move. Kinematic entities move via explicit command. Dynamic ones move under the auspices of the game world.

Components, if they have a defined transform, are relative to the entity they reside on. Components cannot have components, ensuring that components can be processed and rendered in any order.

Entities are defined via a typical scene graph system. This allows for easy reuse of components when designing levels.

When the game is run, the movement relationships are broken, and the physics system takes over. In the physics engine, each entity is a direct child of the world. Objects can still be constrained to other objects via the physics system.
