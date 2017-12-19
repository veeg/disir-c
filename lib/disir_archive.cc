// disir public
#include <disir/disir.h>
#include <disir/archive.h>
#include <disir/version.h>
#include <disir/fslib/util.h>

// disir private
#include "archive_private.h"
extern "C" {
#include "import.h"
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

//! STATIC FUNCTION
static enum disir_status
archive_import_config_entries (struct disir_instance *instance,
                               struct disir_archive *archive,
                               struct disir_import **import)
{
    enum disir_status status;
    struct disir_import_entry **current;
    struct disir_import_entry *entry;
    struct disir_import *im;

    im = (struct disir_import*)calloc (1, sizeof (disir_import));
    if (im == NULL)
    {
        return DISIR_STATUS_NO_MEMORY;
    }

    im->di_num_entries = archive->da_config_entries->size();

    im->di_entries = (struct disir_import_entry**)calloc (1, sizeof (struct disir_import_entry*)
                                                                     *im->di_num_entries);
    if (im->di_entries == NULL)
    {
        status = DISIR_STATUS_NO_MEMORY;
        goto error;
    }

    current = im->di_entries;
    for (const auto& e : *archive->da_config_entries)
    {
        entry = (struct disir_import_entry*)calloc (1, sizeof (struct disir_import_entry));
        if (entry == NULL)
        {
            status = DISIR_STATUS_NO_MEMORY;
            goto error;
        }

        entry->ie_entry_id = strdup (e.de_entry_id.c_str());
        entry->ie_group_id = strdup (e.de_group_id.c_str());
        entry->ie_backend_id = strdup (e.de_backend_id.c_str());
        entry->ie_version = strdup (e.de_version.c_str());

        status = dx_resolve_config_import_status (instance, e.de_filepath.c_str(), entry);
        if (status != DISIR_STATUS_OK &&
            status != DISIR_STATUS_CONFLICT &&
            status != DISIR_STATUS_NO_CAN_DO &&
            status != DISIR_STATUS_CONFIG_INVALID &&
            status != DISIR_STATUS_CONFLICTING_SEMVER)
        {
            goto error;
        }

        entry->ie_status = status;

        (*current++) = entry;
    }

    *import = im;
    return DISIR_STATUS_OK;
error:
    if (im)
        dx_import_destroy (im);

    return status;
}

//! PUBLIC API
enum disir_status
disir_archive_import (struct disir_instance *instance, const char *archive_path,
                      struct disir_import **import, int *entries)
{
    enum disir_status status;
    struct disir_archive *archive = NULL;
    std::map<std::string, std::string> archive_content;
    char *tmp_dir_name = NULL;

    if (instance == NULL || archive_path == NULL || import == NULL || entries == NULL)
    {
        log_debug (0, "invoked with NULL argument(s). instance(%p), archive_path (%p)," \
                      "import (%p), entries (%p)", instance, archive_path, import, entries);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    status = dx_assert_read_permission (archive_path);
    if (status != DISIR_STATUS_OK)
    {
        disir_error_set (instance, "unable to read archive: '%s'", archive_path);
        return status;
    }

    status = dx_archive_create (&archive);
    if (status != DISIR_STATUS_OK)
    {
        return status;
    }

    // Create temp path for extraction
    tmp_dir_name = dx_archive_create_temp_folder();
    if (tmp_dir_name == NULL)
    {
        dx_archive_destroy (archive);
        return DISIR_STATUS_FS_ERROR;
    }

    archive->da_extract_folder_path = tmp_dir_name;

    status = dx_archive_extract (archive_path, archive_content, tmp_dir_name);
    if (status != DISIR_STATUS_OK)
        goto out;

    status = dx_archive_validate (archive, archive_content);
    if (status != DISIR_STATUS_OK)
    {
        disir_error_set (instance, "archive on path '%s' is invalid", archive_path);
        goto out;
    }

    status = archive_import_config_entries (instance, archive, import);
    if (status != DISIR_STATUS_OK)
        goto out;

    *entries = (*import)->di_num_entries;
    // FALL-THROUGH
out:
    if (archive)
        disir_archive_finalize (instance, NULL, &archive);

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

//! STATIC FUNCTION
static enum disir_status
get_dirname (const char *archive_path, char *dirpath)
{
    enum disir_status status;
    char *temp_path = NULL;
    char *temp_dirname;

    status = DISIR_STATUS_OK;

    temp_path = strdup (archive_path);
    if (temp_path == NULL)
    {
        return DISIR_STATUS_NO_MEMORY;
    }

    temp_dirname = dirname (temp_path);

    if (strcmp (temp_dirname, ".") == 0)
    {
        log_error ("cannot format a directory path of given filepath '%s'", archive_path);
        status = DISIR_STATUS_FS_ERROR;
    }

    strcpy (dirpath, temp_dirname);
    free (temp_path);

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
    int ret;
    struct stat st;
    char extract_folder_path[4096];

    if (instance == NULL || archive == NULL)
    {
        log_debug (0, "invoked with NULL pointer (%p, %p)", instance, archive);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    status = DISIR_STATUS_OK;
    ar = *archive;

    // Make sure archive path is not a directory
    if (archive_path && archive_path[strlen (archive_path)-1] == '/')
    {
        disir_error_set (instance, "invalid archive path: '%s' is a directory", archive_path);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    // Get directory path of location to store new archive
    if (archive_path)
    {
        status = get_dirname (archive_path, extract_folder_path);
        if (status != DISIR_STATUS_OK)
        {
            goto out;
        }

        // Assert that we have write permission in directory
        status = dx_assert_write_permission (extract_folder_path);
        if (status != DISIR_STATUS_OK)
        {
            disir_error_set (instance, "unable to write to path: '%s'", extract_folder_path);
            return status;
        }
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
        status = dx_archive_disk_append (archive_path, ar->da_existing_path,
                                         ar->da_temp_archive_path);
        if (status != DISIR_STATUS_OK)
        {
            disir_error_set (instance, "failed to write archive to disk");
        }
    }
    // FALL-THROUGH
out:
    archive_status = status;

    // Remove temp archive.
    if (ar && ar->da_temp_archive_path &&
        fslib_stat_filepath (instance, ar->da_temp_archive_path, &st) == DISIR_STATUS_OK)
    {
        ret = remove (ar->da_temp_archive_path);
        if (ret != 0)
        {
            log_error ("failed to remove temp archive '%s': %s", ar->da_temp_archive_path,
                       strerror (errno));
            status = DISIR_STATUS_FS_ERROR;
        }
    }
    else
    {
        disir_error_clear (instance);
    }

    status = dx_archive_destroy (ar);
    *archive = NULL;

    return (archive_status != DISIR_STATUS_OK) ? archive_status : status;
}
