#include <stdlib.h>
#include <string.h>

#include <disir/disir.h>
#include <disir/io.h>

#include "config.h"
#include "disir_private.h"
#include "log.h"
#include "mqueue.h"
#include "multimap.h"


// String hashing function for the multimap
// http://www.cse.yorku.ca/~oz/hash.html
static unsigned long djb2 (char *str)
{
    unsigned long hash = 5381;
    char c;
    while( (c = *str++) ) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

//! PUBLIC API
enum disir_status
disir_config_read (struct disir_instance *disir, const char *entry_id,
                   struct disir_mold *mold, struct disir_config **config)
{
    enum disir_status status;
    struct disir_plugin_internal *plugin;

    plugin = NULL;

    if (disir == NULL || entry_id == NULL || config == NULL)
    {
        log_debug (0, "invoked with NULL argument(s). disir (%p), entry_id (%p), config (%p)",
                      disir, entry_id, config);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    TRACE_ENTER ("disir (%p) entry_id (%s) mold (%p) config (%p)", disir, entry_id, mold, config);

    MQ_FOREACH (disir->dio_plugin_queue,
    ({
        if (entry->pi_plugin.dp_config_query == NULL)
        {
            log_debug (1, "Plugin '%s' does not implement config_query", entry->pi_name);
            continue;
        }
        status = entry->pi_plugin.dp_config_query (disir, entry->pi_plugin.dp_storage, entry_id);
        if (status == DISIR_STATUS_NOT_EXIST)
            continue;
        if (status != DISIR_STATUS_EXISTS)
        {
            log_warn ("Plugin '%s' (named %s, type %s) failed to query for entry '%s': %s",
                      entry->pi_name, entry->pi_plugin.dp_name,
                      entry->pi_plugin.dp_type, entry_id), disir_status_string (status);
            continue;
        }

        plugin = entry;
        break;
    }));

    if (plugin)
    {
        if (plugin->pi_plugin.dp_config_read)
        {
            *config = NULL;
            status = plugin->pi_plugin.dp_config_read (disir, plugin->pi_plugin.dp_storage,
                                                       entry_id, mold, config);

            // Config is loaded - we inject the loaded pluginname into the structure.
            if (*config != NULL)
            {
                (*config)->cf_plugin_name = strdup (plugin->pi_name);
            }
        }
        else
        {
            status = DISIR_STATUS_NO_CAN_DO;
            log_debug (1, "Plugin '%s' does not implement config_read", plugin->pi_name);
        }
    }
    else
    {
        disir_error_set (disir, "No plugin contains config entry '%s'", entry_id);
        status = DISIR_STATUS_NOT_EXIST;
    }

    TRACE_EXIT ("status: %s", disir_status_string (status));
    return status;
}

//! PUBLIC API
enum disir_status
disir_config_write (struct disir_instance *disir, const char *entry_id,
                    struct disir_config *config)
{
    enum disir_status status;
    struct disir_plugin_internal *plugin;

    plugin = NULL;

    if (disir == NULL || entry_id == NULL || config == NULL)
    {
        log_debug (0, "invoked with NULL argument(s). disir (%p), entry_id (%p), config (%p)",
                      disir, entry_id, config);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    TRACE_ENTER ("disir (%p) entry_id (%s) config (%p)", disir, entry_id, config);

    if (config->cf_plugin_name == NULL)
    {
        log_warn ("Cannot persis config (%p), missing loaded plugin name.", config);
        disir_error_set (disir, "Config not loaded through plugin API."
                                " Cannot infere origin plugin."
                                " Please use plugin specific write operation instead.");
        return DISIR_STATUS_NO_CAN_DO;
    }

    MQ_FOREACH (disir->dio_plugin_queue,
    ({
        if (strcmp (entry->pi_name, config->cf_plugin_name) == 0)
        {
            plugin = entry;
            break;
        }
    }));

    if (plugin)
    {
        if (plugin->pi_plugin.dp_config_write)
        {
            status = plugin->pi_plugin.dp_config_write (disir, plugin->pi_plugin.dp_storage,
                                                        entry_id, config);
        }
        else
        {
            status = DISIR_STATUS_NO_CAN_DO;
            log_debug (1, "Plugin '%s' does not implement config_write", plugin->pi_name);
        }
    }
    else
    {
        log_warn ("The plugin '%s' associated with Config (%p) is not loaded.",
                  config->cf_plugin_name, config);
        disir_error_set (disir, "No loaded plugin available: '%s'.", config->cf_plugin_name);
        status = DISIR_STATUS_NOT_EXIST;
    }

    TRACE_EXIT ("status: %s", disir_status_string (status));
    return status;
}

//! PUBLIC API
enum disir_status
disir_config_entries (struct disir_instance *disir, struct disir_entry **entries)
{
    enum disir_status status;
    struct multimap *map;
    struct disir_entry *queue;
    struct disir_entry *query;
    struct disir_entry *current;

    queue = NULL;
    query = NULL;
    current = NULL;

    if (disir == NULL || entries == NULL)
    {
        log_debug (0, "invoked with NULL argument(s). disir (%p), entries (%p)", disir, entries);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    map = multimap_create ((int (*)(const void *, const void *)) strcmp,
                           (unsigned long (*)(const void*)) djb2);
    if (map == NULL)
    {
        status = DISIR_STATUS_NO_MEMORY;
        goto out;
    }

    MQ_FOREACH (disir->dio_plugin_queue,
    ({
        if (entry->pi_plugin.dp_config_entries == NULL)
        {
            log_debug (1, "Plugin '%s' does not implement config_entries.", entry->pi_name);
            continue;
        }

        status = entry->pi_plugin.dp_config_entries (disir, entry->pi_plugin.dp_storage, &query);
        if (status != DISIR_STATUS_OK)
        {
            log_warn ("Plugin '%s' queried for config entries failed with status: %s",
                      entry->pi_name, disir_status_string (status));
            continue;
        }

        // iterate each entry, add them to queue if they are unique
        current = query;
        while (current != NULL)
        {

            query = current->next;

            if (multimap_contains_key (map, (void *)current->de_entry_name) == 0)
            {
                // Add entry to queue
                MQ_ENQUEUE (queue, current);
                multimap_push_value (map, (void *)current->de_entry_name, NULL);
            }
            else
            {
                // Duplicate entry that a previous plugin provides. Forget about it.
                disir_entry_finished (&current);
            }

            current = query;
        }
    }));

    *entries = queue;

    status = DISIR_STATUS_OK;
    // FALL-THROUGH
out:
    if (map)
        multimap_destroy (map, NULL, NULL);
    return status;
}

//! PUBLIC API
enum disir_status
disir_config_finished (struct disir_config **config)
{
    enum disir_status status;
    struct disir_context *context;

    if (config == NULL || *config == NULL)
        return DISIR_STATUS_INVALID_ARGUMENT;


    context = (*config)->cf_context;
    status = dc_destroy (&context);
    if (status == DISIR_STATUS_OK)
        *config = NULL;
    return status;
}

