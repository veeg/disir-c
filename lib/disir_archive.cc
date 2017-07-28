// disir public
#include <disir/disir.h>
#include <disir/archive.h>
#include <disir/version.h>
#include <disir/fslib/util.h>

// disir private
#include "archive_private.h"
extern "C" {
#include "disir_private.h"
#include "mqueue.h"
#include "update_private.h"
#include "log.h"
}

// external libs
#include <archive.h>
#include <archive_entry.h>
#include <cstdio>
#include <iostream>


//! PUBLIC API
enum disir_status
disir_archive_export_begin (struct disir_instance *instance,
                            const char *archive_path, struct disir_archive **archive)
{
    enum disir_status status;

    if (instance == NULL || archive == NULL)
    {
        log_debug (0, "invoked with NULL pointers (%p %p %p)",
                      instance, archive_path, archive);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    if (archive_path == NULL)
    {
        status = dx_archive_begin_new (archive);
    }
    else
    {
        status = dx_archive_begin_existing (instance, archive_path, archive);
    }

    return status;
}

//! PUBLIC API
enum disir_status
disir_archive_append_entry (struct disir_instance *instance, struct disir_archive *archive,
                            const char *group_id, const char *entry_id)
{
    enum disir_status status;
    struct disir_entry *config_entry = NULL;
    struct disir_register_plugin *plugin = NULL;

    if (instance == NULL || archive == NULL || group_id == NULL || entry_id == NULL)
    {
        log_debug (0, "invoked with NULL pointers (%p %p %p %p)",
                       instance, archive, group_id, entry_id);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    // Check if entry already exist:
    for (auto iter = archive->da_config_entries->begin();
         iter != archive->da_config_entries->end(); ++iter)
    {
        if (iter->de_group_id == group_id && iter->de_entry_id == entry_id)
        {
            disir_error_set (instance, "entry '%s' already exist in archive", entry_id);
            return DISIR_STATUS_EXISTS;
        }
    }

    status = dx_retrieve_plugin_by_group (instance, group_id, &plugin);
    if (status != DISIR_STATUS_OK)
    {
        return status;
    }

    status = disir_config_query (instance, group_id, entry_id, &config_entry);
    if (status != DISIR_STATUS_OK && status != DISIR_STATUS_EXISTS)
    {
        return status;
    }

    status = dx_archive_config_entries_write (instance, archive, plugin, config_entry, group_id);
    if (status != DISIR_STATUS_OK)
    {
        return status;
    }

    return status;
}

//! PUBLIC API
enum disir_status
disir_archive_append_group (struct disir_instance *instance, struct disir_archive *archive,
                            const char *group_id)
{
    enum disir_status status;
    struct disir_entry *config_entries = NULL;
    struct disir_register_plugin *plugin = NULL;

    if (archive == NULL || instance == NULL || group_id == NULL)
    {
        log_debug (0, "invoked with NULL pointers (%p %p %p)",
                      instance, archive, group_id);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    // Check if group already exist:
    for (auto iter = archive->da_config_entries->begin();
         iter != archive->da_config_entries->end(); ++iter)
    {
        if (iter->de_group_id == group_id)
        {
            disir_error_set (instance, "group '%s' already exist in archive", group_id);
            return DISIR_STATUS_EXISTS;
        }
    }

    status = dx_retrieve_plugin_by_group (instance, group_id, &plugin);
    if (status != DISIR_STATUS_OK)
    {
        return status;
    }

    if (plugin == NULL)
    {
        disir_error_set (instance, "no plugin in group '%s' available.", group_id);
        return DISIR_STATUS_GROUP_MISSING;
    }

    status = disir_config_entries (instance, group_id, &config_entries);
    if (status != DISIR_STATUS_OK)
    {
        return status;
    }

    if (config_entries == NULL)
    {
        return DISIR_STATUS_NOT_EXIST;
    }

    status = dx_archive_config_entries_write (instance, archive, plugin, config_entries, group_id);
    if (status != DISIR_STATUS_OK)
    {
        return status;
    }

    return status;
}

//! PUBLIC API
enum disir_status
disir_archive_finalize (struct disir_instance *instance, const char *archive_path,
                        struct disir_archive **archive)
{
    enum disir_status status;
    enum disir_status archive_status;
    struct disir_archive *ar;
    char temp_archive_path[4096];
    int ret;
    struct stat st;

    if (instance == NULL || archive == NULL)
    {
        log_debug (0, "invoked with NULL pointer (%p, %p)", instance, archive);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    status = DISIR_STATUS_OK;
    ar = *archive;

    // Store for clean-up in the end
    if (ar && ar->da_temp_archive_path)
    {
        strcpy (temp_archive_path, ar->da_temp_archive_path);
    }

    // Check for empty archive
    if (ar && archive_path && ar->da_entries->empty())
    {
        disir_error_set (instance, "cannot finalize empty archive");
        status = DISIR_STATUS_NO_CAN_DO;
    }

    if (ar && archive_path && ar->da_entries && !ar->da_entries->empty())
    {
        status = dx_archive_metadata_write (ar);
        if (status != DISIR_STATUS_OK)
            goto out;
    }

    if (ar && ar->da_archive)
    {
        if (archive_write_close (ar->da_archive) != ARCHIVE_OK)
        {
            log_error ("unable to close archive in finalize: %s",
                        archive_error_string (ar->da_archive));
            status = DISIR_STATUS_FS_ERROR;
            goto out;
        }

        if (archive_write_free (ar->da_archive) != ARCHIVE_OK)
        {
            log_error ("unable to free archive in finalize: %s",
                        archive_error_string (ar->da_archive));
            status = DISIR_STATUS_FS_ERROR;
            goto out;
        }
    }

    if (archive_path && status == DISIR_STATUS_OK)
    {
        status = dx_archive_disk_append (instance, archive_path, ar->da_existing_path,
                                         ar->da_temp_archive_path);
    }
    // FALL-THROUGH
out:
    archive_status = status;

    status = dx_archive_destroy (ar);
    *archive = NULL;

    // Remove temp archive.
    if (fslib_stat_filepath (instance, temp_archive_path, &st) == DISIR_STATUS_OK)
    {
        ret = remove (temp_archive_path);
        if (ret != 0)
        {
            log_error ("failed to remove temp archive '%s': %s", temp_archive_path,
                       strerror (errno));
            status = DISIR_STATUS_FS_ERROR;
        }
    }
    else
    {
        disir_error_clear (instance);
    }

    return (archive_status != DISIR_STATUS_OK) ? archive_status : status;
}
