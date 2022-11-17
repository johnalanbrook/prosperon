#include "log.h"

#include "render.h"
#include <time.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#define logLevel 0

//char *logstr[] = { "INFO", "WARN", "\x1b[1;31mERROR\x1b[0m", "CRITICAL" };
char *logstr[] = { "INFO", "WARN", "ERROR", "CRITICAL" };
char *catstr[] = {"ENGINE"};

void mYughLog(int category, int priority, int line, const char *file, const char *message, ...)
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
	snprintf(buffer, ERROR_BUFFER, "%s | %s | %s [ %s:%d ] %s\n", logstr[priority], catstr[0], dt, file, line, msgbuffer);

	printf("%s", buffer);
	fflush(stdout);
    }
}

void FlushGLErrors()
{
    GLenum glErr = GL_NO_ERROR;
    glErr = glGetError();
    while (glErr != GL_NO_ERROR) {
	YughLog(0, 3,
		"GL Error: %d", glErr);
	glErr = glGetError();
    }
}
