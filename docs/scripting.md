# Yugine Scripting Guide

Script hooks exist to allow to modification of the game.

|config.js|called before any game play, including play from editor|
|game.js|called to start the game|
|editorconfig.js|called when the editor is loaded, used to personalize|
|predbg.js|called when play in editor is selected, before level load|
|debug.js|called when play in editor is selected, after level load|
|dbgret.js|called when play in editor returns to editor|

All objects in the Yugine can have an associated script. This script can perform setup, teardown, and handles responses for the object.

|function|description|
|---|---|
|start|called when the object is loaded|
|update(dt)|called once per game frame tick|
|physupdate(dt)|called once per physics tick|
|stop|called when the object is killed|
|debug|use draw functions with the object's world position, during debug pass|
|gui|draw functions in screen space, during gameplay gui pass|
|draw|draw functions in world space|