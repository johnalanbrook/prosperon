# SOUND

Primum's sound system is well integrated and robust. It comes with flexibility in mind, to enable sounds to easily change as the result of occuring gameplay.

At the highest level, audio sources feed into a series of nodes, which eventually output to the mixer. All DSP is handled as floating point.

The game should specify a SAMPLERATE, BUF_FRAMES, and CHANNELS.

SAMPLERATE: samples per second
CHANNELS: number of output channels.
BUF_FRAMES: Number of frames to keep in the buffer. Smaller is lower latency. Too small might mean choppy audio.

## Audio sources
Audio sources include wav files, midi files, mod files, etc.

## Audio instances
Audio sources can be played multiple times after loaded into the game. Each instance 

## DSPs / nodes
Nodes are how audio ends up coming out of the player's speakers. Only one node is specified at runtime: the master node, which outputs its inputs to the computers' speakers. It has 256 inputs.

Each node can have any number of inputs to it, defined when the node is created.

Nodes are run starting with the master node, in a pull fashion. The master node checks for inputs, and from each input pulls audio foward.

Audio instances are a type of node with a single output, and can be fed directly into the master node.

A node has inputs and outputs. All inputs into a node are mixed together, and then processed by the node. The node might clip them, apply a filter, or do any other number of things.

## Scripting
