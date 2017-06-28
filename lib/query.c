#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <disir/disir.h>

#include "context_private.h"
#include "query_private.h"
#include "log.h"


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

//! INTERNAL API
//! This function assumes that the name query did not resolve to a valid context already.
//! It will locate the parent section in the query, and find out if the leaf portion of the
//! query is available for creation.
//!
enum disir_status
dx_query_resolve_parent_context (struct disir_context *parent, struct disir_context **out,
                                 char *keyval_name, const char *query, va_list args)
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

//! PUBLIC API
enum disir_status
dc_query_resolve_context (struct disir_context *parent, const char *name,
                          struct disir_context **out, ...)
{
    enum disir_status status;
    va_list args;

    va_start (args, out);
    status = dc_query_resolve_context_va (parent, name, out, args);
    va_end (args);

    return status;
}

//! PUBLIC API
enum disir_status
dc_query_resolve_context_va (struct disir_context *parent, const char *name,
                             struct disir_context **out, va_list args)
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

    status = CONTEXT_TYPE_CHECK (parent, DISIR_CONTEXT_MOLD,
                                         DISIR_CONTEXT_CONFIG,
                                         DISIR_CONTEXT_SECTION);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
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


