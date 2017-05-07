// public
#include <disir/fslib/util.h>

// system
#include <errno.h>
#include <limits.h>


//! FSLIB API
enum disir_status
fslib_plugin_config_read (struct disir_instance *instance,
                          struct disir_plugin *plugin, const char *entry_id,
                          struct disir_mold *mold, struct disir_config **config,
                          dio_unserialize_config func_unserialize)
{
    enum disir_status status;
    char filepath[PATH_MAX];
    struct stat statbuf;
    struct disir_mold *resolved_mold;

    status = fslib_config_resolve_filepath (instance, plugin, entry_id, filepath);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }

    // TODO: Verify that filepath exists.
    status = fslib_stat_filepath (instance, filepath, &statbuf);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }

    // Locate mold from plugin
    if (mold == NULL)
    {
        status = plugin->dp_mold_read (instance, plugin, entry_id, &resolved_mold);
        if (status != DISIR_STATUS_OK)
        {
            // XXX - Change any of the error state?
            // TODO: Handle invalid_context? Hmm
            return status;
        }
    }
    else
    {
        resolved_mold = mold;
    }


    FILE *file;
    file = fopen (filepath, "r");
    if (file == NULL)
    {
        // TODO: Check errno and set appropriate error
        // TODO: Use threadsafe strerror, or refactor entirely
        disir_error_set (instance, "opening for reading %s: %s", filepath, strerror (errno));
        return DISIR_STATUS_FS_ERROR;
    }

    status = func_unserialize (instance, file, resolved_mold, config);

    // Cleanup
    fclose (file);
    if (mold == NULL)
    {
        // We only decref the mold if we were the one allocating it
        disir_mold_finished (&resolved_mold);
    }

    return status;
}

//! FSLIB API
enum disir_status
fslib_plugin_mold_read (struct disir_instance *instance,
                        struct disir_plugin *plugin, const char *entry_id,
                        struct disir_mold **mold,
                        dio_unserialize_mold func_unserialize)
{
    enum disir_status status;
    char filepath[PATH_MAX];
    struct stat statbuf;
    int namespace_entry;
    FILE *file;

    namespace_entry = 0;

    status = fslib_mold_resolve_entry_id (instance, plugin, entry_id,
                                          filepath, &statbuf, &namespace_entry);
    if (status != DISIR_STATUS_OK)
    {
        return status;
    }

    file = fopen (filepath, "r");
    if (file == NULL)
    {
        // TODO: Check errno and set appropriate error
        // TODO: Use threadsafe strerror, or refactor entirely
        disir_error_set (instance, "opening for reading %s: %s", filepath, strerror (errno));
        return DISIR_STATUS_FS_ERROR;
    }

    status = func_unserialize (instance, file, mold);
    // Cleanup
    fclose (file);

    return status;
}

