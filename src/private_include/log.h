#ifndef _LIBDISIR_LOG_H
#define _LIBDISIR_LOG_H

#include <stdarg.h>

#include <disir/context.h>

//! All defined LOG LEVELS for disir logging
enum disir_log_level {
    //! Nothing will be logged! O-oh..
    DISIR_LOG_LEVEL_NONE    = 1,
    //! Only fatal incidents will be logged.
    DISIR_LOG_LEVEL_FATAL,
    //! Fatal and Erroneous conditions will be logged.
    DISIR_LOG_LEVEL_ERROR,
    //! Warning, erroneous and Fatal conditions are logged.
    DISIR_LOG_LEVEL_WARNING,
    //! Test level - Used within the testsuite.
    DISIR_LOG_LEVEL_TEST,
    //! Informational entries prints will be logged.
    DISIR_LOG_LEVEL_INFO,
    //! User level - users of library may log to this log level through disir_log_user ()
    DISIR_LOG_LEVEL_USER,
    //! Everything will be logged, i mean, really.
    DISIR_LOG_LEVEL_DEBUG = 9,
    DISIR_LOG_LEVEL_DEBUG_01 = 10,
    DISIR_LOG_LEVEL_DEBUG_02 = 20,
    DISIR_LOG_LEVEL_DEBUG_03 = 30,
    DISIR_LOG_LEVEL_DEBUG_04 = 40,
    DISIR_LOG_LEVEL_DEBUG_05 = 50,
    DISIR_LOG_LEVEL_DEBUG_06 = 60,
    DISIR_LOG_LEVEL_DEBUG_07 = 70,
    DISIR_LOG_LEVEL_DEBUG_08 = 80,
    DISIR_LOG_LEVEL_DEBUG_09 = 90,
    DISIR_LOG_LEVEL_DEBUG_10 = 100,
    //! Trace input/output of library API calls
    DISIR_LOG_LEVEL_TRACE_ENTER = 51,
    DISIR_LOG_LEVEL_TRACE_EXIT = 52,
};


//! Generic function signature for all logging methods
void dx_log_disir (enum disir_log_level dll,
                int severity,
                struct disir_context *context,
                struct disir_instance *disir,
                int32_t log_context,
                const char *file,
                const char *function,
                int line,
                const char *message_prefix,
                const char *fmt_message, va_list args);

void dx_log_disir_va (enum disir_log_level dll,
                int severity,
                struct disir_context *context,
                struct disir_instance *disir,
                int32_t log_context,
                const char *file,
                const char *function,
                int line,
                const char *message_prefix,
                const char *fmt_message, ...);


#define _log_disir_full(level, severity, context, disir, log_context, prefix, message, ...) \
    dx_log_disir_va (level, \
                 severity, \
                 context, \
                 disir, \
                 log_context, \
                 __FILE__, \
                 __FUNCTION__, \
                 __LINE__, \
                 prefix, \
                 message, \
                 ##__VA_ARGS__)

// Hide away some details for log_disir
#define _log_disir_level(level, message, ...) \
    _log_disir_full (level, 0, NULL, NULL, 0, NULL, message, ##__VA_ARGS__)
#define _log_disir_level_context(level, severity, context, message, ...) \
    _log_disir_full (level, severity, context, NULL, 0, NULL, message, ##__VA_ARGS__)
#define _log_disir_level_debug(severity, message, ...) \
    _log_disir_full (DISIR_LOG_LEVEL_DEBUG, severity, NULL, NULL, 0, NULL, message, ##__VA_ARGS__)


#define log_fatal_context(context, message, ...) \
    _log_disir_level_context (DISIR_LOG_LEVEL_FATAL, 0, context, message, ##__VA_ARGS__)
#define log_error_context(context, message, ...) \
    _log_disir_level_context (DISIR_LOG_LEVEL_WARNING, 0, context, message, ##__VA_ARGS__)
#define log_warn_context(context, message, ...) \
    _log_disir_level_context (DISIR_LOG_LEVEL_ERROR, 0, context, message, ##__VA_ARGS__)
#define log_info_context(context, message, ...) \
    _log_disir_level_context (DISIR_LOG_LEVEL_INFO, 0, context, message, ##__VA_ARGS__)
#define log_debug_context(severity, context, message, ...) \
    _log_disir_level_context(DISIR_LOG_LEVEL_DEBUG, severity, context, message, ##__VA_ARGS__)

//! Log at different log levels
#define log_fatal(message, ...) _log_disir_level(DISIR_LOG_LEVEL_FATAL, message, ##__VA_ARGS__)
#define log_error(message, ...) _log_disir_level(DISIR_LOG_LEVEL_ERROR, message, ##__VA_ARGS__)
#define log_warn(message, ...) _log_disir_level(DISIR_LOG_LEVEL_WARNING, message, ##__VA_ARGS__)
#define log_test(message, ...) _log_disir_level(DISIR_LOG_LEVEL_TEST, message, ##__VA_ARGS__)
#define log_info(message, ...) _log_disir_level(DISIR_LOG_LEVEL_INFO, message, ##__VA_ARGS__)
#define log_debug(severity, message, ...) _log_disir_level_debug(severity, message, ##__VA_ARGS__)
#define TRACE_ENTER(message, ...) _log_disir_level(DISIR_LOG_LEVEL_TRACE_ENTER, message, ##__VA_ARGS__)
#define TRACE_EXIT(message, ...) _log_disir_level(DISIR_LOG_LEVEL_TRACE_EXIT, message, ##__VA_ARGS__)


// Log specially to context
// Will issue a DISIR_LOG_LEVEL_ERROR log entry to stream.
#define dx_log_context(context, message, ...) \
    _log_disir_full(DISIR_LOG_LEVEL_ERROR, 0, context, NULL, 1, NULL, message, ##__VA_ARGS__)


//! Crash and burn.. Output message on stderr before it aborts.
//! USE WITH EXTREME CARE
void dx_crash_and_burn(const char* message, ...);


#endif // _LIBDISIR_LOG_H

