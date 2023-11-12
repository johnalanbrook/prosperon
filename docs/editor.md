# Primum Editor

The main editor view is made up of objects. Each object can have a number of components attached to it. When an object is selected, its name, position, and list of components are listed.

In addition, a window showing each entity underneath that entity are shown.

## The desktop

The desktop is the topmost object that exists in the editor. Instead of editing specific files, you simply load them into your desktop, and go from there. This makes it easier to see two different entities simultaneously so you can ensure changes to one are congruous with the vision for the others.

## *'s and %'s

When a '*' is seen next to an entity's name, that means it is altered compared to its ur-type and is unsaved. There are a number of ways to take care of a '*'. If you do not do one of the below, something on the entity will be lost.

- Changes can be saved to the ur-type. This makes all other entities derived from the ur-type change.
- Changes can be saved as a sub ur-type. This creates a brand new type that can then be reused over and over again.
- Changes can be saved by saving the containing ur-type. Ur-types remember variances in the entities it 'owns'.

When an entity is different from its ur-type, but the variance is saved due to its container, its name is preceded by a '%'.

The function 'revert()' can be called on any entity to make it like its ur-type again.

## Levels?

The concept of a 'level', a collection of spawned entities, is handled simply by creating sub ur-types of an empty entity. 

## Editing level, ur-types, parents, children, etc

lvl1
  tablebase
    %flipper

In this case, tablebase is saving a modified flipper.

lvl1
  %tablebase
    %flipper

This is ambiguous. lvl1 could be storing the flipper's diff, or tablebase could be. Additionally, tablebase could have a unique flipper, and then lvl1 also alters it.
