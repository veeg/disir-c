// External public includes
#include <stdlib.h>
#include <stdio.h>

// Public disir interface
#include <disir/disir.h>
#include <disir/context.h>

// Private
#include "context_private.h"
#include "config.h"
#include "mold.h"
#include "mqueue.h"
#include "log.h"
#include "element_storage.h"

//! PUBLIC API
struct disir_context *
dc_config_getcontext (struct disir_config *config)
{
    // Check arguments
    if (config == NULL)
    {
        // LOGWARN
        log_debug (0, "invoked with NULL pointer");
        return NULL;
    }

    dx_context_incref (config->cf_context);

    return config->cf_context;
}

//! PUBLIC API
enum disir_status
dc_config_begin (struct disir_mold *mold, struct disir_context **config)
{
    struct disir_context *context;

    context = NULL;

    // Disallow non-null content of passed pointer.
    if (config == NULL || mold == NULL)
    {
        // LOGWARN
        log_debug (0, "invoked with NULL pointer(s) (%p %p)", context, mold);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    // Allocate a context to be returned regardless of outcome.
    context = dx_context_create (DISIR_CONTEXT_CONFIG);
    if (context == NULL)
    {
        // LOGWARN
        log_error ("failed to allocate context for config");
        return DISIR_STATUS_NO_MEMORY;
    }

    // Allocate a disir_config for this context.
    context->cx_config = dx_config_create (context);
    if (context->cx_config == NULL)
    {
        log_error ("failed to allocate config for context");
        dx_context_destroy (&context);
        return DISIR_STATUS_NO_MEMORY;
    }

    // Set associated mold
    context->cx_config->cf_mold = mold;
    mold->mo_reference_count++;

    // Set root context to self (such that children can inherit)
    context->cx_root_context = context;

    *config = context;
    return DISIR_STATUS_OK;
}

// PUBLIC API
enum disir_status
dc_config_finalize (struct disir_context **context, struct disir_config **config)
{
    enum disir_status status;

    TRACE_ENTER ("context: %p, config: %p", context, config);

    status = CONTEXT_DOUBLE_NULL_INVALID_TYPE_CHECK (context);
    if (status != DISIR_STATUS_OK)
    {
        // Already looged
        return status;
    }
    if (config == NULL)
    {
        log_debug (0, "invoked with NULL config pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    if (dx_context_type_sanify ((*context)->cx_type) != DISIR_CONTEXT_CONFIG)
    {
        dx_log_context (*context, "Cannot call %s() on top-level context( %s )",
                        __FUNCTION__, dc_context_type_string (*context));
        return DISIR_STATUS_WRONG_CONTEXT;
    }

    // Perform full config validation.
    status = dx_validate_context (*context);
    if (status == DISIR_STATUS_OK ||
        status == DISIR_STATUS_INVALID_CONTEXT ||
        status == DISIR_STATUS_ELEMENTS_INVALID)
    {
        *config = (*context)->cx_config;
        (*context)->CONTEXT_STATE_FINALIZED = 1;
        (*context)->CONTEXT_STATE_CONSTRUCTING = 0;

        // Only let API return INVALID_CONTEXT if anything is amiss
        if (status == DISIR_STATUS_ELEMENTS_INVALID)
        {
            status = DISIR_STATUS_INVALID_CONTEXT;
        }

        // We do not decref context refcount on finalize
        // Deprive the user of his context reference.
        *context = NULL;
    }
    else
    {
        log_fatal_context (*context, "failed internally with status %s",
                                     disir_status_string (status));
        status = DISIR_STATUS_INTERNAL_ERROR;
    }

    TRACE_EXIT ("status: %s", disir_status_string (status));
    return status;
}

//! INTERNAL API
struct disir_config *
dx_config_create (struct disir_context *context)
{
    struct disir_config *config;

    config = calloc (1, sizeof (struct disir_config));
    if (config ==  NULL)
    {
        goto error;
    }

    config->cf_elements = dx_element_storage_create ();
    if (config->cf_elements == NULL)
    {
        goto error;
    }

    config->cf_context = context;
    config->cf_version.sv_major = 1;

    return config;
error:
    if (config && config->cf_elements)
    {
        dx_element_storage_destroy (&config->cf_elements);
    }
    if (config)
    {
        free (config);
    }

    return NULL;
}

//! INTERNAL API
enum disir_status
dx_config_destroy (struct disir_config **config)
{
    enum disir_status status;
    struct disir_context *context;
    struct disir_collection *collection;

    if (config == NULL || *config == NULL)
        return DISIR_STATUS_INVALID_ARGUMENT;

    // Remove our reference to the mold
    disir_mold_finished (&(*config)->cf_mold);

    // Destroy all element_storage children
    status = dx_element_storage_get_all ((*config)->cf_elements, &collection);
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

    dx_element_storage_destroy (&(*config)->cf_elements);

    free (*config);
    *config = NULL;

    return DISIR_STATUS_OK;
}

//! PUBLIC API
enum disir_status
dc_config_get_version (struct disir_config *config, struct semantic_version *semver)
{
    if (config == NULL || semver == NULL)
    {
        log_debug (0, "invoked with NULL pointer(s)");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    semver->sv_major = config->cf_version.sv_major;
    semver->sv_minor = config->cf_version.sv_minor;
    semver->sv_patch = config->cf_version.sv_patch;

    return DISIR_STATUS_OK;
}
