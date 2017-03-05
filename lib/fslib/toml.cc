#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cerrno>

#include <sys/stat.h>
#include <dirent.h>

#include <disir/disir.h>
#include <disir/fslib/toml.h>
#include <disir/fslib/util.h>

// The headonly implementation of TOML.
#include "tinytoml/toml.h"

// Private
#include "mqueue.h"

//! STATIC API
//!
//! \param[out] filepath Populate the output buffer with the complete, resolved filepath.
//!     Buffer is required to be PATH_MAX sized.
//!
//! \return DISIR_STATUS_INSUFFICIENT_RESOURCES if the resolved path is too large.
//! \return DISIR_STATUS_OK on success.
static enum disir_status
fslib_resolve_filepath (struct disir_instance *instance, struct disir_plugin *plugin,
                        const char *entry_id, char *filepath)
{
    int res;

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

//! STATIC API
static enum disir_status
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
            disir_error_set (instance, "entry resolved to filepath '%s' does not exist: %s",
                            filepath, strerror (errsave));

        }

        return status;
    }

    return DISIR_STATUS_OK;
}

enum disir_status
fslib_query_entries (struct disir_instance *instance, struct disir_plugin *plugin,
                     const char *basedir, struct disir_entry **entries)
{
    enum disir_status status;
    DIR *directory;
    struct dirent *dp;
    struct disir_entry *entry;
    struct disir_entry *mold_entry;

    // File extension is always without the leading dot - add 1 for it
    std::stringstream suffix_stream;
    suffix_stream << "." << plugin->dp_config_entry_type;
    std::string suffix = suffix_stream.str();

    // Directory is a combinarion of plugin config_base_id and input basedir
    std::stringstream sd;
    sd << plugin->dp_config_base_id;
    if (basedir)
    {
        sd << '/';
        sd << basedir;
    }
    std::string searchdir = sd.str();

    directory = opendir (searchdir.c_str());
    while ((dp = readdir(directory)) != NULL)
    {
        // entry file, relative to basedir
        std::stringstream ef;
        if (basedir)
        {
            ef << basedir;
            ef << '/';
        }
        ef << dp->d_name;
        std::string name = ef.str();

        if (dp->d_type == DT_REG)
        {
            // Check that our file extension (suffix) is valid for this file entry
            if (name.size() > suffix.size()
                && name.compare(name.size() - suffix.size(), suffix.size(), suffix) == 0)
            {
                //! Check if we have a mold entry for this directory entry
                std::string entry_name(name, 0, name.size() - suffix.size());
                status = plugin->dp_mold_query (instance, plugin, entry_name.c_str(), &mold_entry);
                if (status != DISIR_STATUS_EXISTS)
                {
                    // We dont really care what happened here...
                    continue;
                }

                entry = (struct disir_entry *) calloc (1, sizeof (struct disir_entry));
                entry->de_entry_name = strdup(entry_name.c_str());
                entry->DE_READABLE = mold_entry->DE_READABLE;
                entry->DE_WRITABLE = mold_entry->DE_WRITABLE;
                entry->DE_NAMESPACE_ENTRY = mold_entry->DE_NAMESPACE_ENTRY;
                MQ_ENQUEUE (*entries, entry);

                disir_entry_finished (&mold_entry);
            }
        }
        else if (dp->d_type == DT_DIR)
        {
            // Must be a better way to check this..
            if (strcmp (dp->d_name, ".") == 0) {}
            else if (strcmp (dp->d_name, "..") == 0) {}
            else
            {
                fslib_query_entries (instance, plugin, name.c_str(), entries);
            }
        }
    }

    return DISIR_STATUS_OK;
}

//! PLUGIN API
enum disir_status
dio_toml_config_read (struct disir_instance *instance,
                      struct disir_plugin *plugin, const char *entry_id,
                      struct disir_mold *mold, struct disir_config **config)
{
    enum disir_status status;
    char filepath[PATH_MAX];
    struct stat statbuf;
    struct disir_mold *resolved_mold;

    status = fslib_resolve_filepath (instance, plugin, entry_id, filepath);
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

    status = dio_toml_unserialize_config (instance, file, resolved_mold, config);

    // Cleanup
    fclose (file);
    if (mold == NULL)
    {
        // We only decref the mold if we were the one allocating it
        disir_mold_finished (&resolved_mold);
    }

    return status;
}

//! PLUGIN API
enum disir_status
dio_toml_config_write (struct disir_instance *instance, struct disir_plugin *plugin,
                       const char *entry_id, struct disir_config *config)
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

    status = fslib_resolve_filepath (instance, plugin, entry_id, filepath);
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

    status = dio_toml_serialize_config (config, file);
    fclose (file);

    return status;
}

//! PLUGIN API
enum disir_status
dio_toml_config_entries (struct disir_instance *instance,
                         struct disir_plugin *plugin,
                         struct disir_entry **entries)
{
    return fslib_query_entries (instance, plugin, NULL, entries);
}

// PLUGIN API
enum disir_status
dio_toml_config_query (struct disir_instance *instance, struct disir_plugin *plugin,
                       const char *entry_id, struct disir_entry **entry)
{
    enum disir_status status;
    char filepath[PATH_MAX];

    status = plugin->dp_mold_query (instance, plugin, entry_id, NULL);
    if (status == DISIR_STATUS_NOT_EXIST)
    {
        disir_error_set (instance, "there exists no mold for entry %s", entry_id);
        return DISIR_STATUS_MOLD_MISSING;
    }
    if (status != DISIR_STATUS_EXISTS)
    {
        return status;
    }

    // Our mold covers it. Now lets check if it exists
    status = fslib_resolve_filepath (instance, plugin, entry_id, filepath);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }

    struct stat statbuf;
    status = fslib_stat_filepath (instance, filepath, &statbuf);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }

    // TODO: Return a disir_entry object?

    return DISIR_STATUS_EXISTS;
}

