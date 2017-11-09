#include <disir/fslib/util.h>

#include <limits.h>
#include <errno.h>

//! FSLIB API
enum disir_status
fslib_config_resolve_filepath (struct disir_instance *instance, struct disir_register_plugin *plugin,
                               const char *entry_id, char *filepath)
{
    int res;

    (void) &instance;
    // QUESTION: Dis-allow access to only '/' query?
    // QUESTION: Dis-allow access to queries starting with /? That is reserved right?

    res = snprintf (filepath, PATH_MAX, "%s/%s.%s",
                     plugin->dp_config_base_id, entry_id, plugin->dp_config_entry_type);

    if (res >= PATH_MAX)
    {
        return DISIR_STATUS_INSUFFICIENT_RESOURCES;
    }

    return DISIR_STATUS_OK;
}

//! FSLIB API
enum disir_status
fslib_mold_resolve_filepath (struct disir_instance *instance, struct disir_register_plugin *plugin,
                             const char *entry_id, char *filepath)
{
    int res;

    (void) &instance;
    // QUESTION: Dis-allow access to only '/' query?
    // QUESTION: Dis-allow access to queries starting with /? That is reserved right?

    res = snprintf (filepath, PATH_MAX, "%s/%s.%s",
                     plugin->dp_mold_base_id, entry_id, plugin->dp_mold_entry_type);

    if (res >= PATH_MAX)
    {
        return DISIR_STATUS_INSUFFICIENT_RESOURCES;
    }

    return DISIR_STATUS_OK;
}

//! FSLIB API
enum disir_status
fslib_stat_filepath (struct disir_instance *instance,
                     const char *filepath, struct stat *statbuf)
{
    enum disir_status status;
    int res;
    int errsave;

    res = stat (filepath, statbuf);
    if (res != 0)
    {
        errsave = errno;

        if (errsave == ENOTDIR)
        {
            status = DISIR_STATUS_FS_ERROR;
            disir_error_set (instance, "non-directory in filepath '%s'", filepath);
        }
        else if (errsave == EACCES)
        {
            status = DISIR_STATUS_PERMISSION_ERROR;
            disir_error_set (instance, "insufficient access to filepath '%s'", filepath);
        }
        else
        {
            // TODO: Use threadsafe strerror
            status = DISIR_STATUS_NOT_EXIST;
        }

        return status;
    }

    return DISIR_STATUS_OK;
}

