// disir public
#include <disir/disir.h>
#include <disir/version.h>
#include <disir/fslib/util.h>

// disir private
#include "archive_private.h"
extern "C" {
#include "disir_private.h"
#include "log.h"
}
#include <limits.h>

// external libs
#include <archive.h>
#include <archive_entry.h>

// cpp
#include <map>
#include <fstream>
#include <iostream>


// STATIC FUNCTION
static enum disir_status
copy_metadata (struct disir_archive *archive, std::map<std::string, std::string>& extract_content)
{
    auto it = extract_content.find (METADATA_FILENAME);
    if (it == extract_content.end())
    {
        log_error ("no metadata.toml in disir archive");
        return DISIR_STATUS_NOT_EXIST;
    }

    std::string toml_filepath = it->second;

    std::ifstream metadata (toml_filepath.c_str());
    toml::ParseResult pr = toml::parse (metadata);

    if (!pr.valid())
    {
        log_error ("unable to parse metadata.toml in disir archive");
        return DISIR_STATUS_FS_ERROR;
    }

    toml::Value& root = pr.value;
    (*archive->da_metadata) = root;

    const toml::Value* backends = root.find (ATTRIBUTE_KEY_BACKEND);
    for (const auto& entry : backends->as<toml::Array>())
    {
        auto id = entry.find (ATTRIBUTE_KEY_ID);

        auto iter = extract_content.find (id->as<std::string>() + "/entries.toml");
        if (iter == extract_content.end())
        {
            log_error ("cannot find entries.toml for backend '%s' in map", id->as<std::string>());
            return DISIR_STATUS_NOT_EXIST;
        }

        auto entries_toml_filepath = iter->second;

        std::ifstream entries_toml (entries_toml_filepath.c_str());
        toml::ParseResult pr = toml::parse (entries_toml);

        if (!pr.valid())
        {
            log_error ("unable to parse entries.toml in backend '%s'", id->as<std::string>());
            return DISIR_STATUS_FS_ERROR;
        }

        auto backend_entry = new (std::nothrow) toml::Value (pr.value);
        if (backend_entry == nullptr)
        {
            return DISIR_STATUS_NO_MEMORY;
        }

        archive->da_entries->insert (std::make_pair (id->as<std::string>(), backend_entry));
    }

    return DISIR_STATUS_OK;
}

//! STATIC FUNCTION
static enum disir_status
serialize_entry_index (struct disir_archive *archive, struct disir_config *config,
                       const char *group_id, const char *entry_id, const char *plugin_name)
{
    enum disir_status status;
    struct disir_version config_version;
    struct disir_context *context_config = NULL;
    toml::Value *plugin_entries;
    toml::Value *group_entries;
    char buf[500];
    char *version_string;

    auto it = archive->da_entries->find (plugin_name);
    if (it == archive->da_entries->end())
    {
        (*archive->da_entries)[plugin_name] = new (std::nothrow) toml::Value (toml::Table());
        if ((*archive->da_entries)[plugin_name] == nullptr)
        {
            return DISIR_STATUS_NO_MEMORY;
        }
    }

    plugin_entries = (*archive->da_entries)[plugin_name];

    group_entries = plugin_entries->find (group_id);
    if (group_entries == NULL)
    {
        group_entries = plugin_entries->setChild (group_id, toml::Table());
    }

    context_config = dc_config_getcontext (config);
    if (context_config == NULL)
    {
        status = DISIR_STATUS_INTERNAL_ERROR;
        goto out;
    }

    status = dc_get_version (context_config, &config_version);
    if (status != DISIR_STATUS_OK)
        goto out;

    version_string = dc_version_string (buf, 500, &config_version);
    if (version_string == NULL)
    {
        status = DISIR_STATUS_INVALID_CONTEXT;
        goto out;
    }
    group_entries->setChild (entry_id, version_string);
    // FALL-THROUGH
out:
    if (context_config)
        dc_putcontext (&context_config);

    return status;
}

//! STATIC FUNCTION
static enum disir_status
populate_archive_entry (struct archive *archive, struct archive_entry *entry,
                        const char *archive_entry_name, int size)
{
    int ret;
    time_t timestamp;

    time (&timestamp);
    archive_entry_set_mtime(entry, timestamp, 0);
    archive_entry_set_atime(entry, timestamp, 0);
    archive_entry_set_ctime(entry, timestamp, 0);
    archive_entry_set_pathname (entry, archive_entry_name);
    archive_entry_set_size (entry, size);
    archive_entry_set_filetype (entry, AE_IFREG);
    archive_entry_set_perm (entry, 0644);

    ret = archive_write_header (archive, entry) != ARCHIVE_OK;
    if (ret != ARCHIVE_OK)
    {
        if (ret == ARCHIVE_FATAL)
        {
            log_error ("error writing archive header: %s", archive_error_string (archive));
            return DISIR_STATUS_FS_ERROR;
        }
        // Non-fatal, log and fall-through
        else
        {
            log_warn ("%s", archive_error_string (archive));
        }
    }

    return DISIR_STATUS_OK;
}

//! STATIC FUNCTION
static enum disir_status
append_config (struct disir_instance *instance, struct archive *archive,
               const char *filepath, const char *archive_entry_name)
{
    enum disir_status status;
    struct archive_entry *entry = NULL;
    struct stat st;
    FILE *infile = NULL;
    char buf[8192];
    int read;

    status = fslib_stat_filepath (instance, filepath, &st);
    if (status != DISIR_STATUS_OK)
    {
        return status;
    }

    entry = archive_entry_new();
    if (entry == NULL)
    {
        log_debug (3, "error creating new archive entry: %s",
                       archive_errno (archive));
        return DISIR_STATUS_NO_MEMORY;
    }

    status = populate_archive_entry (archive, entry, archive_entry_name, st.st_size);
    if (status != DISIR_STATUS_OK)
        goto out;

    infile = fopen (filepath, "r");
    if (infile == NULL)
    {
        log_error ("unable to open %s (%s)", filepath, strerror (errno));
        status = DISIR_STATUS_FS_ERROR;
        goto out;
    }

    do
    {
        read = fread (buf, 1, sizeof (buf), infile);
        if (read == 0)
            break;

        if (archive_write_data (archive, buf, read) < 0)
        {
            log_error ("error writing to archive %s", archive_errno (archive));
            status = DISIR_STATUS_FS_ERROR;
            goto out;
        }
    } while (1);

    if (archive_write_finish_entry (archive) != ARCHIVE_OK)
    {
        log_error ("could not finish archive write entry %s", archive_errno (archive));
        status = DISIR_STATUS_FS_ERROR;
        goto out;
    }
    // FALL-THROUGH
out:
    if (entry)
        archive_entry_free (entry);
    if (infile)
        fclose (infile);

    return status;
}

//! INTERNAL API
enum disir_status
dx_archive_config_entries_write (struct disir_instance *instance, struct disir_archive *archive,
                                 struct disir_register_plugin *plugin,
                                 struct disir_entry *config_entry,
                                 const char *group_id)
{
    enum disir_status status;
    struct disir_config *config = NULL;
    struct disir_entry *current = NULL;
    struct disir_archive_entry archive_entry;
    FILE *entry_file = NULL;
    char filepath[PATH_MAX];
    char archive_entry_name[PATH_MAX];
    char *tmp_dir_name = NULL;
    int ret;

    // setting it now such that
    // it can be deallocated on label out
    current = config_entry;
    filepath[0] = '\0';

    if (plugin->dp_config_fd_write == NULL)
    {
        log_error ("plugin %s does not implement config_fd_write", plugin->dp_name);
        status = DISIR_STATUS_NO_CAN_DO;
        goto out;
    }

    tmp_dir_name = dx_archive_create_temp_folder();
    if (tmp_dir_name == NULL)
    {
        log_error ("unable to create temp folder for config files");
        status = DISIR_STATUS_FS_ERROR;
        goto out;
    }

    do
    {
        config_entry = current->next;

        snprintf(filepath, PATH_MAX, "%s/%s", tmp_dir_name, "temp_entry");

        entry_file = fopen (filepath, "w");
        if (entry_file == NULL)
        {
            log_error ("unable to open temp config file: %s", filepath);
            status = DISIR_STATUS_FS_ERROR;
            goto out;
        }

        status = disir_config_read (instance, group_id, current->de_entry_name, NULL, &config);
        if (status != DISIR_STATUS_OK)
            goto out;

        status = plugin->dp_config_fd_write (instance, config, entry_file);
        if (status != DISIR_STATUS_OK)
            goto out;

        status = serialize_entry_index (archive, config, group_id,
                                        current->de_entry_name, plugin->dp_name);
        if (status != DISIR_STATUS_OK)
            goto out;

        snprintf (archive_entry_name, PATH_MAX, "%s/%s/%s",
                  plugin->dp_name, group_id, current->de_entry_name);

        status = append_config (instance, archive->da_archive, filepath, archive_entry_name);
        if (status != DISIR_STATUS_OK)
            goto out;

        archive_entry.de_backend_id = plugin->dp_name;
        archive_entry.de_group_id = group_id;
        archive_entry.de_entry_id = current->de_entry_name;
        archive_entry.de_filepath = "";
        archive->da_config_entries->insert (archive_entry);

        disir_entry_finished (&current);
        disir_config_finished (&config);

        current = config_entry;

        // Clean-up
        ret = remove (filepath);
        if (ret != 0)
        {
            log_debug (5, "unable to remove temp config file");
            status = DISIR_STATUS_FS_ERROR;
            goto out;
        }
    }
    while (current != NULL);
    // FALL-THROUGH
out:
    if (entry_file)
        fclose (entry_file);

    if (filepath[0] != '\0')
    {
        remove (filepath);
    }

    if (current)
    {
        do
        {
            config_entry = current->next;
            disir_entry_finished (&current);
            current = config_entry;
        }
        while (current != NULL);
    }

    if (tmp_dir_name)
    {
        ret = rmdir (tmp_dir_name);
        if (ret != 0)
        {
            log_debug (5, "unable to remove temp folder");
            status = DISIR_STATUS_FS_ERROR;
        }
        free (tmp_dir_name);
    }

    return status;
}

//! STATIC FUNCTION
static enum disir_status
write_archive_metadata_toml (struct disir_archive *archive)
{
    enum disir_status status;
    std::ostringstream entry_formatted;
    struct archive_entry *entry = NULL;
    const char *archive_entry_name = METADATA_FILENAME;
    const char *disir_org_version = "0/1-draft";

    archive->da_metadata->setChild ("implementation", libdisir_version_string);
    archive->da_metadata->setChild ("disir_org_version", disir_org_version);
    auto toml_arr = archive->da_metadata->setChild (ATTRIBUTE_KEY_BACKEND, toml::Array());

    for (const auto& kv : *archive->da_entries)
    {
        auto toml_arr_table = toml_arr->push (toml::Table());
        toml_arr_table->setChild (ATTRIBUTE_KEY_ID, toml::Value (kv.first));
        auto group_arr = toml_arr_table->setChild (ATTRIBUTE_KEY_GROUPS, toml::Array());

        for (const auto& top_level : kv.second->as<toml::Table>())
        {
            group_arr->push (top_level.first);
        }
    }

    archive->da_metadata->write (&entry_formatted);
    auto str = entry_formatted.str();

    entry = archive_entry_new();
    if (entry == NULL)
    {
        log_debug (5, "error creating new archive entry: %s",
                       archive_errno (archive->da_archive));
        return DISIR_STATUS_NO_MEMORY;
    }

    status = populate_archive_entry (archive->da_archive, entry, archive_entry_name, str.size());
    if (status != DISIR_STATUS_OK)
        goto out;

    if (archive_write_data (archive->da_archive, str.c_str(), str.size()) < 0)
    {
        log_error ("error writing archive entries.toml %s",
                    archive_error_string (archive->da_archive));
        status = DISIR_STATUS_FS_ERROR;
        goto out;
    }

    if (archive_write_finish_entry (archive->da_archive) != ARCHIVE_OK)
    {
        log_error ("could not finish archive write entry in entries.toml: %s",
                    archive_error_string (archive->da_archive));
        status = DISIR_STATUS_FS_ERROR;
        goto out;
    }

    // FALL-THROUGH
out:
    if (entry)
        archive_entry_free (entry);

    return status;
}

//! INTERNAL API
enum disir_status
dx_archive_metadata_write (struct disir_archive *archive)
{
    enum disir_status status;
    struct archive_entry *entry = NULL;
    char archive_entry_name[PATH_MAX];

    // Create metadata.toml
    status = write_archive_metadata_toml (archive);
    if (status != DISIR_STATUS_OK) {
        // Already logged
        return status;
    }

    // Create entries.toml
    for (const auto& kv : *archive->da_entries)
    {
        std::ostringstream entry_formatted;
        kv.second->write (&entry_formatted);
        auto str = entry_formatted.str();

        entry = archive_entry_new();
        if (entry == NULL)
        {
            log_error ("error creating new archive entry: %s",
                        archive_errno (archive->da_archive));
            return DISIR_STATUS_NO_MEMORY;
        }

        snprintf (archive_entry_name, PATH_MAX, "%s/entries.toml", kv.first.c_str());

        status = populate_archive_entry (archive->da_archive, entry,
                                         archive_entry_name, str.size());
        if (status != DISIR_STATUS_OK)
            goto error;

        if (archive_write_data (archive->da_archive, str.c_str(), str.size()) < 0)
        {
            log_error ("error writing archive metadata.toml: %s",
                        archive_errno (archive->da_archive));
            status = DISIR_STATUS_FS_ERROR;
            goto error;
        }

        if (archive_write_finish_entry (archive->da_archive) != ARCHIVE_OK)
        {
            log_error ("could not finish archive write entry in metadata.toml: %s",
                        archive_error_string (archive->da_archive));
            status = DISIR_STATUS_FS_ERROR;
            goto error;
        }

        archive_entry_free (entry);
    }

    return DISIR_STATUS_OK;
error:
    if (entry)
        archive_entry_free (entry);

    return status;
}

//! INTERNAL API
enum disir_status
dx_archive_begin_existing (struct disir_instance *instance, const char *archive_path,
                           struct disir_archive **archive)
{
    enum disir_status status;
    std::map<std::string, std::string> extract_content;
    struct disir_archive *write_archive = NULL;
    struct stat st;
    char *tmp_dir_name = NULL;
    const char *ext = NULL;

    // Make sure archive path is not a directory
    if (archive_path[strlen(archive_path)-1] == '/')
    {
        disir_error_set (instance, "invalid archive path: input path is a directory");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    // Assert that archive contains the .disir extension
    ext = strrchr (archive_path, '.');
    if (ext == NULL || strcmp (ext, ".disir") != 0)
    {
        disir_error_set (instance, "input archive is not a disir archive");
        return DISIR_STATUS_NO_CAN_DO;
    }

    // Make sure file exists
    status = fslib_stat_filepath (instance, archive_path, &st);
    if (status != DISIR_STATUS_OK)
    {
        return status;
    }

    status = dx_archive_begin_new (&write_archive);
    if (status != DISIR_STATUS_OK)
        goto out;

    // Create temporary extract folder
    tmp_dir_name = dx_archive_create_temp_folder();
    if (tmp_dir_name == NULL)
    {
        status = DISIR_STATUS_FS_ERROR;
        goto out;
    }

    // Store temp path for clean up later
    write_archive->da_extract_folder_path = tmp_dir_name;

    status = dx_archive_extract (archive_path, extract_content, tmp_dir_name);
    if (status != DISIR_STATUS_OK)
        goto out;

    status = dx_archive_validate (write_archive, extract_content);
    if (status != DISIR_STATUS_OK)
    {
        disir_error_set (instance, "invalid archive");
        goto out;
    }

    status = copy_metadata (write_archive, extract_content);
    if (status != DISIR_STATUS_OK)
        goto out;

    // Copy content
    for (const auto& entry : extract_content)
    {
        // Ignore metadata files, they will be re-written in finalize.
        if (entry.first.find ("entries.toml") != std::string::npos)
            continue;

        if (entry.first.find ("metadata.toml") != std::string::npos)
            continue;

        status = append_config (instance, write_archive->da_archive, entry.second.c_str(),
                                                                     entry.first.c_str());
        if (status != DISIR_STATUS_OK)
            goto out;
    }

    write_archive->da_existing_path = strdup (archive_path);
    *archive = write_archive;

    //FALL-THROUGH
out:
    if (status != DISIR_STATUS_OK)
    {
        if (write_archive)
        {
            disir_archive_finalize (instance, NULL, &write_archive);
        }
    }

    return status;
}

//! INTERNAL API
enum disir_status
dx_archive_begin_new (struct disir_archive **archive)
{
    enum disir_status status;
    struct disir_archive *ar;

    if (archive == NULL)
    {
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    status = dx_archive_create (&ar);
    if (status != DISIR_STATUS_OK)
    {
        return status;
    }

    status = dx_archive_open_write (ar);
    if (status != DISIR_STATUS_OK)
        goto error;

    ar->da_entries = new (std::nothrow) std::map<std::string, toml::Value*>;
    if (ar->da_entries == nullptr)
    {
        log_debug (0, "unable to allocate archive entries map");
        status = DISIR_STATUS_NO_MEMORY;
        goto error;
    }

    *archive = ar;

    return DISIR_STATUS_OK;
error:
    if (ar)
        dx_archive_destroy (ar);

    return status;
}
