#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <disir/disir.h>
#include <disir/io.h>

#include "log.h"
#include "schema.h"
#include "config.h"


enum disir_status
dio_print_config_write (const char *id, struct disir_config *config)
{
    enum disir_status status;
    dc_t *config_context;
    dc_t *context;
    dcc_t *collection;
    int32_t size;
    const char *name;
    const char *value;
    char buffer[512];
    struct semantic_version semver;


    config_context = dc_config_getcontext (config);

    status = dc_get_elements (config_context, &collection);
    if (status != DISIR_STATUS_OK)
    {
        fprintf (stderr, "Buuhuu: failed to get elements: (%d)\n", status);
        return status;
    }

    status = dc_get_version (config_context, &semver);
    if (status != DISIR_STATUS_OK)
    {
        fprintf (stderr, "Failed to get version of config..\n");
    }

    fprintf (stdout, "# Config ID: %s\n", id);
    fprintf (stdout, "# Config version: %s\n",
             dc_semantic_version_string (buffer, 512, &semver));

    while (dc_collection_next (collection, &context) == DISIR_STATUS_OK)
    {
        status = dc_get_name (context, &name, &size);
        if (status != DISIR_STATUS_OK)
        {
            fprintf (stdout, "(fetching name failed");
        }
        else
        {
            fprintf (stdout, "%s = ", name);
        }

        // TODO: Switch on type
        status = dc_get_value_string (context, &value, &size);
        if (status != DISIR_STATUS_OK)
        {
            fprintf (stdout, "(fetching value failed)\n");
        }
        else
        {
            fprintf (stdout, "%s\n", value);
        }
    }

    dc_collection_finished (&collection);
    dc_putcontext (&config_context);

    return DISIR_STATUS_OK;
}

enum disir_status
dio_print_schema_write (const char *id, struct disir_schema *schema)
{
    enum disir_status status;
    dc_t *schema_context;
    dc_t *context;
    dc_t *def;
    dcc_t *collection;

    int32_t size;
    const char *name;
    int32_t name_size;
    const char *doc;
    char buffer[512];
    struct semantic_version semver;
    dcc_t *defaults_collection;

    memset (&semver, 0, sizeof (struct semantic_version));
    memset (buffer, 0, 512);
    defaults_collection = NULL;
    collection = NULL;

    schema_context = dc_schema_getcontext (schema);

    status = dc_get_elements (schema_context, &collection);
    if (status != DISIR_STATUS_OK)
    {
        fprintf (stderr, "Buuhuu: failed to get elements: (%d)\n", status);
        return status;
    }

    fprintf (stdout, "Schema ID: %s\n", id);

    while (dc_collection_next (collection, &context) == DISIR_STATUS_OK)
    {
        // TODO: Check status output
        // TODO: Verify that context is valid before print?
        status = dc_get_name (context, &name, &name_size);
        if (status != DISIR_STATUS_OK)
        {
            fprintf (stdout, "(fetching name failed");
        }
        else
        {
            fprintf (stdout, "keyval: %s", name);
        }

        status = dc_get_introduced (context, &semver);
        if (status != DISIR_STATUS_OK)
        {
            fprintf (stdout, " (fetching introduced failed: %s)", disir_status_string (status));
        }
        else
        {
            fprintf (stdout, " (%s)", dc_semantic_version_string (buffer, 512, &semver));
        }

        // TODO: get (potentially) deprecrated
        //
        status = dc_get_documentation (context, NULL, &doc, &size);
        if (status != DISIR_STATUS_OK)
        {
            fprintf (stdout, " - (fetching documentation failed");
        }
        else
        {
            fprintf (stdout, " - %s\n", doc);
        }

        status = dc_get_default_contexts (context, &defaults_collection);
        if (status != DISIR_STATUS_OK)
        {
            fprintf (stdout, "  (failed to get defaults: %s)\n", disir_status_string (status));
            continue;
        }
        while (dc_collection_next (defaults_collection, &def) != DISIR_STATUS_EXHAUSTED)
        {
            status = dc_get_default (def, NULL, 512, buffer, &size);
            if (status != DISIR_STATUS_OK)
            {
                snprintf (buffer, 512, "(fetch default value failed: %s)",
                          disir_status_string (status));
            }
            fprintf (stdout, "  default(%s): %s",
                     dc_value_type_string (def), buffer);
            status = dc_get_introduced (def, &semver);
            if (status != DISIR_STATUS_OK)
            {
                fprintf (stdout, " (fetching introduced failed: %s)",
                         disir_status_string (status));
            }
            else
            {
                fprintf (stdout, " (%s)", dc_semantic_version_string (buffer, 512, &semver));
            }
            // TODO: get (potentially) deprecrated

            fprintf (stdout, "\n");
        }
        dc_collection_finished (&defaults_collection);
    }

    dc_collection_finished (&collection);

    return DISIR_STATUS_OK;
}

enum disir_status
dio_register_print (struct disir *disir)
{
    enum disir_status status;

    log_info ("Registering PRINT output type");


    status = disir_register_output (disir, "PRINT", "Print the object content to STDOUT",
                                    dio_print_config_write, dio_print_schema_write);
    return status;

}
