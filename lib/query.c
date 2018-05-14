#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <disir/disir.h>

#include "context_private.h"
#include "query_private.h"
#include "restriction.h"
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

    // If we're not the first invocation to process, we need to add
    // the key separator between invocations to resolved buffer.
    if (resolved[0] != '\0')
    {
        strcat (resolved, ".");
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

//! INTERNAL API
enum disir_status
dx_query_ensure_ancestors (struct disir_context *config,
                           const char *query, va_list args,
                           struct disir_context **ancestor,
                           struct disir_context **parent,
                           char *element_child_name, int *element_child_index)
{
    enum disir_status status;
    va_list args_copy;
    char buffer[2048];
    char resolved[2048];
    char *current_element = NULL;
    char *next_element = NULL;
    char *last_element = NULL;
    int current_index = 0;
    int last_index = 0;

    struct disir_context *current_context = NULL;
    struct disir_context *parent_context = NULL;
    struct disir_context *ancestor_context = NULL;

    TRACE_ENTER ("parent (%p)", config);

    // Populate a buffer with our query that we can process
    va_copy (args_copy, args);
    vsnprintf (buffer, 2048, query, args_copy);
    va_end (args_copy);

    resolved[0] = '\0';
    current_element = buffer;
    last_element = buffer;
    parent_context = config;
    do
    {
        status = dx_query_resolve_name (config, current_element, resolved,
                                        &next_element, &current_index);
        if (status != DISIR_STATUS_OK)
        {
            // config is populated with the appropriate error message
            break;
        }

        // current_element now represent the last_element
        if (!next_element)
        {
            // parent_context has been located!
            last_element = current_element;
            last_index = current_index;
            break;
        }

        // Check if current_element@current_index exist in parent_context
        // If not, create the section and mark it as the ancestor if not already sat.
        status = dc_find_element (parent_context, current_element, current_index, &current_context);
        if (status == DISIR_STATUS_NOT_EXIST)
        {
            int num_elements = 0;
            // Can we create this element?
            // find out how many elements there are, to ensure that we are
            // referecing the index that should be created.
            struct disir_collection *collection;
            status = dc_find_elements (parent_context, current_element, &collection);
            if (status == DISIR_STATUS_OK)
            {
                num_elements = dc_collection_size (collection);
                dc_collection_finished (&collection);
            }
            else if (status != DISIR_STATUS_NOT_EXIST)
            {
                // internal error
                log_warn ("unknown error: %s", disir_status_string (status));
                status = DISIR_STATUS_INTERNAL_ERROR;
                break;
            }

            // index must equal collection size
            // The existing element is (collection size - 1), so
            // when they are equal, we are able to create it.
            if (current_index != num_elements)
            {
                status = DISIR_STATUS_NO_CAN_DO;
                dx_context_error_set (config, "accessing index %s is out of range of existing" \
                                               " number of elements %d, can only access index" \
                                               " that is one greater",
                                               resolved, num_elements);
                break;
            }

            // Get mold equvalent to parent_context.
            // This allows us to find the correct element to query
            // the max number of elements allowed.
            struct disir_context *parent_mold_equiv = NULL;
            struct disir_context *element_mold_equiv = NULL;
            dx_get_mold_equiv (parent_context, &parent_mold_equiv);
            status = dc_find_element (parent_mold_equiv, current_element, 0, &element_mold_equiv);
            // NOTE: parent_mold_equiv is not incref'ed
            if (status != DISIR_STATUS_OK)
            {
                status = DISIR_STATUS_MOLD_MISSING;
                dx_context_error_set (config, "section %s does not exist", resolved);
                break;
            }

            int max_elements = 0;
            status = dx_restriction_entries_value (element_mold_equiv,
                                                   DISIR_RESTRICTION_INC_ENTRY_MAX,
                                                   NULL, &max_elements);
            if (status != DISIR_STATUS_OK)
            {
                dc_putcontext (&element_mold_equiv);
                log_warn ("TODO: HANDLE ERROR MESSAGE: restriction_entries_value");
                break;
            }

            // Check that the element type is a section
            if (dc_context_type (element_mold_equiv) != DISIR_CONTEXT_SECTION)
            {
                dx_context_error_set (config, "expected %s to be a section, is keyval", resolved);
                dc_putcontext (&element_mold_equiv);
                status = DISIR_STATUS_CONFLICT;
                break;
            }

            // Mark ourselves finished with this element
            dc_putcontext (&element_mold_equiv);

            // Check that the maximum restrictions for this context
            // is greater than index
            if (max_elements != 0 && max_elements <= current_index)
            {
                status = DISIR_STATUS_RESTRICTION_VIOLATED;
                dx_context_error_set (config, "accessing index %s exceeds maximum elements %d",
                                      resolved, max_elements);
                break;
            }

            // We can successfully create the section
            status = dc_begin (parent_context, DISIR_CONTEXT_SECTION, &current_context);
            if (status != DISIR_STATUS_OK)
            {
                status = DISIR_STATUS_INTERNAL_ERROR;
                log_warn ("TODO: HANDLE ERROR MESSAGE: dc_begin");
                break;
            }

            // Set the context we've created highest up in the tree.
            if (!ancestor_context)
            {
                ancestor_context = current_context;
            }

            status = dc_set_name (current_context, current_element, strlen(current_element));
            if (status != DISIR_STATUS_OK)
            {
                log_warn ("TODO: HANDLE ERROR MESSAGE: set_name");
                // TODO: Verify / sanify error message
                // This should only happend if the variable referenced is a KEYVAL, not a section.
                dx_context_transfer_logwarn (config, current_context);
                dc_destroy (&current_context);
                break;
            }

            status = dc_generate_from_config_root (current_context);
            if (status != DISIR_STATUS_OK)
            {
                // This should NOT happend.
                log_warn ("TODO: HANDLE ERROR MESSAGE: generate_from_config_root");
                dc_destroy (&current_context);
                break;
            }

            // We do not finalize the section if its our first ancestor
            if (ancestor_context != current_context)
            {
                status = dc_finalize_keep_reference (current_context);
                if (status != DISIR_STATUS_OK)
                {
                    // Ouch - what is wrong here?
                    // TODO: error message
                    log_warn ("TODO: HANDLE ERROR MESSAGE: dc_finalize_keep_reference");
                    status = DISIR_STATUS_INTERNAL_ERROR;
                    dc_destroy (&current_context);
                    break;
                }
            }
        }
        else
        {
            // Check that our current_context is a section
            if (dc_context_type (current_context) != DISIR_CONTEXT_SECTION)
            {
                dx_context_error_set (config, "expected %s to be a section, is keyval", resolved);
                status = DISIR_STATUS_CONFLICT;
                dx_context_decref (&current_context);
                break;
            }
        }
        // QUESTION: Handle any other situation?

        // Update the parent for the next iteration in the loop
        parent_context = current_context;
        current_element = next_element;

        // Do not decref our context if its our ancestor
        if (current_context != ancestor_context)
        {
            dx_context_decref (&current_context);
        }

    } while (1);

    // If we encounter an error condition, and we have created an ancestor,
    // we need to destroy it.
    if (status != DISIR_STATUS_OK && ancestor_context)
    {
        // We should only destroy the ancestor if we havent already destroyed
        // it in our loop above
        if (ancestor_context != current_context)
        {
            dc_destroy (&ancestor_context);
        }
    }

    // Populate the output parameters
    if (status == DISIR_STATUS_OK)
    {
        strcat (element_child_name, last_element);
        *element_child_index = last_index;
        if (parent)
        {
            *parent = parent_context;
            dx_context_incref (*parent);
        }
        if (ancestor_context)
        {
            // We do NOT incref the ancestor, since it is not finalized
            *ancestor = ancestor_context;
        }
        else
        {
            // TODO: finalize the ancestor
        }
    }

    TRACE_EXIT ("status: %s", disir_status_string (status));
    return status;
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
        log_info ("element not found: %s", resolved);
        return status;
    }
    else if (status == DISIR_STATUS_OK)
    {
        // Must we recursve?
        if (next_name)
        {
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


