# The Scene

There are a multitude of representations of your entities in the engine. The two most prominent are the entity system, where entities can be master or thrall to a master. And then there is the physics view, which is mostly flat, where objects are all part of a "world", but can be connected to one another via various physics constraints:

- Pin joints, rigidly connected
- Slide joints, with a mix or max distance
- Pivot points, stuck to position but allow rotation
- Groove joint
- Damped spring
- Damped rotary spring
- Rotary limit
- Ratchet joint
- Gear joint
- Simple motor

The most common sort of connection will be master and thrall. When editing, this relationship allows for simple

## Entity vs Global
The physics engine stores all objects on the global level. The center of the world is [0,0]. Physics does not handle scaling at all.

The master/thrall relationship is closer to a scene graph. When the master moves, its thralls move, and their thralls move, etc. When a master scales, it is the same.

When the game starts simulating, objects ignore their parent relationship after they are created. They become only part of the world and simulate as such.