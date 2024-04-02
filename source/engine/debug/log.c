#include "log.h"

#include "render.h"
#include <time.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include "yugine.h"
#include "resources.h"
#include "quickjs/quickjs.h"

#include "script.h"

#define ESC "\033["
#define BLACK 30
#define RED 31
#define GREEN 32
#define YELLOW 33
#define BLUE 34
#define MAGENTA 35
#define CYAN 36
#define WHITE 37

#define COLOR(TXT, _C) ESC #_C "m" #TXT ESC "0m"

char *logstr[] = { "spam", "debug", "info", "warn", "error", "panic"};
char *logcolor[] = { COLOR(spam,37), COLOR(debug,32), COLOR(info,36), COLOR(warn,33), COLOR(error,31), COLOR(panic,45) };
char *catstr[] = {"engine", "script", "render"};

static FILE *logout; /* where logs are written to */
static FILE *writeout; /* where console is written to */
static FILE *dump; /* where data is dumped to */

int stdout_lvl = LOG_ERROR;

void log_init()
{
#ifndef NDEBUG
  if (!fexists(".prosperon")) {
    logout = tmpfile();
    dump = tmpfile();
    writeout = stdout;
  }
  else {
    logout = fopen(".prosperon/log.txt", "w");
    writeout = fopen(".prosperon/transcript.txt", "w");
    dump = fopen(".prosperon/quickjs.txt", "w");
    quickjs_set_dumpout(dump);
  }
  
  quickjs_set_dumpout(dump);  
#endif
}

void log_shutdown()
{
  fclose(logout);
  fclose(writeout);
  fclose(dump);
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
  
  if (priority == LOG_DEBUG || priority >= stdout_lvl) {
    printf(logfmt, file, line, timebuf, logcolor[priority], catstr[category]);
    va_list args;
    va_start(args,message);
    vprintf(message, args);
    va_end(args);
    printf("\n");
  }

  //if (priority >= LOG_ERROR)
  //js_stacktrace();
  //raise(SIGINT);
#endif
}

/* print to stdout and console */
void log_print(const char *str)
{
#ifndef NDEBUG
  fprintf(writeout, str);
#endif
  fflush(stdout);
}

void sg_logging(const char *tag, uint32_t lvl, uint32_t id, const char *msg, uint32_t line, const char *file, void *data) {
  lvl = LOG_PANIC-lvl;
  mYughLog(LOG_RENDER, lvl, line, file, "tag: %s, id: %d, msg: %s", tag, id, msg);
}
