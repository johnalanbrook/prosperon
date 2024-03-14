#include "log.h"

#include "render.h"
#include <time.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include "yugine.h"
#include "resources.h"

#include "script.h"

char *logstr[] = { "spam", "debug", "info", "warn", "error", "panic"};
char *catstr[] = {"engine", "script", "render"};

static FILE *logout; /* where logs are written to */
static FILE *writeout; /* where console is written to */

void log_init()
{
#ifndef NDEBUG
  logout = fopen(".prosperon/log.txt", "w");
  writeout = fopen(".prosperon/transcript.txt", "w");
#endif
}

void log_shutdown()
{
  fclose(logout);
  fclose(writeout);
}

const char *logfmt = "%s:%d: [%s] %s, %s: ";
void mYughLog(int category, int priority, int line, const char *file, const char *message, ...)
{
#ifndef NDEBUG
  time_t now = time(NULL);
  struct tm *tinfo = localtime(&now);
  char timebuf[80];
  strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", tinfo);

  fprintf(logout, logfmt, file, line, timebuf, logstr[priority], catstr[category]);

  va_list args;
  va_start(args, message);
  vfprintf(logout, message, args);
  va_end(args);
  fprintf(logout, "\n");
  
  if (priority == LOG_DEBUG) {
    printf(logfmt, file, line, timebuf, logstr[priority], catstr[category]);
    va_list args;
    va_start(args,message);
    vprintf(message, args);
    va_end(args);
    printf("\n");
    fflush(stdout);
  }
#endif
}

/* print to stdout and console */
void log_print(const char *str)
{
#ifndef NDEBUG
  fprintf(writeout, str);
#endif
  printf(str);
  fflush(stdout);
}

void sg_logging(const char *tag, uint32_t lvl, uint32_t id, const char *msg, uint32_t line, const char *file, void *data) {
  lvl = LOG_PANIC-lvl;
  mYughLog(LOG_RENDER, lvl, line, file, "tag: %s, id: %d, msg: %s", tag, id, msg);
}
