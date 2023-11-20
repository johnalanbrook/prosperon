# Yugine Input Guide

Any Javascript object can be controlled by a Player, becoming a pawn. Each Player has their input sent to all of their pawns. To receive input, pawns must have a specifically named object. There are multiple possible input systems to receive input on.

## inputs
The 'inputs' object receives emacs-like keyboard controls, like C-K (ctrl-K), M-k (alt-k), k (just k).

inputs['C-k'] | function to call when C-k is pressed
inputs['C-k'].up | function to call when C-k is released
inputs['C-k'].rep | True if this down action is repeatable

These inputs are ideal for editor controls.

For these controls, pawn control order can matter. Pawns that are controlled later consume input commands, making it easy to control a specific editor component to alter the functionality of a portion of the editor.

## actions & input maps
Actions are a named input, ideal for game controls. Rather than say that "x" jumps, it might be better to name a "jump" action.

## Blocking
If 'block' is set to false on the input object, it will not block lower inputs from using the controls.