#ifndef ENGINE_H
#define ENGINE_H

#define FPS30 33
#define FPS60 17
#define FPS120 8
#define FPS144 7
#define FPS300 3

#define sFPS30 0.033
#define sFPS60 0.017
#define sFPS120 0.008
#define sFPS144 0.007
#define sFPS300 0.003

extern double renderMS;
extern double physMS;
extern double updateMS;

void engine_init();
void engine_stop();

#endif
