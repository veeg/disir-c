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
#include "schema.h"
#include "keyval.h"
#include "documentation.h"
#include "mqueue.h"
#include "log.h"

//! INTERNAL API
enum disir_status
dx_documentation_add (dc_t *parent, struct disir_documentation *doc)
{
    struct disir_documentation **doc_queue;
    enum disir_status status;
    int exists;
    char buffer[32];

    status = DISIR_STATUS_INTERNAL_ERROR;

    if (parent == NULL || doc == NULL)
        return DISIR_STATUS_INVALID_ARGUMENT;

    log_info_context (parent, "Adding documentation entry to context,");

    switch(dc_type (parent))
    {
    case DISIR_CONTEXT_SCHEMA:
    {
        doc_queue = &(parent->cx_schema->sc_documentation_queue);
        break;
    }
    case DISIR_CONTEXT_KEYVAL:
    {
        doc_queue = &(parent->cx_keyval->kv_documentation_queue);
        break;
    }
    case DISIR_CONTEXT_SECTION:
    {
        dx_crash_and_burn ("%s: %s unhandled not implemented",
                           __FUNCTION__, dc_type_string (parent));
        break;
    }
    case DISIR_CONTEXT_CONFIG:
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
    // Programming error
    if (doc_queue == NULL)
    {
        // LOGWARN
        log_debug_context (parent, "parent_doc is NULL - we cannot add doc entry");
        status = DISIR_STATUS_WRONG_CONTEXT;
    }

    exists = MQ_SIZE_COND (*doc_queue,
                (dx_semantic_version_compare(&entry->dd_introduced, &doc->dd_introduced) == 0));
    if (exists)
    {
        dx_log_context (parent,
            "already contains a documentation entry with semantic version: %s",
            dc_semantic_version_string (buffer, 40, &doc->dd_introduced));
        status = DISIR_STATUS_CONFLICTING_SEMVER;
    }
    else
    {
        MQ_ENQUEUE_CONDITIONAL (*doc_queue, doc,
            (dx_semantic_version_compare (&entry->dd_introduced, &doc->dd_introduced) > 0));
        status = DISIR_STATUS_OK;
    }

    return status;
}

//! INTERNAL API
int32_t
dx_documentation_numentries (dc_t *context)
{
    struct disir_documentation *queue;

    switch (dc_type (context))
    {
    case DISIR_CONTEXT_SCHEMA:
    {
        queue = context->cx_schema->sc_documentation_queue;
        break;
    }
    case DISIR_CONTEXT_KEYVAL:
    {
        queue = context->cx_keyval->kv_documentation_queue;
        break;
    }
    case DISIR_CONTEXT_SECTION:
    {
        dx_crash_and_burn ("%s: %s unhandled not implemented",
                           __FUNCTION__, dc_type_string (context));
        break;
    }
    case DISIR_CONTEXT_CONFIG:
    case DISIR_CONTEXT_DEFAULT:
    case DISIR_CONTEXT_RESTRICTION:
    case DISIR_CONTEXT_DOCUMENTATION:
    case DISIR_CONTEXT_UNKNOWN:
    {
        // These types do not accept a documentation entry
        return -1;
    }
    // No default - let compiler handle unreferenced context type
    }

    return MQ_SIZE (queue);
}

//! PUBLIC API
enum disir_status
dc_add_documentation (dc_t *parent, const char *doc, int32_t doc_size)
{
    dc_t *context;
    enum disir_status status;

    // Check arguments
    status = CONTEXT_NULL_INVALID_TYPE_CHECK (parent);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged.
        return status;
    }
    if (doc == NULL || doc_size <= 0)
    {
        dx_log_context (parent, "Documentation string must be non-null, of possitive length.");
        log_debug ("(doc: %p\tdoc_size: %d)", doc, doc_size);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    // Construct a simple document context with default values.
    // The SemVer version will be default.
    // Any invalid state building operation is reported to parent context
    log_debug ("Arguments ok - begining doc");

    status = dx_documentation_begin (parent, &context);
    if (status != DISIR_STATUS_OK)
    {
        // dc_documentation_begin performs LOGWARN
        return status;
    }

    status = dc_set_value_string (context, doc, doc_size);
    if (status != DISIR_STATUS_OK)
    {
        dx_context_transfer_logwarn (parent, context);
        dc_destroy (&context);
        return status;
    }

    status = dx_documentation_finalize (&context);
    if (status != DISIR_STATUS_OK)
    {
        dx_context_transfer_logwarn (parent, context);
        dc_destroy (&context);
    }
    return status;
}

//! PUBLIC API
enum disir_status
dc_get_documentation (dc_t *context, struct semantic_version *semver,
                      const char **doc, int32_t *doc_size)
{
    enum disir_status status;
    struct disir_documentation **doc_parent;
    struct disir_documentation *doc_context;

    status = CONTEXT_NULL_INVALID_TYPE_CHECK (context);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }
    status = CONTEXT_TYPE_CHECK (context, DISIR_CONTEXT_KEYVAL,
                                          DISIR_CONTEXT_CONFIG,
                                          DISIR_CONTEXT_SECTION,
                                          DISIR_CONTEXT_SCHEMA);
    if (status != DISIR_STATUS_OK)
    {
        dx_log_context (context, "cannot fetch documentation for context.");
        return status;
    }
    if (doc == NULL || doc_size == NULL)
    {
        // Already logged
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    switch (dc_type (context))
    {
    case DISIR_CONTEXT_KEYVAL:
        doc_parent = &context->cx_keyval->kv_documentation_queue;
        break;
    case DISIR_CONTEXT_SCHEMA:
        doc_parent = &context->cx_schema->sc_documentation_queue;
        break;
    default:
    {
        dx_crash_and_burn ("%s: %s unhandled", __FUNCTION__, dc_type_string (context));
    }
    }

    if (semver == NULL)
    {
        // Get highest semver
        doc_context = MQ_TAIL (*doc_parent);
    }
    else
    {
        // Get the prev entry from the found entry
        //  NULL is returned if the tail is lower than our version compare input
        doc_context = MQ_FIND (*doc_parent,
                (dx_semantic_version_compare (&entry->dd_introduced, semver) > 0));
        if (doc_context != NULL && doc_context->prev != MQ_TAIL (*doc_parent))
        {
            doc_context = doc_context->prev;
        }
        if (doc_context == NULL)
        {
            doc_context = MQ_TAIL (*doc_parent);
        }
    }


    if (doc_context == NULL)
    {
        log_error_context (context, "No doc entry on context" );
        return DISIR_STATUS_INTERNAL_ERROR;
    }

    status = dx_value_get_string (&doc_context->dd_value, doc, doc_size);
    return status;
}

//! INTERNAL API
enum disir_status
dx_documentation_begin (dc_t *parent, dc_t **doc)
{
    dc_t *context;

    // Check arguments
    if (parent == NULL || doc == NULL)
    {
        // LOGWARN
        log_warn ("%s: parent and/or doc invoked with NULL pointers. (parent: %p, doc: %p)",
                __FUNCTION__, parent, doc);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    // Check if we can add single entry
    if ((parent->cx_capabilities & CC_ADD_DOCUMENTATION) == 0)
    {
        // LOGWARN
        dx_log_context (parent, "No capability: %s",
                dx_context_capability_string (CC_ADD_DOCUMENTATION));
        return DISIR_STATUS_NO_CAN_DO;
    }

    log_debug_context (parent, "capable of adding single documentataion.");

    // Check if we can add multiple documentation entries
    if (dx_documentation_numentries(parent) > 0)
    {
        log_debug ("documentation exists");
        if ((parent->cx_capabilities & CC_ADD_MULTIPLE_DOCUMENTATION) == 0)
        {
            // LOGWARN
            dx_log_context (parent, "Contains one documentation entry. It has no capability: %s",
                    dx_context_capability_string (CC_ADD_MULTIPLE_DOCUMENTATION));
            return DISIR_STATUS_EXISTS;
        }
    }

    log_debug_context (parent, "capable of adding documentation context");

    context = dx_context_create (DISIR_CONTEXT_DOCUMENTATION);
    if (context == NULL)
    {
        // LOGWARN
        log_debug_context (parent, "failed to allocate new document context");
        return DISIR_STATUS_NO_MEMORY;
    }

    log_debug_context (parent, "created context: %p", context);

    context->cx_documentation = dx_documentation_create(context);
    if (context->cx_documentation == NULL)
    {
        // LOGWARN
        dx_context_destroy (&context);
        dx_log_context (parent, "cannot allocate new document instance");
        return DISIR_STATUS_NO_MEMORY;
    }

    log_debug_context (parent, "allocated documentation instance: %p", context->cx_documentation);

    dx_context_attach(parent, context);
    *doc = context;
    return DISIR_STATUS_OK;
}

//! INTERNAL API
enum disir_status
dx_documentation_finalize (dc_t **doc)
{
    enum disir_status status;

    status = DISIR_STATUS_OK;

    // Check argument
    if (doc == NULL || *doc == NULL)
    {
        // LOGWARN
        log_debug ("invoked with null parameters");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    status = dx_documentation_add ((*doc)->cx_parent_context, (*doc)->cx_documentation);

    if (status == DISIR_STATUS_OK)
    {
        // Finalize shall not decrement
        *doc = NULL;
    }

    return status;
}

//! INTERNAL API
enum disir_status
dx_documentation_add_value_string (struct disir_documentation *doc,
                                   const char *value,
                                   int32_t value_size)
{
    log_debug ("setting documentation (%p) with value (%p) of size (%d)",
               doc, value, value_size);
    return dx_value_set_string (&doc->dd_value, value, value_size);
}

//! INTERNAL API
struct disir_documentation *
dx_documentation_create (dc_t *context)
{
    struct disir_documentation *doc;

    doc = calloc(1, sizeof (struct disir_documentation));
    if (doc == NULL)
        return NULL;

    doc->dd_context = context;
    doc->dd_value.dv_type = DISIR_VALUE_TYPE_STRING;

    return doc;
}

//! INTERNAL API
enum disir_status
dx_documentation_destroy (struct disir_documentation **documentation)
{
    struct disir_documentation *tmp;
    struct disir_documentation **queue;
    dc_t *context;

    if (documentation == NULL || *documentation == NULL)
        return DISIR_STATUS_INVALID_ARGUMENT;

    tmp = *documentation;

    if (tmp->dd_value.dv_size > 0)
        free(tmp->dd_value.dv_string);

    context = (*documentation)->dd_context;
    if (context && context->cx_parent_context)
    {
        context = context->cx_parent_context;

        switch (dc_type (context))
        {
        case DISIR_CONTEXT_SCHEMA:
        {
            queue = &(context->cx_schema->sc_documentation_queue);
            break;
        }
        case DISIR_CONTEXT_KEYVAL:
        {
            queue = &(context->cx_keyval->kv_documentation_queue);
            break;
        }
        case DISIR_CONTEXT_SECTION:
        {
            dx_crash_and_burn ("%s: %s unhandled not implemented",
                           __FUNCTION__, dc_type_string (context));
            break;
        }
        case DISIR_CONTEXT_CONFIG:
        case DISIR_CONTEXT_DEFAULT:
        case DISIR_CONTEXT_RESTRICTION:
        case DISIR_CONTEXT_DOCUMENTATION:
        case DISIR_CONTEXT_UNKNOWN:
        {
            // These types do not accept a documentation entry
            // No default - let compiler handle unreferenced context type
            dx_crash_and_burn ("invoked on invalid context type (%s)", dc_type_string (context));
        }
        }

        // Must remove safely - cannot guarantee that the instance we are destroying
        // is in its parents queue
        MQ_REMOVE_SAFE (*queue, tmp);
    }

    free(tmp);
    *documentation = NULL;
    return DISIR_STATUS_OK;
}

