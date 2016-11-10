// external public includes
#include <stdlib.h>
#include <stdio.h>

// public disir interface
#include <disir/disir.h>
#include <disir/context.h>

// private
#include "context_private.h"
#include "mold.h"
#include "documentation.h"
#include "mqueue.h"
#include "log.h"


//! PUBLIC API
struct disir_context *
dc_mold_getcontext (struct disir_mold *mold)
{
    if (mold == NULL)
    {
        log_debug (0, "invoked with NULL mold pointer.");
        return NULL;
    }

    dx_context_incref (mold->mo_context);

    return mold->mo_context;
}

//! PUBLIC API
enum disir_status
dc_mold_begin (struct disir_context **mold)
{
    struct disir_context *context;

    if (mold == NULL)
    {
        log_debug (0, "invoked with NULL mold pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    context = dx_context_create (DISIR_CONTEXT_MOLD);
    if (context == NULL)
    {
        log_error ("failed to allocate context for mold.");
        return DISIR_STATUS_NO_MEMORY;
    }

    context->cx_mold = dx_mold_create (context);
    if (context->cx_mold == NULL)
    {
        log_error ("failed to allocate mold for context.");
        dx_context_destroy (&context);
        return DISIR_STATUS_NO_MEMORY;
    }

    // Set root context to self (such that children can inherit)
    context->cx_root_context = context;

    *mold = context;
    return DISIR_STATUS_OK;
}

// PUBLIC API
enum disir_status
dc_mold_finalize (struct disir_context **context, struct disir_mold **mold)
{
    enum disir_status status;

    TRACE_ENTER ("context: %p, mold: %p", context, mold);

    status = CONTEXT_DOUBLE_NULL_INVALID_TYPE_CHECK (context);
    if (status != DISIR_STATUS_OK)
    {
        // Already looged
        return status;
    }
    if (mold == NULL)
    {
        log_debug (0, "invoked with NULL mold pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    if (dx_context_type_sanify ((*context)->cx_type) != DISIR_CONTEXT_MOLD)
    {
        dx_log_context (*context, "Cannot call %s() on top-level context( %s )",
                        __FUNCTION__, dc_context_type_string (*context));
        return DISIR_STATUS_WRONG_CONTEXT;
    }

    // TODO: Perform some validation?

    *mold = (*context)->cx_mold;
    (*context)->CONTEXT_STATE_FINALIZED = 1;
    *context = NULL;
    // We do not decref context refcount on finalize

    TRACE_EXIT ("*mold: %p", mold);
    return DISIR_STATUS_OK;
}


//! INTERNAL API
struct disir_mold *
dx_mold_create (struct disir_context *context)
{
    struct disir_mold *mold;

    mold = calloc (1, sizeof (struct disir_mold));
    if (mold == NULL)
    {
        goto error;
    }

    mold->mo_reference_count = 1;
    mold->mo_context = context;
    mold->mo_elements = dx_element_storage_create ();
    if (mold->mo_elements == NULL)
    {
        goto error;
    }

    // Initialize version to 1.0.0
    mold->mo_version.sv_major = 1;
    mold->mo_version.sv_minor = 0;
    mold->mo_version.sv_patch = 0;

    return mold;
error:
    if (mold && mold->mo_elements)
    {
        dx_element_storage_destroy (&mold->mo_elements);
    }
    if (mold)
    {
        free (mold);
    }

    return NULL;
}

//! INTERNAL API
enum disir_status
dx_mold_destroy (struct disir_mold **mold)
{
    enum disir_status status;
    struct disir_context *context;
    struct disir_documentation *doc;
    struct disir_collection *collection;

    if (mold == NULL || *mold == NULL)
    {
        log_debug (0, "invoked with NULL mold pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    // Free plugin name string (if assigned)
    if ((*mold)->mo_plugin_name)
    {
        free ((*mold)->mo_plugin_name);
    }

    // Destroy all element_storage children
    status = dx_element_storage_get_all ((*mold)->mo_elements, &collection);
    if (status == DISIR_STATUS_OK)
    {
        while (dx_collection_next_noncoalesce (collection, &context) != DISIR_STATUS_EXHAUSTED)
        {
            dx_context_decref (&context);
            dc_putcontext (&context);
        }
        dc_collection_finished (&collection);
    }
    else
    {
        log_warn ("failed to get_all from internal element storage: %s",
                  disir_status_string (status));
    }

    // Destroy element storage in the mold.
    dx_element_storage_destroy (&(*mold)->mo_elements);

    // Destroy the documentation associated with the mold.
    while ((doc = MQ_POP ((*mold)->mo_documentation_queue)))
    {
        context = doc->dd_context;
        dc_destroy (&context);
    }

    free (*mold);
    *mold = NULL;

    return DISIR_STATUS_OK;
}

//! INTERNAL API
enum disir_status
dx_mold_update_version (struct disir_mold *mold, struct semantic_version *semver)
{
    struct semantic_version fact;
    char buffer[32];

    if (mold == NULL || semver == NULL)
    {
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    fact.sv_major = mold->mo_version.sv_major;
    fact.sv_minor = mold->mo_version.sv_minor;
    fact.sv_patch = mold->mo_version.sv_patch;

    if (mold->mo_version.sv_major < semver->sv_major)
    {
        fact.sv_major = semver->sv_major;
        fact.sv_minor = semver->sv_minor;
        fact.sv_patch = semver->sv_patch;
    }
    else if (mold->mo_version.sv_major == semver->sv_major &&
             mold->mo_version.sv_minor < semver->sv_minor)
    {
        fact.sv_minor = semver->sv_minor;
        fact.sv_patch = semver->sv_patch;
    }
    else if (mold->mo_version.sv_minor == semver->sv_minor &&
             mold->mo_version.sv_patch < semver->sv_patch)
    {
        fact.sv_patch = semver->sv_patch;
    }

    mold->mo_version.sv_major = fact.sv_major;
    mold->mo_version.sv_minor = fact.sv_minor;
    mold->mo_version.sv_patch = fact.sv_patch;

    log_debug (6, "mold (%p) version sat to: %s",
               mold, dc_semantic_version_string (buffer, 32, &fact));

    return DISIR_STATUS_OK;
}

//! PUBLIC API
enum disir_status
dc_mold_get_version (struct disir_mold *mold, struct semantic_version *semver)
{
    if (mold == NULL || semver == NULL)
    {
        log_debug (0, "invoked with NULL pointer(s)");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    semver->sv_major = mold->mo_version.sv_major;
    semver->sv_minor = mold->mo_version.sv_minor;
    semver->sv_patch = mold->mo_version.sv_patch;

    return DISIR_STATUS_OK;
}

