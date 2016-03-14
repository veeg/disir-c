#ifndef _LIBDISIR_LOG_H
#define _LIBDISIR_LOG_H

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
    //! Everything will be logged, i mean, really.
    DISIR_LOG_LEVEL_DEBUG,
};


//! Generic function signature for all logging methods
void dx_log_disir( enum disir_log_level dll,
                dc_t *context,
                int32_t log_context,
                const char *file,
                const char *function,
                int line,
                const char *message_prefix,
                const char *fmt_message, ...);

#define _log_disir_full(level, context, log_context, prefix, message, ...) \
    dx_log_disir (level, \
                 context, \
                 log_context, \
                 __FILE__, \
                 __FUNCTION__, \
                 __LINE__, \
                 prefix, \
                 message, \
                 ##__VA_ARGS__)

// Hide away some details for log_disir
#define _log_disir_level(level, message, ...) \
    _log_disir_full (level, NULL, 0, NULL, message, ##__VA_ARGS__)
#define _log_disir_level_context(level, context, message, ...) \
    _log_disir_full (level, context, 0, NULL, message, ##__VA_ARGS__)


#define log_fatal_context(context, message, ...) \
    _log_disir_level_context (DISIR_LOG_LEVEL_FATAL, context, message, ##__VA_ARGS__)
#define log_error_context(context, message, ...) \
    _log_disir_level_context (DISIR_LOG_LEVEL_WARNING, context, message, ##__VA_ARGS__)
#define log_warn_context(context, message, ...) \
    _log_disir_level_context (DISIR_LOG_LEVEL_ERROR, context, message, ##__VA_ARGS__)
#define log_info_context(context, message, ...) \
    _log_disir_level_context (DISIR_LOG_LEVEL_INFO, context, message, ##__VA_ARGS__)
#define log_debug_context(context, message, ...) \
    _log_disir_level_context(DISIR_LOG_LEVEL_DEBUG, context, message, ##__VA_ARGS__)

//! Log at different log levels
#define log_fatal(message, ...) _log_disir_level(DISIR_LOG_LEVEL_FATAL, message, ##__VA_ARGS__)
#define log_error(message, ...) _log_disir_level(DISIR_LOG_LEVEL_ERROR, message, ##__VA_ARGS__)
#define log_warn(message, ...) _log_disir_level(DISIR_LOG_LEVEL_WARNING, message, ##__VA_ARGS__)
#define log_test(message, ...) _log_disir_level(DISIR_LOG_LEVEL_TEST, message, ##__VA_ARGS__)
#define log_info(message, ...) _log_disir_level(DISIR_LOG_LEVEL_INFO, message, ##__VA_ARGS__)
#define log_debug(message, ...) _log_disir_level(DISIR_LOG_LEVEL_DEBUG, message, ##__VA_ARGS__)


// Log specially to context
// Will issue a DISIR_LOG_LEVEL_ERROR log entry to stream.
#define dx_log_context(context, message, ...) \
    _log_disir_full(DISIR_LOG_LEVEL_ERROR, context, 1, NULL, message, ##__VA_ARGS__)


//! Crash and burn.. Output message on stderr before it aborts.
//! USE WITH EXTREME CARE
void dx_crash_and_burn(const char* message, ...);

#endif // _LIBDISIR_LOG_H
