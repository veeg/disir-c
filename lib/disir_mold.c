#include <stdlib.h>
#include <string.h>

#include <disir/disir.h>
#include <disir/io.h>

#include "disir_private.h"
#include "log.h"
#include "mold.h"
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
disir_mold_read (struct disir_instance *instance, const char *group_id,
                 const char *entry_id, struct disir_mold **mold)
{
    enum disir_status status;
    struct disir_plugin_internal *plugin;

    plugin = NULL;

    if (instance == NULL || entry_id == NULL || mold == NULL)
    {
        log_debug (0, "invoked with NULL argument(s)." \
                      " instance (%p), group_id (%p), entry_id (%p), mold (%p)",
                      instance, group_id, entry_id, mold);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    TRACE_ENTER ("instance (%p) group_id (%s) entry_id (%s) mold (%p)",
                 instance, group_id, entry_id, mold, mold);

    MQ_FOREACH (instance->dio_plugin_queue,
    ({
        if (strcmp (entry->pi_group_id, group_id) != 0)
        {
            continue;
        }
        if (entry->pi_plugin.dp_mold_query == NULL)
        {
            log_debug (1, "Plugin '%s' does not implement mold_query", entry->pi_io_id);
            continue;
        }
        status = entry->pi_plugin.dp_mold_query (instance, entry->pi_plugin.dp_storage, entry_id);
        if (status == DISIR_STATUS_NOT_EXIST)
            continue;
        if (status != DISIR_STATUS_EXISTS)
        {
            log_warn ("Plugin '%s' (id %s) failed to query for entry '%s': %s",
                      entry->pi_io_id, entry->pi_plugin.dp_name,
                      entry_id), disir_status_string (status);
            continue;
        }

        plugin = entry;
        break;
    }));

    if (plugin)
    {
        if (plugin->pi_plugin.dp_mold_read)
        {
            *mold = NULL;
            status = plugin->pi_plugin.dp_mold_read (instance, plugin->pi_plugin.dp_storage,
                                                     entry_id, mold);
        }
        else
        {
            status = DISIR_STATUS_NO_CAN_DO;
            log_debug (1, "Plugin '%s' does not implement mold_read", plugin->pi_io_id);
        }
    }
    else
    {
        disir_error_set (instance, "No plugin in group '%s' contains mold entry '%s'",
                         group_id, entry_id);
        status = DISIR_STATUS_NOT_EXIST;
    }

    TRACE_EXIT ("status: %s", disir_status_string (status));
    return status;
}

//! PUBLIC API
enum disir_status
disir_mold_write (struct disir_instance *instance, const char *group_id,
                  const char *entry_id, struct disir_mold *mold)
{
    enum disir_status status;
    struct disir_plugin_internal *plugin;

    plugin = NULL;

    if (instance == NULL || entry_id == NULL || mold == NULL)
    {
        log_debug (0, "invoked with NULL argument(s). instance (%p), entry_id (%p), mold (%p)",
                      instance, entry_id, mold);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    TRACE_ENTER ("instance (%p) group_id (%s) entry_id (%s) mold (%p)",
                 instance, group_id, entry_id, mold);

    MQ_FOREACH (instance->dio_plugin_queue,
    ({
        if (strcmp (entry->pi_group_id, group_id) != 0)
        {
            continue;
        }

        plugin = entry;
        break;
    }));

    if (plugin)
    {
        if (plugin->pi_plugin.dp_mold_write)
        {
            status = plugin->pi_plugin.dp_mold_write (instance, plugin->pi_plugin.dp_storage,
                                                      entry_id, mold);
        }
        else
        {
            // TODO: Change status to something more sensible.
            status = DISIR_STATUS_INTERNAL_ERROR;;
            log_debug (1, "Plugin '%s' does not implement mold_write", plugin->pi_io_id);
        }
    }
    else
    {
        disir_error_set (instance, "No plugin in group '%s' available: '%s'.", group_id);
        status = DISIR_STATUS_NOT_EXIST;
    }

    TRACE_EXIT ("status: %s", disir_status_string (status));
    return status;

    return DISIR_STATUS_INTERNAL_ERROR;
}

//! PUBLIC API
enum disir_status
disir_mold_entries (struct disir_instance *instance,
                    const char *group_id, struct disir_entry **entries)
{
    enum disir_status status;
    struct multimap *map;
    struct disir_entry *queue;
    struct disir_entry *query;
    struct disir_entry *current;

    queue = NULL;
    query = NULL;
    current = NULL;

    if (instance == NULL || group_id == NULL || entries == NULL)
    {
        log_debug (0, "invoked with NULL argument(s). instance (%p), group_id (%p), entries (%p)",
                      instance, group_id, entries);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    map = multimap_create ((int (*)(const void *, const void *)) strcmp,
                           (unsigned long (*)(const void*)) djb2);
    if (map == NULL)
    {
        status = DISIR_STATUS_NO_MEMORY;
        goto out;
    }

    MQ_FOREACH (instance->dio_plugin_queue,
    ({
        if (strcmp (entry->pi_group_id, group_id) != 0)
        {
            continue;
        }
        if (entry->pi_plugin.dp_mold_entries == NULL)
        {
            log_debug (1, "Plugin '%s' does not implement mold_entries.", entry->pi_io_id);
            continue;
        }

        status = entry->pi_plugin.dp_mold_entries (instance, entry->pi_plugin.dp_storage, &query);
        if (status != DISIR_STATUS_OK)
        {
            log_debug (1, "Plugin '%s' queried for mold entries failed with status: %s",
                          entry->pi_io_id, disir_status_string (status));
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
disir_mold_finished (struct disir_mold **mold)
{
    enum disir_status status;
    struct disir_context *context;

    status = DISIR_STATUS_OK;

    if (mold == NULL || *mold == NULL)
    {
        log_debug (0, "invoked with NULL mold pointer");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    TRACE_ENTER ("mold: %p", *mold);

    (*mold)->mo_reference_count--;
    if ((*mold)->mo_reference_count == 0)
    {
        log_debug (6, "Mold reached reference count 0 - destroying context.");
        context = (*mold)->mo_context;
        status = dc_destroy (&context);
    }

    if (status == DISIR_STATUS_OK)
        *mold = NULL;

    TRACE_EXIT ("status: %s", disir_status_string (status));
    return status;
}

