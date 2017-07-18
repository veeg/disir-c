// external public includes
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// public disir interface
#include <disir/disir.h>
#include <disir/util.h>
#include <disir/context.h>

// private
#include "context_private.h"
#include "disir_private.h"
#include "config.h"
#include "mold.h"
#include "default.h"
#include "keyval.h"
#include "log.h"
#include "mqueue.h"
#include "restriction.h"


// INTERNAL STATIC
static enum disir_status
generate_config_from_mold_recursive_step (struct disir_context *mold_parent,
                                          struct disir_context *config_parent,
                                          struct disir_version *version)
{
    enum disir_status status;
    struct disir_collection *collection;
    struct disir_context *equiv;
    struct disir_context *context;
    struct disir_default *def;
    const char *name;
    int32_t size;
    int min_entries;
    int i;

    // Get each element from the element storage of mold_parent
    status = dc_get_elements (mold_parent, &collection);
    if (status != DISIR_STATUS_OK)
    {
        goto error;
    }

    while (dc_collection_next (collection, &equiv) != DISIR_STATUS_EXHAUSTED)
    {

        status = dx_restriction_entries_value (equiv, DISIR_RESTRICTION_INC_ENTRY_MIN,
                                               version, &min_entries);
        if (status != DISIR_STATUS_OK)
        {
            // XXX
            goto error;
        }
        if (min_entries == 0)
        {
            min_entries = 1;
        }

        // Generate min_entries entries of this element.
        for (i = 0; i < min_entries; i++)
        {
            status = dc_begin (config_parent, dc_context_type (equiv), &context);
            if (status != DISIR_STATUS_OK)
            {
                // Already logged
                goto error;
            }

            status = dc_get_name (equiv, &name, &size);
            if (status != DISIR_STATUS_OK)
            {
                log_debug (2, "failed to get name (%s). output size: %d\n",
                           disir_status_string (status), size);
                goto error;
            }

            status = dc_set_name (context, name, size);
            if (status != DISIR_STATUS_OK)
            {
                log_debug (2, "failed to add name: %s", disir_status_string (status));
                goto error;
            }

            if (dc_context_type (equiv) == DISIR_CONTEXT_KEYVAL)
            {
                // Get default entry matching version
                //

                dx_default_get_active (equiv, version, &def);

                status = dx_value_copy (&context->cx_keyval->kv_value, &def->de_value);
                if (status != DISIR_STATUS_OK)
                {
                    log_debug (2, "failed to copy value: %s", disir_status_string (status));
                    goto error;
                }
            }
            else if (dc_context_type (equiv) == DISIR_CONTEXT_SECTION)
            {
                // Send down parent and context -
                generate_config_from_mold_recursive_step (equiv, context, version);
            }

            status = dc_finalize (&context);
            if (status != DISIR_STATUS_OK)
            {
                goto error;
            }
        }

        dc_putcontext (&equiv);
    }

    status = DISIR_STATUS_OK;
error:
    if (collection)
    {
        dc_collection_finished (&collection);
    }

    return status;

}

//! PUBLIC API
enum disir_status
disir_generate_config_from_mold (struct disir_mold *mold, struct disir_version *config_version,
                                 struct disir_config **config)
{
    enum disir_status status;
    struct disir_context *config_context;
    char buffer[512];
    struct disir_version version;

    TRACE_ENTER ("mold: %p, version: %p", mold, config_version);

    if (mold == NULL)
    {
        log_debug (0, "invoked with mold NULL pointer");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    // TODO: Validate integrity of mold first?

    status = dc_config_begin (mold, &config_context);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }

    // Set version to config.
    if (config_version == NULL)
    {
        status = dc_get_version (mold->mo_context, &version);
        if (status != DISIR_STATUS_OK)
        {
            // Already logged
            return status;
        }
    }
    else
    {
        dc_version_set (&version, config_version);
        if (status != DISIR_STATUS_OK)
        {
            // Already logged ?
            return status;
        }
    }
    status = dc_set_version (config_context, &version);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }

    generate_config_from_mold_recursive_step (mold->mo_context, config_context, &version);

    status = dc_config_finalize (&config_context, config);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        goto error;
    }

    if (config_version)
    {
        dc_version_set (&(*config)->cf_version, config_version);
    }
    else
    {
        dc_version_set (&(*config)->cf_version, &mold->mo_version);
    }
    log_debug (6, "sat config version to: %s",
               dc_version_string (buffer, 32, &(*config)->cf_version));

    TRACE_EXIT ("config: %p", *config);
    return DISIR_STATUS_OK;
error:
    if (config_context)
    {
        dc_destroy (&config_context);
    }

    return status;
}


