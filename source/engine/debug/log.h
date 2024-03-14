#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdint.h>

#define LOG_SPAM 0
#define LOG_DEBUG 1
#define LOG_INFO 2
#define LOG_WARN 3
#define LOG_ERROR 4
#define LOG_PANIC 5

#define LOG_ENGINE 0
#define LOG_SCRIPT 1
#define LOG_RENDER 2

void log_init();
void log_shutdown();

#ifndef NDEBUG
#define YughLog(cat, pri, msg, ...) mYughLog(cat, pri, __LINE__, __FILE__, msg, ##__VA_ARGS__)
#define YughSpam(msg,  ...) mYughLog(0, LOG_SPAM, __LINE__, __FILE__, msg, ##__VA_ARGS__);
#define YughInfo(msg, ...) mYughLog(0, LOG_INFO, __LINE__, __FILE__, msg, ##__VA_ARGS__);
#define YughWarn(msg, ...) mYughLog(0, LOG_WARN, __LINE__, __FILE__, msg, ##__VA_ARGS__);
#define YughError(msg, ...) mYughLog(0, LOG_ERROR, __LINE__, __FILE__, msg, ##__VA_ARGS__);
#define YughCritical(msg, ...) mYughLog(0, LOG_PANIC, __LINE__, __FILE__, msg, ##__VA_ARGS__);
#else
#define YughLog(cat, pri, msg, ...)
#define YughInfo(msg, ...)
#define YughWarn(msg, ...)
#define YughError(msg, ...)
#define YughCritical(msg, ...)
#endif

/* These log to the correct areas */
void mYughLog(int category, int priority, int line, const char *file, const char *message, ...);
void sg_logging(const char *tag, uint32_t lvl, uint32_t id, const char *msg, uint32_t line, const char *file, void *data);

void log_print(const char *str);

#endif
