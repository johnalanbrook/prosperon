# Yugine Scripting Guide

Primum programs are organized into two different types of source files: scripts and entities. Scripts end with .js, entities end with .jso.

## Scripts

Script hooks exist to allow to modification of the game.

|config.js|called before any game play, including play from editor|
|game.js|called to start the game|
|editorconfig.js|called when the editor is loaded, used to personalize|
|predbg.js|called when play in editor is selected, before level load|
|debug.js|called when play in editor is selected, after level load|
|dbgret.js|called when play in editor returns to editor|

In addition, any script can be run by running "load".

## Entities

Entities are defined in a jso file. The "this" parameter in the jso file is a reference to the actor, allowing you to define properties on it.

Computation takes place in turns. Each entity has functions called, if they exist. If a machine has multiple threads, multiple entities may be taking turns at a time.

|function|description|
|---|---|
|start|called when the object is loaded|
|update(dt)|called once per game frame tick|
|physupdate(dt)|called once per physics tick|
|stop|called when the object is killed|
|debug|use draw functions with the object's world position, during debug pass|
|gui|draw functions in screen space, during gameplay gui pass|
|draw|draw functions in world space|