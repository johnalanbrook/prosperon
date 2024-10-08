#ifndef CONFIG_H
#define CONFIG_H

#define MAXPATH 256 /* 255 chars + null */
#define MAXNAME 50

#define PI 3.14159265358979323846264338327950288f
#define DEG2RADS 0.0174532925199432957692369076848861271344287188854172545609719144f
#define RAD2DEGS 57.2958f

#if defined __linux__
  #define SOKOL_GLCORE
#elif __EMSCRIPTEN__
  #define SOKOL_WGPU
#elif __WIN32
  #define SOKOL_D3D11
#elif __APPLE__
  #define SOKOL_METAL
#endif

#endif
