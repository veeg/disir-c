// external public includes
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

// public disir interface
#include <disir/disir.h>
#include <disir/util.h>
#include <disir/context.h>
#include <disir/fslib/toml.h>

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
    struct disir_context *context_section;

    context = NULL;

    status = dc_mold_begin (&context);
    if (status != DISIR_STATUS_OK)
        return status;

    status = dc_add_documentation (context,
                                   INTERNAL_MOLD_DOCSTRING,
                                   strlen (INTERNAL_MOLD_DOCSTRING));
    if (status != DISIR_STATUS_OK)
        goto error;

    // Section defining plugins
    status = dc_begin (context, DISIR_CONTEXT_SECTION, &context_section);
    if (status != DISIR_STATUS_OK)
        goto error;
    status = dc_set_name (context_section, "plugin", strlen ("plugin"));
    if (status != DISIR_STATUS_OK)
        goto error;
    status = dc_add_restriction_entries_max (context_section, 0, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_keyval_string (context_section, "group_id", "",
                                   "Which group this plugin shall identify with.", NULL, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_keyval_string (context_section, "io_id", "",
                                   "A unique, human-readable, identifier for this plugin instance.",
                                   NULL, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_keyval_string (context_section, "config_base_id", "/etc/disir/configs",
                                   "The base identifier used to resolve config entries." \
                                   " For filesystem based plugins, this will be the base" \
                                   " directory path where configuration file entries are located.",
                                   NULL, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_keyval_string (context_section, "mold_base_id", "/etc/disir/configs",
                                   "The base identifier used to resolve mold entries." \
                                   " For filesystem based plugins, this will be the base" \
                                   " directory path where mold definition entries are located.",
                                   NULL, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_keyval_string (context_section, "plugin_filepath", "/usr/lib/disir/plugins/",
                                   "Filepath to specified I/O plugin shared library", NULL, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_finalize (&context_section);
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

//! PUBLIC API
enum disir_status
disir_libdisir_config_from_disk (struct disir_instance *instance, const char *filepath,
                                 struct disir_mold *mold, struct disir_config **config)
{
    enum disir_status status;
    FILE *file;
    int errsv;

    file = fopen (filepath, "r");
    if (file == NULL)
    {
        errsv = errno;
        log_error ("failed to open libdisir config file: %s", strerror (errsv));
        // XXX: This return code should probably be better...
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    status = dio_toml_unserialize_config (instance, file, mold, config);

    fclose (file);

    return status;
}

//! PUBLIC API
enum disir_status
disir_libdisir_config_to_disk (struct disir_instance *instance, struct disir_config *config,
                               const char *filepath)
{
    enum disir_status status;
    FILE *file;
    int errsv;

    file = fopen (filepath, "w+");
    if (file == NULL)
    {
        errsv = errno;
        log_error ("failed to open libdisir config file: %s", strerror (errsv));
        // XXX: This return code should probably be better...
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    status = dio_toml_serialize_config (instance, config, file);
    fclose (file);

    return status;
}

