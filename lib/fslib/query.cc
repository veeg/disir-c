// private
#include "mqueue.h"

// public
#include <disir/fslib/util.h>

// system
#include <algorithm>
#include <dirent.h>
#include <sstream>

#include <iostream>

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
    struct stat statbuf;

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

    // Stat if directory exists. If it does not, we try to create it
    status = fslib_stat_filepath (instance, searchdir.c_str(), &statbuf);
    if (status == DISIR_STATUS_NOT_EXIST)
    {
        // lets create it
        disir_error_clear (instance);
        status = fslib_mkdir_p (instance, searchdir.c_str());
    }
    if (status != DISIR_STATUS_OK)
    {
        // Error already set
        return status;
    }

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

    closedir (directory);

    return DISIR_STATUS_OK;
}

//! FSLIB API
//
// We handle it like so:
// __namespace = OK - NAMESPACE_ENTRY
// __namespace + entry_name.oe.suffix = OK
// __namespace + entry_name.suffix + entry_name.oe.suffix = OK
// entry_name.suffix = OK
// entry_name.suffix + entry_name.oe.suffix = OK
// entry_name.oe.suffix = IGNORED
//
enum disir_status
fslib_mold_query_entries (struct disir_instance *instance, struct disir_register_plugin *plugin,
                          const char *basedir, struct disir_entry **entries)
{
    enum disir_status status;
    bool has_namespace = false;
    DIR *directory;
    struct dirent *dp;
    struct disir_entry *entry;
    struct stat statbuf;

    // File extension is always without the leading dot - add 1 for it
    std::stringstream suffix_stream;
    suffix_stream << "." << plugin->dp_mold_entry_type;
    std::string suffix = suffix_stream.str();

    // The namespace override entry has a fixed suffix with a leading dot
    std::string oeid(".oe");

    // Directory is a combinarion of plugin config_base_id and input basedir
    std::stringstream sd;
    sd << plugin->dp_mold_base_id;
    if (basedir)
    {
        sd << '/';
        sd << basedir;
    }
    std::string searchdir = sd.str();

    // Stat if directory exists. If it does not, we try to create it
    status = fslib_stat_filepath (instance, searchdir.c_str(), &statbuf);
    if (status == DISIR_STATUS_NOT_EXIST)
    {
        // lets create it
        disir_error_clear (instance);
        status = fslib_mkdir_p (instance, searchdir.c_str());
    }
    if (status != DISIR_STATUS_OK)
    {
        // Error already set
        return status;
    }

    directory = opendir (searchdir.c_str());
    if (directory == NULL)
    {
        disir_error_set (instance, "Unable to open directory: %s", searchdir.c_str());
        return DISIR_STATUS_FS_ERROR;
    }

    // Check ONCE if we have a namespace entry in this directory
    std::stringstream nspath_stream;
    nspath_stream << searchdir << "/__namespace" << suffix;
    std::string nspath = nspath_stream.str();
    std::cerr << " XXX SEARCHDIR " << nspath << std::endl;
    if (fslib_stat_filepath (instance, nspath.c_str(), &statbuf) == DISIR_STATUS_OK)
    {
        has_namespace = true;
        std::cerr << " XXX SEARCHDIR HAS NS " << nspath << std::endl;
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
                bool namespace_entry = false;

                std::string direntry (dp->d_name);
                // Check if the direntry is namespace
                if (direntry.compare (0, direntry.size() - suffix.size(), "__namespace") == 0)
                {
                    std::cerr << " DDD namespace entry: " << entry_name << std::endl;
                    namespace_entry = true;
                    std::size_t found = entry_name.find_last_of('/');
                    if (found != std::string::npos)
                    {
                        entry_name.resize(found+1);
                    }
                    else
                    {
                        entry_name = std::string("/");
                    }
                    std::cerr << " DDD modified namespace entry: " << entry_name << std::endl;
                }
                // Check if this entry is a namespace override entry
                else if (direntry.size() > oeid.size() + suffix.size()
                         && direntry.compare (direntry.size() - oeid.size() - suffix.size(),
                                              oeid.size(), oeid) == 0)
                {

                    // Here we handle the case where we have:
                    //     entry_name.oe.suffix
                    //     __namespace + entry_name.oe.suffix
                    // The remainder of the cases  is handled in the else clause

                    // Strip away the override entry suffix
                    // Check if this exists - if it does, ignore this and move to the next
                    std::stringstream oestripped_stream;
                    oestripped_stream << searchdir
                                      << entry_name.substr(0, entry_name.size() - oeid.size())
                                      << suffix;
                    std::string oestripped = oestripped_stream.str();
                    std::cerr << " DDD checking for original entry: " << oestripped << std::endl;
                    if (fslib_stat_filepath (instance, oestripped.c_str(), &statbuf)
                            == DISIR_STATUS_OK)
                    {
                        std::cerr << " DDD oe has original entry - skipping" << std::endl;
                        continue;
                    }

                    // if has_namespace is false, we move to the next,
                    // since we have neither a namespace nor an original entry name.
                    if (!has_namespace)
                    {
                        std::cerr << " DDD oe does not have namespace - ignoring" << std::endl;
                        continue;
                    }

                    entry_name = entry_name.substr(0, entry_name.size() - oeid.size());
                    std::cerr << " DDD have namespace!!  " << entry_name << std::endl;
                }
                else
                {
                    // We handle the following cases here
                    //     __namespace + entry_name.suffix + entry_name.oe.suffix
                    //     entry_name.suffix
                    //     entry_name.suffix + entry_name.oe.suffix
                    // In reality, we dont have to check for anything else here because
                    // we know we are already a valid entry which is NOT a namespace entry.
                    // Simply let it pass through
                    std::cerr << " DDD else: " << direntry << std::endl;
                }

                // TODO: Apply name filter - we do not allow any entry not using only lowercase
                // letters a-z
                // UNICODE: Not really considered...
                auto illegal = std::find_if(entry_name.begin(), entry_name.end(),
                    [](const char c)
                {
                    if (c >= 'a' && c <= 'z') return false;
                    if (c == '_' || c == '/') return false;
                    return true;
                });
                if (illegal != entry_name.end())
                {
                    std::cerr << " DDD illegal entry " << entry_name << std::endl;
                    continue;
                }

                std::cerr << " XXX FOUND ENTRY " << entry_name << std::endl;

                entry = (struct disir_entry *) calloc (1, sizeof (struct disir_entry));
                entry->de_entry_name = strdup(entry_name.c_str());
                // TODO: stat entry to get READABLE and WRITABLE
                entry->flag.DE_READABLE = 1;
                entry->flag.DE_WRITABLE = 1;
                entry->flag.DE_NAMESPACE_ENTRY = namespace_entry;
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

    closedir (directory);

    return DISIR_STATUS_OK;
}

//! FSLIB API
enum disir_status
fslib_plugin_config_query (struct disir_instance *instance, struct disir_register_plugin *plugin,
                           const char *entry_id, struct disir_entry **entry)
{
    enum disir_status status;
    char filepath[PATH_MAX];
    struct disir_entry *ret = NULL;

    status = plugin->dp_mold_query (instance, plugin, entry_id, &ret);
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
    status = fslib_config_resolve_filepath (instance, plugin, entry_id, filepath);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        goto error;
    }

    struct stat statbuf;
    status = fslib_stat_filepath (instance, filepath, &statbuf);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        goto error;
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
error:
    if (ret)
    {
        disir_entry_finished (&ret);
    }
    return status;
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
    // TODO: We need to check if we have an override entry

    // Fall back to checking if we have a namespace for this entry
    // if so, we validate this entry id, but with the filepath to the namespace
    if (status != DISIR_STATUS_OK)
    {
        // Find the directory of the file we attempted to find
        sep = strrchr (filepath, '/');
        if (sep == NULL)
            return status;

        disir_error_clear (instance);

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
