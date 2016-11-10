#include <stdlib.h>
#include <string.h>

#include <disir/disir.h>

#include "disir_private.h"
#include "log.h"
#include "mqueue.h"

//! PUBLIC API
enum disir_status
disir_plugin_register (struct disir_instance *disir, struct disir_plugin *plugin)
{
    enum disir_status status;
    struct disir_plugin_internal *internal;

    internal = NULL;

    if (disir == NULL || plugin == NULL)
    {
        log_debug (0, "invoked with NULL pointer(s). (%p %p)", disir, plugin);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    // TODO: Check version compatibility

    internal = calloc (1, sizeof (struct disir_plugin_internal));
    if (internal == NULL)
    {
        status = DISIR_STATUS_NO_MEMORY;
        goto error;
    }

    // Copy the plugin input structure verbatim to our internal copy.
    memcpy (&internal->pi_plugin, plugin, sizeof (*plugin));

    // Make actual copies of the strings, since we do not own the strings
    // in the input plugin
    internal->pi_plugin.dp_description = strndup (plugin->dp_description, 512);
    if (internal->pi_plugin.dp_description == NULL)
    {
        status = DISIR_STATUS_NO_MEMORY;
        goto error;
    }
    internal->pi_plugin.dp_name = strndup (plugin->dp_name, 512);
    if (internal->pi_plugin.dp_name == NULL)
    {
        status = DISIR_STATUS_NO_MEMORY;
        goto error;
    }
    internal->pi_plugin.dp_type = strndup (plugin->dp_type, 512);
    if (internal->pi_plugin.dp_type == NULL)
    {
        status = DISIR_STATUS_NO_MEMORY;
        goto error;
    }

    MQ_PUSH (disir->dio_plugin_queue, internal);

    return DISIR_STATUS_OK;

error:
    if (internal->pi_plugin.dp_name)
    {
        free (internal->pi_plugin.dp_name);
    }
    if (internal->pi_plugin.dp_description)
    {
        free (internal->pi_plugin.dp_description);
    }
    if (internal->pi_plugin.dp_type)
    {
        free (internal->pi_plugin.dp_type);
    }
    if (internal)
    {
        free (internal);
    }

    return status;
}

