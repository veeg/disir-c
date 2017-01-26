// external public includes
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// public disir interface
#include <disir/disir.h>
#include <disir/util.h>
#include <disir/context.h>

// private
#include "log.h"

#define INTERNAL_MOLD_DOCSTRING "The Disir Schema for the internal libdisir configuration."
#define LOG_FILEPATH_DOCSTRING "The full filepath to the logfile libdisir will output to."
#define MOLD_DIRPATH_DOCSTRING "The full directory path where libdisir will locate " \
            "it the installed molds to match against installed configuration files."


//! PUBLIC API
enum disir_status
disir_libdisir_mold (struct disir_mold **mold)
{
    enum disir_status status;
    struct disir_context *context;

    context = NULL;

    status = dc_mold_begin (&context);
    if (status != DISIR_STATUS_OK)
        return status;

    status = dc_add_documentation (context,
                                   INTERNAL_MOLD_DOCSTRING,
                                   strlen (INTERNAL_MOLD_DOCSTRING));
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_keyval_string (context, "log_filepath", "/var/log/disir.log",
                                   "Log filepath for the internal disir log", NULL, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_keyval_string (context, "mold_dirpath", "/etc/disir/molds",
                                   "Root directory to resolve mold lookups from.", NULL, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_keyval_string (context, "plugin_filepath", "/usr/lib/disir/plugins/",
                                   "Load specified I/O plugin", NULL, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;


    status = dc_mold_finalize (&context, mold);
    if (status != DISIR_STATUS_OK)
        goto error;

    return DISIR_STATUS_OK;
error:
    if (context)
    {
        dc_destroy (&context);
    }

    *mold = NULL;
    return status;
}

