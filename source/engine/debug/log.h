#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdint.h>

#define LOG_SPAM 0
#define LOG_DEBUG 1
#define LOG_INFO 2
#define LOG_WARN 3
#define LOG_ERROR 4

#define LOG_ENGINE 0
#define LOG_SCRIPT 1
#define LOG_RENDER 2

extern char *consolelog;
extern int logLevel;

void log_init();

#ifndef NDEBUG
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
void sg_logging(const char *tag, uint32_t lvl, uint32_t id, const char *msg, uint32_t line, const char *file, void *data);

void log_setfile(char *file);
void log_cat(FILE *f);
void log_print(const char *str);
void log_clear();
void console_print(const char *str);

#endif
