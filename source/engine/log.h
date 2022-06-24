#ifndef LOG_H
#define LOG_H

#define ERROR_BUFFER 2048

#define LOG_INFO 0
#define LOG_WARN 1
#define LOG_ERROR 2
#define LOG_CRITICAL 3

#define YughLog(cat, pri, msg, ...) mYughLog(cat, pri, __LINE__, __FILE__, msg, __VA_ARGS__)
#define YughInfo(msg, ...) mYughLog(0, LOG_INFO, __LINE__, __FILE__, msg, __VA_ARGS__)
#define YughWarn(msg, ...) mYughLog(0, LOG_WARN, __LINE__, __FILE__, msg, __VA_ARGS__)
#define YughError(msg, ...) mYughLog(0, LOG_ERROR, __LINE__, __FILE__, msg, __VA_ARGS__)
#define YughCritical(msg, ...) mYughLog(0, LOG_CRITICAL, __LINE__, __FILE__, msg, __VA_ARGS__)

void mYughLog(int category, int priority, int line, const char *file, const char *message, ...);

void FlushGLErrors();

int TestSDLError(int sdlErr);

#endif
