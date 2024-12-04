#ifndef CV_H
#define CV_H

#include <SDL3/SDL.h>

#ifdef __cplusplus
extern "C" {
#endif
  bool detectImageInWebcam(SDL_Surface *webcam, SDL_Surface *img, double threshold);

#ifdef __cplusplus
}
#endif

#endif
