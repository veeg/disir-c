// disir puplic
#include <disir/disir.h>
#include <disir/version.h>
#include <disir/archive.h>
#include <disir/fslib/util.h>

// disir private
#include "import.h"
#include "disir_private.h"
#include "query_private.h"
#include "update_private.h"
#include "log.h"
#include "config.h"
#include "mold.h"
#include "mqueue.h"

// external libs
#include <stdlib.h>
#include <limits.h>

//! STATIC FUNCTION
static void
import_info_set (struct disir_import_entry *entry, const char *message, ...)
{
    va_list args;
    char buf[1000];
    int msgsize;

    if (entry == NULL || message == NULL)
        return;

    if (entry->ie_info)
        free (entry->ie_info);

    va_start (args, message);
    msgsize = vsnprintf (buf, 1000, message, args);
    va_end (args);

    entry->ie_info = (char *)malloc (sizeof (char) * msgsize + 1);
    buf[msgsize] = '\0';

    strcpy (entry->ie_info, buf);
}

//! STATIC FUNCTION
static enum disir_status
verify_mold_support (struct disir_instance *instance, struct disir_import_entry *current,
                     struct disir_mold **mold)
{
    enum disir_status status;
    struct disir_register_plugin *plugin = NULL;

    dx_retrieve_plugin_by_group (instance, current->ie_group_id, &plugin);
    if (plugin == NULL)
    {
        log_debug (6, "reqested plugin on group_id '%s' does not exist", current->ie_group_id);
        import_info_set (current, "group '%s' is not supported by system", current->ie_group_id);
        // no plugin for group_id
        return DISIR_STATUS_GROUP_MISSING;
    }

    // backend_id must match plugin id
    if (strcmp (current->ie_backend_id, plugin->dp_name) != 0)
    {
        log_debug (6, "reqested plugin with backend name '%s' does not match '%s'",
                      current->ie_backend_id, plugin->dp_name);
        import_info_set (current, "mismatch between groups on system");
        return DISIR_STATUS_NOT_EXIST;
    }

    if (plugin->dp_mold_read)
    {
        status = plugin->dp_mold_read (instance, plugin, current->ie_entry_id, mold);
    }
    else
    {
        log_debug (6, "reqested plugin with backend name '%s' does not support reading molds",
                      plugin->dp_name);
        // return plugin does not support mold read
        return DISIR_STATUS_NO_CAN_DO;
    }
    if (status == DISIR_STATUS_NOT_EXIST)
    {
        import_info_set (current, "system does not recognize archive entry");
    }

    return status;
}

//! STATIC FUNCTION
static enum disir_status
import_determine_conflict (struct disir_instance *instance,
                           struct disir_import_entry *current,
                           struct disir_mold *mold,
                           struct disir_config *imported)
{
    enum disir_status status;
    enum disir_status status_version;
    struct disir_register_plugin *plugin;
    struct disir_config *existing = NULL;
    struct disir_context *context_existing = NULL;
    struct disir_context *context_imported = NULL;
    char buf[500];
    int res;

    dx_retrieve_plugin_by_group (instance, current->ie_group_id, &plugin);
    if (plugin == NULL)
    {
        log_debug (10, "requested plugin on group '%s' does not exist", current->ie_group_id);
        // no plugin for group_id
        return DISIR_STATUS_GROUP_MISSING;
    }

    // backend_id must match plugin id
    if (strcmp (current->ie_backend_id, plugin->dp_name) != 0)
    {
        log_debug (6, "reqested plugin with backend name '%s' does not match '%s'",
                      current->ie_backend_id, plugin->dp_name);
        import_info_set (current, "mismatch between groups on system");
        return DISIR_STATUS_NO_CAN_DO;
    }

    if (plugin->dp_config_read != NULL)
    {
        status = plugin->dp_config_read (instance, plugin, current->ie_entry_id,
                                         mold, &existing);
    }
    else
    {
        return DISIR_STATUS_NO_CAN_DO;
    }
    if (status == DISIR_STATUS_NOT_EXIST)
    {
        return status;
    }
    else if (status != DISIR_STATUS_OK)
    {
        status = status == DISIR_STATUS_INVALID_CONTEXT ? DISIR_STATUS_CONFIG_INVALID : status;
        goto out;
    }

    status_version = DISIR_STATUS_OK;
    // Compare versions
    res = dc_version_compare (&existing->cf_version, &imported->cf_version);
    if (res > 0)
    {
        import_info_set (current, "version of existing entry (%s) "\
                                  "is higher than archive entry (%s)",
                                  dc_version_string (buf, 256, &existing->cf_version),
                                  dc_version_string (buf + 256, 256, &imported->cf_version));
        current->ie_conflict = 1;
        status_version = DISIR_STATUS_CONFLICTING_SEMVER;
    }
    else if (res < 0)
    {
        import_info_set (current, "version of archive entry (%s) "\
                                  "is higher than existing entry (%s)",
                                  dc_version_string (buf, 256, &imported->cf_version),
                                  dc_version_string (buf + 256, 256, &existing->cf_version));
        current->ie_conflict = 1;
        status_version = DISIR_STATUS_CONFLICTING_SEMVER;

    }

    context_existing = dc_config_getcontext (existing);
    context_imported = dc_config_getcontext (imported);

    status = dc_compare (context_existing, context_imported, NULL);
    if (status == DISIR_STATUS_CONFLICT && status_version == DISIR_STATUS_OK)
    {
        current->ie_conflict = 1;
        import_info_set (current, "archive entry conflicts with existing entry");
    }
    else if (status == DISIR_STATUS_OK && status_version == DISIR_STATUS_OK)
    {
        // Entries are equal, yet we have a conflicts since the config already
        // exists in the system
        current->ie_conflict = 1;
        import_info_set (current, "archive_entry is identical to existing entry");
    }
    status = status == DISIR_STATUS_OK ? status_version : status;
    // FALL-THROUGH
out:
    if (context_existing)
        dc_putcontext (&context_existing);
    if (context_imported)
        dc_putcontext (&context_imported);
    if (existing)
        disir_config_finished (&existing);

    return status;
}

//! STATIC FUNCTION
static enum disir_status
read_config_imported (struct disir_instance *instance, const char *filepath, const char *group_id,
                      struct disir_mold *mold, struct disir_config **config)
{
    enum disir_status status;
    struct disir_register_plugin *plugin;
    FILE *file = NULL;

    dx_retrieve_plugin_by_group (instance, group_id, &plugin);
    if (plugin == NULL)
    {
        return DISIR_STATUS_NO_CAN_DO;
    }

    file = fopen (filepath, "r");
    if (file == NULL)
    {
        log_debug (10, "unable to read imported config on filepath '%s'", filepath);
        return DISIR_STATUS_FS_ERROR;
    }

    if (plugin->dp_config_fd_read != NULL)
    {
        status = plugin->dp_config_fd_read (instance, file, mold, config);
    }
    else
    {
        status = DISIR_STATUS_NO_CAN_DO;
    }

    if (file)
        fclose (file);

    return status;
}

// INTERNAL API
enum disir_status
dx_resolve_config_import_status (struct disir_instance *instance, const char *config_path,
                                 struct disir_import_entry *import)
{
    enum disir_status status;
    struct disir_mold *mold = NULL;
    struct disir_config *config_imported;
    char buf[500];

    status = verify_mold_support (instance, import, &mold);
    if (status != DISIR_STATUS_OK)
    {
        // logged and error set
        return DISIR_STATUS_NO_CAN_DO;
    }

    status = read_config_imported (instance, config_path, import->ie_group_id,
                                   mold, &config_imported);

    import->ie_config = config_imported;

    if (status != DISIR_STATUS_OK)
    {
        // mold has lower version than config
        if (status == DISIR_STATUS_CONFLICTING_SEMVER)
        {
            import_info_set (import, "archive entry is newer than system version");
            status = DISIR_STATUS_NO_CAN_DO;
        }
        // We either got a parse error or config is invalid
        else if (status == DISIR_STATUS_FS_ERROR || status == DISIR_STATUS_INVALID_CONTEXT)
        {
            import_info_set (import, "archive entry is invalid");
            status = DISIR_STATUS_CONFIG_INVALID;
        }
        // Plugin does not support reading from FILE
        else if (status == DISIR_STATUS_NO_CAN_DO)
        {
            import_info_set (import, "group does implement support for import");
        }
        goto out;
    }

    status = import_determine_conflict (instance, import, mold, config_imported);
    if (status != DISIR_STATUS_OK &&
        status != DISIR_STATUS_CONFLICT &&
        status != DISIR_STATUS_CONFLICTING_SEMVER &&
        status != DISIR_STATUS_NOT_EXIST)
    {
        import_info_set (import, "archive entry cannot be imported");
    }
    // If equivalent config does not exist on the system
    if (status == DISIR_STATUS_NOT_EXIST)
    {
        import->ie_conflict = 0;
        status = DISIR_STATUS_OK;

        if (dc_version_compare (&mold->mo_version, &config_imported->cf_version) != 0)
        {
            import_info_set (import, "archive entry version is (%s) but system version is (%s)",
                                     dc_version_string (buf, 256, &config_imported->cf_version),
                                     dc_version_string (buf + 256, 256, &mold->mo_version));
            status = DISIR_STATUS_CONFLICTING_SEMVER;
        }
    }
    // FALL-THROUGH
out:
    if (mold)
        disir_mold_finished (&mold);

    return status;
}

//! PUBLIC API
enum disir_status
disir_import_entry_status (struct disir_import *import, int entry,
                           const char **entry_id, const char **group_id,
                           const char **version, const char **info)
{
    enum disir_status status;
    struct disir_import_entry *current;

    if (import == NULL || entry_id == NULL || group_id == NULL || version == NULL)
    {
        log_debug (0, "invoked with NULL pointers (%p %p %p %p)",
                      import, entry_id, group_id, version);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    if (entry < 0 || entry >= import->di_num_entries)
    {
        log_debug (0, "invoked with index out-of-bounds (%d)", entry);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    current = import->di_entries[entry];

    *entry_id = current->ie_entry_id;
    *group_id = current->ie_group_id;
    *version = current->ie_version;

    if (info)
    {
        *info = NULL;
        if (current->ie_info)
        {
            *info = current->ie_info;
        }
    }

    status = current->ie_status;

    // Archive entry is identical to entry in system, but
    // we mark is as a conflict nonetheless
    if (status == DISIR_STATUS_OK && current->ie_conflict)
    {
        status = DISIR_STATUS_CONFLICT;
    }

    return status;
}

//! PUBLIC API
enum disir_status
disir_import_resolve_entry (struct disir_import *import, int entry, enum disir_import_option opt)
{
    enum disir_status status;
    struct disir_import_entry *current;

    if (import == NULL || entry < 0 || entry >= import->di_num_entries)
    {
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    current = import->di_entries[entry];

    // Configs that does not have mold or are invalid
    // cannot be imported. Therefore we prohibit
    // them from being resolved in any way
    if (current->ie_status == DISIR_STATUS_NO_CAN_DO ||
        current->ie_status == DISIR_STATUS_CONFIG_INVALID)
    {
        return DISIR_STATUS_NO_CAN_DO;
    }

    // if entry is already resolved the only valid option is to discard it
    if (current->ie_status == DISIR_STATUS_OK && current->ie_import &&
        opt != DISIR_IMPORT_DISCARD)
    {
        return DISIR_STATUS_NO_CAN_DO;
    }

    switch (opt)
    {
    // if existing config > imported config - imported config wins
    case DISIR_IMPORT_UPDATE:
    {
        // configs are of different versions - update?
        if (current->ie_status == DISIR_STATUS_CONFLICT ||
            current->ie_status == DISIR_STATUS_CONFLICTING_SEMVER)
        {
            status = dx_update_config_with_changes (&current->ie_config, 0);
            if (status != DISIR_STATUS_OK)
            {
                return status;
            }
        }
        else
        {
            return DISIR_STATUS_NO_CAN_DO;
        }
        break;
    }
    case DISIR_IMPORT_FORCE:
    {
        status = DISIR_STATUS_OK;
        break;
    }
    case DISIR_IMPORT_DO:
    {
        status = DISIR_STATUS_OK;
        // Archive entry is identical to entry on system, this case should
        // be resolved by force.
        if (current->ie_status == DISIR_STATUS_OK && current->ie_conflict)
        {
            status = DISIR_STATUS_NO_CAN_DO;
        }
        //! Invalid to do a regular import if there exist conflicts
        else if (current->ie_status != DISIR_STATUS_OK && current->ie_conflict)
        {
            status = DISIR_STATUS_NO_CAN_DO;
        }
        // if mold version is higher than config, yet no config exists on system
        // config can still be imported without updating it to the newest version
        else if (current->ie_status == DISIR_STATUS_CONFLICTING_SEMVER &&
                 current->ie_conflict == 0)
        {
            status = DISIR_STATUS_OK;
        }
        else if (current->ie_status == DISIR_STATUS_OK && current->ie_conflict == 0)
        {
            status = DISIR_STATUS_OK;
        }
        break;
    }
    case DISIR_IMPORT_DISCARD:
    {
        current->ie_import = 0;
        return DISIR_STATUS_OK;
    }
    case DISIR_IMPORT_UPDATE_WITH_DISCARD:
    {
        if (current->ie_status == DISIR_STATUS_CONFLICT ||
            current->ie_status == DISIR_STATUS_CONFLICTING_SEMVER)
        {
            status = dx_update_config_with_changes (&current->ie_config, 1);
            if (status != DISIR_STATUS_OK)
            {
                return status;
            }
        }
        else
        {
            return DISIR_STATUS_NO_CAN_DO;
        }
        break;
    }
    default:
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    // ALL OK
    if (status == DISIR_STATUS_OK)
    {
        // resolve that config is valid at this point
        status = disir_config_valid (current->ie_config, NULL);
        if (status != DISIR_STATUS_OK)
        {
            return status;
        }
        current->ie_import = 1;
        current->ie_status = DISIR_STATUS_OK;
    }

    return status;
}

//! PUBLIC API
enum disir_status
disir_import_report_destroy (struct import_report **report)
{
    int i;
    struct import_report *rep;

    if (report == NULL)
    {
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    rep = *report;

    for (i = 0; i < rep->ir_internal; i++)
    {
        free (rep->ir_entry[i]);
    }

    free (rep->ir_entry);
    free (rep);

    *report = NULL;

    return DISIR_STATUS_OK;
}

//! PUBLIC API
enum disir_status
disir_import_finalize (struct disir_instance *instance, enum disir_import_option opt,
                       struct disir_import **import, struct import_report **report)
{
    enum disir_status status;
    int i;
    unsigned int failed = 0;
    unsigned int total = 0;
    struct disir_import_entry *current;
    struct import_report *rep = NULL;
    char buf[4096];
    char version[500];

    if (instance == NULL || import == NULL)
    {
        log_debug (0, "invoked with NULL argument(s). instance(%p), import (%p)",
                      instance, import);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    disir_error_clear (instance);

    if (*import == NULL || (*import)->di_entries == NULL)
    {
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    switch (opt)
    {
    case DISIR_IMPORT_DISCARD:
    {
        return dx_import_destroy (*import);
    }
    case DISIR_IMPORT_DO:
    {
        if (report)
        {
            rep = (struct import_report*)calloc (1, sizeof (struct import_report));
            if (rep == NULL)
            {
                return DISIR_STATUS_NO_MEMORY;
            }

            rep->ir_entry = (char**)calloc (1, sizeof (char*) * (*import)->di_num_entries);
            if (rep->ir_entry == NULL)
            {
                status = DISIR_STATUS_NO_MEMORY;
                goto error;
            }

            rep->ir_entries = (*import)->di_num_entries;
            rep->ir_internal = (*import)->di_num_entries;
        }

        for (i = 0; i < (*import)->di_num_entries; i++)
        {
           buf[0] = '\n';
           current = (*import)->di_entries[i];
           if (current == NULL)
               continue;

           // Only import configs that have been resolved
           if (current->ie_status == DISIR_STATUS_OK && current->ie_import)
           {
               status = disir_config_write (instance, current->ie_group_id, current->ie_entry_id,
                                            current->ie_config);

               total++;

               if (status != DISIR_STATUS_OK)
               {
                    failed++;
               }
           }
           if (report)
           {
               if (status != DISIR_STATUS_OK &&
                   current->ie_status == DISIR_STATUS_OK &&
                   current->ie_import)
               {
                   snprintf (buf, 4096, "Failed:         %s", current->ie_entry_id);
                   rep->ir_entry[i] = strdup (buf);
               }
               else if (status == DISIR_STATUS_OK && current->ie_import)
               {
                   snprintf (buf, 4096, "Imported:       %s (%s)",
                                         current->ie_entry_id,
                                         dc_version_string (version, 256,
                                                            &current->ie_config->cf_version));
                   rep->ir_entry[i] = strdup (buf);

               }
                if (current->ie_status != DISIR_STATUS_NO_CAN_DO &&
                    current->ie_status != DISIR_STATUS_CONFIG_INVALID &&
                    current->ie_import == 0)
                {
                   snprintf (buf, 4096, "Discarded:      %s", current->ie_entry_id);
                   rep->ir_entry[i] = strdup (buf);
                }
                if (current->ie_status == DISIR_STATUS_NO_CAN_DO ||
                    current->ie_status == DISIR_STATUS_CONFIG_INVALID)
                {
                   snprintf (buf, 4096, "Not importable: %s", current->ie_entry_id);
                   rep->ir_entry[i] = strdup (buf);
                }
           }
        }
        break;
    }
    default:
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    if (failed)
    {
        disir_error_set (instance, "%d of %d entries were not imported", failed, total);
    }

    status = dx_import_destroy (*import);
    if (status != DISIR_STATUS_OK)
    {
        goto error;
    }

    *import = NULL;

    if (report)
    {
        *report = rep;
    }

    return status;
error:
    if (rep)
    {
        status = disir_import_report_destroy (&rep);
    }
    return status;
}


//! INTERNAL API
enum disir_status
dx_import_destroy (struct disir_import *entry)
{
    int i;

    if (entry == NULL)
        return DISIR_STATUS_OK;

    i = entry->di_num_entries - 1;

    if (entry->di_entries)
    {
        do
        {
            if (entry->di_entries[i] != NULL)
                dx_import_entry_destroy (entry->di_entries[i]);
        }
        while (i--);

        free (entry->di_entries);
    }

    free (entry);

    return DISIR_STATUS_OK;
}

//! INTERNAL API
enum disir_status
dx_import_entry_destroy (struct disir_import_entry *entry)
{
    if (entry->ie_entry_id)
        free (entry->ie_entry_id);
    if (entry->ie_group_id)
        free (entry->ie_group_id);
    if (entry->ie_backend_id)
        free (entry->ie_backend_id);
    if (entry->ie_info)
        free (entry->ie_info);
    if (entry->ie_version)
        free (entry->ie_version);
    if (entry->ie_config)
        disir_config_finished (&entry->ie_config);
    if (entry)
        free (entry);

    return DISIR_STATUS_OK;
}

