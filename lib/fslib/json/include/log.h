#ifndef DIO_JSON_LOG_H
#define DIO_JSON_LOG_H

#include <string.h>
#include <stdarg.h>

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

void log_disir_plugin (const char *message, ...);

void _log_debug (const char *function,
                 const char *file,
                 int line,
                 const char *message, ...);

#define log_debug(message, ...) \
    _log_debug (__FUNCTION__, \
            __FILENAME__, \
            __LINE__, \
            message, \
            ##__VA_ARGS__)

#endif
