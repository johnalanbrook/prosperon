# Exporting your game
Prosperon is a multiplatform engine. Bundling your game for these platforms essentially involves three steps:

- Baking static content
- Conversion of assets
- Packing into a CDB

To distribute your game for a given platform, run `prosperon build {platform}`.

| platform |
|----------|
| Linux    |
| MacOS    |
| Windows  |

You will find your game ready to go. Rename the executable to the name of your game and run it to play. Congratulations!

## Building static content
Static content creation involves any number of optimizations.

- Bitmap font creation
- Texture map creation

Creation of these assets is invisible. Prosperon updates its understanding of how to pull assets based on the existance of these packed ones.

## Converting assets
Images, videos, and sounds, are converted to assets most suitable for the target platform. This may be for speed or simple compatability. *You do not need to do anything*. Use your preferred asset types during production.

## Packing into a CDB
A *cdb* is known as a "constant database". It is a write once type of database, with extremely fast retrieval times. Packing your game into a cdb means to create a database with key:value pairs of the filenames of your game. The Prosperon executable is already packed with a core cdb. Your game assets are packed alongside it as the game cdb.

You can create your game's cdb by running `prosperon -b`. You will find a *game.cdb* in the root directory.

* Modding & Patching
When an asset is requested in Prosperon, it is searched for in the following manner.

1. The cwd
2. The game cdb (not necessarily present)
3. The core cdb
   
Game modification is trivial using this described system. By putting an asset in the same path as the asset's location in the game cdb, when that asset is requested it will be pulled from the file system instead of the game cdb.

Given a Prosperon-built game, you can unpack its content into a directory by running `prosperon unpack {game}`.

## Shipping
Once a game's assets are modified, it may be desirable to ship them. Run `prosperon patch create {game}` to create a /patch.cdb/ filled only with the files that are different compared to those found in the /game.cdb/ in the /game/.

To update /game/ to use the new patch, run `prosperon patch apply {patch}`, replacing /patch/ with the name of the cdb file generated above.

Many patches can be bundled by running `prosperon patch bundle {list of patches}`. This creates a patch that will update the game as if the user had updated each patch in order.

Mods can be distributed with the same idea.
