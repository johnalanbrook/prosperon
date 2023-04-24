#ifndef LOG_H
#define LOG_H

#include <stdio.h>

#define ERROR_BUFFER 1024*1024

#define LOG_INFO 0
#define LOG_WARN 1
#define LOG_ERROR 2
#define LOG_CRITICAL 3

#define M_PI 3.14

extern char lastlog[];
extern char consolelog[];
extern int logLevel;

#if DBG
#define YughLog(cat, pri, msg, ...) mYughLog(cat, pri, __LINE__, __FILE__, msg, ##__VA_ARGS__)
#define YughInfo(msg, ...) mYughLog(0, 0, __LINE__, __FILE__, msg, ##__VA_ARGS__);
#define YughWarn(msg, ...) mYughLog(0, 1, __LINE__, __FILE__, msg, ##__VA_ARGS__);
#define YughError(msg, ...) mYughLog(0, 2, __LINE__, __FILE__, msg, ##__VA_ARGS__);
#define YughCritical(msg, ...) mYughLog(0, 3, __LINE__, __FILE__, msg, ##__VA_ARGS__);
#else
#define YughLog(cat, pri, msg, ...)
#define YughInfo(msg, ...)
#define YughWarn(msg, ...)
#define YughError(msg, ...)
#define YughCritical(msg, ...)
#endif

void mYughLog(int category, int priority, int line, const char *file, const char *message, ...);

void FlushGLErrors();

void log_setfile(char *file);
void log_cat(FILE *f);
void log_print(const char *str);

#endif
