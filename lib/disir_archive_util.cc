// Disir public
#include <disir/disir.h>
#include <disir/fslib/util.h>
#include <disir/version.h>

// disir private
#include "archive_private.h"
extern "C" {
#include "disir_private.h"
#include "log.h"
}

// external libs
#include <archive.h>
#include <archive_entry.h>
#include <limits.h>
#include <fstream>
#include <iostream>
#include <ftw.h>
#include <ctime>

//! INTERNAL API
char *dx_archive_create_temp_folder (void)
{
    char  basename[] = "/tmp/disir.XXXXXX";
    char *tmp_dir_name = mkdtemp (basename);

    if (tmp_dir_name != NULL)
    {
        return strdup(tmp_dir_name);
    }
    else
    {
        log_error ("unable to create temporary folder: %s", strerror (errno));
        return NULL;
    }
}

//! INTERNAL API
enum disir_status
dx_archive_create (struct disir_archive **archive)
{
    enum disir_status status;

    struct disir_archive *ar = (struct disir_archive*) calloc (1, sizeof (struct disir_archive));
    if (ar == NULL)
    {
        return DISIR_STATUS_NO_MEMORY;
    }

    ar->da_archive = NULL;
    ar->da_entries = NULL;
    ar->da_extract_folder_path = NULL;
    ar->da_existing_path = NULL;
    ar->da_temp_archive_path = NULL;

    ar->da_metadata = new (std::nothrow) toml::Value (toml::Table());
    if (ar->da_metadata == nullptr)
    {
        log_debug (3, "failed to allocate new toml table");
        status = DISIR_STATUS_NO_MEMORY;
        goto error;
    }

    ar->da_config_entries = new (std::nothrow) std::set<struct disir_archive_entry, cmp_entry>;
    if (ar->da_config_entries == nullptr)
    {
        log_debug (3, "failed to allocate config entries set");
        status = DISIR_STATUS_NO_MEMORY;
        goto error;
    }

    *archive = ar;
    return DISIR_STATUS_OK;
error:
    if (ar && ar->da_metadata)
        delete ar->da_metadata;
    if (ar && ar->da_config_entries)
        delete ar->da_config_entries;
    if (ar)
        free (ar);

    return status;
}

//! INTERNAL API
enum disir_status
dx_archive_extract (const char *archive_path, std::map<std::string, std::string>& extract_content,
                    const char *extract_path)
{
    enum disir_status status;
    struct archive *write_archive = NULL;
    struct archive *read_archive = NULL;
    struct archive_entry *entry;
    int flags;
    int ret;
    int64_t offset;
    size_t size;
    const void *buff;
    char path[PATH_MAX];

    // Check for write permission
    status = dx_assert_write_permission (extract_path);
    if (status != DISIR_STATUS_OK)
    {
        return status;
    }

    flags = ARCHIVE_EXTRACT_TIME;
    flags |= ARCHIVE_EXTRACT_PERM;
    flags |= ARCHIVE_EXTRACT_ACL;
    flags |= ARCHIVE_EXTRACT_FFLAGS;

    write_archive = archive_write_disk_new();
    if (write_archive == NULL)
    {
        log_debug (3, "unable to open new disk write archive");
        return DISIR_STATUS_NO_MEMORY;
    }

    if (archive_write_disk_set_options (write_archive, flags) != ARCHIVE_OK)
    {
        log_error ("could not set permission flags: %s", archive_error_string (write_archive));
        status = DISIR_STATUS_FS_ERROR;
        goto out;
    }

    if (archive_write_disk_set_standard_lookup (write_archive))
    {
        log_error ("could not set convenience functions %s", archive_error_string (write_archive));
        status = DISIR_STATUS_FS_ERROR;
        goto out;
    }

    status = dx_archive_open_read (archive_path, &read_archive);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        goto out;
    }

    status = DISIR_STATUS_OK;

    while (archive_read_next_header (read_archive, &entry) == ARCHIVE_OK)
    {
        snprintf(path, PATH_MAX, "%s/%s", extract_path, archive_entry_pathname (entry));
        extract_content.insert (std::make_pair (archive_entry_pathname (entry), path));
        archive_entry_set_pathname (entry, path);

        if (archive_write_header (write_archive, entry) == ARCHIVE_OK &&
                                  archive_entry_size (entry) > 0)
        {
            while (1)
            {
                ret = archive_read_data_block (read_archive, &buff, &size, &offset);
                if (ret == ARCHIVE_EOF)
                    break;

                if (ret != ARCHIVE_OK)
                {
                    log_error ("could not read archive data: %s",
                                archive_error_string (read_archive));
                    status = DISIR_STATUS_FS_ERROR;
                    goto out;
                }

                ret = archive_write_data_block (write_archive, buff, size, offset);
                if (ret != ARCHIVE_OK)
                {
                    log_error ("could not write archive to disk: %s",
                                archive_error_string (write_archive));
                    status = DISIR_STATUS_FS_ERROR;
                    goto out;
                }
            }
        }

        if (archive_write_finish_entry (write_archive) != ARCHIVE_OK)
        {
            log_error ("could not finish archive write entry: %s",
                        archive_error_string (read_archive));
            status = DISIR_STATUS_FS_ERROR;
            break;
        }
    }
    // FALL-THROUGH
out:
    if (write_archive)
    {
        if (archive_write_close (write_archive) != ARCHIVE_OK)
        {
            log_error ("unable to close write archive during extraction: %s",
                        archive_error_string (write_archive));
            status = DISIR_STATUS_FS_ERROR;
        }

        if (archive_write_free (write_archive) != ARCHIVE_OK)
        {
            log_error ("unable to free archive during extraction: %s",
                        archive_error_string (write_archive));
            status = DISIR_STATUS_FS_ERROR;
        }
    }

    if (read_archive)
    {
        if (archive_read_free (read_archive) != ARCHIVE_OK)
        {
            log_error ("unable to free read archive during extraction: %s",
                        archive_error_string (read_archive));
            status = DISIR_STATUS_FS_ERROR;
        }
    }

    return status;
}

//! STATIC FUNCTION
static void
dx_random_string (unsigned int length, unsigned int seed, char *dest)
{
    std::srand (std::time (0) + seed);

    for (auto i = 0u ; i < length; i++)
    {
        *(dest++) = std::rand() % 25 + 97;
    }

    *dest = '\0';
}

//! INTERNAL API
enum disir_status
dx_archive_open_write (struct disir_archive *archive)
{
    enum disir_status status;
    char temp_archive_path[PATH_MAX];
    char random_suffix[PATH_MAX];
    struct archive *ar;
    struct stat st;
    int ret;
    int retry_count = 1;

    if (archive == NULL)
    {
        log_debug (0, "invoked with NULL archive pointer");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    status = DISIR_STATUS_OK;

    ar = archive_write_new();
    if (ar == NULL)
    {
        log_debug (3, "unable to open new write archive");
        return DISIR_STATUS_NO_MEMORY;
    }

    if (archive_write_set_format (ar, ARCHIVE_FORMAT_TAR) != ARCHIVE_OK)
    {
        log_error ("unable set archive format to tar: %s", archive_error_string (ar));
        status = DISIR_STATUS_FS_ERROR;
        goto out;
    }

    if (archive_write_add_filter_xz (ar) != ARCHIVE_OK)
    {
        log_error ("unable to set archive compression: %s", archive_error_string (ar));
        status = DISIR_STATUS_FS_ERROR;
        goto out;
    }

    // Seed until temp file path is unique
    while (retry_count <= 100)
    {
        dx_random_string (10, retry_count, random_suffix);
        temp_archive_path[0] = '\0';
        snprintf (temp_archive_path, PATH_MAX, "/tmp/arc.%s", random_suffix);

        archive->da_temp_archive_path = strdup (temp_archive_path);
        if (archive->da_temp_archive_path == NULL)
        {
            status = DISIR_STATUS_NO_MEMORY;
        }
        ret = stat (temp_archive_path, &st);
        if (ret == 0)
        {
            log_debug (3, "archive with temp name '%s' already exists, retrying...",
                          temp_archive_path);
            status = DISIR_STATUS_FS_ERROR;
        }
        else
        {
            status = DISIR_STATUS_OK;
            break;
        }
        retry_count++;
    }

    if (status != DISIR_STATUS_OK)
    {
        log_error ("archive with temp name '%s' already exists, failed after %d retries",
                   temp_archive_path, retry_count);
        goto out;
    }

    if (archive_write_open_filename (ar, temp_archive_path) != ARCHIVE_OK)
    {
        log_error ("unable open archive on path '%s': %s",
                   temp_archive_path, archive_error_string (ar));
        status = DISIR_STATUS_FS_ERROR;
        goto out;
    }

    archive->da_archive = ar;
    // FALL-THROUGH
out:
    if (status != DISIR_STATUS_OK)
    {
        if (ar)
        {
            if (archive_write_close (ar) != ARCHIVE_OK)
                log_error ("unable to close archive: %s", archive_error_string (ar));

            if (archive_write_free (ar) != ARCHIVE_OK)
                log_error ("unable to free archive: %s", archive_error_string (ar));
        }
    }

    return status;
}

//! INTERNAL API
enum disir_status
dx_archive_open_read (const char *archive_path, struct archive **archive)
{
    enum disir_status status;
    int err;
    struct archive *ar;

    ar = archive_read_new();
    if (ar == NULL)
    {
        log_debug (3, "unable to open new read archive");
        return DISIR_STATUS_NO_MEMORY;
    }

    status = DISIR_STATUS_OK;

    if (archive_read_support_format_tar (ar) != ARCHIVE_OK)
    {
        log_error ("could not set read format: %s", archive_error_string (ar));
        status = DISIR_STATUS_FS_ERROR;
        goto out;
    }

    err = archive_read_support_filter_xz (ar);
    if (err != ARCHIVE_OK)
    {
        if (err != ARCHIVE_WARN)
        {
            log_error ("could not set read compression: %s", archive_error_string (ar));
            status = DISIR_STATUS_FS_ERROR;
            goto out;
        }
        else
        {
            log_warn ("%s", archive_error_string (ar));
        }
    }

    // For other devices than tape, LibArchive will adjust blocksize for performance.
    if (archive_read_open_filename (ar, archive_path, 10240) != ARCHIVE_OK)
    {
        log_error ("could not open archive: %s", archive_error_string (ar));
        status = DISIR_STATUS_FS_ERROR;
        goto out;
    }

    *archive = ar;
    // FALL-THROUGH
out:
    if (status != DISIR_STATUS_OK)
    {
        if (archive_read_free (ar) != ARCHIVE_OK)
            log_error ("unable to close read archive: %s", archive_error_string (ar));
    }

    return status;
}

//! STATIC FUNCTION
static enum disir_status
validate_archive_backend_entries (struct disir_archive *archive,
                                  std::map<std::string, std::string>& extract_content,
                                  const std::string backend,
                                  const toml::Value& groups)
{
    struct stat file_exist;
    struct disir_archive_entry archive_entry;
    toml::Value *entries_group_table;
    std::string config_entry;

    // Locate entries.toml within backend
    auto iter = extract_content.find (backend + "/entries.toml");
    if (iter == extract_content.end())
    {
        log_error ("cannot find entries.toml for backend '%s' in map", backend);
        return DISIR_STATUS_NOT_EXIST;
    }

    auto toml_filepath = iter->second;

    std::ifstream entries_toml (toml_filepath.c_str());
    toml::ParseResult pr = toml::parse (entries_toml);

    if (!pr.valid())
    {
        log_error ("unable to parse entries.toml in backend '%s'", backend);
        return DISIR_STATUS_FS_ERROR;
    }

    // Root in entries.toml
    toml::Value& entries_root = pr.value;

    // Iterate groups
    for (const auto& group : groups.as<toml::Array>())
    {
        // Find group table in entries.toml
        entries_group_table = entries_root.find (group.as<std::string>());

        if (entries_group_table == nullptr)
        {
            log_error ("unable to find the group '%s' in entries.toml, in backend '%s'",
                        group.as<std::string>(), backend);
            return DISIR_STATUS_NOT_EXIST;
        }

        if (entries_group_table->is<toml::Table>() == false)
        {
            log_error ("group '%s' in '%s' is not a toml table", group.as<std::string>(), backend);
            return DISIR_STATUS_WRONG_VALUE_TYPE;
        }

        // Locate config on disk
        for (const auto& entry : entries_group_table->as<toml::Table>())
        {
            config_entry = backend + "/" + group.as<std::string>() + "/" + entry.first;
            iter = extract_content.find (config_entry);
            if (iter == extract_content.end ())
            {
                log_error ("cannot find config in map: %s", config_entry.c_str());
                return DISIR_STATUS_NOT_EXIST;
            }

            // Validate existence of file on disk. Necessary?
            if (stat (iter->second.c_str(), &file_exist) != 0)
            {
                log_error ("file does not exist on disk: %s", config_entry.c_str());
                return DISIR_STATUS_NOT_EXIST;
            }

            archive_entry.de_backend_id = backend;
            archive_entry.de_group_id = group.as<std::string>();
            archive_entry.de_entry_id = entry.first;
            archive_entry.de_filepath = iter->second;
            archive_entry.de_version  = entry.second.as<std::string>();
            archive->da_config_entries->insert (archive_entry);
        }
    }

    return DISIR_STATUS_OK;
}

//! INTERNAL API
enum disir_status
dx_archive_validate (struct disir_archive *archive,
                     std::map<std::string, std::string>& extract_content)
{
    enum disir_status status;

    // Parse metadata.toml
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
        log_error ("unable to parse metadata.toml");
        return DISIR_STATUS_FS_ERROR;
    }

    toml::Value& root = pr.value;

    // Validate metadata.toml
    const toml::Value* org_version = root.find ("disir_org_version");
    if (org_version == nullptr)
    {
        log_error ("disir_org_version not found in metadata.toml");
        return DISIR_STATUS_NOT_EXIST;
    }

    if (org_version->is<std::string>() == false)
    {
        log_error ("disir_org_version is not a string in metadata.toml");
        return DISIR_STATUS_WRONG_VALUE_TYPE;
    }

    // TODO: Fix hardcoded org_version when available.
    if (strcmp (org_version->as<std::string>().c_str(), "0/1-draft"))
    {
        log_error ("'disir_org_version' in existing archive differs from current system");
        return DISIR_STATUS_NO_CAN_DO;
    }

    const toml::Value* implementation = root.find ("implementation");
    if (implementation == nullptr)
    {
        log_error ("'implementation' entry not found in metadata.toml");
        return DISIR_STATUS_NOT_EXIST;
    }

    if (implementation->is<std::string>() == false)
    {
        log_error ("'implementation' entry is not a string in metadata.toml");
        return DISIR_STATUS_WRONG_VALUE_TYPE;
    }

    if (strcmp (implementation->as<std::string>().c_str(), libdisir_version_string))
    {
        log_error ("'implementation' version in existing archive differs from current system");
        return DISIR_STATUS_NO_CAN_DO;
    }

    const toml::Value* backends = root.find (ATTRIBUTE_KEY_BACKEND);
    if (backends == nullptr)
    {
        log_error ("no backends present in metadata.toml");
        return DISIR_STATUS_NOT_EXIST;
    }

    if (backends->is<toml::Array>() == false)
    {
        log_error ("metadata backend is not a toml array");
        return DISIR_STATUS_WRONG_VALUE_TYPE;
    }

    for (const auto& entry : backends->as<toml::Array>())
    {
        auto id = entry.find (ATTRIBUTE_KEY_ID);

        if (id == nullptr)
        {
            log_error ("key 'id' not present in backend");
            return DISIR_STATUS_NOT_EXIST;
        }

        if (id->is<std::string>() == false)
        {
            log_error ("value of key 'id' in backend is not a string");
            return DISIR_STATUS_WRONG_VALUE_TYPE;
        }

        auto groups = entry.find (ATTRIBUTE_KEY_GROUPS);
        if (groups == nullptr)
        {
            log_error ("key 'groups' not present in backend '%s'", id->as<std::string>());
            return DISIR_STATUS_NOT_EXIST;
        }

        if (groups->is<toml::Array>() == false)
        {
            log_error ("value of key 'groups' in backend is not an array");
            return DISIR_STATUS_WRONG_VALUE_TYPE;
        }

        if (groups->empty())
        {
            log_error ("'groups' array in backend '%s' is empty", id->as<std::string>());
            return DISIR_STATUS_NOT_EXIST;
        }

        // Validate entries.toml
        status = validate_archive_backend_entries (archive, extract_content,
                                                   id->as<std::string>(), *groups);
        if (status != DISIR_STATUS_OK)
        {
            // Already logged
            return status;
        }
    }

    return DISIR_STATUS_OK;
}

//! STATIC FUNCTION
static int
remove_folder_and_content (const char *fpath, const struct stat *sb,
                           int typeflag, struct FTW *ftwbuf)
{
    (void) sb;
    (void) ftwbuf;
    (void) typeflag;
    std::string path_formatted = fpath;
    std::string tmp_folder = path_formatted.substr(0, path_formatted.find("/", 1));

    // Safeguard: Make sure path resides in tmp folder.
    if (tmp_folder != "/tmp")
    {
        log_error ("tried to remove folder '%s' which is outside /tmp", fpath);
        return DISIR_STATUS_FS_ERROR;
    }

    int ret = remove (fpath);

    if (ret)
    {
        log_debug (5, "could not remove '%s'", fpath);
    }

    return ret;
}

//! INTERNAL API
enum disir_status
dx_archive_destroy (struct disir_archive *ar)
{
    struct stat st;
    int ret;

    // Remove temp files
    if (ar && ar->da_extract_folder_path && stat (ar->da_extract_folder_path, &st) == 0)
    {
        // Use 'remove_path' duplicate as remove function may modify arguments.
        const char *remove_path = ar->da_extract_folder_path;
        ret = nftw (remove_path, remove_folder_and_content, 64, FTW_DEPTH | FTW_PHYS);
        if (ret != 0)
        {
            // Already logged
            return DISIR_STATUS_FS_ERROR;
        }
    }

    if (ar && ar->da_entries)
    {
        for (auto& entry : *ar->da_entries)
        {
            delete entry.second;
        }
        delete ar->da_entries;
    }

    if (ar && ar->da_temp_archive_path)
        free (ar->da_temp_archive_path);
    if (ar && ar->da_existing_path)
        free (ar->da_existing_path);
    if (ar && ar->da_metadata != nullptr)
        delete ar->da_metadata;
    if (ar && ar->da_extract_folder_path)
        free (ar->da_extract_folder_path);
    if (ar && ar->da_config_entries != nullptr)
        delete ar->da_config_entries;
    if (ar)
        free (ar);

    return DISIR_STATUS_OK;
}

//! INTERNAL API
enum disir_status
dx_assert_write_permission (const char *archive_path)
{
    struct stat st;
    int ret;

    // Assert that directory exists
    ret = stat (archive_path, &st);
    if (ret != 0)
    {
        log_error ("unable to write: '%s' does not exist", archive_path);
        return DISIR_STATUS_NOT_EXIST;
    }

    // Assert write permission
    if (access (archive_path, W_OK) != 0)
    {
        log_error ("no write permission for folder: '%s'", archive_path);
        return DISIR_STATUS_PERMISSION_ERROR;
    }

    return DISIR_STATUS_OK;
}

//! INTERNAL API
enum disir_status
dx_assert_read_permission (const char *archive_path)
{
    struct stat st;
    int ret;

    // Assert that directory exists
    ret = stat (archive_path, &st);
    if (ret != 0)
    {
        log_error ("unable to read: '%s' does not exist", archive_path);
        return DISIR_STATUS_NOT_EXIST;
    }

    // Assert read permission
    if (access (archive_path, R_OK) != 0)
    {
        log_error ("no read permission for folder: '%s'", archive_path);
        return DISIR_STATUS_PERMISSION_ERROR;
    }

    return DISIR_STATUS_OK;
}

//! STATIC FUNCTION
static enum disir_status
copy_file (const char *from, const char *to)
{
    std::ifstream source (from, std::ios::binary);
    if (source.is_open() == false)
    {
        return DISIR_STATUS_FS_ERROR;
    }

    std::ofstream destination (to, std::ios::binary);
    if (destination.is_open() == false)
    {
        return DISIR_STATUS_FS_ERROR;
    }

    destination << source.rdbuf();

    return DISIR_STATUS_OK;
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
        log_error ("cannot find parent directory of file at path '%s'", archive_path);
        status = DISIR_STATUS_FS_ERROR;
    }

    strcpy (dirpath, temp_dirname);
    free (temp_path);
    return status;
}

//! INTERNAL API
enum disir_status
dx_archive_disk_append (struct disir_instance *instance, const char *new_archive_path,
                        const char *existing_archive_path, const char *temp_archive_path)
{
    enum disir_status status;
    int ret;
    char backup_path[4096];
    char archive_path_with_extension[4096];
    const char *ext = NULL;
    char extract_folder_path[PATH_MAX];

    strcpy (archive_path_with_extension, new_archive_path);

    // Append extension if not already exists
    ext = strrchr (new_archive_path, '.');
    if (ext == NULL || strcmp (ext, ".disir") != 0)
    {
        strcat (archive_path_with_extension, ".disir");
    }

    // CASE 1: Finalizing a new archive to given location
    if (existing_archive_path == NULL)
    {
        status = get_dirname (new_archive_path, extract_folder_path);
        if (status != DISIR_STATUS_OK)
        {
            return status;
        }

        status = dx_assert_write_permission (extract_folder_path);
        if (status != DISIR_STATUS_OK)
        {
            disir_error_set (instance, "unable to write to path: '%s'", extract_folder_path);
            return status;
        }

        status = copy_file (temp_archive_path, archive_path_with_extension);
        if (status != DISIR_STATUS_OK)
        {
            log_error ("failed to move archive to '%s': %s", archive_path_with_extension,
                                                             strerror (errno));
            return DISIR_STATUS_FS_ERROR;
        }
    }

    // CASE 2: If working on existing archive and want to export result to a new location
    if (existing_archive_path && strcmp (existing_archive_path, archive_path_with_extension) != 0)
    {
        status = copy_file (temp_archive_path, archive_path_with_extension);
        if (status != DISIR_STATUS_OK)
        {
            log_error ("failed to move archive to '%s': %s", archive_path_with_extension,
                                                             strerror (errno));
            return DISIR_STATUS_FS_ERROR;
        }

        status = get_dirname (existing_archive_path, extract_folder_path);
        if (status != DISIR_STATUS_OK)
        {
            return status;
        }

        status = dx_assert_write_permission (extract_folder_path);
        if (status != DISIR_STATUS_OK)
        {
            disir_error_set (instance, "unable to write to path: '%s'", extract_folder_path);
            return status;
        }

        ret = remove (existing_archive_path);
        if (ret != 0)
        {
            log_error ("failed to remove existing archive in '%s': %s",
                        existing_archive_path, strerror (errno));
            return DISIR_STATUS_FS_ERROR;
        }
    }

    // CASE 3: Existing archive should be overwritten
    if (existing_archive_path &&
        (strcmp (existing_archive_path, archive_path_with_extension) == 0))
    {
        status = get_dirname (existing_archive_path, extract_folder_path);
        if (status != DISIR_STATUS_OK)
        {
            return status;
        }

        status = dx_assert_write_permission (extract_folder_path);
        if (status != DISIR_STATUS_OK)
        {
            disir_error_set (instance, "unable to write to path: '%s'", extract_folder_path);
            return status;
        }

        // Rename existing archive with backup extension.
        *backup_path = '\0';
        strcat (backup_path, existing_archive_path);
        strcat (backup_path, ".backup");
        ret = rename (existing_archive_path, backup_path);
        if (ret != 0)
        {
            log_error ("failed to create backup of existing archive: %s", strerror (errno));
            return DISIR_STATUS_FS_ERROR;
        }

        // Rename new archive to same as existing
        status = copy_file (temp_archive_path, existing_archive_path);
        if (status != DISIR_STATUS_OK)
        {
            log_error ("failed to rename archive '%s': %s", temp_archive_path, strerror (errno));

            // Error again?! Re-rename existing file back to original name
            ret = rename (backup_path, existing_archive_path);
            if (ret != 0)
            {
                log_error ("failed to rename archive back to existing name: %s", strerror (errno));
            }
            return DISIR_STATUS_FS_ERROR;
        }

        // All good, remove old archive
        ret = remove (backup_path);
        if (ret != 0)
        {
            log_error ("failed to remove original archive: %s", strerror (errno));
            return DISIR_STATUS_FS_ERROR;
        }
    }

    return DISIR_STATUS_OK;
}
