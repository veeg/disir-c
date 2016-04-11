// external public includes
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// public disir interface
#include <disir/disir.h>
#include <disir/context.h>

// private
#include "context_private.h"
#include "disir_private.h"
#include "schema.h"
#include "log.h"

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

