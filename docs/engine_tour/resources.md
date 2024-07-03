# Resources
Assets can generally be used simply with their filename. Assets can be modified with a sidecar file named *filename.asset*, so, a file `ball.png` can have additional parameters through its `ball.png.asset` file.

| sigil  | meaning                |
|--------|------------------------|
| \slash | root of project        |
| @      | root of save directory |
| #      | root of link           |

Resources can be referenced in a relative manner by actor scripts. When it comes to actors using assets, relative filepaths are useful and encouraged.

```
/
  score.wav
  /bumper
    hit.wav
    bumper.jso
  /ball
    hit.wav
    ball.jso
```

Path resolution occurs during actor creation. In effect, a reference to *hit.wav* in *bumper.jso* will resolve to the absolute path */bumper/hit.wav*.

If the asset is not found, it is searched for until the project root is reached. The bumper can reference *score.wav* and have the path resolution take place. Later, if the it is decided for the bumper to have a unique score sound, a new /score.wav/ can be placed in its folder and it will work without changing any code.

!!! caution
    Because the path is resolved during object load, you will need to fresh the bumper's ur or spawn a new bumper for it to use the newly placed /score.wav/.

#### Links
Links can be specified using the "#" sign. These are shortcuts you can specify for large projects. Specify them in the array `Resources.links`.

An example is of the form `trees:/world/assets/nature/trees`. Links are called with `#`, so you can now make a "fern" with `Primum.spawn("#trees/fern.jso")`.

### Ur auto creation
Instead of coding all the ur type creation by hand, Prosperon can automatically search your project's folder and create the ur types for you. Any /[name].jso/ file is converted into an ur with the name. Any /[name].json/ file is then applied over it, should it exist. If there is a /.json/ file without a corresponding /.jso/, it can still be turned into an ur, if it is a valid ur format.

Folders and files beginning with a '.' (hidden) or a '_' will be ignored for ur creation.

The folder hierarchy of your file system determines the ur prototype chain. /.jso/ files inside of a folder will be subtyped off the folder ur name.

Only one ur of any name can be created.

```
@/
  flipper.js
  flipper/
    left.js

@/
  flipper/
    flipper.js
    left/
      left.js
```

`prototypes.generate_ur(path)` will generate all ur-types for a given path. You can preload specific levels this way, or the entire game using `prototypes.generate_ur('.')`. If your game is small enough, this can have a massive runtime improvement.
