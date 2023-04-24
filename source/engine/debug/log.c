#include "log.h"

#include "render.h"
#include <time.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

int logLevel = 1;

/* Four levels of log:
   0 info
   1 warn
   2 error
   3 critical
*/

//char *logstr[] = { "INFO", "WARN", "\x1b[1;31mERROR\x1b[0m", "CRITICAL" };
char *logstr[] = { "info", "warn", "error", "critical" };
char *catstr[] = {"engine", "script"};

FILE *logfile = NULL;

#define CONSOLE_BUF 1024*1024*5/* 5MB */

char lastlog[ERROR_BUFFER] = {'\0'};
char consolelog[CONSOLE_BUF] = {'\0'};

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
	snprintf(buffer, ERROR_BUFFER, "%s:%d: %s, %s: %s\n", file, line, logstr[priority], catstr[category], msgbuffer);

	log_print(buffer);
  }
}

void log_print(const char *str)
{
  fprintf(stderr, "%s", str);
  fflush(stderr);

  strncat(consolelog, str, CONSOLE_BUF);

  if (logfile) {
    fprintf(logfile, "%s", str);
    fflush(logfile);
  }

  snprintf(lastlog, ERROR_BUFFER, "%s", str);
}

void log_setfile(char *file) {
    freopen(file, "w", stderr);
    freopen(file, "w", stdout);
}

void log_cat(FILE *f) {
    char out[1024];

    while (fgets(out, sizeof(out), f)) {
        out[strcspn(out, "\n")] = '\0';
        YughInfo(out);
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
