#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <disir/disir.h>
#include <disir/context.h>
#include <disir/config.h>

#include "context_private.h"
#include "query_private.h"
#include "log.h"
#include "value.h"

//! Nested naming scheme
//! e.g. "section@2.keyval@3, which accesses the section element 'section', whose element
//! third element 'keyval'
//!
//! Normal procedure would simply be "section.section.keyval"
//!
//! Setters with invalid range (e.g., "keyval@3", where there only exists 1 entry already,
//! meaning the logical entry to set is either 1 (already exists, override) or 2 (add entry)




//! STATIC API
static enum disir_status
set_value_generic (struct disir_context *context, enum disir_value_type type,
                   const char *value_string, uint8_t value_boolean, int64_t value_integer,
                   double value_float)
{
    enum disir_status status;

    switch (type)
    {
    case DISIR_VALUE_TYPE_ENUM:
        status = dc_set_value_enum (context, value_string, strlen (value_string));
        break;
    case DISIR_VALUE_TYPE_STRING:
        status = dc_set_value_string (context, value_string, strlen (value_string));
        break;
    case DISIR_VALUE_TYPE_BOOLEAN:
        status = dc_set_value_boolean (context, value_boolean);
        break;
    case DISIR_VALUE_TYPE_INTEGER:
        status = dc_set_value_integer (context, value_integer);
        break;
    case DISIR_VALUE_TYPE_FLOAT:
        status = dc_set_value_float (context, value_float);
        break;
    case DISIR_VALUE_TYPE_UNKNOWN:
        status = DISIR_STATUS_INTERNAL_ERROR;
    }

    return status;
}

//! STATIC API
static enum disir_status
get_value_generic (struct disir_context *context, enum disir_value_type type,
                   const char **value_string, uint8_t *value_boolean, int64_t *value_integer,
                   double *value_float)
{
    enum disir_status status;

    switch (type)
    {
    case DISIR_VALUE_TYPE_ENUM:
        status = dc_get_value_enum (context, value_string, NULL);
        break;
    case DISIR_VALUE_TYPE_STRING:
        status = dc_get_value_string (context, value_string, NULL);
        break;
    case DISIR_VALUE_TYPE_BOOLEAN:
        status = dc_get_value_boolean (context, value_boolean);
        break;
    case DISIR_VALUE_TYPE_INTEGER:
        status = dc_get_value_integer (context, value_integer);
        break;
    case DISIR_VALUE_TYPE_FLOAT:
        status = dc_get_value_float (context, value_float);
        break;
    case DISIR_VALUE_TYPE_UNKNOWN:
        status = DISIR_STATUS_INTERNAL_ERROR;
    }

    return status;
}

//! STATIC API
static enum disir_status
config_get_keyval_generic (struct disir_context *parent, enum disir_value_type type,
                           const char *query, va_list args,
                           const char **value_string, uint8_t *value_boolean,
                           int64_t *value_integer, double *value_float)
{
    enum disir_status status;
    struct disir_context *context;

    status = DISIR_STATUS_OK;
    context = NULL;

    if (parent == NULL || query == NULL)
    {
        log_debug (0, "invoked with NULL pointer(s) (parent (%p), query (%p)",
                      parent, query);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }
    switch (type)
    {
    case DISIR_VALUE_TYPE_ENUM:
        // FALL-THROUGH
    case DISIR_VALUE_TYPE_STRING:
        if (value_string == NULL)
            status = DISIR_STATUS_INVALID_ARGUMENT;
        break;
    case DISIR_VALUE_TYPE_BOOLEAN:
        if (value_boolean == NULL)
            status = DISIR_STATUS_INVALID_ARGUMENT;
        break;
    case DISIR_VALUE_TYPE_INTEGER:
        if (value_integer == NULL)
            status = DISIR_STATUS_INVALID_ARGUMENT;
        break;
    case DISIR_VALUE_TYPE_FLOAT:
        if (value_float == NULL)
            status = DISIR_STATUS_INVALID_ARGUMENT;
        break;
    case DISIR_VALUE_TYPE_UNKNOWN:
        status = DISIR_STATUS_INTERNAL_ERROR;
    }
    if (status != DISIR_STATUS_OK)
    {
        log_debug (0, "%s invoked with NULL value pointer (string/enum (%p), boolean (%p),"
                      " integer (%p), float (%p))", dx_value_type_string (type),
                      value_string, value_boolean, value_integer, value_float);
        return status;
    }

    status = dc_query_resolve_context_va (parent, query, &context, args);
    if (status == DISIR_STATUS_OK)
    {
        status = get_value_generic (context, type, value_string,
                                    value_boolean, value_integer, value_float);
        dc_putcontext (&context);
    }

    return status;
}

//! STATIC API
static enum disir_status
config_set_keyval_generic (struct disir_context *parent, enum disir_value_type type,
                           const char *query, va_list args,
                           const char *value_string, uint8_t value_boolean, int64_t value_integer,
                           double value_float)
{
    enum disir_status status;
    struct disir_context *context;
    struct disir_context *section;
    char keyval_name[2048];

    if (parent == NULL || query == NULL)
    {
        log_debug (0, "invoked with NULL pointer(s) (parent (%p), query (%p)",
                      parent, value_string, query);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }
    if (value_string == NULL &&
         (type == DISIR_VALUE_TYPE_STRING || type == DISIR_VALUE_TYPE_ENUM))
    {
        log_debug (0, "value_string invoked with NULL pointer");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    context = NULL;
    section = NULL;

    status = dc_query_resolve_context_va (parent, query, &context, args);
    if (status == DISIR_STATUS_OK)
    {
        status = set_value_generic (context, type, value_string, value_boolean,
                                    value_integer, value_float);
    }
    else if (status == DISIR_STATUS_NOT_EXIST)
    {
        // Lets find the context of the section that should contain the leaf keyval
        status = dx_query_resolve_parent_context (parent, &section, keyval_name,
                                                  query, args);
        if (status != DISIR_STATUS_OK)
        {
            // The section containing the leaf keyval does not exist. Bail
            return status;
        }

        // Create the leaf keyval
        status = dc_begin (section, DISIR_CONTEXT_KEYVAL, &context);
        if (status != DISIR_STATUS_OK)
        {
            goto error;
        }
        status = dc_set_name (context, keyval_name, strlen (keyval_name));
        if (status != DISIR_STATUS_OK)
        {
            // Signal to the caller that the requested key was infact, invalid.
            if (status == DISIR_STATUS_NOT_EXIST)
            {
                status = DISIR_STATUS_MOLD_MISSING;
            }
            goto error;
        }

        // Check that the keyval type is actually string
        if (dc_value_type (context) != type)
        {
            dx_context_error_set (context, "expected type %s", dc_value_type_string(context));
            status = DISIR_STATUS_WRONG_VALUE_TYPE;
            goto error;
        }

        status = set_value_generic (context, type, value_string, value_boolean,
                                    value_integer, value_float);
        if (status != DISIR_STATUS_OK)
        {
            // TODO: Set error?
            goto error;
        }

        status = dc_finalize (&context);
        if (status != DISIR_STATUS_OK)
        {
            // Abort the creation
            goto error;
        }
    }

    // FALL-THROUGH
error:
    if (status != DISIR_STATUS_OK)
    {
        dx_context_transfer_logwarn (parent, context);
    }
    else
    {
        // We do not DESTROY the keyval if it went OK, only put it away
        dc_putcontext (&context);
    }
    if (section)
    {
        dc_putcontext (&section);
    }
    if (context)
    {
        // Since the context was not put away, we are in an error condition and want to destroy it
        dc_destroy (&context);
    }

    return status;
}

//! PUBLIC API: high-level
enum disir_status
disir_config_get_keyval_boolean (struct disir_config *config, uint8_t *value,
                                 const char *query, ...)
{
    enum disir_status status;
    struct disir_context *context;
    va_list args;

    TRACE_ENTER ("");

    if (config == NULL)
    {
        status = DISIR_STATUS_INVALID_ARGUMENT;
        goto error;
    }

    context = dc_config_getcontext (config);
    if (context != NULL)
    {
        va_start (args, query);
        status = config_get_keyval_generic (context, DISIR_VALUE_TYPE_BOOLEAN, query, args,
                                            NULL, value, NULL, NULL);
        va_end (args);
        dc_putcontext (&context);
    }

    // FALL-THROUGH
error:
    TRACE_EXIT ("%s", disir_status_string (status));
    return status;
}

//! PUBLIC API: high-level
enum disir_status
disir_config_set_keyval_boolean (struct disir_config *config, uint8_t value,
                                 const char *query, ...)
{
    enum disir_status status;
    struct disir_context *context;
    va_list args;

    TRACE_ENTER ("");

    if (config == NULL)
    {
        status = DISIR_STATUS_INVALID_ARGUMENT;
        goto error;
    }

    context = dc_config_getcontext (config);
    if (context != NULL)
    {
        va_start (args, query);
        status = config_set_keyval_generic (context, DISIR_VALUE_TYPE_BOOLEAN, query, args,
                                            NULL, value, 0, 0);
        va_end (args);
        dc_putcontext (&context);
    }

    // FALL-THROUGH
error:
    TRACE_EXIT ("%s", disir_status_string (status));
    return status;
}

//! PUBLIC API: high-level
enum disir_status
disir_config_get_keyval_integer (struct disir_config *config, int64_t *value,
                                 const char *query, ...)
{
    enum disir_status status;
    struct disir_context *context;
    va_list args;

    TRACE_ENTER ("");

    if (config == NULL)
    {
        status = DISIR_STATUS_INVALID_ARGUMENT;
        goto error;
    }

    context = dc_config_getcontext (config);
    if (context != NULL)
    {
        va_start (args, query);
        status = config_get_keyval_generic (context, DISIR_VALUE_TYPE_INTEGER, query, args,
                                            NULL, NULL, value, NULL);
        va_end (args);
        dc_putcontext (&context);
    }

    // FALL-THROUGH
error:
    TRACE_EXIT ("%s", disir_status_string (status));
    return status;
}

//! PUBLIC API: high-level
enum disir_status
disir_config_set_keyval_integer (struct disir_config *config, int64_t value,
                                 const char *query, ...)
{
    enum disir_status status;
    struct disir_context *context;
    va_list args;

    TRACE_ENTER ("");

    if (config == NULL)
    {
        status = DISIR_STATUS_INVALID_ARGUMENT;
        goto error;
    }

    context = dc_config_getcontext (config);
    if (context != NULL)
    {
        va_start (args, query);
        status = config_set_keyval_generic (context, DISIR_VALUE_TYPE_INTEGER, query, args,
                                            NULL, 0, value, 0);
        va_end (args);
        dc_putcontext (&context);
    }

    // FALL-THROUGH
error:
    TRACE_EXIT ("%s", disir_status_string (status));
    return status;
}

//! PUBLIC API: high-level
enum disir_status
disir_config_get_keyval_float (struct disir_config *config, double *value,
                               const char *query, ...)
{
    enum disir_status status;
    struct disir_context *context;
    va_list args;

    TRACE_ENTER ("");

    if (config == NULL)
    {
        status = DISIR_STATUS_INVALID_ARGUMENT;
        goto error;
    }

    context = dc_config_getcontext (config);
    if (context != NULL)
    {
        va_start (args, query);
        status = config_get_keyval_generic (context, DISIR_VALUE_TYPE_FLOAT, query, args,
                                            NULL, NULL, NULL, value);
        va_end (args);
        dc_putcontext (&context);
    }

    // FALL-THROUGH
error:
    TRACE_EXIT ("%s", disir_status_string (status));
    return status;
}


//! PUBLIC API: high-level
enum disir_status
disir_config_set_keyval_float (struct disir_config *config, double value,
                               const char *query, ...)
{
    enum disir_status status;
    struct disir_context *context;
    va_list args;

    TRACE_ENTER ("");

    if (config == NULL)
    {
        status = DISIR_STATUS_INVALID_ARGUMENT;
        goto error;
    }

    context = dc_config_getcontext (config);
    if (context != NULL)
    {
        va_start (args, query);
        status = config_set_keyval_generic (context, DISIR_VALUE_TYPE_FLOAT, query, args,
                                            NULL, 0, 0, value);
        va_end (args);
        dc_putcontext (&context);
    }

    // FALL-THROUGH
error:
    TRACE_EXIT ("%s", disir_status_string (status));
    return status;
}

//! PUBLIC API: high-level
enum disir_status
disir_config_get_keyval_string (struct disir_config *config, const char **value,
                                const char *query, ...)
{
    enum disir_status status;
    struct disir_context *context;
    va_list args;

    TRACE_ENTER ("");

    if (config == NULL)
    {
        status = DISIR_STATUS_INVALID_ARGUMENT;
        goto error;
    }

    context = dc_config_getcontext (config);
    if (context != NULL)
    {
        va_start (args, query);
        status = config_get_keyval_generic (context, DISIR_VALUE_TYPE_STRING, query, args,
                                            value, NULL, NULL, NULL);
        va_end (args);
        dc_putcontext (&context);
    }

    // FALL-THROUGH
error:
    TRACE_EXIT ("%s", disir_status_string (status));
    return status;
}

//! PUBLIC API: high-level
enum disir_status
disir_config_set_keyval_string (struct disir_config *config, const char *value,
                                const char *query, ...)
{
    enum disir_status status;
    struct disir_context *context;
    va_list args;

    TRACE_ENTER ("");

    if (config == NULL)
    {
        status = DISIR_STATUS_INVALID_ARGUMENT;
        goto error;
    }

    context = dc_config_getcontext (config);
    if (context != NULL)
    {
        va_start (args, query);
        status = config_set_keyval_generic (context, DISIR_VALUE_TYPE_STRING, query, args,
                                            value, 0, 0, 0);
        va_end (args);
        dc_putcontext (&context);
    }

    // FALL-THROUGH
error:
    TRACE_EXIT ("%s", disir_status_string (status));
    return status;
}

//! PUBLIC API: high-level
enum disir_status
disir_config_get_keyval_enum (struct disir_config *config, const char **value,
                              const char *query, ...)
{
    enum disir_status status;
    struct disir_context *context;
    va_list args;

    TRACE_ENTER ("");

    if (config == NULL)
    {
        status = DISIR_STATUS_INVALID_ARGUMENT;
        goto error;
    }

    context = dc_config_getcontext (config);
    if (context != NULL)
    {
        va_start (args, query);
        status = config_get_keyval_generic (context, DISIR_VALUE_TYPE_ENUM, query, args,
                                            value, NULL, NULL, NULL);
        va_end (args);
        dc_putcontext (&context);
    }

    // FALL-THROUGH
error:
    TRACE_EXIT ("%s", disir_status_string (status));
    return status;
}

//! PUBLIC API: high-level
enum disir_status
disir_config_set_keyval_enum (struct disir_config *config, const char *value,
                              const char *query, ...)
{
    enum disir_status status;
    struct disir_context *context;
    va_list args;

    TRACE_ENTER ("");

    if (config == NULL)
    {
        status = DISIR_STATUS_INVALID_ARGUMENT;
        goto error;
    }

    context = dc_config_getcontext (config);
    if (context != NULL)
    {
        va_start (args, query);
        status = config_set_keyval_generic (context, DISIR_VALUE_TYPE_ENUM, query, args,
                                            value, 0, 0, 0);
        va_end (args);
        dc_putcontext (&context);
    }

    // FALL-THROUGH
error:
    TRACE_EXIT ("%s", disir_status_string (status));
    return status;
}

//! PUBLIC API: low-level
enum disir_status
dc_config_get_keyval_string (struct disir_context *parent, const char **value,
                             const char *query, ...)
{
    enum disir_status status;
    va_list args;

    TRACE_ENTER ("");

    va_start (args, query);
    status = config_get_keyval_generic (parent, DISIR_VALUE_TYPE_STRING, query, args,
                                        value, NULL, NULL, NULL);
    va_end (args);

    TRACE_EXIT ("%s", disir_status_string (status));
    return status;
}

//! PUBLIC API: low-level
enum disir_status
dc_config_set_keyval_string (struct disir_context *parent, const char *value,
                             const char *query, ...)
{
    enum disir_status status;
    va_list args;

    TRACE_ENTER ("");

    va_start (args, query);
    status = config_set_keyval_generic (parent, DISIR_VALUE_TYPE_STRING, query, args,
                                        value, 0, 0, 0);
    va_end (args);

    TRACE_EXIT ("%s", disir_status_string (status));
    return status;
}
