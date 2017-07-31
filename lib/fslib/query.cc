// private
#include "mqueue.h"

// public
#include <disir/fslib/util.h>

// system
#include <dirent.h>
#include <sstream>


//! FSLIB API
enum disir_status
fslib_config_query_entries (struct disir_instance *instance, struct disir_register_plugin *plugin,
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
    if (directory == NULL)
    {
        disir_error_set (instance, "Unable to open directory: %s", searchdir.c_str());
        return DISIR_STATUS_FS_ERROR;
    }
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
                entry->de_attributes = mold_entry->de_attributes;
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
                fslib_config_query_entries (instance, plugin, name.c_str(), entries);
            }
        }
    }

    return DISIR_STATUS_OK;
}

//! FSLIB API
enum disir_status
fslib_mold_query_entries (struct disir_instance *instance, struct disir_register_plugin *plugin,
                          const char *basedir, struct disir_entry **entries)
{
    DIR *directory;
    struct dirent *dp;
    struct disir_entry *entry;

    // File extension is always without the leading dot - add 1 for it
    std::stringstream suffix_stream;
    suffix_stream << "." << plugin->dp_mold_entry_type;
    std::string suffix = suffix_stream.str();

    // Directory is a combinarion of plugin config_base_id and input basedir
    std::stringstream sd;
    sd << plugin->dp_mold_base_id;
    if (basedir)
    {
        sd << '/';
        sd << basedir;
    }
    std::string searchdir = sd.str();

    directory = opendir (searchdir.c_str());
    if (directory == NULL)
    {
        disir_error_set (instance, "Unable to open directory: %s", searchdir.c_str());
        return DISIR_STATUS_FS_ERROR;
    }
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
                && name.compare (name.size() - suffix.size(), suffix.size(), suffix) == 0)
            {
                std::string entry_name(name, 0, name.size() - suffix.size());
                entry = (struct disir_entry *) calloc (1, sizeof (struct disir_entry));

                std::string direntry (dp->d_name);
                if (direntry.compare (0, direntry.size() - suffix.size(), "__namespace") == 0)
                {
                    entry->flag.DE_NAMESPACE_ENTRY = 1;
                    std::size_t found = entry_name.find_last_of('/');
                    if (found != std::string::npos)
                    {
                        entry_name[found+1] = '\0';
                    }
                    else
                    {
                        entry_name = std::string("/");
                    }
                }
                else
                {
                    entry->flag.DE_NAMESPACE_ENTRY = 0;
                }

                entry->de_entry_name = strdup(entry_name.c_str());
                // TODO: stat entry to get READABLE and WRITABLE
                entry->flag.DE_READABLE = 1;
                entry->flag.DE_WRITABLE = 1;
                MQ_ENQUEUE (*entries, entry);
            }
        }
        else if (dp->d_type == DT_DIR)
        {
            // Must be a better way to check this..
            if (strcmp (dp->d_name, ".") == 0) {}
            else if (strcmp (dp->d_name, "..") == 0) {}
            else
            {

                fslib_mold_query_entries (instance, plugin, name.c_str(), entries);
            }
        }
    }

    return DISIR_STATUS_OK;
}

//! FSLIB API
enum disir_status
fslib_plugin_config_query (struct disir_instance *instance, struct disir_register_plugin *plugin,
                           const char *entry_id, struct disir_entry **entry)
{
    enum disir_status status;
    char filepath[PATH_MAX];
    struct disir_entry *ret;

    status = plugin->dp_mold_query (instance, plugin, entry_id, &ret);
    if (status == DISIR_STATUS_NOT_EXIST)
    {
        disir_error_set (instance, "there exists no mold for entry %s", entry_id);
        disir_entry_finished (&ret);
        return DISIR_STATUS_MOLD_MISSING;
    }
    if (status != DISIR_STATUS_EXISTS)
    {
        return status;
    }

    // Our mold covers it. Now lets check if it exists
    status = fslib_config_resolve_filepath (instance, plugin, entry_id, filepath);
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

    // TODO: Update ret with READABLE and WRITABLE from statbuf

    if (entry != NULL)
    {
        *entry = ret;
    }
    else
    {
        disir_entry_finished  (&ret);
    }
    return DISIR_STATUS_EXISTS;
}

//! FSLIB API
enum disir_status
fslib_plugin_mold_query (struct disir_instance *instance, struct disir_register_plugin *plugin,
                         const char *entry_id, struct disir_entry **entry)
{
    enum disir_status status;
    char filepath[PATH_MAX];
    struct disir_entry *query_entry;
    int namespace_entry;
    struct stat statbuf;

    namespace_entry = 0;

    status = fslib_mold_resolve_entry_id (instance, plugin, entry_id,
                                          filepath, &statbuf, &namespace_entry);
    if (status != DISIR_STATUS_OK)
    {
        return status;
    }

    if (entry != NULL)
    {
        query_entry = (struct disir_entry *) calloc (1, sizeof (struct disir_entry));
        query_entry->de_entry_name = strdup(entry_id);
        // TODO: Get readable and writable from statbuf
        query_entry->flag.DE_READABLE = 1;
        query_entry->flag.DE_WRITABLE = 1;
        query_entry->flag.DE_NAMESPACE_ENTRY = namespace_entry;

        *entry = query_entry;
    }

    return DISIR_STATUS_EXISTS;
}

//! FSLIB API
enum disir_status
fslib_mold_resolve_entry_id (struct disir_instance *instance, struct disir_register_plugin *plugin,
                             const char *entry_id, char *filepath,
                             struct stat *statbuf, int *namespace_entry)
{
    enum disir_status status;
    char *sep = NULL;

    // Query if this mold exists
    status = fslib_mold_resolve_filepath (instance, plugin, entry_id, filepath);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }

    status = fslib_stat_filepath (instance, filepath, statbuf);
    if (status != DISIR_STATUS_OK)
    {
        // Find the directory of the file we attempted to find
        sep = strrchr (filepath, '/');
        if (sep == NULL)
            return status;

        // Create the namespace entry filepath
        *sep = '\0';
        strcat (filepath, "/__namespace.");
        strcat (filepath, plugin->dp_mold_entry_type);

        // Attempt to find the namespace entry file.
        status = fslib_stat_filepath (instance, filepath, statbuf);
        if (status != DISIR_STATUS_OK)
            return status;

        // This is a namespace entry
        *namespace_entry = 1;
    }

    return DISIR_STATUS_OK;
}
