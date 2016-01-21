// External public includes
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

// Public disir interface
#include <disir/disir.h>
#include <disir/context.h>

// Private
#include "context_private.h"
#include "config.h"
#include "log.h"

//! INTERNAL API
struct disir_documentation *
dx_documentation_fetch(dc_t *context)
{
    struct disir_documentation *doc;

    doc = NULL;

    switch(dc_type(context))
    {
    case DISIR_CONTEXT_CONFIG:
    {
        doc = context->cx_config->cf_documentation;
        break;
    }
    case DISIR_CONTEXT_SCHEMA:
    case DISIR_CONTEXT_TEMPLATE:
    case DISIR_CONTEXT_GROUP:
    case DISIR_CONTEXT_KEYVAL:
    {
        dx_crash_and_burn("unhandled not implemented");
        break;
    }
    case DISIR_CONTEXT_TYPE:
    case DISIR_CONTEXT_DEFAULT:
    case DISIR_CONTEXT_RESTRICTION:
    case DISIR_CONTEXT_DOCUMENTATION:
    case DISIR_CONTEXT_UNKNOWN:
    {
        // These types do not accept a documentation entry
        break;
    }
    // No default - let compiler handle unreferenced context type
    }

    return doc;
}

//! INTERNAL API
enum disir_status
dx_documentation_add(dc_t *parent, struct disir_documentation *doc)
{
    struct disir_documentation **parent_doc;
    struct disir_documentation *tmp;
    enum disir_status status;
    int res;
    char buffer[32];

    status = DISIR_STATUS_INTERNAL_ERROR;
    parent_doc = NULL;

    if (parent == NULL || doc == NULL)
        return DISIR_STATUS_INVALID_ARGUMENT;

    log_info_context(parent, "Adding documentation entry to context,");

    switch(dc_type(parent))
    {
    case DISIR_CONTEXT_CONFIG:
    {
        parent_doc = &(parent->cx_config->cf_documentation);
        break;
    }
    case DISIR_CONTEXT_SCHEMA:
    case DISIR_CONTEXT_TEMPLATE:
    case DISIR_CONTEXT_GROUP:
    case DISIR_CONTEXT_KEYVAL:
    {
        dx_crash_and_burn("unhandled not implemented");
        break;
    }
    case DISIR_CONTEXT_TYPE:
    case DISIR_CONTEXT_DEFAULT:
    case DISIR_CONTEXT_RESTRICTION:
    case DISIR_CONTEXT_DOCUMENTATION:
    case DISIR_CONTEXT_UNKNOWN:
    {
        // These types do not accept a documentation entry
        break;
    }
    // No default - let compiler  detect unhandled type
    }

    // Function invoked on wrong context. 
    // Programming error - 
    if (parent_doc == NULL)
    {
        // LOGWARN
        log_debug_context(parent, "parent_doc is NULL - we cannot add doc entry");
        status = DISIR_STATUS_WRONG_CONTEXT;
    }
    // Add to parent storage if no entry exists
    else if (*parent_doc == NULL)
    {
        log_debug_context(parent, "doesn't contain any doc entries. Adding as only element.");
        *parent_doc = doc;
        status = DISIR_STATUS_OK;
    }
    // There exists multiple entries - insert into storage
    else
    {
        log_debug_context(parent, "contains documentation entires. Adding in-place");
        // Assume everything is okay
        status = DISIR_STATUS_OK;

        // Special first case - need to change the parent_doc pointer.
        res = dx_semantic_version_compare(&((*parent_doc)->dd_introduced), &(doc->dd_introduced));
        if (res > 0)
        {
            doc->dd_next = *parent_doc;
            (*parent_doc)->dd_prev = doc;
            *parent_doc = doc;
        }
        else
        {
            tmp = *parent_doc;
            while (1)
            {
                // Entry exists - deny this operation
                if (res == 0)
                {
                    // LOGWARN
                    dx_log_context (parent,
                            "already contains a documentation entry with semantic version: %s",
                            dx_semver_string (buffer, 40, &doc->dd_introduced));
                    status = DISIR_STATUS_CONFLICTING_SEMVER;
                    break;
                }

                // Insert inplace of tmp
                if (res > 0)
                {
                    doc->dd_next = tmp;
                    doc->dd_prev = tmp->dd_prev;
                    if (tmp->dd_prev)
                    {
                        tmp->dd_prev->dd_next = doc;
                    }
                    tmp->dd_prev = doc;
                    break;
                }

                // Doc is greatest entry, append to list
                if (tmp->dd_next == NULL)
                {
                    doc->dd_prev = tmp;
                    tmp->dd_next = doc;
                    break;
                }

                tmp = tmp->dd_next;
                res = dx_semantic_version_compare(&tmp->dd_introduced, &doc->dd_introduced);
            }
        }
    }

    return status;
}

uint32_t
dx_documentation_exists(dc_t *context)
{
    struct disir_documentation *doc;

    doc = dx_documentation_fetch(context);

    return (doc != NULL ? 1 : 0);
}


//! PUBLIC API
enum disir_status
dc_add_documentation(dc_t *parent, char *doc, int32_t doc_size)
{
    dc_t *context;
    enum disir_status status;

    // Check arguments
    status = CONTEXT_NULL_INVALID_TYPE_CHECK(parent);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged.
        return status;
    }
    if (doc == NULL || doc_size <= 0)
    {
        dx_log_context(parent, "Documentation string must be non-null, of possitive length.");
        log_debug("(doc: %p\tdoc_size: %d)", doc, doc_size);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    // Construct a simple document context with default values.
    // The SemVer version will be default.
    // Any invalid state building operation is reported to parent context
    log_debug("Arguments ok - begining doc");

    status = dx_documentation_begin(parent, &context);
    if (status != DISIR_STATUS_OK)
    {
        // dc_documentation_begin performs LOGWARN
        return status;
    }

    status = dc_add_value_string(context, doc, doc_size);
    if (status != DISIR_STATUS_OK)
    {
        // dc_add_value_string performs LOGWARN
        dx_context_transfer_logwarn(parent, context);
        dc_destroy(&context);
        return status;
    }

    status = dx_documentation_finalize(&context);
    if (status != DISIR_STATUS_OK)
    {
        dx_context_transfer_logwarn(parent, context);
        dc_destroy(&context);
    }
    return status;
}

//! INTERNAL API
enum disir_status
dx_documentation_begin(dc_t *parent, dc_t **doc)
{
    dc_t *context;

    // Check arguments
    if (parent == NULL || doc == NULL)
    {
        // LOGWARN
        log_warn("%s: parent and/or doc invoked with NULL pointers. (parent: %p, doc: %p)", __FUNCTION__, parent, doc);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    // Check if we can add single entry
    if ((parent->cx_capabilities & CC_ADD_DOCUMENTATION) == 0)
    {
        // LOGWARN
        dx_log_context(parent, "No capability: %s", dx_context_capability_string(CC_ADD_DOCUMENTATION));
        return DISIR_STATUS_NO_CAN_DO;
    }

    log_debug_context(parent, "capable of adding single documentataion.");

    // Check if we can add multiple documentation entries
    if (dx_documentation_exists(parent))
    {
        log_debug("documentation exists");
        if ((parent->cx_capabilities & CC_ADD_MULTIPLE_DOCUMENTATION) == 0)
        {
            // LOGWARN
            dx_log_context(parent, "Contains one documentation entry. It has no capability: %s", dx_context_capability_string(CC_ADD_MULTIPLE_DOCUMENTATION));
            return DISIR_STATUS_EXISTS;
        }
    }

    log_debug_context(parent, "capable of adding documentation context");

    context = dx_context_create(DISIR_CONTEXT_DOCUMENTATION);
    if (context == NULL)
    {
        // LOGWARN
        log_debug_context(parent, "failed to allocate new document context");
        return DISIR_STATUS_NO_MEMORY;
    }

    log_debug_context(parent, "created context: %p", context);

    context->cx_documentation = dx_documentation_create(context);
    if (context->cx_documentation == NULL)
    {
        // LOGWARN
        dx_context_destroy(&context);
        dx_log_context(parent, "cannot allocate new document instance");
        return DISIR_STATUS_NO_MEMORY;
    }

    log_debug_context(parent, "allocated documentation instance: %p", context->cx_documentation);

    dx_context_attach(parent, context);
    *doc = context;
    return DISIR_STATUS_OK;
}

//! INTERNAL API
enum disir_status
dx_documentation_finalize(dc_t **doc)
{
    enum disir_status status;

    status = DISIR_STATUS_OK;

    // Check argument
    if (doc == NULL || *doc == NULL)
    {
        // LOGWARN
        log_debug("invoked with null parameters");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    status = dx_documentation_add((*doc)->cx_parent_context, (*doc)->cx_documentation);

    if (status == DISIR_STATUS_OK)
    {
        // Finalize shall not decrement
        *doc = NULL;
    }

    return status;
}

//! INTERNAL API
enum disir_status
dx_documentation_add_value_string(struct disir_documentation *doc, const char *value, int32_t value_size)
{
    // At this point, simply allocate /reallocate and override exiting shizzle

    if (doc->dd_string == NULL || doc->dd_string_allocated - 1 < value_size)
    {   
        // Just free the existing memory. We allocate a larger one below
        if (doc->dd_string_allocated - 1 < value_size )
        {
            free(doc->dd_string);
            doc->dd_string = NULL;
        }
        // Allocate doc string, if need be
        if (doc->dd_string == NULL)
        {
            // Size of requested string + 1 for NULL terminator
            doc->dd_string = calloc(1, value_size + 1);
            if (doc->dd_string == NULL)
            {
                // LOGWARN
                fprintf(stderr, "Failed to allocate memory for doc string");
                return DISIR_STATUS_NO_MEMORY;
            }
            doc->dd_string_allocated = value_size;
        }
    }

    // Copy the incoming docstring to freely available space
    memcpy(doc->dd_string, value, value_size);
    doc->dd_string_size = value_size;

    // Terminate it with a zero terminator. Just to be safe.
    doc->dd_string[value_size] = '\0';

    return DISIR_STATUS_OK;
}

//! INTERNAL API
struct disir_documentation *
dx_documentation_create(dc_t *context)
{
    struct disir_documentation *doc;

    doc = calloc(1, sizeof(struct disir_documentation));
    if (doc == NULL)
        return NULL;

    doc->dd_context = context;

    return doc;
}

//! INTERNAL API
enum disir_status
dx_documentation_destroy(struct disir_documentation **documentation)
{
    struct disir_documentation *tmp;

    if (documentation == NULL || *documentation == NULL)
        return DISIR_STATUS_INVALID_ARGUMENT;

    tmp = *documentation;

    if (tmp->dd_string != NULL)
        free(tmp->dd_string);

    // Unhook from double linked list.
    if (tmp->dd_prev)
        tmp->dd_prev->dd_next = tmp->dd_next;
    if (tmp->dd_next)
        tmp->dd_next->dd_prev = tmp->dd_prev;

    free(tmp);
    *documentation = NULL;
    return DISIR_STATUS_OK;
}
