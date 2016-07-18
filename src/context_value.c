// External public includes
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

// Public disir interface
#include <disir/disir.h>
#include <disir/context.h>

// Private
#include "context_private.h"
#include "config.h"
#include "section.h"
#include "keyval.h"
#include "log.h"
#include "mold.h"

static enum disir_status
get_value_input_check (struct disir_context *context, const char *type,
                       struct disir_value **storage)
{
    enum disir_status status;

    status = CONTEXT_NULL_INVALID_TYPE_CHECK (context);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }

    status = CONTEXT_TYPE_CHECK (context, DISIR_CONTEXT_KEYVAL, DISIR_CONTEXT_DEFAULT);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged ?
        return status;
    }

    if (dc_context_type (context) == DISIR_CONTEXT_KEYVAL)
    {
        if (dc_context_type (context->cx_root_context) == DISIR_CONTEXT_MOLD)
        {
            dx_context_error_set (context,
                              "cannot get %s value on KEYVAL whose top-level is MOLD", type);
            return DISIR_STATUS_WRONG_CONTEXT;
        }
        if (context->cx_keyval->kv_mold_equiv == NULL)
        {
            dx_context_error_set (context,
                                  "cannot get value on context without a MOLD");
            return DISIR_STATUS_MOLD_MISSING;
        }

        *storage = &context->cx_keyval->kv_value;
    }
    else if (dc_context_type (context) == DISIR_CONTEXT_DEFAULT)
    {
        *storage = &context->cx_default->de_value;
    }
    else
    {
        dx_context_error_set (context, "Context %s slipped through guard. Not handled.",
                              dc_context_type_string (context));
        return DISIR_STATUS_INTERNAL_ERROR;
    }

    return DISIR_STATUS_OK;
}

static enum disir_status
set_value_input_check (struct disir_context *context, const char *type,
                       struct disir_value **storage)
{
    enum disir_status status;

    status = CONTEXT_NULL_INVALID_TYPE_CHECK (context);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }

    status = CONTEXT_TYPE_CHECK (context,
                                 DISIR_CONTEXT_DEFAULT,
                                 DISIR_CONTEXT_KEYVAL);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged ?
        return status;
    }

    if (dc_context_type (context) == DISIR_CONTEXT_KEYVAL)
    {
        if (dc_context_type (context->cx_root_context) == DISIR_CONTEXT_MOLD)
        {
            dx_context_error_set (context,
                              "cannot set %s value on KEYVAL whose top-level is MOLD", type);
            return DISIR_STATUS_WRONG_CONTEXT;
        }
        if (context->cx_keyval->kv_mold_equiv == NULL)
        {
            dx_context_error_set (context,
                                  "cannot set value on context without a MOLD");
            return DISIR_STATUS_MOLD_MISSING;
        }

        *storage = &context->cx_keyval->kv_value;
    }
    else if (dc_context_type (context) == DISIR_CONTEXT_DEFAULT)
    {
        *storage = &context->cx_default->de_value;
    }
    else
    {
        dx_context_error_set (context, "Context %s slipped through guard. Not handled.",
                              dc_context_type_string (context));
        return DISIR_STATUS_INTERNAL_ERROR;
    }

    return DISIR_STATUS_OK;
}


// PUBLIC API
enum disir_status
dc_set_value (struct disir_context *context, const char *value, int32_t value_size)
{
    return DISIR_STATUS_INTERNAL_ERROR;
}

//! PUBLIC API
enum disir_status
dc_get_value (struct disir_context *context, int32_t output_buffer_size,
              char *output, int32_t *output_size)
{
    enum disir_status status;

    status = CONTEXT_NULL_INVALID_TYPE_CHECK (context);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged;
        return status;
    }

    status = CONTEXT_TYPE_CHECK (context, DISIR_CONTEXT_KEYVAL, DISIR_CONTEXT_FREE_TEXT);
    if (status != DISIR_STATUS_OK)
    {
        dx_log_context (context, "cannot get value from context type");
        return status;
    }
    switch (dc_context_type (context))
    {
    case DISIR_CONTEXT_KEYVAL:
    {
        if (dc_context_type (context->cx_root_context) != DISIR_CONTEXT_CONFIG)
        {
            dx_log_context (context,
                            "cannot get value from KEYVAL context whose root is not CONFIG");
            return DISIR_STATUS_WRONG_CONTEXT;
        }
        status = dx_value_stringify (&context->cx_keyval->kv_value,
                                     output_buffer_size, output, output_size);
        break;
    }
    case DISIR_CONTEXT_FREE_TEXT:
    {
        status = dx_value_stringify (context->cx_value, output_buffer_size, output, output_size);
        break;
    }
    default:
    {
        status = DISIR_STATUS_INTERNAL_ERROR;
        dx_crash_and_burn ("%s - UNHANDLED_CONTEXT_TYPE: %s",
                           __FUNCTION__, dc_context_type_string (context));
    }
    }

    return status;
}

//! PUBLIC API
enum disir_status
dc_set_value_string (struct disir_context *context, const char *value, int32_t value_size)
{
    enum disir_status status;

    TRACE_ENTER ("context: %p, value: %s, value_size: %d", context, value, value_size);

    // Check arguments
    status = CONTEXT_NULL_INVALID_TYPE_CHECK (context);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged.
        return status;
    }
    if (value == NULL || value_size <= 0)
    {
        dx_log_context (context, "value must be non-null and of positive length.");
        log_debug (6, "value: %p, value_size: %d", value_size);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

     status = CONTEXT_TYPE_CHECK (context,
                                 DISIR_CONTEXT_KEYVAL,
                                 DISIR_CONTEXT_DEFAULT,
                                 DISIR_CONTEXT_DOCUMENTATION,
                                 DISIR_CONTEXT_FREE_TEXT);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged ?
        return status;
    }

    switch (dc_context_type (context))
    {
    case DISIR_CONTEXT_DOCUMENTATION:
    {
        status = dx_documentation_add_value_string (context->cx_documentation, value, value_size);
        break;
    }
    case DISIR_CONTEXT_KEYVAL:
    {
        if (dc_context_type (context->cx_root_context) != DISIR_CONTEXT_CONFIG)
        {
            dx_log_context (context,
                            "cannot set string value on KEYVAL whose top-level is MOLD");
            return DISIR_STATUS_WRONG_CONTEXT;
        }
        if (context->cx_keyval->kv_mold_equiv == NULL)
        {
            dx_log_context (context, "cannot set value on context without a MOLD");
            return DISIR_STATUS_MOLD_MISSING;
        }
        // TODO: Validate input against mold
        status = dx_value_set_string (&context->cx_keyval->kv_value, value, value_size);
        break;
    }
    case DISIR_CONTEXT_DEFAULT:
    {
        // TODO: Validate against default restrictions
        status = dx_value_set_string (&context->cx_default->de_value, value, value_size);
        break;
    }
    case DISIR_CONTEXT_CONFIG:
    case DISIR_CONTEXT_MOLD:
    case DISIR_CONTEXT_SECTION:
    case DISIR_CONTEXT_FREE_TEXT:
    case DISIR_CONTEXT_RESTRICTION:
        dx_crash_and_burn ("%s - UNHANDLED CONTEXT TYPE: %s",
                __FUNCTION__, dc_context_type_string (context));
    case DISIR_CONTEXT_UNKNOWN:
        status = DISIR_STATUS_BAD_CONTEXT_OBJECT;
    // No default case - let compiler warn us of unhandled context
    }

    if (status != DISIR_STATUS_OK)
    {
        dx_context_error_set (context,
                              "cannot set string value on context whose value type is %s",
                              dc_value_type_string (context));
    }

    TRACE_EXIT ("status: %s", disir_status_string (status));
    return status;
}

//! PUBLIC API
enum disir_status
dc_get_value_string (struct disir_context *context, const char **output, int32_t *size)
{
    enum disir_status status;
    enum disir_value_type type;

    TRACE_ENTER ("contexT: %p, output: %p, size: %p", context, output, size);

    status = CONTEXT_NULL_INVALID_TYPE_CHECK (context);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }

    status = CONTEXT_TYPE_CHECK (context,
                                 DISIR_CONTEXT_KEYVAL,
                                 DISIR_CONTEXT_DOCUMENTATION,
                                 DISIR_CONTEXT_FREE_TEXT,
                                 DISIR_CONTEXT_DEFAULT);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged ?
        return status;
    }

    if (output == NULL)
    {
        log_debug (0, "invoked with output NULL pointer");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    status = dc_get_value_type (context, &type);
    if (status != DISIR_STATUS_OK)
    {
        dx_log_context (context, "cannot retrieve value type of context: %s",
                        disir_status_string (status));
        return status;
    }

    if (type != DISIR_VALUE_TYPE_STRING)
    {
        dx_log_context (context, "cannot get string value on context whose value type is %s",
                        dx_value_type_string (type));
        return DISIR_STATUS_WRONG_VALUE_TYPE;
    }

    switch (dc_context_type (context))
    {
    case DISIR_CONTEXT_KEYVAL:
    {
        if (dc_context_type (context->cx_root_context) != DISIR_CONTEXT_CONFIG)
        {
            dx_log_context (context, "cannot retrieve value when root context is not CONFIG");
            return DISIR_STATUS_WRONG_CONTEXT;
        }
        status = dx_value_get_string (&context->cx_keyval->kv_value, output, size);
        break;
    }
    case DISIR_CONTEXT_DOCUMENTATION:
    {
        status = dx_value_get_string (&context->cx_documentation->dd_value, output, size);
        break;
    }
    case DISIR_CONTEXT_FREE_TEXT:
    {
        status = dx_value_get_string (context->cx_value, output, size);
        break;
    }
    case DISIR_CONTEXT_DEFAULT:
        status = dx_value_get_string (&context->cx_default->de_value, output, size);
        break;
    default:
    {
        log_fatal_context (context, "slipped through guard - unsupported.");
        return DISIR_STATUS_INTERNAL_ERROR;
    }
    }

    TRACE_EXIT ("status: %s", disir_status_string (status));
    return status;
}

//! PUBLIC API
enum disir_status
dc_get_value_integer (struct disir_context *context, int64_t *value)
{
    enum disir_status status;
    struct disir_value *storage;

    status = get_value_input_check (context, "integer", &storage);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }

    status = dx_value_get_integer (storage, value);
    if (status == DISIR_STATUS_INVALID_ARGUMENT)
    {
        dx_context_error_set (context,
                              "cannot get integer with value NULL pointer");
    }
    if (status == DISIR_STATUS_WRONG_VALUE_TYPE)
    {
        dx_context_error_set (context,
                              "cannot get integer value on context whose value type is %s",
                              dc_value_type_string (context));
    }

    return status;
}

//! PUBLIC API
enum disir_status
dc_set_value_integer (struct disir_context *context, int64_t value)
{
    enum disir_status status;
    struct disir_value *value_storage;

    status = set_value_input_check (context, "integer", &value_storage);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }

    status = dx_value_set_integer (value_storage, value);
    if (status != DISIR_STATUS_OK)
    {
        dx_context_error_set (context,
                              "cannot set integer value on context whose value type is %s",
                              dc_value_type_string (context));
    }

    return status;
}

//! PUBLIC API
enum disir_status
dc_get_value_float (struct disir_context *context, double *value)
{
    enum disir_status status;
    struct disir_value *storage;

    status = get_value_input_check (context, "float", &storage);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }

    status = dx_value_get_float(storage, value);
    if (status == DISIR_STATUS_INVALID_ARGUMENT)
    {
        dx_context_error_set (context,
                              "cannot get float with value NULL pointer");
    }
    if (status == DISIR_STATUS_WRONG_VALUE_TYPE)
    {
        dx_context_error_set (context,
                              "cannot get float value on context whose value type is %s",
                              dc_value_type_string (context));
    }

    return status;
}

//! PUBLIC API
enum disir_status
dc_set_value_float (struct disir_context *context, double value)
{
    enum disir_status status;
    struct disir_value *value_storage;

    status = set_value_input_check (context, "float", &value_storage);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }

    status = dx_value_set_float (value_storage, value);
    if (status != DISIR_STATUS_OK)
    {
        dx_context_error_set (context,
                              "cannot set float value on context whose value type is %s",
                              dc_value_type_string (context));
    }

    return status;
}

//! PUBLIC API
enum disir_status
dc_get_value_boolean (struct disir_context *context, uint8_t *value)
{
    enum disir_status status;
    struct disir_value *storage;

    status = get_value_input_check (context, "boolean", &storage);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }

    status = dx_value_get_boolean (storage, value);
    if (status == DISIR_STATUS_INVALID_ARGUMENT)
    {
        dx_context_error_set (context,
                              "cannot get boolean with value NULL pointer");
    }
    if (status == DISIR_STATUS_WRONG_VALUE_TYPE)
    {
        dx_context_error_set (context,
                              "cannot get boolean value on context whose value type is %s",
                              dc_value_type_string (context));
    }

    return status;
}

//! PUBLIC API
enum disir_status
dc_set_value_boolean (struct disir_context *context, uint8_t value)
{
    enum disir_status status;
    struct disir_value *value_storage;

    status = set_value_input_check (context, "boolean", &value_storage);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }

    status = dx_value_set_boolean (value_storage, value);
    if (status != DISIR_STATUS_OK)
    {
        dx_context_error_set (context,
                              "cannot set boolean value on context whose value type is %s",
                              dc_value_type_string (context));
    }

    return status;
}

