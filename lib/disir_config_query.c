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
#include "restriction.h"

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

static enum disir_status
config_ensure_keyval_leaf (struct disir_context *immediate_parent, const char *keyval_name,
                           int keyval_index, enum disir_value_type value_type,
                           const char *value_string, uint8_t value_boolean, int64_t value_integer,
                           double value_float)
{
    enum disir_status status;
    struct disir_context *parent_mold_equiv = NULL;
    struct disir_context *element_mold_equiv = NULL;

    // Get the mold equivalen of our keyval, to check if it exists and that we can create it.
    dx_get_mold_equiv (immediate_parent, &parent_mold_equiv);
    status = dc_find_element (parent_mold_equiv, keyval_name, 0, &element_mold_equiv);
    // NOTE: parent_mold_equiv is not incref'ed
    if (status != DISIR_STATUS_OK)
    {
        // FIXME: We should have the path to the immediate_parent here as well.
        dx_context_error_set (immediate_parent, "keyval %s does not exist", keyval_name);
        log_debug (2, "unable to find keyval mold: %s", disir_status_string (status));
        return DISIR_STATUS_MOLD_MISSING;
    }

    int max_elements = 0;
    enum disir_context_type element_type = dc_context_type (element_mold_equiv);

    // Check if our keyval_index is out of bounds of the maximum number of instances.
    status = dx_restriction_entries_value (element_mold_equiv,
                                           DISIR_RESTRICTION_INC_ENTRY_MAX,
                                           NULL, &max_elements);
    dc_putcontext (&element_mold_equiv);
    if (status != DISIR_STATUS_OK)
    {
        log_warn ("TODO: HANDLE ERROR MESSAGE: restriction_entries_value");
        return status;
    }

    log_debug (2, "ensuring leaf keyval %s, max: %d, index: %d",
               keyval_name, max_elements, keyval_index);
    // Ensure that the index we are creating is within bounds
    // 1: index does not exceed max
    if (max_elements != 0 && max_elements <= keyval_index)
    {
        dx_context_error_set (immediate_parent,
                              "accessing index %s@%d exceeds maximum allowed instances of %d",
                              keyval_name, keyval_index, max_elements);
        log_debug (2, "%s", dc_context_error (immediate_parent));
        return DISIR_STATUS_RESTRICTION_VIOLATED;
    }

    int num_elements = 0;
    struct disir_collection *collection;
    status = dc_find_elements (immediate_parent, keyval_name, &collection);
    if (status == DISIR_STATUS_OK)
    {
        num_elements = dc_collection_size (collection);
        dc_collection_finished (&collection);
    }
    if (keyval_index != num_elements)
    {
        dx_context_error_set (immediate_parent,
                              "accessing non-existent index %s@%d, expected index %d",
                              keyval_name, keyval_index, num_elements);
        log_debug (2, "%s", dc_context_error (immediate_parent));
        return DISIR_STATUS_CONFLICT;
    }

    // Check that the element type is a section
    if (element_type != DISIR_CONTEXT_KEYVAL)
    {
        dx_context_error_set (immediate_parent,
                              "expected %s to be a keyval, is section", keyval_name);
        return DISIR_STATUS_CONFLICT;
    }

    // Hurray, we can create this leaf keyval.
    struct disir_context *context = NULL;

    status = dc_begin (immediate_parent, DISIR_CONTEXT_KEYVAL, &context);
    if (status != DISIR_STATUS_OK)
    {
        return status;
    }
    status = dc_set_name (context, keyval_name, strlen (keyval_name));
    if (status != DISIR_STATUS_OK)
    {
        log_warn ("set_name failed, even though it shouldnt have: %s",
                  disir_status_string (status));
        return status;
    }

    // Check that the keyval type is what we expect
    if (dc_value_type (context) != value_type)
    {
        dx_context_error_set (immediate_parent, "expected type %s, found %s",
                              dx_value_type_string (value_type),
                              dc_value_type_string (context));
        dc_destroy (&context);
        return DISIR_STATUS_WRONG_VALUE_TYPE;
    }

    status = set_value_generic (context, value_type, value_string, value_boolean,
                                value_integer, value_float);
    if (status != DISIR_STATUS_OK)
    {
        dx_context_transfer_logwarn (immediate_parent, context);
        dc_destroy (&context);
        return status;
    }

    status = dc_finalize (&context);
    if (status != DISIR_STATUS_OK)
    {
        dx_context_transfer_logwarn (immediate_parent, context);
        dc_destroy (&context);
        return status;
    }

    return DISIR_STATUS_OK;
}

//! STATIC API
static enum disir_status
config_set_keyval_generic (struct disir_context *parent, enum disir_value_type type,
                           const char *query, va_list args,
                           const char *value_string, uint8_t value_boolean, int64_t value_integer,
                           double value_float)
{
    enum disir_status status;
    struct disir_context *context = NULL;
    char keyval_name[2048];
    int keyval_index = 0;
    va_list args_copy;

    keyval_name[0] = '\0';

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

    // We need to copy the args before we use them
    va_copy (args_copy, args);

    status = dc_query_resolve_context_va (parent, query, &context, args);
    if (status == DISIR_STATUS_OK)
    {
        log_debug (2, "set_keyval query resolved to existing keyval");
        status = set_value_generic (context, type, value_string, value_boolean,
                                    value_integer, value_float);
        if (status != DISIR_STATUS_OK)
        {
            dx_context_transfer_logwarn (parent, context);
        }

        dc_putcontext (&context);
    }
    else if (status == DISIR_STATUS_NOT_EXIST)
    {
        struct disir_context *ancestor = NULL;
        struct disir_context *immediate_parent = NULL;

        log_debug (2, "set_keyval_query didn't resolve - ensuring ancestors exist.");
        status = dx_query_ensure_ancestors (parent, query, args_copy, &ancestor, &immediate_parent,
                                            keyval_name, &keyval_index);
        if (status != DISIR_STATUS_OK)
        {
            // Status already set
            goto out;
        }

        // Check if we've been able to create it during ensure_ancestors
        status = dc_find_element(immediate_parent, keyval_name, keyval_index, &context);
        if (status == DISIR_STATUS_OK)
        {
            log_debug (2, "Our keyval was created during ensure_ancestors");
            // Lucily, we've can just set the value!
            // If this fails, we will destroy the ancestor
            status = set_value_generic (context, type, value_string, value_boolean,
                                        value_integer, value_float);
            // If this fails, transfer the
            if (status != DISIR_STATUS_OK)
            {
                dx_context_transfer_logwarn (parent, context);
            }
            dc_putcontext (&context);
        }
        else
        {
            log_debug (2, "Our keyval doesnt exist (%s)- validate query and create it",
                          disir_status_string (status));
            // We need to find the mold equivalent of our immediate_parent,
            // and ensure that the keyval_name@keyval_index is the only one we can create
            status = config_ensure_keyval_leaf (immediate_parent, keyval_name,
                                                keyval_index, type,
                                                value_string, value_boolean,
                                                value_integer, value_float);

            if (status != DISIR_STATUS_OK)
            {
                dx_context_transfer_logwarn (parent, immediate_parent);
            }
        }

        dc_putcontext (&immediate_parent);

        // If we've created an ancestor, we need to finalize/destroy it
        if (ancestor && status == DISIR_STATUS_OK)
        {
            status = dc_finalize (&ancestor);
            if (status != DISIR_STATUS_OK)
            {
                dx_context_transfer_logwarn (parent, ancestor);
                dc_destroy (&ancestor);
            }
        }
        else if (ancestor)
        {
            // We've failed!
            dc_destroy (&ancestor);
        }
    }

out:
    // Must ensure that va_copy is matched with a va_end
    va_end (args_copy);

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

//! PUBLIC API: low-level
enum disir_status
dc_config_get_keyval_enum (struct disir_context *context, const char **value,
                           const char *query, ...)
{
    enum disir_status status;
    va_list args;

    TRACE_ENTER ("");

    va_start (args, query);
    status = config_get_keyval_generic (context, DISIR_VALUE_TYPE_ENUM, query, args,
                                        value, NULL, NULL, NULL);
    va_end (args);

    TRACE_EXIT ("%s", disir_status_string (status));
    return status;
}

//! PUBLIC API: low-level
enum disir_status
dc_config_set_keyval_enum (struct disir_context *context, const char *value,
                           const char *query, ...)
{
    enum disir_status status;
    va_list args;

    TRACE_ENTER ("");

    va_start (args, query);
    status = config_set_keyval_generic (context, DISIR_VALUE_TYPE_ENUM, query, args,
                                        value, 0, 0, 0);
    va_end (args);

    TRACE_EXIT ("%s", disir_status_string (status));
    return status;
}

//! PUBLIC API: low-level
enum disir_status
dc_config_get_keyval_float (struct disir_context *context, double *value,
                            const char *query, ...)
{
    enum disir_status status;
    va_list args;

    TRACE_ENTER ("");

    va_start (args, query);
    status = config_get_keyval_generic (context, DISIR_VALUE_TYPE_FLOAT, query, args,
                                        NULL, NULL, NULL, value);
    va_end (args);

    TRACE_EXIT ("%s", disir_status_string (status));
    return status;
}


//! PUBLIC API: low-level
enum disir_status
dc_config_set_keyval_float (struct disir_context *context, double value,
                            const char *query, ...)
{
    enum disir_status status;
    va_list args;

    TRACE_ENTER ("");

    va_start (args, query);
    status = config_set_keyval_generic (context, DISIR_VALUE_TYPE_FLOAT, query, args,
                                        NULL, 0, 0, value);
    va_end (args);

    TRACE_EXIT ("%s", disir_status_string (status));
    return status;
}


//! PUBLIC API: low-level
enum disir_status
dc_config_get_keyval_boolean (struct disir_context *context, uint8_t *value,
                              const char *query, ...)
{
    enum disir_status status;
    va_list args;

    TRACE_ENTER ("");

    va_start (args, query);
    status = config_get_keyval_generic (context, DISIR_VALUE_TYPE_BOOLEAN, query, args,
                                        NULL, value, NULL, NULL);
    va_end (args);

    TRACE_EXIT ("%s", disir_status_string (status));
    return status;
}

//! PUBLIC API: low-level
enum disir_status
dc_config_set_keyval_boolean (struct disir_context *context, uint8_t value,
                              const char *query, ...)
{
    enum disir_status status;
    va_list args;

    TRACE_ENTER ("");

    va_start (args, query);
    status = config_set_keyval_generic (context, DISIR_VALUE_TYPE_BOOLEAN, query, args,
                                        NULL, value, 0, 0);
    va_end (args);

    TRACE_EXIT ("%s", disir_status_string (status));
    return status;
}

//! PUBLIC API: low-level
enum disir_status
dc_config_get_keyval_integer (struct disir_context *context, int64_t *value,
                              const char *query, ...)
{
    enum disir_status status;
    va_list args;

    TRACE_ENTER ("");

    va_start (args, query);
    status = config_get_keyval_generic (context, DISIR_VALUE_TYPE_INTEGER, query, args,
                                        NULL, NULL, value, NULL);
    va_end (args);

    TRACE_EXIT ("%s", disir_status_string (status));
    return status;
}

//! PUBLIC API: low-level
enum disir_status
dc_config_set_keyval_integer (struct disir_context *context, int64_t value,
                              const char *query, ...)
{
    enum disir_status status;
    va_list args;

    TRACE_ENTER ("");

    va_start (args, query);
    status = config_set_keyval_generic (context, DISIR_VALUE_TYPE_INTEGER, query, args,
                                        NULL, 0, value, 0);
    va_end (args);

    TRACE_EXIT ("%s", disir_status_string (status));
    return status;
}
