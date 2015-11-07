#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#include "log.h"
#include "context_private.h"


//! Variable to control the loglevel
//! Hardcode default for now.
enum disir_log_level runtime_loglevel = DISIR_LOG_LEVEL_DEBUG;

//! String representation of the different disir_log_level
static const char * disir_log_level_string[] = {
    "UNUSED",
    "NONE ",
    "FATAL",
    "ERROR",
    "WARN ",
    "INFO ",
    "DEBUG",
};

//! INTERNAL API
void
dx_crash_and_burn(const char* message, ...)
{
    va_list args;
    va_start(args, message);
    vfprintf(stderr, message, args);
    fprintf(stderr, "\n");
    fflush(stderr);
    va_end(args);
    abort();
}

//! implements the actual stream logging
//! prefix is injected between the fmt message and the timestamp/loglevel
//! Ignored if null
static void
dx_log_format(enum disir_log_level dll, const char *prefix, const char* fmt_message, va_list args)
{
    char buffer[100];
    size_t  time_written;
    int res;
    int buffer_size;
    FILE *stream;
    time_t now;
    struct tm *utctime;
    
    buffer_size = 100;
    
    // Dont log anything if loglevel doesnt match
    if (dll > runtime_loglevel)
    {
        return;
    }
    
    if (dll <= DISIR_LOG_LEVEL_NONE || dll > DISIR_LOG_LEVEL_DEBUG)
    {
        dx_crash_and_burn("Passed log level is of the charts!: %d", dll);
    }
    
    // Get UTC time
    time(&now);
    utctime = gmtime(&now);
    
    time_written = strftime(buffer, buffer_size, "[%Y-%m-%d %H:%M:%S]", utctime);
    if (time_written >= buffer_size || time_written <= 0)
    {
        dx_crash_and_burn("strftime returned: %d - not within buffer size: %d", time_written, buffer_size);
    }
    
    res = snprintf(buffer + time_written, buffer_size - time_written, 
            "-[%s] %s%s", 
            disir_log_level_string[dll], 
            (prefix != NULL ? prefix : ""),
            (prefix != NULL ? " " : ""));
    if (res >= buffer_size - time_written || res < 0)
    {
        // Buffer not large enough, or encoding error..
        dx_crash_and_burn("snprintf() returned res: %d - size: %d", res, buffer_size - time_written);
    }
    
    // Prepare outgoing stream
    stream = fopen("/var/log/disir.log", "a");
    
    // Output buffer with time, loglevel and context
    fwrite(buffer, 1, time_written + res, stream);
    
    // Write incomming log message
    vfprintf(stream, fmt_message, args);
    
    // Append newline to log message
    fprintf(stream, "\n");
    
    // close output stream. We dont want to keep it open forever, atleast not yet.
    fclose(stream);
}


//! INTERNAL API
//! Log the message to the context, such that the error message isalnum
//! programmatically retrievable.
//! Will also log to standard log source with DISIR_LOG_LEVEL_ERROR
static void
dx_internal_log_context(dc_t *context, const char* fmt_message, va_list args)
{
    int message_size;
    int res;
    char dummy[4];
    va_list args_copy;
    
    // Make two instances of variadic arguments list
    // one to count buffer size, and one to populate
    va_copy(args_copy, args);

    // Calculate buffer size
    // vsnprintf will write size - 1 bytes to buffer
    // -1 is to leave room for null terminator
    // This will only count the size, not write any bytes
    message_size = vsnprintf(dummy, 1, fmt_message, args_copy);
    va_end(args_copy);
    
    if (message_size <= 0)
    {
        // Nothing to log!
        return;
    }
    
    // Allocate sufficiently large buffer, write message
    if (context->cx_error_message_size < message_size + 1)
    {
        context->cx_error_message = malloc(message_size + 1);
        if (context->cx_error_message == NULL)
        {
            // What should we do!? Throw a tantrum!?
            return;
        }
        context->cx_error_message_size = message_size + 1;
    }
    
    res = vsnprintf(context->cx_error_message, message_size + 1, fmt_message, args);
    
    // Put a null terminator to it.
    context->cx_error_message[message_size] = '\0';
    
    if (res < 0 || res > message_size)
    {
        // Another error case to report.. If only i had memory..
    }
}

void
dx_log_disir(enum disir_log_level dll, 
            dc_t *context,
            int32_t log_context,
            const char *file, 
            const char *function, 
            int line, 
            const char *message_prefix,
            const char *fmt_message, 
            ...)
{
    char *prefix;
    char buffer[60];
    int prefix_size = 60;
    int res;
    va_list args;
    
    prefix = NULL;
    
    if (log_context)
    {
        va_start(args, fmt_message);
        dx_internal_log_context(context, fmt_message, args);
        va_end(args);
    }
    
    if (context)
    {
        //! Prepare the log prefix message.
        res = snprintf(buffer, prefix_size, "Context[%s]", dc_type_string(context));
        if (res > 0 || res < prefix_size)
        {
            buffer[res] = '\0';
            prefix = buffer;
        }
    }

    va_start(args, fmt_message);
    dx_log_format(dll, (prefix != NULL ? prefix : message_prefix), fmt_message, args);
    va_end(args);
}
