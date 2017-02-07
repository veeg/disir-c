#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <disir/disir.h>
#include <disir/context.h>
#include <disir/config.h>

#include "context_private.h"
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


//! INTERNAL API
//!
//! The allocated buffer for the resolved parameter is the same as the original name parameter.
//! Thus, all modification operations will have adequate space to since we are only adding
//! the exact same data as from the name parameter.
//!
enum disir_status
dx_query_resolve_name (struct disir_context *parent, char *name, char *resolved,
                       char **next, int *index)
{
    char *name_below;
    char *name_above;
    char *name_index;
    char *key_seperator;
    char *index_indicator;
    char *endptr;

    *index = 0;
    name_below = NULL;
    name_above = name;

    if (parent == NULL)
    {
        log_fatal ("parent NULL pointer passed to internal function, that should never be called with invalid context...");
        return DISIR_STATUS_INTERNAL_ERROR;
    }

    key_seperator = strchr (name_above, '.');
    if (key_seperator != NULL)
    {
        *key_seperator = '\0';
        name_below = key_seperator + 1;
    }
    // Error check that we dont start off with a seperator
    if (name_above == key_seperator)
    {
        strcat (resolved, ".");
        dx_log_context (parent, "'%s' missing key before key seperator.", resolved);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    // We know that if there is any match, it will match within this key entry, and not the next,
    // since we replaced the key seperator with a NULL.
    index_indicator = strchr (name_above, '@');
    if (index_indicator != NULL)
    {
        *index_indicator = '\0';
        name_index = index_indicator + 1;
    }
    // Error check that we dont start off with a index indicator
    if (name_above == index_indicator)
    {
        strcat (resolved, "@");
        dx_log_context (parent, "'%s' missing key before index indicator.", resolved);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    // Add the active name to the resolved list
    strcat (resolved, name_above);

    if (index_indicator != NULL)
    {
        // Add the index indicator
        strcat (resolved, "@");

        // Make sure that since the indicator was present, we also need something in this one..
        if (*name_index == '\0')
        {
            dx_log_context (parent, "'%s' missing index after indicator.", resolved);
            return DISIR_STATUS_INVALID_ARGUMENT;
        }

        // Add everything between name_index and key_seperator
        strcat (resolved, name_index);

        // Find the index
        *index = strtol (name_index, &endptr, 10);
        if (name_index == endptr)
        {
            dx_log_context (parent, "'%s' is an invalid index indicator.", resolved);
            return DISIR_STATUS_INVALID_ARGUMENT;
        }

        if (*endptr != '\0')
        {
            dx_log_context (parent, "'%s' contains additional trailing non-integer characters.",
                            resolved);
            return DISIR_STATUS_INVALID_ARGUMENT;
        }
    }

    // Parsing went a-okay. Populate output parameters
    *next = name_below;

    return DISIR_STATUS_OK;
}

//! STATIC API
static enum disir_status
query_resolve_name_to_context (struct disir_context *parent, char *name,
                         char *resolved, struct disir_context **out)
{

    enum disir_status status;
    struct disir_context *context_found;
    char *next_name;
    int index;

    status = dx_query_resolve_name (parent, name, resolved, &next_name, &index);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }

    // Attempt to find the element at the requested index
    status = dc_find_element (parent, name, index, &context_found);
    if (status == DISIR_STATUS_NOT_EXIST)
    {
        // TODO: Print out entire resolved name until this point where it failed.
        dx_log_context (parent, "Unable to locate entry");
        return status;
    }
    else if (status == DISIR_STATUS_OK)
    {
        // Must we recursve?
        if (next_name)
        {
            log_debug (0, "we must recurse: %p", context_found);
            status = query_resolve_name_to_context (context_found, next_name, resolved, out);
            dx_context_transfer_logwarn (parent, context_found);
            dc_putcontext (&context_found);
            return status;
        }

        // We found the last one! Success!
        *out = context_found;
        return status;
    }

    // Some fatal error has ocuredk
    dx_log_context (parent, "unknown error occurred in name resolution.");

    return status;
}

//! STATIC API
//! Caller is required to dc_putcontext on the output context
static enum disir_status
query_get_context_va (struct disir_context *parent, struct disir_context **out,
                      const char *name, va_list args)
{
    enum disir_status status;
    char buffer[2048];
    char resolved[2048];

    resolved[0] = '\0';

    // Check input arguments
    status = CONTEXT_NULL_INVALID_TYPE_CHECK (parent);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }

    status = CONTEXT_TYPE_CHECK (parent, DISIR_CONTEXT_CONFIG, DISIR_CONTEXT_SECTION);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }

    // Limit operation to contexts derived from a config
    if (dc_context_type (parent->cx_root_context) != DISIR_CONTEXT_CONFIG)
    {
        dx_log_context (parent, "Operation must be performed within a config.");
        return DISIR_STATUS_WRONG_CONTEXT;
    }

    // Write out the entire name
    if (vsnprintf (buffer, 2048, name, args) >= 2048)
    {
        // TODO: Refine error code. Library just has an assumption on size. Rework maybe..
        dx_log_context (parent, "Insufficient buffer. Name request exceeded 2048 bytes.");
        return DISIR_STATUS_INSUFFICIENT_RESOURCES;
    }

    // Resolve the context name recursively
    status = query_resolve_name_to_context (parent, buffer, resolved, out);
    if (status != DISIR_STATUS_OK)
    {
        // XXX: Do anything?
        return status;
    }


    return DISIR_STATUS_OK;
}

//! STATIC API
//! This function assumes that the name query did not resolve to a valid context already.
//! It will locate the parent section in the query, and find out if the leaf portion of the
//! query is available for creation.
//!
//! example:
//!     "first@2.second@4.third@2"
//!
//!     If "first@2.second@4" exists, we are assumaing that the leaf context "third@2" does not
//!     exist. But for us to be able to create this leaf context, "third@1" must exist.
static enum disir_status
resolve_parent_context_of_setable_query_va (struct disir_context *parent,
                                            struct disir_context **out,
                                            char *keyval_name,
                                            const char *query, va_list args)
{
    enum disir_status status;
    char buffer[2048];
    char resolved[2048];
    char *last = NULL;
    struct disir_context *section;
    va_list args_copy;
    char *next;
    int index;
    struct disir_collection *collection;
    char *keyval_entry;

    section = NULL;
    collection = NULL;

    va_copy (args_copy, args);
    vsnprintf (buffer, 2048, query, args_copy);
    va_end (args_copy);

    // Find the last key seperator.
    last = strrchr (buffer, '.');
    if (last == NULL)
    {
        section = parent;
        keyval_entry = buffer;

        // Incref the section here since the resolving in the else clause
        // will incref the section.
        dx_context_incref (section);
    }
    else
    {
        *last = '\0';
        keyval_entry = last + 1;

        // The query was nested; e.g., "first@4.second@2.third"
        // Here we will resolve "first@4.second@2" section. If this does not exist,
        // the query is invalid.

        resolved[0] = '\0';
        status = query_resolve_name_to_context (parent, buffer, resolved, &section);
        if (status != DISIR_STATUS_OK)
        {
            goto error;
        }
    }

    if (section == NULL)
    {
        status = DISIR_STATUS_NOT_EXIST;
        goto error;
    }

    // We have a section - now we need to seperate name and index
    resolved[0] = '\0';
    status = dx_query_resolve_name (section, keyval_entry, resolved, &next, &index);
    if (status == DISIR_STATUS_OK)
    {
        // Query section for the collection of entries matching keyval name.
        // We let the NOT_EXIST go through, so that the collection size == 0 is valid
        status = dc_find_elements (section, keyval_entry, &collection);
        if (status != DISIR_STATUS_OK && status != DISIR_STATUS_NOT_EXIST)
        {
            goto error;
        }

        // If the size equals the index (that is the index is refering to size + 1 th entry
        // then this is a valid query
        if (dc_collection_size (collection) != index)
        {
            status = DISIR_STATUS_NOT_EXIST;
            goto error;
        }
    }

    // Copy the keyval_entry to the output keyval_name buffer
    strcpy (keyval_name, keyval_entry);

    *out = section;
    section = NULL;
    status = DISIR_STATUS_OK;
    // FALL-THROUGH
error:
    if (section)
    {
        dc_putcontext (&section);
    }
    if (collection)
    {
        dc_collection_finished (&collection);
    }

    return status;
}

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
        // FALL-THROUGH
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
        // FALL-THROUGH
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

    status = query_get_context_va (parent, &context, query, args);
    if (status == DISIR_STATUS_OK)
    {
        status = get_value_generic (context, DISIR_VALUE_TYPE_STRING, value_string,
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

    if (parent == NULL || value_string == NULL || query == NULL)
    {
        log_debug (0, "invoked with NULL pointer(s) (parent (%p), value_string (%p), query (%p)",
                      parent, value_string, query);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    context = NULL;
    section = NULL;

    status = query_get_context_va (parent, &context, query, args);
    if (status == DISIR_STATUS_OK)
    {
        status = set_value_generic (context, type, value_string, value_boolean,
                                    value_integer, value_float);
        dc_putcontext (&context);
    }
    else if (status == DISIR_STATUS_NOT_EXIST)
    {
        // Lets find the context of the section that should contain the leaf keyval
        status = resolve_parent_context_of_setable_query_va (parent, &section, keyval_name,
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
            // TODO: Set error
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
            dc_destroy (&context);
            goto error;
        }
    }

    status = DISIR_STATUS_OK;;
    // FALL-THROUGH
error:
    if (section)
    {
        dc_putcontext (&section);
    }
    if (context)
    {
        dc_destroy (&context);
    }

    return status;
}

enum disir_status
disir_config_get_keyval_integer (struct disir_config *config, int64_t *value,
                                 const char *name, ...)
{
    return DISIR_STATUS_INTERNAL_ERROR;
}

enum disir_status
disir_config_set_keyval_integer (struct disir_config *config, int64_t value,
                                 const char *name, ...)
{
    return DISIR_STATUS_INTERNAL_ERROR;
}

enum disir_status
disir_config_get_keyval_float (struct disir_config *config, double value,
                               const char *name, ...)
{
    return DISIR_STATUS_INTERNAL_ERROR;
}


enum disir_status
disir_config_set_keyval_float (struct disir_config *config, double value,
                               const char *name, ...)
{
    return DISIR_STATUS_INTERNAL_ERROR;
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

