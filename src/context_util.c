// External public includes
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

// Public disir interface
#include <disir/disir.h>
#include <disir/context.h>

// Private
#include "context_private.h"
#include "log.h"
#include "keyval.h"

//! Array  of string representations corresponding to the
//! disir_context_type enumeration value.
const char *disir_context_type_string[] = {
    "INVALID_CONTEXT_VALUE",
    "FILE_CONFIG",
    "FILE_MOLD",
    "SECTION",
    "KEYVAL",
    "DOCUMENTATION",
    "DEFAULT",
    "RESTRICTION",
    "FREE TEXT",
    "UNKNOWN",
};

//! Array of string representations of enum disir_value_type
const char *disir_value_type_string[] = {
    "INVALID_VALUE_TYPE",
    "STRING",
    "INTEGER",
    "FLOAT",
    "BOOLEAN",
    "ENUM",
    "UNKNOWN",
};

//! Array of string representations of the context capabilities
const char *disir_capability_string[] = {
    "add documentation",
    "add multiple documentation",
    "add string value"
};

//! PUBLIC API
enum disir_context_type
dc_context_type (dc_t *context)
{
    if (context == NULL)
        return DISIR_CONTEXT_UNKNOWN;

    return dx_context_type_sanify (context->cx_type);
}

//! PUBLIC API
const char *
dc_context_type_string (dc_t *context)
{
    return disir_context_type_string[dc_context_type (context)];
}

//! INTERNAL API
const char *
dx_context_capability_string (uint64_t capability)
{
    switch(capability)
    {
    case CC_ADD_DOCUMENTATION:
        return disir_capability_string[0];
    case CC_ADD_MULTIPLE_DOCUMENTATION:
        return disir_capability_string[1];
    case CC_ADD_VALUE_STRING:
        return disir_capability_string[2];
    default:
        return "<unknown capability>";
    }
}

//! INTERNAL API
const char *
dx_context_type_string (enum disir_context_type type)
{
    return disir_context_type_string[dx_context_type_sanify (type)];
}

//! INTERNAL API
enum disir_context_type
dx_context_type_sanify (enum disir_context_type type)
{
    if (type >= DISIR_CONTEXT_UNKNOWN || type <= 0)
    {
        return DISIR_CONTEXT_UNKNOWN;
    }

    return type;
}

enum disir_value_type
dc_value_type (dc_t *context)
{
    enum disir_value_type value_type;
    enum disir_context_type context_type;

    if (context == NULL)
    {
        return DISIR_VALUE_TYPE_UNKNOWN;
    }

    value_type = DISIR_VALUE_TYPE_UNKNOWN;
    context_type = dc_context_type (context);

    if (context_type == DISIR_CONTEXT_DEFAULT)
        value_type = context->cx_default->de_value.dv_type;
    else if (context_type == DISIR_CONTEXT_KEYVAL)
        value_type = context->cx_keyval->kv_type;

    return dx_value_type_sanify (value_type);
}

const char *
dc_value_type_string (dc_t *context)
{
    return dx_value_type_string (dc_value_type (context));
}

//! INTERNAL API
enum disir_value_type
dx_value_type_sanify (enum disir_value_type type)
{
    if (type >= DISIR_VALUE_TYPE_UNKNOWN || type <= 0)
    {
        return DISIR_VALUE_TYPE_UNKNOWN;
    }

    return type;
}

//! INTERNAL API
const char *
dx_value_type_string (enum disir_value_type type)
{
    return disir_value_type_string[dx_value_type_sanify (type)];
}

//! INTERNAL API
uint32_t
dx_context_type_is_toplevel (enum disir_context_type context_type)
{
    if (
        context_type == DISIR_CONTEXT_CONFIG    ||
        context_type == DISIR_CONTEXT_MOLD      ||
        context_type == DISIR_CONTEXT_FREE_TEXT
       )
    {
        return 1;
    }

    return 0;
}

//! INTERNAL API
void
dx_context_attach (dc_t *parent, dc_t *context)
{
    if (parent == NULL || context == NULL)
        return;

    context->cx_parent_context = parent;
    // Child holds a reference to its parent. If parent is destroyed,
    // without the child having finalized, the child wont be destroyed
    // and may attempt to access parent context after it is destroyed.
    dx_context_incref (parent);
}

//! INTERNAL API
void
dx_context_incref (dc_t *context)
{
    context->cx_refcount++;
    log_debug_context (context, "(%p) increased refcount to: %d", context, context->cx_refcount);
}

//! INTERNAL API
void
dx_context_decref (dc_t **context)
{
    if (context == NULL)
        return;

    (*context)->cx_refcount--;

    if ((*context)->cx_refcount == 0)
    {
        dx_context_destroy (context);
    }
    else
    {
        log_debug_context (*context, "(%p) reduced refcount to: %d",
                           *context, (*context)->cx_refcount);
    }
}

//! INTERNAL API
dc_t *
dx_context_create (enum disir_context_type type)
{
    dc_t *context;

    context = NULL;

    if (dx_context_type_sanify (type) == DISIR_CONTEXT_UNKNOWN)
    {
        log_debug ("invoked with unknown context type( %d )", type);
        return NULL;
    }

    log_debug ("Allocating disir_context for %s", dx_context_type_string (type));

    context = calloc (1, sizeof (struct disir_context));
    if (context == NULL)
        return NULL;

    context->cx_type = type;

    // Set default context state to CONSTRUCTING
    context->cx_state = CONTEXT_STATE_CONSTRUCTING;

    // Set refcount to 1 - object is owned by creator.
    context->cx_refcount = 1;

    // Set capabilities on context
    switch (type)
    {
    case DISIR_CONTEXT_CONFIG:
    {
        // By default, the top-level config capabilities
        // are limited until a mold is attached
        context->CONTEXT_CAPABILITY_ADD_DOCUMENTATION = 1;
        break;
    }
    case DISIR_CONTEXT_DOCUMENTATION:
    {
        context->CONTEXT_CAPABILITY_ADD_VALUE_STRING = 1;
        context->CONTEXT_CAPABILITY_INTRODUCED = 1;
        break;
    }
    case DISIR_CONTEXT_MOLD:
    {
        // XXX: What capabilitties?
        context->CONTEXT_CAPABILITY_ADD_DOCUMENTATION = 1;
        break;
    }
    case DISIR_CONTEXT_SECTION:
    case DISIR_CONTEXT_KEYVAL:
    {
        // XXX: What capabilitites?
        context->CONTEXT_CAPABILITY_ADD_DOCUMENTATION = 1;
        context->CONTEXT_CAPABILITY_ADD_VALUE_STRING = 1;
        context->CONTEXT_CAPABILITY_INTRODUCED = 1;
        break;
    }
    case DISIR_CONTEXT_DEFAULT:
    {
        // XXX: What capabilities?
        context->CONTEXT_CAPABILITY_INTRODUCED = 1;
        break;
    }
    case DISIR_CONTEXT_FREE_TEXT:
    {
        context->CONTEXT_CAPABILITY_ADD_VALUE_STRING = 1;
        break;
    }
    case DISIR_CONTEXT_RESTRICTION:
        dx_crash_and_burn ("%s UNHANDLED CONTEXT TYPE: %s",
                __FUNCTION__,
                dx_context_type_string (type));
        break;
    case DISIR_CONTEXT_UNKNOWN:
        // Already handled
        break;
    // No default handler - let compiler warn us of unhandled context
    }

    return context;
}

//! INTERNAL API
void
dx_context_destroy  (dc_t **context)
{
    if (context == NULL || *context == NULL)
        return;

    // Free the error message, if it exists
    if ((*context)->cx_error_message)
    {
        free ((*context)->cx_error_message);
    }

    log_debug_context (*context, " (%p) reached refcount zero. Freeing.", *context);

    free(*context);
    *context = NULL;
}

//! INTERNAL API
void
dx_context_transfer_logwarn (dc_t *destination, dc_t *source)
{
    // Simply inherhit the memory allocated by source to destination

    // Check arguments
    if (destination == NULL || source == NULL)
        return;

    // No error message to transfer
    if (source->cx_error_message == NULL)
        return;

    // No size on source error message - uhmm..
    if (source->cx_error_message_size <= 0)
        return;

    // Remove existing error message at destination
    if (destination->cx_error_message)
    {
        free (destination->cx_error_message);
        destination->cx_error_message_size = 0;
    }

    destination->cx_error_message = source->cx_error_message;
    destination->cx_error_message_size = source->cx_error_message_size;

    source->cx_error_message = NULL;
    source->cx_error_message_size = 0;
}

//! INTERNAL API
enum disir_status
dx_context_sp_full_check_log_error (dc_t *context, const char *function_name)
{
    // Check argument
    if (context == NULL)
    {
        log_debug ("%s() invoked with context NULL pointer", function_name);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    if (context->cx_state == CONTEXT_STATE_DESTROYED)
    {
        dx_log_context (context, "%s() invoked on destroyed context %s",
                function_name, dc_context_type_string (context));
        return DISIR_STATUS_DESTROYED_CONTEXT;
    }

    if (dx_context_type_sanify(context->cx_type) == DISIR_CONTEXT_UNKNOWN)
    {
        dx_log_context (context, "%s() invoked on unknown context( %d )", context->cx_type);
        return DISIR_STATUS_CONTEXT_IN_WRONG_STATE;
    }

    return DISIR_STATUS_OK;
}

//! INTERNAL API
enum disir_status
dx_context_dp_full_check_log_error (dc_t **context, const char *function_name)
{
    // Check argument
    if (context == NULL)
    {
        log_debug ("%s() invoked with context double NULL pointer", function_name);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    return dx_context_sp_full_check_log_error (*context, function_name);
}



//! INTERNAL API
enum disir_status
dx_context_type_check_log_error (dc_t *context, ...)
{
    enum disir_status status;
    enum disir_context_type variadic_type;
    enum disir_context_type context_type;
    va_list ap;
    char buffer[512];
    uint8_t correct_context;
    uint32_t variadic_list_length;
    int32_t buffer_space;
    int written;
    char *buf;

    // init
    correct_context = 0;
    variadic_list_length = 0;
    status = DISIR_STATUS_OK;
    variadic_type = DISIR_CONTEXT_UNKNOWN;
    context_type = dx_context_type_sanify (context->cx_type);

    // Check if the passed context matches any of the context types
    // passed in the variadic list.
    // If the passed context is unknown, skip straight to fail.
    if (context_type != DISIR_CONTEXT_UNKNOWN)
    {
        va_start(ap, context);
        for (variadic_type = va_arg (ap, enum disir_context_type);
             variadic_type;
             variadic_type = va_arg (ap, enum disir_context_type))
        {
            variadic_list_length += 1;

            if (context_type == dx_context_type_sanify (variadic_type))
            {
                correct_context = 1;
                break;
            }
        }
        va_end (ap);
    }

    // Failure - log error and set return status
    if (correct_context == 0)
    {
        buffer_space = 512;
        buf = buffer;

        if (context_type == DISIR_CONTEXT_UNKNOWN)
        {
            written = snprintf (buf, buffer_space,
                        "Context[%s] of unknown type cannot be processed.",
                        dx_context_type_string (context_type));
            status = DISIR_STATUS_INVALID_CONTEXT;
        }
        else if (variadic_list_length == 0)
        {
            written = snprintf (buf, buffer_space,
                        "Context[%s] typechecked against empty list. Oops.",
                        dx_context_type_string (context_type));
            status = DISIR_STATUS_TOO_FEW_ARGUMENTS;
        }
        else
        {
            written = snprintf (buf, buffer_space,
                        "Context[%s] is not of required type: ",
                        dx_context_type_string (context_type));
            status = DISIR_STATUS_WRONG_CONTEXT;
        }

        // Error check buffer.
        if (written < 0 || written >= buffer_space)
        {
            if (written < 0)
            {
                // Encoding error
                status = DISIR_STATUS_INTERNAL_ERROR;
            }
            else
            {
                // Insufficient buffer
                dx_crash_and_burn ("TODO: Re-allocate log buffer - too small");
                status = DISIR_STATUS_INSUFFICIENT_RESOURCES;
            }
        }
        else
        {
            // Advance buffers
            buf += written;
            buffer_space -= written;
        }

        // Write out comma separated list of strings for the passed variadic contexts
        if (context_type != DISIR_CONTEXT_UNKNOWN && variadic_list_length != 0)
        {
            va_start (ap, context);
            for (variadic_type = va_arg (ap, enum disir_context_type);
                 variadic_type;
                 variadic_type = va_arg (ap, enum disir_context_type))
            {
                if (dx_context_type_sanify (variadic_type) != DISIR_CONTEXT_UNKNOWN)
                {
                    written = snprintf(buf, buffer_space, " %s,",
                                dx_context_type_string (variadic_type));
                }
                else
                {
                    // The variadic_type is DISIR_CONTEXT_UNKNOWN, append numeric
                    // of original for logging purposes.
                    written = snprintf(buf, buffer_space, " %s(%d),",
                                dx_context_type_string (variadic_type), variadic_type);
                }

                if (written < 0 || written >= buffer_space)
                {
                    // Abort appending, just printout what we have..
                    break;
                }
                else
                {
                    buf += written;
                    buffer_space -= written;
                }
            }
            va_end (ap);

            // Remove the last comma
            *(buf - 1) = '\0';
        }

        // Do we have anything to write?
        if (buffer != buf)
        {
            log_error (buffer);
        }
    }

    return status;
}

//! PUBLIC API
enum disir_status
dc_set_value_type (dc_t *context, enum disir_value_type type)
{
    enum disir_status status;

    status = CONTEXT_NULL_INVALID_TYPE_CHECK (context);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }
    if (dx_value_type_sanify (type) == DISIR_VALUE_TYPE_UNKNOWN)
    {
        log_debug_context (context, "invoked with invalid/unknown value type (%d)", type);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    status = DISIR_STATUS_OK;
    switch (dc_context_type (context))
    {
    case DISIR_CONTEXT_KEYVAL:
    {
        //TODO XXX: Cannot set type if keyval has defaults
        //TODO XXX: Cannot set type if toplevel context is not DISIR_MOLD
        context->cx_keyval->kv_type = type;
        break;
    }
    default:
    {
        dx_crash_and_burn ("%s: %s not handled", __FUNCTION__, dc_context_type_string (context));
        return DISIR_STATUS_INTERNAL_ERROR; // not reached
    }
    }

    if (status == DISIR_STATUS_OK)
    {
        log_debug_context (context, "sat value type to %s", dx_value_type_string (type));
    }

    return status;
}


//! PUBLIC API
enum disir_status
dc_get_value_type (dc_t *context, enum disir_value_type *type)
{
    enum disir_status status;

    if (type == NULL)
    {
        log_debug ("invoked with type NULL pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }
    *type = DISIR_VALUE_TYPE_UNKNOWN;

    status = CONTEXT_NULL_INVALID_TYPE_CHECK (context);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }

    status = DISIR_STATUS_OK;
    switch (dc_context_type (context))
    {
    case DISIR_CONTEXT_DEFAULT:
        *type = context->cx_default->de_value.dv_type;
        break;
    case DISIR_CONTEXT_KEYVAL:
        *type = context->cx_keyval->kv_type;
        break;
    case DISIR_CONTEXT_FREE_TEXT:
        *type = context->cx_value->dv_type;
        break;
    default:
        log_debug_context (context, "invoked but does not contain/handle type");
        status = DISIR_STATUS_WRONG_CONTEXT;
    }

    return status;
}

