// Standard includes
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

// Public disir interface
#include <disir/disir.h>

// Private disir includes
#include "value.h"
#include "log.h"

//! Array of strings describing the various DISIR_STATUS_* enumerations
const char *disir_status_strings[] = {
    "OK",
    "NO CAN DO",
    "INVALID ARGUMENT",
    "TOO FEW ARGUMENTS",
    "CONTEXT IN WRONG STATE",
    "WRONG CONTEXT",
    "INVALID CONTEXT",
    "DESTROYED CONTEXT",
    "FATAL CONTEXT",
    "BAD CONTEXT OBJECT",
    "NO MEMORY",
    "NO ERROR",
    "INTERNAL ERROR",
    "INSUFFICIENT RESOURCES",
    "EXISTS",
    "CONFLICTING SEMVER",
    "CONFLICT",
    "EXHAUSTED",
    "MOLD MISSING",
    "WRONG VALUE TYPE",
    "NOT EXIST",
    "RESTRICTION VIOLATED",
    "ELEMENTS INVALID",
    "NOT SUPPORTED",
    "PLUGIN ERROR",
    "LOAD ERROR",
    "CONFIG INVALID",
    "GROUP MISSING",
    "PERMISSION ERROR",
    "FILESYSTEM ERROR",
    "DEFAULT MISSING",
    "UNKNOWN",
};

//! PUBLIC API
const char *
disir_status_string (enum disir_status status)
{
    if (status > DISIR_STATUS_UNKNOWN)
    {
        status = DISIR_STATUS_UNKNOWN;
    }

    return disir_status_strings[status];
}

//! PUBLIC API
char *
dc_version_string (char *buffer, int32_t buffer_size, struct disir_version *version)
{
    int res;
    if (buffer == NULL || version == NULL)
    {
        log_warn ("%s invoked with NULL pointer(s) (buffer %p, version: %p)",
                  __func__, buffer, version);
        return NULL;
    }

    res = snprintf (buffer, buffer_size,
            "%u.%u", version->sv_major, version->sv_minor);
    if (res < 0 || res >= buffer_size)
    {
        log_warn ("Insufficient buffer( %p ) size( %d ) to copy version (%u.%u) - Res: %d",
                    buffer, buffer_size, version->sv_major, version->sv_minor, res);
        return NULL;
    }

    return buffer;
}

//! PUBLIC API
int
dc_version_compare (struct disir_version *s1, struct disir_version *s2)
{
    int res;

    res = 0;

    res = s1->sv_major - s2->sv_major;
    if (res == 0)
        res = s1->sv_minor - s2->sv_minor;

    log_debug (8, "s1 (%u.%u) vs s2 (%u.%u) res (%d)",
                    s1->sv_major, s1->sv_minor,
                    s2->sv_major, s2->sv_minor, res);

    return res;
}

//! PUBLIC API
void
dc_version_set (struct disir_version *destination, struct disir_version *source)
{
    if (destination == NULL || source == NULL)
    {
        return;
    }

    log_debug (5, "setting version (%p) to %u.%u from source (%p)",
                  destination, source->sv_major, source->sv_minor, source);

    destination->sv_major = source->sv_major;
    destination->sv_minor = source->sv_minor;
}

//! PUBLIC API
enum disir_status
dc_version_convert (const char *input, struct disir_version *version)
{
    long int parsed_integer;
    char *endptr;
    const char *start;

    if (input == NULL || version == NULL)
    {
        log_debug (0, "invoked with NULL pointer(s). input: %p, version: %p", input, version);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    log_debug (3, "Converting version input: %s", input);

    // Get Major version
    start = input;
    parsed_integer = strtol (start, &endptr, 10);
    if (start == endptr)
    {
        log_debug (6, "no int parsed");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }
    if (*endptr != '.')
    {
        log_debug (6, "missing period seperator after major");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }
    version->sv_major = parsed_integer;

    // Get minor version
    endptr++;
    start = endptr;
    parsed_integer = strtol (start, &endptr, 10);
    if (start == endptr)
    {
        log_debug (6, "no int parsed");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }
    version->sv_minor = parsed_integer;

    return DISIR_STATUS_OK;
}

// TODO: Refactor this function. Return int as normal string functions
//! INTERNAL API
enum disir_status
dx_value_stringify (struct disir_value *value, int32_t output_buffer_size,
                    char *output, int32_t *output_size)
{
    enum disir_status status;
    int32_t size;

    status = DISIR_STATUS_OK;
    switch (value->dv_type)
    {
    case DISIR_VALUE_TYPE_ENUM:
    case DISIR_VALUE_TYPE_STRING:
    {
        size = value->dv_size;

        if (output_size)
        {
            *output_size = size;
        }

        if (size >= output_buffer_size)
        {
            // Set size to be output_buffer_size - 1,
            // so we can signal that the supplied buffer is insufficient.
            size = output_buffer_size - 1;
            log_debug (3, "Insufficient buffer provided - decrementing output size by one");
        }

        memcpy (output, value->dv_string, size);
        output[size] = '\0';
        break;
    }
    case DISIR_VALUE_TYPE_INTEGER:
    {
        snprintf (output, output_buffer_size, "%ld", value->dv_integer);
        break;
    }
    case DISIR_VALUE_TYPE_FLOAT:
    {
        snprintf (output, output_buffer_size, "%lf", value->dv_float);
        break;
    }
    case DISIR_VALUE_TYPE_BOOLEAN:
    {
        snprintf(output, output_buffer_size, "%s",
                (value->dv_boolean ? "True" : "False"));
        break;
    }
    default:
    {
        status = DISIR_STATUS_INVALID_ARGUMENT;
    }
    }

    return status;
}

//! INTERNAL API
int
dx_value_compare (struct disir_value *v1, struct disir_value *v2)
{
    if (v1->dv_type != v2->dv_type)
        return (INT_MIN);

    if (v1->dv_size != v2->dv_size)
        return (v1->dv_size - v2->dv_size);

    // FIXME: There is probably some parts of this
    //  compare that is sustainable to underflow/overflow..
    switch (dx_value_type_sanify (v1->dv_type))
    {
    case DISIR_VALUE_TYPE_ENUM:
        // FALL-THROUGH
    case DISIR_VALUE_TYPE_STRING:
        return memcmp (v1->dv_string, v2->dv_string, v1->dv_size);
    case DISIR_VALUE_TYPE_INTEGER:
        return (v1->dv_integer - v2->dv_integer);
    case DISIR_VALUE_TYPE_FLOAT:
        return (int)(v1->dv_float - v2->dv_float);
    case DISIR_VALUE_TYPE_BOOLEAN:
        return (v1->dv_boolean - v2->dv_boolean);
    default:
        return (INT_MIN);
    }
}

//! INTERNAL API
enum disir_status
dx_value_copy (struct disir_value *destination, struct disir_value *source)
{
    switch (dx_value_type_sanify (source->dv_type))
    {
    case DISIR_VALUE_TYPE_ENUM:
        // FALL-THROUGH
    case DISIR_VALUE_TYPE_STRING:
        return dx_value_set_string (destination, source->dv_string, source->dv_size);
        break;
    case DISIR_VALUE_TYPE_INTEGER:
        return dx_value_set_integer (destination, source->dv_integer);
        break;
    case DISIR_VALUE_TYPE_FLOAT:
        return dx_value_set_float (destination, source->dv_float);
        break;
    case DISIR_VALUE_TYPE_BOOLEAN:
        return dx_value_set_boolean (destination, source->dv_boolean);
        break;
    case DISIR_VALUE_TYPE_UNKNOWN:
        dx_crash_and_burn ("%s unhandled", __func__);
        break;
    }

    // Never reached
    return DISIR_STATUS_INTERNAL_ERROR;
}

// INTERNAL API
enum disir_status
dx_value_set_integer (struct disir_value *value, int64_t integer)
{
    if (value == NULL)
    {
        log_debug (0, "invoked with NULL value pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }
    if (value->dv_type != DISIR_VALUE_TYPE_INTEGER)
    {
        log_debug (0, "invoked with non-integer value type %s (%d)",
                   dx_value_type_string (value->dv_type), value->dv_type);
        return DISIR_STATUS_WRONG_VALUE_TYPE;
    }

    value->dv_integer = integer;
    return DISIR_STATUS_OK;
}

//! INTERNAL API
enum disir_status
dx_value_get_integer (struct disir_value *value, int64_t *integer)
{
    if (value == NULL)
    {
        log_debug (0, "invoked with NULL value pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }
    if (integer == NULL)
    {
        log_debug (0, "invoked with NULL integer pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }
    if (value->dv_type != DISIR_VALUE_TYPE_INTEGER)
    {
        log_debug (0, "invoked with non-integer value type %s (%d)",
                   dx_value_type_string (value->dv_type), value->dv_type);
        return DISIR_STATUS_WRONG_VALUE_TYPE;
    }

    *integer = value->dv_integer;
    return DISIR_STATUS_OK;
}

// INTERNAL API
enum disir_status
dx_value_set_boolean (struct disir_value *value, uint8_t boolean)
{
    if (value == NULL)
    {
        log_debug (0, "invoked with NULL value pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }
    if (value->dv_type != DISIR_VALUE_TYPE_BOOLEAN)
    {
        log_debug (0, "invoked with non-boolean value type %s (%d)",
                   dx_value_type_string (value->dv_type), value->dv_type);
        return DISIR_STATUS_WRONG_VALUE_TYPE;
    }

    value->dv_boolean = boolean;
    return DISIR_STATUS_OK;
}

//! INTERNAL API
enum disir_status
dx_value_get_boolean (struct disir_value *value, uint8_t *boolean)
{
    if (value == NULL)
    {
        log_debug (0, "invoked with NULL value pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }
    if (boolean == NULL)
    {
        log_debug (0, "invoked with NULL boolean pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }
    if (value->dv_type != DISIR_VALUE_TYPE_BOOLEAN)
    {
        log_debug (0, "invoked with non-boolean value type %s (%d)",
                   dx_value_type_string (value->dv_type), value->dv_type);
        return DISIR_STATUS_WRONG_VALUE_TYPE;
    }

    *boolean = value->dv_boolean;
    return DISIR_STATUS_OK;
}

// INTERNAL API
enum disir_status
dx_value_set_float (struct disir_value *value, double input_double)
{
    if (value == NULL)
    {
        log_debug (0, "invoked with NULL value pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }
    if (value->dv_type != DISIR_VALUE_TYPE_FLOAT)
    {
        log_debug (0, "invoked with non-float value type %s (%d)",
                   dx_value_type_string (value->dv_type), value->dv_type);
        return DISIR_STATUS_WRONG_VALUE_TYPE;
    }

    value->dv_float = input_double;
    return DISIR_STATUS_OK;
}

//! INTERNAL API
enum disir_status
dx_value_get_float (struct disir_value *value, double *output_double)
{
    if (value == NULL)
    {
        log_debug (0, "invoked with NULL value pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }
    if (output_double == NULL)
    {
        log_debug (0, "invoked with NULL output_double pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }
    if (value->dv_type != DISIR_VALUE_TYPE_FLOAT)
    {
        log_debug (0, "invoked with non-float value type %s (t%d)",
                   dx_value_type_string (value->dv_type), value->dv_type);
        return DISIR_STATUS_WRONG_VALUE_TYPE;
    }

    *output_double = value->dv_float;
    return DISIR_STATUS_OK;
}

//! INTERNAL API
enum disir_status
dx_value_set_string (struct disir_value *value, const char *input, int32_t size)
{
    if (value == NULL)
    {
        log_debug (0, "invoked with NULL value pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }
    if (value->dv_type != DISIR_VALUE_TYPE_STRING &&
        value->dv_type != DISIR_VALUE_TYPE_ENUM)
    {
        log_debug (0, "invoked with non-string value type %s (%d)",
                   dx_value_type_string (value->dv_type), value->dv_type);
        return DISIR_STATUS_WRONG_VALUE_TYPE;
    }
    if (input == NULL)
    {
        if (value->dv_size > 0)
        {
            free (value->dv_string);
        }
        value->dv_size = 0;
        value->dv_string = NULL;
        return DISIR_STATUS_OK;
    }

    if (value->dv_string == NULL || value->dv_size - 1 < size)
    {
        // Just free the existing memory. We allocate a larger one below
        if (value->dv_size - 1 < size)
        {
            free (value->dv_string);
            value->dv_string = NULL;
        }

        // Size of requested string + 1 for NULL terminator
        value->dv_string = calloc (1, size + 1);
        if (value->dv_string == NULL)
        {
            log_error ("failed to allocate sufficient memory for value string (%d)",
                       size + 1);
            return DISIR_STATUS_NO_MEMORY;
        }
    }

    // Copy the incoming docstring to freely available space
    memcpy (value->dv_string, input, size);
    value->dv_size = size;

    // Terminate it with a zero terminator. Just to be safe.
    value->dv_string[size] = '\0';

    log_debug (5, "stored string in disir_value (%p) of size (%d): %s\n",
               value, value->dv_size, value->dv_string);

    return DISIR_STATUS_OK;
}

//! INTERNAL API
enum disir_status
dx_value_get_string (struct disir_value *value, const char **output, int32_t *size)
{
    if (value == NULL)
    {
        log_debug (0, "invoked with NULL value pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }
    if (output == NULL)
    {
        log_debug (0, "invoked with NULL string pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }
    if (value->dv_type != DISIR_VALUE_TYPE_STRING && value->dv_type != DISIR_VALUE_TYPE_ENUM)
    {
        log_debug (0, "invoked with non-string value type (%d)", value->dv_type);
        return DISIR_STATUS_WRONG_VALUE_TYPE;
    }

    if (size)
    {
        *size = value->dv_size;
    }
    *output = value->dv_string;
    return DISIR_STATUS_OK;
}

