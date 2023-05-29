# Yugine Scripting

Scripting is done with JS.

Load up objects with functions that are called back at specific times.

Levels and objects have certain functions you can use that will be
called at particular times during the course of the game running.

setup
  Called once, when the object is created.

start
  Called once when the gameplay is simulating.

update(dt)
  Called once per frame, while the game is simulating

physupdate(dt)
  Called once per physics step

stop
  Called when the object is destroyed, either by being killed or otherwise.

.Collider functions
Colliders get special functions to help with collision handling.

collide(hit)
  Called when an object collides with the object this function is on.

"hit" object
  normal - A vector in the direction the hit happened
  hit - Object ID of colliding object
  sensor - True if the colliding object is a sensor
  velocity - A vector of the velocity of the collision

.Draw functions
draw()
  Called during lower rendering

gui()
  Called for screen space over the top rendering

debug()
  Called during a debug phase; Will not be called when debug draw is off
  
