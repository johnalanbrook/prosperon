#ifndef LOG_H
#define LOG_H

#include <SDL_log.h>

#define ERROR_BUFFER 2048

#define YughLog(cat, pri, msg, ...) mYughLog(cat, pri, msg, __LINE__, __FILE__, __VA_ARGS__)

void mYughLog(int category, int priority, const char *message,
	      int line, const char *file, ...);

void FlushGLErrors();

int TestSDLError(int sdlErr);

#endif