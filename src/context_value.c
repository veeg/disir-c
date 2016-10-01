// External public includes
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

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
#include "value.h"
#include "restriction.h"

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
set_value_input_check (struct disir_context *context, enum disir_value_type type,
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
                                 DISIR_CONTEXT_DOCUMENTATION,
                                 DISIR_CONTEXT_FREE_TEXT,
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
                              "cannot set %s value on KEYVAL whose top-level is MOLD",
                              dx_value_type_string (type));
            return DISIR_STATUS_WRONG_CONTEXT;
        }

        *storage = &context->cx_keyval->kv_value;

        // Assign keyval to its mold equiv type
        // (Makes a correction in the type previously sat wrongfully (on purpose) below.)
        if (context->cx_keyval->kv_mold_equiv)
        {
            context->cx_keyval->kv_value.dv_type
                = context->cx_keyval->kv_mold_equiv->cx_keyval->kv_value.dv_type;
        }
    }
    else if (dc_context_type (context) == DISIR_CONTEXT_DEFAULT)
    {
        *storage = &context->cx_default->de_value;
    }
    else if (dc_context_type (context) == DISIR_CONTEXT_DOCUMENTATION)
    {
        *storage = &context->cx_documentation->dd_value;
    }
    else if (dc_context_type (context) == DISIR_CONTEXT_FREE_TEXT)
    {
        *storage = context->cx_value;
    }
    else
    {
        dx_context_error_set (context, "Context %s slipped through guard. Not handled.",
                              dc_context_type_string (context));
        return DISIR_STATUS_INTERNAL_ERROR;
    }



    // Use cases this function shall handle:
    // 1: Config Keyval in construction state with no mold equivalent
    //      (no set_name has been invoked)
    //      solution: Store input value in FFA value storage. set INVALID state.
    //                Construct error message.
    //      return: DISIR_STATUS_MOLD_MISSING
    // 2: Config Keyval in construction state with invalid context
    //      (set_name has been attempted - still no mold equivalent)
    //      solution: Store input value in FFA value storage.
    //      return: DISIR_STATUS_INVALID_CONTEXT
    // 3: Config Keyval in construction state with mold equivalent but wrong input type.
    //      solution: Change config keyval storage type to input type and set invalid context
    //      return: DISIR_STATUS_INVALID_CONTEXT
    // 4: Config Keyval in finalized state with with no mold equivalent
    //      (By extension, also invalid state)
    //      solution: store input value in FFA value storage.
    // 5: Config Keyval in finalized state with mold equivalent
    //      Nominal use case.


    if (dc_context_type (context) == DISIR_CONTEXT_KEYVAL)
    {
        // Use-case 1, 2 & 3
        if (context->CONTEXT_STATE_CONSTRUCTING)
        {
            // Use-case 1
            // set_name has never been called - therefore we do not have a mold equivalent.
            if (context->cx_keyval->kv_name.dv_size == 0)
            {
                dx_context_error_set (context, "cannot set value on context without a MOLD");
                status = DISIR_STATUS_MOLD_MISSING;

            }
            // Use-case 2
            // set_name has been called and failed - therefore we do not have a mold equivalent
            else if (context->cx_keyval->kv_mold_equiv == NULL)
            {
                // context error message already set by set_name
                status = DISIR_STATUS_INVALID_CONTEXT;
                // Set storage type to mirror input
                (*storage)->dv_type = type;

            }
            // Use-case 3
            // set_name has been called and succeed, but wrong type assigned.
            else if (dc_value_type (context->cx_keyval->kv_mold_equiv) != type)
            {
                // We should allow this through - set invalid value
                status = DISIR_STATUS_INVALID_CONTEXT;
                context->CONTEXT_STATE_INVALID = 1;
                dx_context_error_set (context, "Assigned value type %s, expecting %s",
                                      dx_value_type_string (type),
                                      dx_value_type_string ((*storage)->dv_type));
                // Set storage type to mirror input
                (*storage)->dv_type = type;
            }
        }
        // Use-case 4
        else if (context->CONTEXT_STATE_FINALIZED &&
                 context->CONTEXT_STATE_INVALID)
        {
            // Set storage type to mirror input
            (*storage)->dv_type = type;
            status = DISIR_STATUS_INVALID_CONTEXT;
        }
    }

    return status;
}


// PUBLIC API
enum disir_status
dc_set_value (struct disir_context *context, const char *value, int32_t value_size)
{
    enum disir_status status;
    uint8_t parsed_boolean;
    long int parsed_integer;
    double parsed_float;
    char *endptr;
    char c;

    status = CONTEXT_NULL_INVALID_TYPE_CHECK (context);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged;
        return status;
    }

    status = CONTEXT_TYPE_CHECK (context, DISIR_CONTEXT_KEYVAL);
    if (status != DISIR_STATUS_OK)
    {
        dx_context_error_set (context, "Cannot set value to %s.",
                                       dc_context_type_string (context));
        return status;
    }
    if (dc_context_type (context->cx_root_context) != DISIR_CONTEXT_CONFIG)
    {
        dx_context_error_set (context, "Cannot set value to %s whose top-level is %s.",
                                       dc_context_type_string (context),
                                       dc_context_type_string (context->cx_root_context));
        return DISIR_STATUS_WRONG_CONTEXT;
    }

    switch (dc_value_type (context))
    {
    case DISIR_VALUE_TYPE_STRING:
    {
        status = dc_set_value_string (context, value, value_size);
        break;
    }
    case DISIR_VALUE_TYPE_BOOLEAN:
    {
        // Handles single character input 1/0, t/f and T/F.
        // XXX: Falsely identifies input fLOL as false and tROFL as true.
        c = *value;
        parsed_boolean = 2;
        if (c == '0')
        {
            parsed_boolean = 0;
        }
        else if (c == '1')
        {
            parsed_boolean = 1;
        }
        else if (tolower(c) == 't')
        {
            parsed_boolean = 1;
        }
        else if (tolower(c) == 'f')
        {
            parsed_boolean = 0;
        }

        if (parsed_boolean == 2)
        {
            status = DISIR_STATUS_INVALID_ARGUMENT;
        }
        else
        {
            status = dc_set_value_boolean (context, parsed_boolean);
        }
        break;
    }
    case DISIR_VALUE_TYPE_INTEGER:
    {
        parsed_integer = strtol (value, &endptr, 10);
        if (value == endptr)
        {
            status = DISIR_STATUS_INVALID_ARGUMENT;
        }
        else
        {
            status = dc_set_value_integer (context, parsed_integer);
        }
        break;
    }
    case DISIR_VALUE_TYPE_FLOAT:
    {
        parsed_float = strtod (value, &endptr);
        if (value == endptr)
        {
            status = DISIR_STATUS_INVALID_ARGUMENT;
        }
        else
        {
            status = dc_set_value_float (context, parsed_float);
        }
        break;
    }
    case DISIR_VALUE_TYPE_ENUM:
    {
        dx_crash_and_burn ("dc_set_value ENUM NOT HANDLED!");
    }
    case DISIR_VALUE_TYPE_UNKNOWN:
    {
        log_fatal_context (context, "value type unknown.");
        status = DISIR_STATUS_INTERNAL_ERROR;
    }
    }

    return status;
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
    enum disir_status invalid;
    struct disir_value *value_storage;

    TRACE_ENTER ("context: %p, value: %s, value_size: %d", context, value, value_size);

    invalid = set_value_input_check (context, DISIR_VALUE_TYPE_STRING, &value_storage);
    if (invalid != DISIR_STATUS_OK && invalid != DISIR_STATUS_INVALID_CONTEXT)
    {
        // Already logged
        return invalid;
    }

    status = dx_value_set_string (value_storage, value, value_size);
    if (status != DISIR_STATUS_OK)
    {
        dx_context_error_set (context,
                              "cannot set %s value on context whose value type is %s.",
                              dx_value_type_string (DISIR_VALUE_TYPE_STRING),
                              dc_value_type_string (context));
    }
    else
        status = invalid;

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
    enum disir_status invalid;
    struct disir_value *value_storage;

    invalid = set_value_input_check (context, DISIR_VALUE_TYPE_INTEGER, &value_storage);
    if (invalid != DISIR_STATUS_OK && invalid != DISIR_STATUS_INVALID_CONTEXT)
    {
        // Already logged
        return invalid;
    }

    // Restriction check
    // Can only be applied if input context is valid.
    // Can only be applied to top-level CONFIG
    // We only permit setting an unfulfilled value in constructing mode.

    if (invalid != DISIR_STATUS_INVALID_CONTEXT)
    {
        invalid = dx_restriction_exclusive_value_check (context, value, 0);
    }

    // Do not allow finalized context to change value
    if (context->CONTEXT_STATE_FINALIZED && invalid == DISIR_STATUS_RESTRICTION_VIOLATED)
    {
        return invalid;
    }

    // Mark unfulfilled value sat in constructing mode as invalid context
    if (invalid == DISIR_STATUS_RESTRICTION_VIOLATED)
    {
        context->CONTEXT_STATE_INVALID = 1;
    }

    status = dx_value_set_integer (value_storage, value);
    if (status != DISIR_STATUS_OK)
    {
        dx_context_error_set (context,
                              "cannot set %s value on context whose value type is %s.",
                              dx_value_type_string (DISIR_VALUE_TYPE_INTEGER),
                              dc_value_type_string (context));
    }

    return (status == DISIR_STATUS_OK ? invalid : status);
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
    enum disir_status invalid;
    struct disir_value *value_storage;

    invalid = set_value_input_check (context, DISIR_VALUE_TYPE_FLOAT, &value_storage);
    if (invalid != DISIR_STATUS_OK && invalid != DISIR_STATUS_INVALID_CONTEXT)
    {
        // Already logged
        return invalid;
    }

    if (invalid != DISIR_STATUS_INVALID_CONTEXT)
    {
        invalid = dx_restriction_exclusive_value_check (context, 0, value);
    }

    // Do not allow finalized context to change value
    if (context->CONTEXT_STATE_FINALIZED && invalid == DISIR_STATUS_RESTRICTION_VIOLATED)
    {
        return invalid;
    }

    // Mark unfulfilled value sat in constructing mode as invalid context
    if (invalid == DISIR_STATUS_RESTRICTION_VIOLATED)
    {
        context->CONTEXT_STATE_INVALID = 1;
    }

    status = dx_value_set_float (value_storage, value);
    if (status != DISIR_STATUS_OK)
    {
        dx_context_error_set (context,
                              "cannot set %s value on context whose value type is %s.",
                              dx_value_type_string (DISIR_VALUE_TYPE_FLOAT),
                              dc_value_type_string (context));
    }
    else
        status = invalid;

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
    enum disir_status invalid;
    struct disir_value *value_storage;

    invalid = set_value_input_check (context, DISIR_VALUE_TYPE_BOOLEAN, &value_storage);
    if (invalid != DISIR_STATUS_OK && invalid != DISIR_STATUS_INVALID_CONTEXT)
    {
        // Already logged
        return invalid;
    }

    status = dx_value_set_boolean (value_storage, value);
    if (status != DISIR_STATUS_OK)
    {
        dx_context_error_set (context,
                              "cannot set %s value on context whose value type is %s.",
                              dx_value_type_string (DISIR_VALUE_TYPE_BOOLEAN),
                              dc_value_type_string (context));
    }
    else
        status = invalid;

    return status;
}

