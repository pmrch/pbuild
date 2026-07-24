#include <stdio.h>
#include <stdarg.h>

#include "log.h"

void log_internal(const char* level_str, const char* color, const char* file, int line, const char* func, const char* fmt, ...) {
    #if LOG_LEVEL == LOG_LEVEL_DEPLOYMENT
    (void)file;
    (void)line;
    (void)func;

    fprintf(stderr, "%s[%s]\x1b[0m: ", color, level_str);

    #else
    fprintf(stderr, "%s[%s]\x1b[0m %s:%d:%s: ", color, level_str, file, line, func);
    #endif
    
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    
    fprintf(stderr, "\n");    
}
