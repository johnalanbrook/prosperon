# Scripting
The scripting language used in Prosperon is Javascript, with QuickJS. It is ES2023 compliant. It is fast.

!!! scholium
    Javascript is used here mostly to set up objects and tear them down. Although computationally expensive tasks can be done using QuickJS, Prosperon makes it easy to extend with raw C.

### How Prosperon games are structured
Prosperon schews the CommonJS and ES module systems for a custom one suitable for a computer game actor model. It is more restricted than either system, while retaining their full power.

Prosperon games are structured into two types of source files:
- scripts
- actors

Scripts end with a return statement. A script can return a function, an object of functions, or any other sort of value.

!!! scholium
    It is a common requirement to add some amount of functionality to an object. It can be easily done with a script file.
    
    ```
    *script.js*
    function hello() { say("hello"); };
    return hello;
    ```

    ```
    var o = {};
    o.use("hello.js");
    o.hello();
    ```

    The `use` function on any object loads a module, and `assign`s its return to the object.

Scripts are loaded into memory only once. Further `use` statements only generate references to the statements. Because *scripts* are executed in a lambda environment, any `var` declared inside the script files are effectively private variables, persistent variables.

In a *script*, `this` refers to `undefined`. It is nothng.

In an *actor* source file, `this` refers to the actor. Actors do not end in a `return` statement. *actor* source is intended to set up a new agent in your game. Set up the new entity by loading modules and assigning functions to `this`.

### Script entry points
The first way you can customize Prosperon is by adding scripts to the folder you're running it from. Any file ending with *.js* is a *script* which can be ran by Prosperon.

| script          | When called                                 |
|-----------------|---------------------------------------------|
| config.js       | First script on Prosperon load              |
| game.js         | Entry point for running the game            |
| editorconfig.js | Entry point for running the editor          |
| predbg.js       | Called before running debug                 |
| debug.js        | Debug set up                                |
| dbgret.js       | After stopping debug mode, used for cleanup |

!!! scholium
    Try it out. Add a script called `config.js` to your folder, with this.

    ```
    console.log("Hello world!");
    Game.quit();
    ```
    Run `prosperon`. You will see "Hello world!" in the console, and it shuts down.


Using `config.js` and `game.js`, you can write an entire game, without reaching any further. When you want to populate a world with independent actors, entities are what you will reach for.
