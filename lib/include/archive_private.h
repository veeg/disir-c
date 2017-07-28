#ifndef _LIBDISIR_PRIVATE_ARCHIVE_H
#define _LIBDISIR_PRIVATE_ARCHIVE_H

#include "tinytoml/toml.h"
#include <disir/archive.h>
#include <set>
#include <libgen.h>

//! Convenience macros:
//! toml key for a backend table
#define ATTRIBUTE_KEY_BACKEND "backend"
//! toml key for entry_id
#define ATTRIBUTE_KEY_ID "id"
//! toml key for gourp table
#define ATTRIBUTE_KEY_GROUPS "groups"
//! name of metadata file in archive
#define METADATA_FILENAME "metadata.toml"

// Spesifies a config entry in a disir archive
struct disir_archive_entry
{
    // The backend id from which a config is serialized
    std::string de_backend_id;
    // The group to which the config belongs
    std::string de_group_id;
    // The entry if of the config
    std::string de_entry_id;
    // path to its location on disk
    std::string de_filepath;
    // archive entry version (config)
    std::string de_version;
};

// compare function for disir_archive_entry
struct cmp_entry
{
    bool operator()(const struct disir_archive_entry& a, const struct disir_archive_entry& b)
    {
        return a.de_backend_id < b.de_backend_id || a.de_group_id < b.de_group_id ||
               a.de_entry_id < b.de_entry_id;
    }
};

//! Holding structures involving archive operations
struct disir_archive
{
    // libarchive object
    struct archive *da_archive;
    // Path for temporary extraction
    char *da_extract_folder_path;
    // Temp archive path for until finalize
    char *da_temp_archive_path;
    // Existing archive path
    char *da_existing_path;
    // Archive metadata
    toml::Value *da_metadata;
    // Archive entry index
    std::map<std::string, toml::Value*> *da_entries;
    // Config entries in open disir_archive
    std::set<struct disir_archive_entry, cmp_entry> *da_config_entries;
};

//! Create a disir_archive
enum disir_status
dx_archive_create (struct disir_archive **archive);

//! Extracts the contents of archive to 'extract_path' on disk and populate
//! 'extract_content' with all its entries and their location on disk.
enum disir_status
dx_archive_extract (const char *archive_path, std::map<std::string, std::string>& extract_content,
                    const char *extract_path);

//! Opens the archive in write-mode.
//! Caller is responsible for closing archive, unless disir_archive_finalize is called.
enum disir_status
dx_archive_open_write (struct disir_archive *archive);

//! Opens the archive in read-mode.
//! Caller is responsible for closing archive when finished.
enum disir_status
dx_archive_open_read (const char *archive_path, struct archive **archive);

//! Validates the integrity of the entries retrieved from dx_archive_extract
enum disir_status
dx_archive_validate (struct disir_archive *archive,
                     std::map<std::string, std::string>& archive_entries);

//! Begin existing archive referred to by 'archive_path'
enum disir_status
dx_archive_begin_existing (struct disir_instance *instance, const char *archive_path,
                           struct disir_archive **archive);

//! Begin new archive open for appends
enum disir_status
dx_archive_begin_new (struct disir_archive **archive);

//! Write config entries to archive.
enum disir_status
dx_archive_config_entries_write (struct disir_instance *instance, struct disir_archive *archive,
                                 struct disir_register_plugin *plugin,
                                 struct disir_entry *config_entry,
                                 const char *group_id);

//! Write archive metadata (metadata.toml and entries.toml)
enum disir_status
dx_archive_metadata_write (struct disir_archive *archive);

//! Create a temporary folder in /tmp
char *
dx_archive_create_temp_folder (void);

//! Move archive to given location on disk. Overwrite existing archive with backup.
enum disir_status
dx_archive_disk_append (struct disir_instance *instance, const char *new_archive_path,
                        const char *existing_archive_path, const char *temp_archive_path);

//! Clean up temporary files and free data structures
enum disir_status
dx_archive_destroy (struct disir_archive *ar);

#endif // _LIBDISIR_PRIVATE_ARCHIVE_H
