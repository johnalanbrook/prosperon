# Input
Input is done in a highly generic and customizable manner. *players* can take control of any object (actor or otherwise) in Prosperon, after which it is referred to as a *pawn* of a player. If the object has a defined *input* object, it is a valid pawn. One player can have many pawns, but each pawn may have only one player.

Pawns are added as a stack, with the newest ones getting priority, and handled first. It is possible for pawns to block input to lower pawns on the stack.

```
*newest*
car <== When a key is pressed, this is the first pawn to handle input
player
ui <== /block/ is set to true here, so editor recieves no input!
editor
*oldest*
```

The default player can be obtained with `Player.players[0]`. Players are all local, and the highest number is determined by platform.

The **input** object defines a number of keys or actions, with their values being functions.

## Editor input
The editor input style defines keystrokes. It is good for custom editors, or any sort of game that requires many hotkeys. Keystrokes are case sensitive and can be augmented with auxiliary keys.

| symbol | key   |
|--------|-------|
| C      | ctrl  |
| M      | alt   |
| S      | super |

```
var orc = Primum.spawn(ur.orc);
orc.inputs = {};
orc.inputs.a = function() { ... };
orc.inputs.A = function() { ... }; /* This is only called with a capital A! */
orc.inputs['C-a'] = function() { ... }; /* Control-a */
Player.players[0].control(orc); /* player 0 is now in control of the orc */
```

The input object can be modified to customize how it handles input.

| property       | type     | effect                               |
|----------------|----------|--------------------------------------|
| post           | function | called after any input is processed  |
| =release_post= | function | called after any input is released   |
| fallthru       | bool     | false if input should stop with this |
| block          | bool     | true if input should stop with this  |

The input can be modified by setting properties on the associated function.

| property | type     | effect                                                 |
|----------|----------|--------------------------------------------------------|
| released | function | Called when the input is released                      |
| rep      | bool     | true if holding the input should repeatedly trigger it |
| down     | function | called while the input is down                         |
