#include <stdlib.h>
#include <string.h>

#include <disir/disir.h>

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
    struct disir_register_plugin_internal *plugin;

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

    disir_error_clear (instance);

    MQ_FOREACH (instance->dio_plugin_queue,
    ({
        if (strcmp (entry->pi_group_id, group_id) != 0)
        {
            entry = entry->next;
            continue;
        }
        if (entry->pi_plugin.dp_mold_query == NULL)
        {
            log_debug (1, "Plugin '%s' does not implement mold_query", entry->pi_io_id);
            entry = entry->next;
            continue;
        }

        status = entry->pi_plugin.dp_mold_query (instance, &entry->pi_plugin, entry_id, NULL);
        if (status == DISIR_STATUS_NOT_EXIST)
        {
            entry = entry->next;
            continue;
        }
        if (status != DISIR_STATUS_EXISTS)
        {
            log_warn ("Plugin '%s' (id %s) failed to query for entry '%s': %s",
                      entry->pi_io_id, entry->pi_plugin.dp_name,
                      entry_id), disir_status_string (status);
            entry = entry->next;
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
            status = plugin->pi_plugin.dp_mold_read (instance, &plugin->pi_plugin,
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
    struct disir_register_plugin_internal *plugin;

    plugin = NULL;

    if (instance == NULL || entry_id == NULL || mold == NULL)
    {
        log_debug (0, "invoked with NULL argument(s). instance (%p), entry_id (%p), mold (%p)",
                      instance, entry_id, mold);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    TRACE_ENTER ("instance (%p) group_id (%s) entry_id (%s) mold (%p)",
                 instance, group_id, entry_id, mold);

    disir_error_clear (instance);

    MQ_FOREACH (instance->dio_plugin_queue,
    ({
        if (strcmp (entry->pi_group_id, group_id) != 0)
        {
            entry = entry->next;
            continue;
        }

        plugin = entry;
        break;
    }));

    if (plugin)
    {
        if (plugin->pi_plugin.dp_mold_write)
        {
            status = plugin->pi_plugin.dp_mold_write (instance, &plugin->pi_plugin,
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

    TRACE_ENTER ("instance (%p) group_id (%s), entries (%p)", instance, group_id, entries);

    disir_error_clear (instance);

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
            entry = entry->next;
            continue;
        }
        if (entry->pi_plugin.dp_mold_entries == NULL)
        {
            log_debug (1, "Plugin '%s' does not implement mold_entries.", entry->pi_io_id);
            entry = entry->next;
            continue;
        }

        status = entry->pi_plugin.dp_mold_entries (instance, &entry->pi_plugin, &query);
        if (status != DISIR_STATUS_OK)
        {
            log_debug (1, "Plugin '%s' queried for mold entries failed with status: %s",
                          entry->pi_io_id, disir_status_string (status));
            entry = entry->next;
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

    TRACE_EXIT ("%s", disir_status_string (status));
    return status;
}

//! PUBLIC API
enum disir_status
disir_mold_query (struct disir_instance *instance, const char *group_id,
                  const char *entry_id, struct disir_entry **entry_internal)
{
    enum disir_status status;
    struct disir_register_plugin_internal *plugin;

    plugin = NULL;

    if (instance == NULL || group_id == NULL || entry_id == NULL)
    {
        log_debug (0, "invoked with NULL argument(s). instance(%p), group_id (%p), entry_id (%p)",
                      instance, group_id, entry_id);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    TRACE_ENTER ("instance (%p) group_id (%s) entry_id (%s) entry_internal (%p)",
                 instance, group_id, entry_id, entry_internal);

    disir_error_clear (instance);

    MQ_FOREACH (instance->dio_plugin_queue,
    ({
        if (strcmp (entry->pi_group_id, group_id) != 0)
        {
            entry = entry->next;
            continue;
        }

        if (entry->pi_plugin.dp_mold_query == NULL)
        {
            log_debug (1, "Plugin '%s' does not implement mold_query.", entry->pi_io_id);
            entry = entry->next;
            continue;
        }

        plugin = entry;
    }));

    if (plugin)
    {
        status = plugin->pi_plugin.dp_mold_query (instance, &plugin->pi_plugin,
                                                  entry_id, entry_internal);
        if (status != DISIR_STATUS_EXISTS && status != DISIR_STATUS_NOT_EXIST)
        {
            log_warn ("Plugin '%s' config_query failed with status: %s",
                      plugin->pi_io_id, disir_status_string (status));
        }
        if (status == DISIR_STATUS_NOT_EXIST)
        {
            disir_error_set (instance, "entry_id '%s' in group '%s' does not exist",
                                        entry_id, group_id);
        }
    }
    else
    {
        disir_error_set (instance, "No plugin available to handle operation.");
        status = DISIR_STATUS_GROUP_MISSING;
    }

    TRACE_EXIT ("%s", disir_status_string (status));
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

//! PUBLIC API
enum disir_status
disir_mold_valid (struct disir_mold *mold, struct disir_collection **collection)
{
    enum disir_status status;
    struct disir_collection *col;

    TRACE_ENTER ("mold (%p) collection (%p)", mold, collection);

    col = NULL;
    if (mold == NULL)
    {
        status = DISIR_STATUS_INVALID_ARGUMENT;
        goto error;
    }

    if (collection)
    {
        col = dc_collection_create ();
    }

    status = dx_invalid_elements (mold->mo_context, col);

    if (collection)
    {
        if (dc_collection_size (col))
        {
            *collection = col;
        }
        else
        {
            dc_collection_finished (&col);
        }
    }

    // FALL-THROUGH
error:
    TRACE_EXIT ("%s", disir_status_string (status));
    return status;
}

