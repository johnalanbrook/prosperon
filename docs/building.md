# Building Primum

Building is done via a basic makefile with flags. Type "make" to build for the platform you are on. Targets include:

|crossmac|On an ARM mac, build a bundled intel and arm mac version|
|crosswin|On any POSIX computer with a GCC win32 toolchain, build for windows platforms|
|clean|clean up|
|tags|build tags file|
|cdb|build the cdb tool to inspect primum pak files|
|shaders|rebuild all shader headers|
|packer|tool for building primum pak files|
|input.md|editor input documentation|
|api.md|scripting API|
|switch|build for the nintendo switch|
|ps5|build for the ps5|
|playdate|build for the playdate|

Basic boolean flags, set to 0 or 1, to enable or disable features in the build
|DBG|debugging info in binary|
|NFLAC|true to not build flac ability|
|NMP3|true to not build mp3|
|NSVG|true to build without SVG|
|NEDITOR|true to not include editor|
|STEAM|true to build steam support|
|EGS|true to build with EGS support|
|GOG|true to build with GOG support|

## OPT
 -0 No optimization
 -1 Full optimization
 -small Optimize for smallest binary

## Building for Steam
 -Get the steam SDK
 -Unpack it into a folder named 'steam' at the top level directory
 -Move the steam libs into the lib folders arm64, x86, and x86_64
 -Make with STEAM=1

Steam uses a C++ based SDK, so a C-only compiler like TCC will not work.

## Building with Discord
 -Get the steam SDK
 -Make with DISCORD=1