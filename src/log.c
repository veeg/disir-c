#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>

#include <disir/disir.h>

#include "log.h"
#include "context_private.h"
#include "disir_private.h"


//! Variable to control the loglevel
//! Hardcode default for now.
enum disir_log_level runtime_loglevel = DISIR_LOG_LEVEL_DEBUG_09;

//! Variable to control whether or not to forcefully output TRACE log messages
//! regardless of loglevel setting
//! Hardcode default for now.
int force_enable_trace = 0;

//! STATIC USAGE
static const char *
map_dll_to_string (enum disir_log_level dll)
{
    switch (dll)
    {
    case DISIR_LOG_LEVEL_NONE:
        return "NONE ";
    case DISIR_LOG_LEVEL_FATAL:
        return "FATAL";
    case DISIR_LOG_LEVEL_ERROR:
        return "ERROR";
    case DISIR_LOG_LEVEL_WARNING:
        return "WARN ";
    case DISIR_LOG_LEVEL_TEST:
        return "TEST ";
    case DISIR_LOG_LEVEL_INFO:
        return "INFO ";
    case DISIR_LOG_LEVEL_USER:
        return "USER ";
    case DISIR_LOG_LEVEL_DEBUG:
        return "DEBUG";
    case DISIR_LOG_LEVEL_TRACE_ENTER:
    case DISIR_LOG_LEVEL_TRACE_EXIT:
        return "TRACE";
    default:
        return "UNHANDLED";
    }
}

//! INTERNAL API
void
dx_crash_and_burn (const char* message, ...)
{
    va_list args;
    va_start (args, message);
    vfprintf (stderr, message, args);
    fprintf (stderr, "\n");
    fflush (stderr);
    va_end (args);
    abort ();
}

//! implements the actual stream logging
//! prefix is injected between the fmt message and the timestamp/loglevel
//! Ignored if null
static void
dx_log_format (enum disir_log_level dll, int severity, const char *prefix,
               const char *suffix, const char* fmt_message, va_list args)
{
    char buffer[512];
    size_t time_written;
    int res;
    int buffer_size;
    FILE *stream;
    time_t now;
    struct tm *utctime;
    char dll_prefix[10];

    buffer_size = 512;

    // Normalize debug level
    if (dll == DISIR_LOG_LEVEL_DEBUG)
    {
        switch (severity)
        {
        case 0:
            dll = DISIR_LOG_LEVEL_DEBUG;
            break;
        case 1:
            dll = DISIR_LOG_LEVEL_DEBUG_01;
            break;
        case 2:
            dll = DISIR_LOG_LEVEL_DEBUG_02;
            break;
        case 3:
            dll = DISIR_LOG_LEVEL_DEBUG_03;
            break;
        case 4:
            dll = DISIR_LOG_LEVEL_DEBUG_04;
            break;
        case 5:
            dll = DISIR_LOG_LEVEL_DEBUG_05;
            break;
        case 6:
            dll = DISIR_LOG_LEVEL_DEBUG_06;
            break;
        case 7:
            dll = DISIR_LOG_LEVEL_DEBUG_07;
            break;
        case 8:
            dll = DISIR_LOG_LEVEL_DEBUG_08;
            break;
        case 9:
            dll = DISIR_LOG_LEVEL_DEBUG_09;
            break;
        case 10:
            dll = DISIR_LOG_LEVEL_DEBUG_10;
            break;
        }
    }

    // Dont log anything if loglevel doesnt match
    // Let TRACE messages through if force_enable_trace is set
    if (dll != DISIR_LOG_LEVEL_TRACE_ENTER && dll != DISIR_LOG_LEVEL_TRACE_EXIT)
    {
        if (dll > runtime_loglevel)
            return;
    }
    else if (dll > runtime_loglevel && !force_enable_trace)
    {
        return;
    }

    // Get UTC time
    time (&now);
    utctime = gmtime (&now);

    time_written = strftime (buffer, buffer_size, "[%Y-%m-%d %H:%M:%S]", utctime);
    if (time_written >= buffer_size || time_written <= 0)
    {
        dx_crash_and_burn ("strftime returned: %d - not within buffer size: %d",
            time_written, buffer_size);
    }

    // Prepare the log level prefix string
    if (dll >= DISIR_LOG_LEVEL_DEBUG &&
        dll != DISIR_LOG_LEVEL_TRACE_ENTER &&
        dll != DISIR_LOG_LEVEL_TRACE_EXIT)
    {
        if (severity < 0)
        {
            severity = 0;
        }
        if (severity > 10)
        {
            severity = 10;
        }
        snprintf (dll_prefix, 6, "DB %02d", severity);
    }
    else
    {
        snprintf (dll_prefix, 6, "%s", map_dll_to_string (dll));
    }

    res = snprintf (buffer + time_written, buffer_size - time_written,
            "-[%s] %s%s",
            dll_prefix,
            (prefix != NULL ? prefix : ""),
            (prefix != NULL ? " " : "")
            );
    if (res >= buffer_size - time_written || res < 0)
    {
        // Buffer not large enough, or encoding error..
        dx_crash_and_burn ("snprintf() returned res: %d - size: %d",
            res, buffer_size - time_written);
    }

    // Prepare outgoing stream
    stream = fopen ("/var/log/disir.log", "a");

    // Cant open file output stream! Bleh!
    if (stream == NULL)
    {
      // Throw a tantrum?
      return;
    }

    // Output buffer with time, loglevel and context
    fwrite (buffer, time_written + res, sizeof (char), stream);

    // Write incomming log message
    vfprintf (stream, fmt_message, args);

    // Write suffix
    if (suffix != NULL)
        fwrite (suffix, strlen (suffix), sizeof (char), stream); // XXX

    // Append newline to log message
    fprintf (stream, "\n");

    // close output stream. We dont want to keep it open forever, atleast not yet.
    fclose (stream);
}

//! INTERNAL API
//! Log the message to the a storage pointer
static void
dx_internal_log_to_storage (char **error_message, int32_t *error_message_size,
                            const char* fmt_message, va_list args)
{
    int message_size;
    int res;
    char dummy[4];
    va_list args_copy;

    // Make two instances of variadic arguments list
    // one to count buffer size, and one to populate
    va_copy (args_copy, args);

    // Calculate buffer size
    // vsnprintf will write size - 1 bytes to buffer
    // -1 is to leave room for null terminator
    // This will only count the size, not write any bytes
    message_size = vsnprintf (dummy, 1, fmt_message, args_copy);
    va_end (args_copy);

    if (message_size <= 0)
    {
        // Nothing to log!
        return;
    }

    // Allocate sufficiently large buffer, write message
    if (*error_message_size < message_size + 1)
    {
        *error_message = realloc (*error_message, message_size + 1);
        if (*error_message == NULL)
        {
            // What should we do!? Throw a tantrum!?
            return;
        }
        *error_message_size = message_size + 1;
    }

    res = vsnprintf (*error_message, message_size + 1, fmt_message, args);

    // Put a null terminator to it.
    (*error_message)[message_size] = '\0';

    if (res < 0 || res > message_size)
    {
        // Another error case to report.. If only i had memory..
    }
}

//! INTERNAL API
void
dx_context_error_set (struct disir_context *context, const char *fmt_message, ...)
{
    va_list args;
    va_start (args, fmt_message);
    dx_internal_log_to_storage (&context->cx_error_message,
                                &context->cx_error_message_size, fmt_message, args);

    va_end (args);
}

void
dx_log_disir_va (enum disir_log_level dll,
            int severity,
            struct disir_context *context,
            struct disir_instance *disir,
            int32_t log_context,
            const char *file,
            const char *function,
            int line,
            const char *message_prefix,
            const char *fmt_message,
            ...)
{
    va_list args;
    va_start (args, fmt_message);
    dx_log_disir (dll, severity, context, disir, log_context,
                  file, function, line, message_prefix, fmt_message, args);
    va_end (args);
}

void
dx_log_disir (enum disir_log_level dll,
            int severity,
            struct disir_context *context,
            struct disir_instance *disir,
            int32_t log_context,
            const char *file,
            const char *function,
            int line,
            const char *message_prefix,
            const char *fmt_message,
            va_list args)
{
    char *prefix;
    char *suffix;
    char buffer[60];
    char suffix_buffer[255];
    int prefix_size = 60;
    int res;
    va_list args_copy;

    char enter_string[] = "ENTER";
    char exit_string[] = "EXIT";

    prefix = NULL;
    suffix = NULL;

    if (log_context)
    {
        va_copy (args_copy, args);

        dx_internal_log_to_storage (&context->cx_error_message,
                                    &context->cx_error_message_size, fmt_message, args_copy);
        va_end (args_copy);
    }

    if (disir != NULL)
    {
        va_copy (args_copy, args);

        dx_internal_log_to_storage (&disir->disir_error_message,
                                    &disir->disir_error_message_size, fmt_message, args_copy);
        va_end (args_copy);
    }

    if (context)
    {
        //! Prepare the log prefix message.
        res = snprintf (buffer, prefix_size, "Context[%s]", dc_context_type_string (context));
        if (res > 0 || res < prefix_size)
        {
            buffer[res] = '\0';
            prefix = buffer;
        }
    }

    if (dll == DISIR_LOG_LEVEL_DEBUG)
    {
        res = snprintf (suffix_buffer, 255, " (%s%s%s:%d)",
                // file, "/",
                "", "",
                function,
                line);
        if (res > 0 || res < 255)
        {
            suffix_buffer[res] = '\0';
            suffix = suffix_buffer;
        }
    }

    if (dll == DISIR_LOG_LEVEL_TRACE_ENTER ||
        dll == DISIR_LOG_LEVEL_TRACE_EXIT)
    {
        res = snprintf (suffix_buffer, 255, "%s %s: ",
                        (dll == DISIR_LOG_LEVEL_TRACE_ENTER ? enter_string : exit_string),
                        function);
        if (res > 0 || res < 255)
        {
            suffix_buffer[res] = '\0';
            prefix = suffix_buffer;
        }
    }

    dx_log_format (dll,
            severity,
            (prefix != NULL ? prefix : message_prefix),
            (suffix != NULL ? suffix : ""),
            fmt_message,
            args);
}

