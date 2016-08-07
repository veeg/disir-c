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
#include "mold.h"
#include "keyval.h"
#include "documentation.h"
#include "section.h"
#include "mqueue.h"
#include "log.h"

//! INTERNAL API
enum disir_status
dx_documentation_add (struct disir_context *parent, struct disir_documentation *doc)
{
    struct disir_documentation **doc_queue;
    enum disir_status status;
    int exists;
    char buffer[32];

    status = DISIR_STATUS_INTERNAL_ERROR;

    if (parent == NULL || doc == NULL)
        return DISIR_STATUS_INVALID_ARGUMENT;

    log_info_context (parent, "Adding documentation entry to context,");

    switch(dc_context_type (parent))
    {
    case DISIR_CONTEXT_MOLD:
    {
        doc_queue = &(parent->cx_mold->mo_documentation_queue);
        break;
    }
    case DISIR_CONTEXT_KEYVAL:
    {
        doc_queue = &(parent->cx_keyval->kv_documentation_queue);
        break;
    }
    case DISIR_CONTEXT_SECTION:
    {
        doc_queue = &(parent->cx_section->se_documentation_queue);
        break;
    }
    case DISIR_CONTEXT_CONFIG:
    case DISIR_CONTEXT_DEFAULT:
    case DISIR_CONTEXT_RESTRICTION:
    case DISIR_CONTEXT_DOCUMENTATION:
    case DISIR_CONTEXT_FREE_TEXT:
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
        log_debug_context (0, parent, "parent_doc is NULL - we cannot add doc entry");
        status = DISIR_STATUS_WRONG_CONTEXT;
    }

    exists = MQ_SIZE_COND (*doc_queue,
                (dc_semantic_version_compare (&entry->dd_introduced, &doc->dd_introduced) == 0));
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
            (dc_semantic_version_compare (&entry->dd_introduced, &doc->dd_introduced) > 0));
        status = DISIR_STATUS_OK;
    }

    return status;
}

//! INTERNAL API
int32_t
dx_documentation_numentries (struct disir_context *context)
{
    struct disir_documentation *queue;

    switch (dc_context_type (context))
    {
    case DISIR_CONTEXT_MOLD:
    {
        queue = context->cx_mold->mo_documentation_queue;
        break;
    }
    case DISIR_CONTEXT_KEYVAL:
    {
        queue = context->cx_keyval->kv_documentation_queue;
        break;
    }
    case DISIR_CONTEXT_SECTION:
    {
        queue = context->cx_section->se_documentation_queue;
        break;
    }
    case DISIR_CONTEXT_CONFIG:
    case DISIR_CONTEXT_DEFAULT:
    case DISIR_CONTEXT_RESTRICTION:
    case DISIR_CONTEXT_DOCUMENTATION:
    case DISIR_CONTEXT_FREE_TEXT:
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
dc_add_documentation (struct disir_context *parent, const char *doc, int32_t doc_size)
{
    struct disir_context *context;
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
        log_debug (6, "(doc: %p\tdoc_size: %d)", doc, doc_size);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    // Construct a simple document context with default values.
    // The SemVer version will be default.
    // Any invalid state building operation is reported to parent context
    log_debug (6, "Arguments ok - begining doc");

    status = dc_begin (parent, DISIR_CONTEXT_DOCUMENTATION, &context);
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

    status = dx_documentation_finalize (context);
    if (status != DISIR_STATUS_OK)
    {
        dx_context_transfer_logwarn (parent, context);
        dc_destroy (&context);
    }
    return status;
}

//! PUBLIC API
enum disir_status
dc_get_documentation (struct disir_context *context, struct semantic_version *semver,
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
                                          DISIR_CONTEXT_MOLD);
    if (status != DISIR_STATUS_OK)
    {
        dx_log_context (context, "cannot fetch documentation for context.");
        return status;
    }
    if (doc == NULL)
    {
        // Already logged
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    switch (dc_context_type (context))
    {
    case DISIR_CONTEXT_KEYVAL:
        doc_parent = &context->cx_keyval->kv_documentation_queue;
        break;
    case DISIR_CONTEXT_MOLD:
        doc_parent = &context->cx_mold->mo_documentation_queue;
        break;
    case DISIR_CONTEXT_SECTION:
        doc_parent = &context->cx_section->se_documentation_queue;
        break;
    default:
    {
        dx_crash_and_burn ("%s: %s unhandled", __FUNCTION__, dc_context_type_string (context));
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
                (dc_semantic_version_compare (&entry->dd_introduced, semver) > 0));
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
dx_documentation_begin (struct disir_context *parent, struct disir_context **doc)
{
    enum disir_status status;
    struct disir_context *context;

    status = CONTEXT_NULL_INVALID_TYPE_CHECK (parent);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }
    // Check arguments
    if (doc == NULL)
    {
        // LOGWARN
        log_debug (0, "invoked with doc NULL pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    status = CONTEXT_TYPE_CHECK (parent, DISIR_CONTEXT_MOLD,
                                         DISIR_CONTEXT_KEYVAL,
                                         DISIR_CONTEXT_SECTION,
                                         DISIR_CONTEXT_RESTRICTION);
    if (status != DISIR_STATUS_OK)
    {
        // LOGWARN
        dx_log_context (parent, "cannot add documentation to %s.",
                                dc_context_type_string (parent));
        return DISIR_STATUS_NO_CAN_DO;
    }

    log_debug_context (6, parent, "capable of adding single documentataion.");

    // Check if we can add multiple documentation entries
    if (dx_documentation_numentries(parent) > 0)
    {
        // LOGWARN
        dx_log_context (parent, "cannot add multiple documentations to %s.",
                                dc_context_type_string (parent));
        return DISIR_STATUS_EXISTS;
    }

    log_debug_context (6, parent, "capable of adding documentation context");

    context = dx_context_create (DISIR_CONTEXT_DOCUMENTATION);
    if (context == NULL)
    {
        // LOGWARN
        log_debug_context (1, parent, "failed to allocate new document context");
        return DISIR_STATUS_NO_MEMORY;
    }

    log_debug_context (8, parent, "created context: %p", context);

    context->cx_documentation = dx_documentation_create(context);
    if (context->cx_documentation == NULL)
    {
        // LOGWARN
        dx_context_destroy (&context);
        dx_log_context (parent, "cannot allocate new document instance");
        return DISIR_STATUS_NO_MEMORY;
    }

    log_debug_context (8, parent,
                       "allocated documentation instance: %p", context->cx_documentation);

    dx_context_attach(parent, context);
    *doc = context;
    return DISIR_STATUS_OK;
}

//! INTERNAL API
enum disir_status
dx_documentation_finalize (struct disir_context *doc)
{
    // Check argument
    if (doc == NULL)
    {
        // LOGWARN
        log_debug (0, "invoked with NULL doc argument");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    return dx_documentation_add (doc->cx_parent_context, doc->cx_documentation);
}

//! INTERNAL API
struct disir_documentation *
dx_documentation_create (struct disir_context *context)
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
    struct disir_context *context;

    if (documentation == NULL || *documentation == NULL)
        return DISIR_STATUS_INVALID_ARGUMENT;

    tmp = *documentation;

    if (tmp->dd_value.dv_size > 0)
        free(tmp->dd_value.dv_string);

    context = (*documentation)->dd_context;
    if (context && context->cx_parent_context)
    {
        context = context->cx_parent_context;

        switch (dc_context_type (context))
        {
        case DISIR_CONTEXT_MOLD:
        {
            queue = &(context->cx_mold->mo_documentation_queue);
            break;
        }
        case DISIR_CONTEXT_KEYVAL:
        {
            queue = &(context->cx_keyval->kv_documentation_queue);
            break;
        }
        case DISIR_CONTEXT_SECTION:
        {
            queue = &(context->cx_section->se_documentation_queue);
            break;
        }
        case DISIR_CONTEXT_CONFIG:
        case DISIR_CONTEXT_DEFAULT:
        case DISIR_CONTEXT_RESTRICTION:
        case DISIR_CONTEXT_DOCUMENTATION:
        case DISIR_CONTEXT_FREE_TEXT:
        case DISIR_CONTEXT_UNKNOWN:
        {
            // These types do not accept a documentation entry
            // No default - let compiler handle unreferenced context type
            dx_crash_and_burn ("invoked on invalid context type (%s)",
                               dc_context_type_string (context));
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

