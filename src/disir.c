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
#include "schema.h"
#include "default.h"
#include "keyval.h"
#include "log.h"
#include "util_private.h"

#define INTERNAL_SCHEMA_DOCSTRING "The Disir Schema for the internal libdisir configuration."
#define LOG_FILEPATH_DOCSTRING "The full filepath to the logfile libdisir will output to."
#define SCHEMA_DIRPATH_DOCSTRING "The full directory path where libdisir will locate " \
            "it the installed schemas to match against installed configuration files."


enum disir_status
dx_internal_schema (struct disir_schema **schema)
{
    enum disir_status status;
    dc_t *context;

    context = NULL;

    status = dc_schema_begin (&context);
    if (status != DISIR_STATUS_OK)
        return status;

    status = dc_add_documentation (context,
                                   INTERNAL_SCHEMA_DOCSTRING,
                                   strlen (INTERNAL_SCHEMA_DOCSTRING));
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_keyval_string (context, "log_filepath", "/var/log/disir.log",
                                   "Log filepath for the internal disir log", NULL);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_keyval_string (context, "schema_dirpath", "/etc/disir/schemas",
                                   "Root directory to resolve schema lookups from.", NULL);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_schema_finalize (&context, schema);
    if (status != DISIR_STATUS_OK)
        goto error;

    return DISIR_STATUS_OK;
error:
    if (context)
    {
        dc_destroy (&context);
    }

    *schema = NULL;
    return status;
}

// PUBLIC API
enum disir_status
disir_instance_create (struct disir **disir)
{
    enum disir_status status;
    struct disir *dis;

    dis = calloc (1, sizeof (struct disir));
    if (dis == NULL)
    {
        return DISIR_STATUS_NO_MEMORY;
    }

    status = dio_register_print (dis);
    if (status != DISIR_STATUS_OK)
    {
        goto error;
    }

    status = dio_register_ini (dis);
    if (status != DISIR_STATUS_OK)
    {
        goto error;
    }

    // TODO: Load internal schema
    // TODO: Load external config file

    *disir = dis;
    return DISIR_STATUS_OK;
error:
    if (dis)
    {
        free (dis);
    }

    return status;
}

//! PUBLIC API
enum disir_status
disir_instance_destroy (struct disir **disir)
{
    if (disir == NULL || *disir == NULL)
        return DISIR_STATUS_INVALID_ARGUMENT;

    // TODO: Remove allocated input/output instances
    free (*disir);

    *disir = NULL;
    return DISIR_STATUS_OK;
}

//! PUBLIC API
enum disir_status
disir_generate_config_from_schema (struct disir_schema *schema, struct semantic_version *semver,
                                   struct disir_config **config)
{
    enum disir_status status;
    dc_t *config_context;
    dc_t *context;
    dc_t *parent;
    dcc_t *collection;
    char buffer[512];
    const char *name;
    int32_t size;

    TRACE_ENTER ("schema: %p, semver: %p", schema, semver);

    if (schema == NULL)
    {
        log_debug ("invoked with schema NULL pointer");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    // TODO: Validate integrity of schema first?

    status = dc_config_begin (schema, &config_context);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }

    // Get each element from the element storage. add it
    status = dc_get_elements (schema->sc_context, &collection);
    if (status != DISIR_STATUS_OK)
    {
        goto error;
    }

    while (dc_collection_next (collection, &parent) != DISIR_STATUS_EXHAUSTED)
    {
        status = dc_begin (config_context, dc_type (parent), &context);
        if (status != DISIR_STATUS_OK)
        {
            // Already logged
            goto error;
        }

        status = dc_get_name (parent, &name, &size);
        if (status != DISIR_STATUS_OK)
        {
            log_debug ("failed to get name (%s). output size: %d\n",
                       disir_status_string (status), size);
            goto error;
        }

        status = dc_set_name (context, name, size);
        if (status != DISIR_STATUS_OK)
        {
            log_debug ("failed to add name: %s", disir_status_string (status));
            goto error;
        }

        status = dc_get_default (parent, semver, 512, buffer, &size);
        if (status != DISIR_STATUS_OK || size >= 512)
        {
            log_debug ("failed to get default (%s). output size: %d",
                       disir_status_string (status), size);
            goto error;
        }

        status = dc_set_value_string (context, buffer, size);
        if (status != DISIR_STATUS_OK)
        {
            log_debug ("failed to add default: %s", disir_status_string (status));
            goto error;
        }

        status = dc_finalize (&context);
        if (status != DISIR_STATUS_OK)
        {
            goto error;
        }
    }

    status = dc_config_finalize (&config_context, config);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        goto error;
    }

    if (semver)
    {
        dx_semantic_version_set (&(*config)->cf_version, semver);
    }
    else
    {
        dx_semantic_version_set (&(*config)->cf_version, &schema->sc_version);
    }
    log_debug ("sat config version to: %s",
               dc_semantic_version_string (buffer, 32, &(*config)->cf_version));

    dc_collection_finished (&collection);

    TRACE_EXIT ("config: %p", *config);
    return DISIR_STATUS_OK;
error:
    if (config_context)
    {
        dc_destroy (&config_context);
    }
    if (context)
    {
        dc_destroy (&context);
    }
    if (collection)
    {
        dc_collection_finished (&collection);
    }

    return status;
}

