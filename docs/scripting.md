# Yugine Scripting Guide

All objects in the Yugine can have an associated script. This script can perform setup, teardown, and handles responses for the object.

|function|description|
|---|---|
|setup|called before the object is loaded|
|start|called when the object is loaded|
|update(dt)|called once per game frame|
|physupdate(dt)|called once per physics tick|
|stop|called when the object is killed|