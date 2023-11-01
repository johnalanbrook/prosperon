#include "log.h"

#include "render.h"
#include <time.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include "yugine.h"

#include "script.h"

int logLevel = 0;

/* Four levels of log:
   0 info
   1 warn
   2 error
   3 critical
*/

char *logstr[] = { "info", "warn", "error", "critical" };
char *catstr[] = {"engine", "script", "render"};

FILE *logfile = NULL;

#define ERROR_BUFFER 1024
#define CONSOLE_BUF 1024*1024 /* 5MB */

char *consolelog;

static FILE *sout;

void log_init()
{
  consolelog = malloc(CONSOLE_BUF+1);
  sout = fdopen(dup(1),"w");
  sout = stdout;
}

const char *logfmt = "%s:%d: %s, %s: %s\n";
void mYughLog(int category, int priority, int line, const char *file, const char *message, ...)
{
#ifndef NDEBUG
  if (priority >= logLevel) {
    time_t now = time(0);
    struct tm *tinfo = localtime(&now);

    double ticks = (double)clock()/CLOCKS_PER_SEC;

    va_list args, arg2;
    va_start(args, message);
    va_copy(arg2, args);
    int len = vsnprintf(NULL, 0, message, args)+1;
    char *msg = malloc(len);
    va_end(args);
    vsprintf(msg, message, arg2);
    va_end(arg2);
    
    len = snprintf(NULL, 0, logfmt, file,line,logstr[priority], catstr[category], msg)+1;
    char *buffer = malloc(len);
    sprintf(buffer, logfmt, file, line, logstr[priority], catstr[category], msg);

    fprintf(stderr, buffer);
    fflush(stderr);
    
    free(msg);
    free(buffer);
  }

#endif
}

void log_print(const char *str)
{
  fprintf(sout, "%s", str);
  fflush(sout);
  
#ifndef NDEBUG
  strncat(consolelog, str, CONSOLE_BUF);

  if (logfile) {
    fprintf(logfile, "%s", str);
    fflush(logfile);
  }
#endif
}

void log_clear()
{
  consolelog[0] = 0;
}

void console_print(const char *str)
{
#ifndef NDEBUG
  strncat(consolelog, str, CONSOLE_BUF);
#endif
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

void sg_logging(const char *tag, uint32_t lvl, uint32_t id, const char *msg, uint32_t line, const char *file, void *data) {
  lvl = 3-lvl;
  mYughLog(2, lvl, line, file, "tag: %s, id: %d, msg: %s", tag, id, msg);
}
