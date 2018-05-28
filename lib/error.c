// external public includes
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// public disir interface
#include <disir/disir.h>
#include <disir/util.h>
#include <disir/context.h>

// private
#include "disir_private.h"
#include "log.h"


//! PUBLIC API
void
disir_log_user (struct disir_instance *instance, const char *message, ...)
{
    va_list args;

    // TODO: disir argument not error checked. its not USED anyway.. (yet)
    (void) &instance;
    if (message == NULL)
        return;

    va_start (args, message);
    dx_log_disir (DISIR_LOG_LEVEL_USER, 0, NULL, NULL, 0, NULL, NULL, 0, NULL, message, args);
    va_end (args);
}

//! PUBLIC API
void
disir_error_set (struct disir_instance *instance, const char *message, ...)
{
    va_list args;

    if (instance == NULL || message == NULL)
        return;

    va_start (args, message);
    dx_log_disir (DISIR_LOG_LEVEL_DEBUG, 0, NULL, instance, 0, __FILE__, __func__, __LINE__, NULL, message, args);
    va_end (args);
}

//! PUBLIC API
void
disir_error_clear (struct disir_instance *instance)
{
    if (instance->disir_error_message_size != 0)
    {
        instance->disir_error_message_size = 0;
        free (instance->disir_error_message);
        instance->disir_error_message = NULL;
    }
}

//! PUBLIC API
enum disir_status
disir_error_copy (struct disir_instance *instance,
                  char *buffer, int32_t buffer_size, int32_t *bytes_written)
{
    enum disir_status status;
    int32_t size;

    if (instance == NULL || buffer == NULL)
    {
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    status = DISIR_STATUS_OK;

    if (buffer_size < 4)
    {
        return DISIR_STATUS_INSUFFICIENT_RESOURCES;
    }

    size = instance->disir_error_message_size;
    if (bytes_written)
    {
        // Write the total size of the error message
        // If insufficient, return status will indicate and passed
        // buffer will be filled with partial error message
        *bytes_written = size;
    }
    if (buffer_size <= size)
    {
        size -= 4;
        status = DISIR_STATUS_INSUFFICIENT_RESOURCES;
    }

    memcpy (buffer, instance->disir_error_message, size);
    if (status == DISIR_STATUS_INSUFFICIENT_RESOURCES)
    {
        sprintf (buffer + size, "...");
        size += 3;
    }
    buffer[size] = '\0';

    return status;
}

//! PUBLIC API
const char *
disir_error (struct disir_instance *instance)
{
    return instance->disir_error_message;
}

