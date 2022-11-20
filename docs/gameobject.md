# Yugine Gameobjects

## The world

A game is defined by the world. The "world" is a list of gameobjects. Each gameobject has a unique ID it can be referenced by. Gameobjects have a position and rotation in the world

## Components
Components are part of a gameobjects. Different components, and varying values on those components, are what make game objects unique.

## Prefabs
A "prefab" is simply a gameobject blueprint. It defines the gameobject and all of its components.

Prefabs have a hierarchy you can trace. Each prefab descendent can have modified values compared to its parent. When the parent changes, the lower prefabs change, too, IF they have no altered the given value from the parent.