#ifndef CV_H
#define CV_H

#include <SDL3/SDL.h>

#ifdef __cplusplus
extern "C" {
#endif
  int detectImageInWebcam(SDL_Surface *webcam, SDL_Surface *img);

#ifdef __cplusplus
}
#endif

#endif
