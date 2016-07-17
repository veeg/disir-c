// Standard includes
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

// Public disir interface
#include <disir/disir.h>

#include "context_private.h"

// Private disir includes
#include "value.h"
#include "log.h"

//! PUBLIC API
enum disir_status
dc_free_text_create (const char *text, struct disir_context **free_text)
{
    enum disir_status status;
    struct disir_context *context;

    if (text == NULL)
    {
        log_debug (0, "invoked with text NULL pointer");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }
    if (free_text == NULL)
    {
        log_debug (0, "invoked with free_text NULL pointer");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    context = dx_context_create (DISIR_CONTEXT_FREE_TEXT);
    if (context == NULL)
    {
        log_error ("failed to allocate context for free_text");
        return DISIR_STATUS_NO_MEMORY;
    }

    context->cx_root_context = context;

    context->cx_value = calloc (1, sizeof (struct disir_value));
    if (context->cx_value == NULL)
    {
        log_error ("failed to allocate value structure for context.");
        dx_context_destroy (&context);
        return DISIR_STATUS_NO_MEMORY;
    }

    context->cx_value->dv_type = DISIR_VALUE_TYPE_STRING;
    status = dx_value_set_string (context->cx_value, text, strlen (text));
    if (status != DISIR_STATUS_OK)
    {
        free (context->cx_value);
        dx_context_destroy (&context);
        return status;
    }

    *free_text = context;
    return DISIR_STATUS_OK;
}
