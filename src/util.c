// Standard includes
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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
    "BAD CONTEXT OBJECT",
    "NO MEMORY",
    "NO ERROR",
    "INTERNAL ERROR",
    "INSUFFICIENT RESOURCES",
    "EXISTS",
    "CONFLICTING SEMVER",
    "EXHAUSTED",
    "UNKNOWN",
};

//! PUBLIC API
const char *
disir_status_string (enum disir_status status)
{
    if (status < 0 || status > DISIR_STATUS_UNKNOWN)
    {
        status = DISIR_STATUS_UNKNOWN;
    }

    return disir_status_strings[status];
}

//! PUBLIC API
char *
dx_semver_string (char *buffer, int32_t buffer_size, struct semantic_version *semver)
{
    int res;
    if (buffer == NULL || semver == NULL)
    {
        log_warn ("%s invoked with NULL buffer or semverpointer", __FUNCTION__);
        return "<invalid_arguments>";
    }

    res = snprintf (buffer, buffer_size,
            "%d.%d.%d", semver->sv_major, semver->sv_minor, semver->sv_patch);
    if (res < 0 || res >= buffer_size)
    {
        log_warn ("Insufficient buffer( %p ) size( %d ) to copy semver (%d.%d.%d) - Res: %d",
                    buffer, buffer_size, semver->sv_major, semver->sv_minor,
                    semver->sv_patch, res);
        return "<insufficient_buffer>";
    }

    return buffer;
}

//! PUBLIC API
int
dx_semantic_version_compare (struct semantic_version *s1, struct semantic_version *s2)
{
    int res;

    res = 0;

    res = s1->sv_major - s2->sv_major;
    if (res == 0)
        res = s1->sv_minor - s2->sv_minor;
    if (res == 0)
        res = s1->sv_patch - s2->sv_patch;

    return res;
}

// INTERNAL API
enum disir_status
dx_value_set_integer (struct disir_value *value, int64_t integer)
{
    if (value == NULL)
    {
        log_debug ("invoked with NULL value pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }
    if (value->dv_type != DISIR_VALUE_TYPE_INTEGER)
    {
        log_debug ("invoked with non-integer value type %s (%d)",
                   dx_value_type_string (value->dv_type), value->dv_type);
        return DISIR_STATUS_INVALID_ARGUMENT;
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
        log_debug ("invoked with NULL value pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }
    if (integer == NULL)
    {
        log_debug ("invoked with NULL integer pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }
    if (value->dv_type != DISIR_VALUE_TYPE_INTEGER)
    {
        log_debug ("invoked with non-integer value type %s (%d)",
                   dx_value_type_string (value->dv_type), value->dv_type);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    *integer = value->dv_integer;
    return DISIR_STATUS_OK;
}

// INTERNAL API
enum disir_status
dx_value_set_float (struct disir_value *value, double input_double)
{
    if (value == NULL)
    {
        log_debug ("invoked with NULL value pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }
    if (value->dv_type != DISIR_VALUE_TYPE_FLOAT)
    {
        log_debug ("invoked with non-float value type %s (%d)",
                   dx_value_type_string (value->dv_type), value->dv_type);
        return DISIR_STATUS_INVALID_ARGUMENT;
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
        log_debug ("invoked with NULL value pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }
    if (output_double == NULL)
    {
        log_debug ("invoked with NULL output_double pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }
    if (value->dv_type != DISIR_VALUE_TYPE_FLOAT)
    {
        log_debug ("invoked with non-float value type %s (%d)",
                   dx_value_type_string (value->dv_type), value->dv_type);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    *output_double = value->dv_float;
    return DISIR_STATUS_OK;
}

//! INTERNAL API
enum disir_status
dx_value_set_string (struct disir_value *value, const char *string, int32_t string_size)
{
    if (value == NULL)
    {
        log_debug ("invoked with NULL value pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }
    if (value->dv_type != DISIR_VALUE_TYPE_STRING)
    {
        log_debug ("invoked with non-string value type %s (%d)",
                   dx_value_type_string (value->dv_type), value->dv_type);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }
    if (string == NULL)
    {
        if (value->dv_size > 0)
        {
            free (value->dv_string);
        }
        value->dv_size = 0;
        value->dv_string = NULL;
        return DISIR_STATUS_OK;
    }

    if (value->dv_string == NULL || value->dv_size - 1 < string_size)
    {
        // Just free the existing memory. We allocate a larger one below
        if (value->dv_size - 1 < string_size)
        {
            free (value->dv_string);
            value->dv_string = NULL;
        }

        // Size of requested string + 1 for NULL terminator
        value->dv_string = calloc (1, string_size + 1);
        if (value->dv_string == NULL)
        {
            log_error ("failed to allocate sufficient memory for value string (%d)",
                       string_size + 1);
            return DISIR_STATUS_NO_MEMORY;
        }
    }

    // Copy the incoming docstring to freely available space
    memcpy (value->dv_string, string, string_size);
    value->dv_size = string_size;

    // Terminate it with a zero terminator. Just to be safe.
    value->dv_string[string_size] = '\0';

    return DISIR_STATUS_OK;
}

//! INTERNAL API
enum disir_status
dx_value_get_string (struct disir_value *value, const char **string, int32_t *size)
{
    if (value == NULL)
    {
        log_debug ("invoked with NULL value pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }
    if (string == NULL)
    {
        log_debug ("invoked with NULL string pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }
    if (value->dv_type != DISIR_VALUE_TYPE_STRING)
    {
        log_debug ("invoked with non-string value type (%d)", value->dv_type);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    if (size)
    {
        *size = value->dv_size;
    }
    *string = value->dv_string;
    return DISIR_STATUS_OK;
}

