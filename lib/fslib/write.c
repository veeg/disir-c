// public
#include <disir/fslib/util.h>

// system
#include <errno.h>
#include <limits.h>


//! FSLIB API
enum disir_status
fslib_plugin_config_write (struct disir_instance *instance, struct disir_register_plugin *plugin,
                           const char *entry_id, struct disir_config *config,
                           dio_serialize_config func_serialize)
{
    enum disir_status status;
    char filepath[PATH_MAX];
    struct stat statbuf;

    status = plugin->dp_mold_query (instance, plugin, entry_id, NULL);
    if (status == DISIR_STATUS_NOT_EXIST)
    {
        disir_error_set (instance, "there exists no mold for entry %s", entry_id);
        return DISIR_STATUS_MOLD_MISSING;
    }
    if (status != DISIR_STATUS_EXISTS)
    {
        // Unknown error ?
        return status;
    }

    status = fslib_config_resolve_filepath (instance, plugin, entry_id, filepath);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }

    status = fslib_stat_filepath (instance, filepath, &statbuf);
    if (status == DISIR_STATUS_NOT_EXIST)
    {
        // Create the directory strcutreu
        char dirpath[PATH_MAX];
        char *separator = NULL;
        int res;

        res = snprintf (dirpath, PATH_MAX, "%s/%s", plugin->dp_config_base_id, entry_id);
        if (res >= 4096)
        {
            disir_error_set (instance, "filepath exceeded internal buffer of %d bytes", PATH_MAX);
            return DISIR_STATUS_INSUFFICIENT_RESOURCES;
        }

        // Reverse search for directory separator
        separator = strrchr (dirpath, '/');
        if (separator == NULL)
        {
            // Mamma mia! What does this entail?
            disir_error_set (instance, "unable to determine directory location for entry: '%s'",
                             dirpath);
            return DISIR_STATUS_FS_ERROR;
        }

        *separator = '\0';

        status = fslib_mkdir_p (instance, dirpath);
        if (status != DISIR_STATUS_OK)
        {
            return status;
        }
    }
    else if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }
    else
    {
        // Check if stat'd file is a directory - that should fail
        if (S_ISREG (statbuf.st_mode) == 0)
        {
            disir_error_set (instance, "resolved filepath is not a file: %s", filepath);
            return DISIR_STATUS_FS_ERROR;
        }
    }

    FILE *file;

    file = fopen (filepath, "w+");
    if (file == NULL)
    {
        // TODO: Check errno and set appropriate error
        // TODO: Use threadsafe strerror, or refactor entirely
        disir_error_set (instance, "opening for writing %s: %s", filepath, strerror (errno));
        return DISIR_STATUS_FS_ERROR;
    }

    status = func_serialize (instance, config, file);
    fclose (file);

    return status;
}

//! FSLIB API
enum disir_status
fslib_plugin_mold_write (struct disir_instance *instance, struct disir_register_plugin *plugin,
                         const char *entry_id, struct disir_mold *mold,
                         dio_serialize_mold func_serialize)
{
    enum disir_status status;
    char filepath[PATH_MAX];
    struct stat statbuf;

    status = fslib_mold_resolve_filepath (instance, plugin, entry_id, filepath);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }

    status = fslib_stat_filepath (instance, filepath, &statbuf);
    if (status == DISIR_STATUS_NOT_EXIST)
    {
        // Create the directory strcutreu
        char dirpath[PATH_MAX];
        char *separator = NULL;
        int res;

        res = snprintf (dirpath, PATH_MAX, "%s/%s", plugin->dp_mold_base_id, entry_id);
        if (res >= 4096)
        {
            disir_error_set (instance, "filepath exceeded internal buffer of %d bytes", PATH_MAX);
            return DISIR_STATUS_INSUFFICIENT_RESOURCES;
        }

        // Reverse search for directory separator
        separator = strrchr (dirpath, '/');
        if (separator == NULL)
        {
            // Mamma mia! What does this entail?
            disir_error_set (instance, "unable to determine directory location for entry: '%s'",
                             dirpath);
            return DISIR_STATUS_FS_ERROR;
        }

        *separator = '\0';

        status = fslib_mkdir_p (instance, dirpath);
        if (status != DISIR_STATUS_OK)
        {
            return status;
        }
    }
    else if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }
    else
    {
        // Check if stat'd file is a directory - that should fail
        if (S_ISREG (statbuf.st_mode) == 0)
        {
            disir_error_set (instance, "resolved filepath is not a file: %s", filepath);
            return DISIR_STATUS_FS_ERROR;
        }
    }

    FILE *file;

    file = fopen (filepath, "w+");
    if (file == NULL)
    {
        // TODO: Check errno and set appropriate error
        // TODO: Use threadsafe strerror, or refactor entirely
        disir_error_set (instance, "opening for writing %s: %s", filepath, strerror (errno));
        return DISIR_STATUS_FS_ERROR;
    }

    status = func_serialize (instance, mold, file);
    fclose (file);

    return status;
}

//! FSLIB API
enum disir_status
fslib_plugin_config_remove (struct disir_instance *instance,
                            struct disir_register_plugin *plugin,
                            const char *entry_id)
{
    enum disir_status status;
    char filepath[PATH_MAX];
    struct stat statbuf;

    status = plugin->dp_mold_query (instance, plugin, entry_id, NULL);
    if (status == DISIR_STATUS_NOT_EXIST)
    {
        disir_error_set (instance, "there exists no mold for entry %s", entry_id);
        return DISIR_STATUS_MOLD_MISSING;
    }
    if (status != DISIR_STATUS_EXISTS)
    {
        // Unknown error ?
        return status;
    }

    status = fslib_config_resolve_filepath (instance, plugin, entry_id, filepath);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }

    status = fslib_stat_filepath (instance, filepath, &statbuf);
    if (status == DISIR_STATUS_OK)
    {
        // Delete the file
        remove (filepath);
    }
    else if (status == DISIR_STATUS_NOT_EXIST)
    {
        disir_error_set (instance, "these exists no config for entry %s", entry_id);
    }

    return status;
}
