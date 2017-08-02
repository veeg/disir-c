#include <stdlib.h>
#include <string.h>

#include <disir/disir.h>

#include "disir_private.h"
#include "log.h"
#include "mqueue.h"

#define PLUGIN_STRING_MEMBER_COPY(name) { do {              \
    internal->pi_plugin.name = strndup (plugin->name, 512); \
    if (internal->pi_plugin.name == NULL)                   \
    {                                                       \
        status = DISIR_STATUS_NO_MEMORY;                    \
        goto error;                                         \
    }                                                       \
    } while (0);}

//! PUBLIC API
enum disir_status
disir_plugin_register (struct disir_instance *instance, struct disir_register_plugin *plugin,
                       const char *io_id, const char *group_id)
{
    enum disir_status status;
    struct disir_register_plugin_internal *internal;

    internal = NULL;

    if (instance == NULL || plugin == NULL || io_id == NULL || group_id == NULL)
    {
        log_debug (0, "invoked with NULL pointer(s). (%p %p %p %p)",
                   instance, plugin, io_id, group_id);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    // TODO: Check version compatibility

    internal = calloc (1, sizeof (struct disir_register_plugin_internal));
    if (internal == NULL)
    {
        status = DISIR_STATUS_NO_MEMORY;
        goto error;
    }

    // Copy the plugin input structure verbatim to our internal copy.
    memcpy (&internal->pi_plugin, plugin, sizeof (*plugin));

    internal->pi_io_id = strdup (io_id);
    internal->pi_group_id = strdup (group_id);

    // Make actual copies of the strings, since we do not own the strings
    // in the input plugin
    // NOTE: These macros route control flow to error:
    PLUGIN_STRING_MEMBER_COPY (dp_name);
    PLUGIN_STRING_MEMBER_COPY (dp_description);
    PLUGIN_STRING_MEMBER_COPY (dp_config_entry_type);
    PLUGIN_STRING_MEMBER_COPY (dp_mold_entry_type);

    log_info ("[register plugin] name: %s", internal->pi_plugin.dp_name);
    log_info ("[register plugin] description: %s", internal->pi_plugin.dp_description);
    log_info ("[register plugin] config_base_id: %s", internal->pi_plugin.dp_config_base_id);
    log_info ("[register plugin] config_entry_type: %s", internal->pi_plugin.dp_config_entry_type);
    log_info ("[register plugin] mold_base_id: %s", internal->pi_plugin.dp_mold_base_id);
    log_info ("[register plugin] mold_entry_type: %s", internal->pi_plugin.dp_mold_entry_type);

    MQ_ENQUEUE (instance->dio_plugin_queue, internal);

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
    if (internal->pi_plugin.dp_config_entry_type)
    {
        free (internal->pi_plugin.dp_config_entry_type);
    }
    if (internal->pi_plugin.dp_mold_entry_type)
    {
        free (internal->pi_plugin.dp_mold_entry_type);
    }
    if (internal)
    {
        free (internal);
    }

    return status;
}

// PUBLIC API
enum disir_status
disir_plugin_registered (struct disir_instance *instance, struct disir_plugin **plugins)
{
    enum disir_status status;
    struct disir_register_plugin_internal *internal;
    struct disir_plugin *head, *current;

    TRACE_ENTER ("instance (%p) plugins (%p)", instance, plugins);
    if (instance == NULL || plugins == NULL)
    {
        status = DISIR_STATUS_INVALID_ARGUMENT;
        goto error;
    }

    head = NULL;
    internal = instance->dio_plugin_queue;
    do
    {
        if (internal == NULL)
            break;

        current = calloc (1, sizeof (struct disir_plugin));
        if (current == NULL)
            break;

        current->pl_group_id = strdup (internal->pi_group_id);
        MQ_ENQUEUE (head, current);

        internal = internal->next;
    } while (1);

    *plugins = head;
    status = DISIR_STATUS_OK;

    // FALL-THROUGH
error:
    TRACE_EXIT ("%s", disir_status_string (status));
    return status;
}

// PUBLIC API
enum disir_status
disir_plugin_finished (struct disir_plugin **plugin)
{
    if (plugin && *plugin)
    {
        free ((*plugin)->pl_group_id);
        free (*plugin);
    }

    return DISIR_STATUS_OK;
}

//! INTERNAL API
enum disir_status
dx_retrieve_plugin_by_group (struct disir_instance *instance, const char *group_id,
                             struct disir_register_plugin **plugin)
{
    if (instance == NULL || group_id == NULL || plugin == NULL)
    {
        log_debug (0, "invoked with NULL pointer(s). (%p %p %p)",
                   instance, group_id, plugin);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    *plugin = NULL;

    MQ_FOREACH (instance->dio_plugin_queue,
    ({
        if (strcmp (entry->pi_group_id, group_id) != 0)
        {
            entry = entry->next;
            continue;
        }

        *plugin = &entry->pi_plugin;
        break;
    }));

    return DISIR_STATUS_OK;
}

