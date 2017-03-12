// private
#include "mqueue.h"

// public
#include <disir/fslib/util.h>

// system
#include <dirent.h>
#include <sstream>


//! FSLIB API
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

//! FSLIB API
enum disir_status
fslib_plugin_config_query (struct disir_instance *instance, struct disir_plugin *plugin,
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

