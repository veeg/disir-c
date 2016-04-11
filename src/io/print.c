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
    log_debug ("invoked");
    return DISIR_STATUS_INTERNAL_ERROR;
}

enum disir_status
dio_print_schema_write (const char *id, struct disir_schema *schema)
{
    enum disir_status status;
    dc_t *schema_context;
    dc_t *context;
    dcc_t *collection;

    int32_t size;
    const char *name;
    int32_t name_size;
    const char *doc;

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
        // TODO: Verify that context is valid before print?
        status = dc_get_name (context, &name, &name_size);
        fprintf (stdout, "KEYVAL: %s", name);
        status = dc_get_documentation (context, NULL, &doc, &size);
        fprintf (stdout, " - %s\n", doc);
        // Get doc
        // get introduced
        // get (potentially) deprecrated
        // get collelction of defaults
    }

    status = dc_collection_finished (&collection);

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
