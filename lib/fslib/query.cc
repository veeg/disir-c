// private
#include "mqueue.h"

// public
#include <disir/fslib/util.h>

// system
#include <algorithm>
#include <dirent.h>
#include <sstream>

#include <iostream>

//! STATIC API
bool
validate_entry_id_characters(std::string& entry_id)
{
    // Apply name filter - we do not allow any entry not using only lowercase
    // letters a-z
    // UNICODE: Not really considered...
    auto illegal = std::find_if(entry_id.begin(), entry_id.end(),
        [](const char c)
    {
        if (c >= 'a' && c <= 'z') return false;
        if (c >= '0' && c <= '9') return false;
        if (c == '_' || c == '/') return false;
        return true;
    });

    return (illegal == entry_id.end());
}

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
// __namespace + entry_name.o.suffix = OK
// __namespace + entry_name.suffix + entry_name.o.suffix = OK
// entry_name.suffix = OK
// entry_name.suffix + entry_name.o.suffix = OK
// entry_name.o.suffix = IGNORED
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
    std::string oid(".o");

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
    if (fslib_stat_filepath (instance, nspath.c_str(), &statbuf) == DISIR_STATUS_OK)
    {
        has_namespace = true;
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
                }
                // Check if this entry is a namespace override entry
                else if (direntry.size() > oid.size() + suffix.size()
                         && direntry.compare (direntry.size() - oid.size() - suffix.size(),
                                              oid.size(), oid) == 0)
                {

                    // Here we handle the case where we have:
                    //     entry_name.oe.suffix
                    //     __namespace + entry_name.oe.suffix
                    // The remainder of the cases  is handled in the else clause

                    // Strip away the override entry suffix
                    // Check if this exists - if it does, ignore this and move to the next
                    std::stringstream oestripped_stream;
                    oestripped_stream << searchdir
                                      << entry_name.substr(0, entry_name.size() - oid.size())
                                      << suffix;
                    std::string oestripped = oestripped_stream.str();
                    if (fslib_stat_filepath (instance, oestripped.c_str(), &statbuf)
                            == DISIR_STATUS_OK)
                    {
                        continue;
                    }

                    // if has_namespace is false, we move to the next,
                    // since we have neither a namespace nor an original entry name.
                    if (!has_namespace)
                    {
                        continue;
                    }

                    entry_name = entry_name.substr(0, entry_name.size() - oid.size());
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
                }

                if (!validate_entry_id_characters(entry_name))
                {
                    continue;
                }

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
    char override_filepath[PATH_MAX];
    struct disir_entry *query_entry;
    int namespace_entry;
    struct stat statbuf;

    std::string eid(entry_id);
    if (!validate_entry_id_characters(eid))
    {
        disir_error_set (instance, "entry_id is ill-formed");
        return DISIR_STATUS_RESTRICTION_VIOLATED;
    }

    namespace_entry = 0;

    status = fslib_mold_resolve_entry_id (instance, plugin, entry_id,
                                          filepath, override_filepath, &statbuf, &namespace_entry);
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

        if (filepath[0] == '\0' && namespace_entry != 1)
        {
            query_entry->flag.DE_SINGLE = 1;
        }

        if (override_filepath[0] != '\0')
        {
            query_entry->flag.DE_OVERRIDE = 1;
        }

        query_entry->flag.DE_NAMESPACE_ENTRY = namespace_entry;

        *entry = query_entry;
    }

    return DISIR_STATUS_EXISTS;
}

//! FSLIB_API
enum disir_status
fslib_mold_resolve_entry_filepath (struct disir_instance *instance, const char *entry_path,
                                   char *filepath, char *override_filepath, struct stat *statbuf,
                                   int *namespace_entry)
{
    enum disir_status status;
    enum disir_status override_status;
    char *sep = NULL;
    char suffix[128];

    strncpy (override_filepath, entry_path, PATH_MAX);

    sep = strrchr (override_filepath, '.');
    if (sep == NULL)
        return DISIR_STATUS_FS_ERROR;

    strncpy (suffix, sep, 128);

    // first check if the requested entry is an override entry
    if (strncmp (sep - strlen (".o"), ".o", strlen (".o")) == 0)
    {
        strncpy (filepath, entry_path, PATH_MAX);

        sep = strrchr (filepath, '.');
        if (sep == NULL)
            return DISIR_STATUS_FS_ERROR;

        sep -= strlen(".o");
        *sep = '\0';

        strcat (filepath, suffix);

        override_status = fslib_stat_filepath (instance, override_filepath, statbuf);
    }
    else
    {
        *override_filepath = '\0';
        strncpy (filepath, entry_path, PATH_MAX);
    }

    status = fslib_stat_filepath (instance, filepath, statbuf);
    if (status == DISIR_STATUS_OK)
    {
        return status;
    }

    sep = strrchr (filepath, '/');
    if (sep == NULL)
    {
        return DISIR_STATUS_FS_ERROR;
    }

    strcat (filepath, "/__namespace.");
    strcat (filepath, suffix);

    status = fslib_stat_filepath (instance, filepath, statbuf);
    if (status != DISIR_STATUS_OK)
    {
        return status;
    }

    // Only set namspace entry flag if it's the only one we found
    if (override_status != DISIR_STATUS_OK && status == DISIR_STATUS_OK
        && namespace_entry != NULL)
    {
        *namespace_entry = 1;
    }

    return (override_status == DISIR_STATUS_OK
            && status != DISIR_STATUS_OK
            ? DISIR_STATUS_NOT_EXIST : DISIR_STATUS_OK);
}

//! FSLIB API
enum disir_status
fslib_mold_resolve_entry_id (struct disir_instance *instance, struct disir_register_plugin *plugin,
                             const char *entry_id, char *filepath, char *override_filepath,
                             struct stat *statbuf, int *namespace_entry)
{
    enum disir_status status;
    enum disir_status override_status;
    char *sep = NULL;

    override_status = DISIR_STATUS_OK;
    if (override_filepath != NULL)
    {
        std::stringstream ss;
        ss << entry_id << ".o";

        std::string oe = ss.str();

        status = fslib_mold_resolve_filepath (instance, plugin, oe.c_str(), override_filepath);
        if (status != DISIR_STATUS_OK)
        {
            return status;
        }

        // Check whether override entry exists, and hold onto the status
        override_status = fslib_stat_filepath (instance, override_filepath, statbuf);
        if (override_status != DISIR_STATUS_OK)
        {
            // Clear the disir error if the override entry did not exist
            disir_error_clear(instance);
            // Null-terminate first character to indicate a non-existent override entry
            *override_filepath = '\0';
        }
    }

    status = fslib_mold_resolve_filepath (instance, plugin, entry_id, filepath);
    if (status != DISIR_STATUS_OK)
    {
        return status;
    }

    // The nominal file exists  this is what we return
    status = fslib_stat_filepath (instance, filepath, statbuf);
    if (status == DISIR_STATUS_OK)
    {
        return status;
    }

    // It did not exist, clear the error
    disir_error_clear(instance);

    sep = strrchr (filepath, '/');
    if (sep == NULL)
    {
        return DISIR_STATUS_FS_ERROR;
    }

    *sep = '\0';

    strcat (filepath, "/__namespace.");
    strcat (filepath, plugin->dp_mold_entry_type);

    // Attempt to find the namespace entry file.
    status = fslib_stat_filepath (instance, filepath, statbuf);
    if (status != DISIR_STATUS_OK)
    {
        *filepath = '\0';
        // Neither nominal nor namespace entry exists
        // Set an appropriate error
        disir_error_set(instance,
                "entry_id '%s' is not covered by a normal nor namespace mold", entry_id);
    }

    // Only set namspace entry flag if it's the only one we found
    if (override_status != DISIR_STATUS_OK && status == DISIR_STATUS_OK
        && namespace_entry != NULL)
    {
        *namespace_entry = 1;
    }

    // Regardless of what override status is, we will just care about
    // whether or not the nominal mold exists
    // The output of override is messured in output argument, not status
    return status;
}
