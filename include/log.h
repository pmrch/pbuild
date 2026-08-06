#ifndef LOG_H
#define LOG_H

// Log levels
#define LOG_LEVEL_VERBOSE 0
#define LOG_LEVEL_DEBUG 1
#define LOG_LEVEL_INFO 2
#define LOG_LEVEL_WARN 3
#define LOG_LEVEL_ERROR 4
#define LOG_LEVEL_DEPLOYMENT 5

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_VERBOSE
#endif

#if defined(__clang__) || defined (_GNUC_)
    #define PRINTF_LIKE(fmt_idx, arg_idx) __attribute__((format(printf, 6, 7)))
#else
    #define PRINTF_LIKE(fmt_idx, arg_idx)
#endif

// Forward declaration of the internal worker function
void log_internal(const char* level_str, const char* color, const char* file, int line, const char* func, const char* fmt, ...)
PRINTF_LIKE(6, 7); 

#if LOG_LEVEL <= LOG_LEVEL_VERBOSE
#define LOG_VERBOSE(fmt, ...) log_internal("VERBOSE", "\x1b[35m", __FILE__, __LINE__, __func__, fmt __VA_OPT__(,) __VA_ARGS__) // NOLINT
#else
#define LOG_VERBOSE(fmt, ...)
#endif

#if LOG_LEVEL <= LOG_LEVEL_DEBUG
#define LOG_DEBUG(fmt, ...) log_internal("DEBUG", "\x1b[36m", __FILE__, __LINE__, __func__, fmt __VA_OPT__(,) __VA_ARGS__)
#else
#define LOG_DEBUG(fmt, ...)
#endif

#if LOG_LEVEL <= LOG_LEVEL_INFO
#define LOG_INFO(fmt, ...)  log_internal("INFO",  "\x1b[32m", __FILE__, __LINE__, __func__, fmt __VA_OPT__(,) __VA_ARGS__)
#else
#define LOG_INFO(fmt, ...)
#endif

#if LOG_LEVEL <= LOG_LEVEL_WARN
#define LOG_WARN(fmt, ...)  log_internal("WARN",  "\x1b[33m", __FILE__, __LINE__, __func__, fmt __VA_OPT__(,) __VA_ARGS__)
#else
#define LOG_WARN(fmt, ...)
#endif

#if LOG_LEVEL <= LOG_LEVEL_ERROR
#define LOG_ERROR(fmt, ...) log_internal("ERROR", "\x1b[31m", __FILE__, __LINE__, __func__, fmt __VA_OPT__(,) __VA_ARGS__)
#else
#define LOG_ERROR(fmt, ...)
#endif


#if LOG_LEVEL == LOG_LEVEL_DEPLOYMENT
#undef LOG_DEBUG
#undef LOG_INFO
#undef LOG_WARN
#undef LOG_ERROR

#define LOG_ERROR(fmt, ...) log_internal("ERROR", "\x1b[31m", NULL, 0, NULL, fmt __VA_OPT__(,) __VA_ARGS__)
#define LOG_WARN(fmt, ...)  log_internal("WARN",  "\x1b[33m", NULL, 0, NULL, fmt __VA_OPT__(,) __VA_ARGS__)
#define LOG_INFO(fmt, ...)
#define LOG_DEBUG(fmt, ...)
#endif

#endif
