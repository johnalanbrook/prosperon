#include "log.h"

#include <time.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <GL/glew.h>

#define logLevel 0

void mYughLog(int category, int priority, const char *message,
	      int line, const char *file, ...)
{
    if (priority >= logLevel) {
	time_t now = time(0);
	char *dt = ctime(&now);
	dt[strlen(dt) - 1] = '\0';	// The above time conversion adds a \n; this removes it

	va_list args;
	va_start(args, message);
	char msgbuffer[ERROR_BUFFER] = { '\0' };
	vsnprintf(msgbuffer, ERROR_BUFFER, message, args);
	va_end(args);

	char buffer[ERROR_BUFFER] = { '\0' };
	snprintf(buffer, ERROR_BUFFER, "LEVEL %d :: %s [ %s:%d ] %s\n",
		 priority, msgbuffer, file, line, dt);

	//SDL_LogMessage(category, priority, buffer);
	printf(buffer);
	fflush(stdout);
    }
}

void FlushGLErrors()
{
    GLenum glErr = GL_NO_ERROR;
    glErr = glGetError();
    while (glErr != GL_NO_ERROR) {
	YughLog(SDL_LOG_CATEGORY_RENDER, SDL_LOG_PRIORITY_ERROR,
		"GL Error: %d", glErr);
	glErr = glGetError();
    }
}

int TestSDLError(int sdlErr)
{
    if (sdlErr != 0) {
	YughLog(0, SDL_LOG_PRIORITY_ERROR, "SDL Error :: %s",
		SDL_GetError());
	return 0;
    }

    return 1;
}