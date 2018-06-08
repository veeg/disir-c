#ifndef _LIBDISIR_ARCHIVE_H
#define _LIBDISIR_ARCHIVE_H

#ifdef __cplusplus
extern "C"{
#endif // _cplusplus

#include <disir/disir.h>

// Forward decleration
struct disir_archive;
struct disir_import;

//! Conatiner holding failed and succesfully imported archive entries
struct import_report
{
    int ir_entries;
    //! result of an import: "STATUS: entry_id (version)"
    char **ir_entry;

    int ir_internal;
};

//! disir import options
enum disir_import_option
{
    //! if config to be imported is of a lower
    //! version than mold, non-default key-values will be
    //! applied in the upgrade.
    DISIR_IMPORT_UPDATE = 1,
    //! if imported config conflicts with an equivalent config
    //! on the system, regardless of semver conflicts, the import
    //! must be forced
    DISIR_IMPORT_FORCE,
    //! if config to be imported does not exist on the system and
    //! its version is equal to mold version on system.
    DISIR_IMPORT_DO,
    //! discard a configuration entry
    DISIR_IMPORT_DISCARD,
    //! if an upgrade results in violating restrictions untroduced later than
    //! the imported config's version, this option will discard those values.
    DISIR_IMPORT_UPDATE_WITH_DISCARD,
};

//! \brief Begin exporting a new or existing archive.
//!
//! Function initiates exporting a new or already existing archive
//! containing configs. It returns a disir_archive structure which can
//! be further populated with entries by calls to disir_archive_append_entry or
//! disir_archive_append_group.
//!
//! \param[in] instance The disir instance.
//! \param[in] archive_path Filepath either path to an archive to which entries can be
//!     appended, or NULL to start a new one.
//! \param[out] archive The disir archive structure containing archive state.
//!
//! \return DISIR_STATUS_OK on success.
//! \return DISIR_STATUS_FS_ERROR if archive is invalid. May involve invalid metadata, failure to
//!     extract data, failure when writing to new archive or writing config entries to disk.
//! \return DISIR_STATUS_INVALID_ARGUMENT if either of the inputs are NULL or
//!     invalid options.
//! \return DISIR_STATUS_NO_MEMORY if unable to allocate enough memory for disir archive structure.
//!
DISIR_EXPORT
enum disir_status
disir_archive_export_begin (struct disir_instance *instance,
                            const char *archive_path, struct disir_archive **archive);

//! \brief Append configs within group to a disir archive.
//!
//! Function will append all configs within the given group to an archive created
//! by a disir_archive_export_begin call. If the group already exist in
//! the archive, an error is returned.
//!
//! \param[in] instance The disir instance.
//! \param[in] archive The open disir archive initiated by disir_archive_export_begin.
//! \param[in] group_id Name of group whose configs shall be appended.
//!
//! \return DISIR_STATUS_OK on success.
//! \return DISIR_STATUS_INVALID_ARGUMENT if either of the inputs are NULL or
//!     invalid options.
//! \return DISIR_STATUS_EXISTS if appending group already exist in archive.
//! \return DISIR_STATUS_NOT_EXIST if no config entries exists for given group id.
//! \return DISIR_STATUS_FS_ERROR if unable to write to disk or clean up temporary files.
//! \return DISIR_STATUS_NO_CAN_DO if plugin does not support serializing configuration entries.
//!
DISIR_EXPORT
enum disir_status
disir_archive_append_group (struct disir_instance *instance, struct disir_archive *archive,
                            const char *group_id);

//! \brief Append a config entry to a disir archive.
//!
//! Function will append a single config entry to an archive created
//! by a disir_archive_export_begin call. If the entry already exists
//! in the archive, an error is returned.
//!
//! \param[in] instance The disir instance.
//! \param[in] archive The open disir archive initiated by disir_archive_export_begin.
//! \param[in] group_id Name of group in which the entry resides.
//! \param[in] entry_id The config's entry ID.
//!
//! \return DISIR_STATUS_OK on success.
//! \return DISIR_STATUS_INVALID_ARGUMENT if either of the inputs are NULL or
//!     invalid options.
//! \return DISIR_STATUS_EXISTS if appending entry id already exist in archive.
//! \return DISIR_STATUS_FS_ERROR if unable to write to disk or clean up temporary files.
//! \return DISIR_STATUS_NO_CAN_DO if plugin does not support serializing configuartion entries.
//!
DISIR_EXPORT
enum disir_status
disir_archive_append_entry (struct disir_instance *instance, struct disir_archive *archive,
                            const char *group_id, const char *entry_id);

//! \brief Finalizes and writes a disir archive to disk.
//!
//! Function finalizes a disir archive by writing the archive metadata, cleaning up
//! temporary extraction folders and deallocates the disir archive structure.
//! The archive is closed and written to disk at the provided filepath. If
//! filepath is the same as an existing archive, it will be overwritten (with care).
//! If called with archive_path = NULL, the archive is discarded.
//!
//! \param[in] instance The disir instance.
//! \param[in] archive_path The path where the finished archive is written. Null to discard
//      the archive entirely. This path will be modified to include a trailing '.disir'
//      if it is not part of the input archive_path.
//! \param[in] archive The disir archive to be finalized.
//!
//! \return DISIR_STATUS_OK on success.
//! \return DISIR_STATUS_FS_ERROR if unable to properly clean up temp files, or remove
//!     or rename existing archives.
//! \return DISIR_STATUS_INVALID_ARGUMENT if archive_path is invalid (e.g. path to a directory).
//! \return DISIR_STATUS_PERMISSION_ERROR if insufficient permissions to write archive
//!     to given path. Archive will not be destroyed and caller must re-call
//!     finalize with a new (valid) path or discard it with archive_path = NULL.
//! \return DISIR_STATUS_NOT_EXIST if given path does not exist. Archive will not be
//!     destroyed and caller must re-call finalize with a new (valid) path or discard it with
//!     archive_path = NULL.
//!
DISIR_EXPORT
enum disir_status
disir_archive_finalize (struct disir_instance *instance, const char *archive_path,
                        struct disir_archive **archive);

//! \brief Retrive config entries from disir_archive.
//!
//! Function will return an import structure containing the configs
//! that were retrieved from the disir_archive along with the number of entries
//! Info about the entries may be retrieved from disir_import_entry_status.
//!
//! \param[in] instance The disir instance.
//! \param[in] archive_path Filepath to the disir archive
//! \param[out] import The structure containing import state.
//! \param[out] import_entries The number of configuration entries
//!
//! \return DISIR_STATUS_OK on success.
//! \return DISIR_STATUS_FS_ERROR if archive is invalid.
//! \return DISIR_STATUS_NOT_EXIST if no archive on archive_path exist.
//!
DISIR_EXPORT
enum disir_status
disir_archive_import (struct disir_instance *instance, const char *archive_path,
                      struct disir_import **import, int *entries);

//! \brief Retrieve information about a single configuration entry.
//!
//! Function will return the information about a import configuration entry
//! denoted by an index.
//!
//! \param[in] import The import structure retrived from disir_archive_import.
//! \param[in] entry Index to the archive entry.
//! \param[out] entry_id The config's entry_id.
//! \param[out] group_id Name of which group the entry_id belongs to.
//! \param[out] version Version of the archive entry.
//! \param[out] info spesific status info - NULL if not populated.
//!
//! \return DISIR_STATUS_OK if an import of config on entry_id causes no conflicts.
//! \return DISIR_STATUS_CONFLICT if importing the entry will conflict with existing entry.
//! \return DISIR_STATUS_CONFLICTING_SEMVER either the archive entry and its equivalent
//      on the system only differs in version,
//!     or there is no equivalent entry on the system and
//!     the archive entry's version is lower than mold version.
//! \return DISIR_STATUS_NO_CAN_DO if config cannot be imported.
//! \return DISIR_STATUS_CONFIG_INVALID if config read from archive is invalid.
//! \return DISIR_STATUS_ARGUMENT_INVALID if one or several input parameters are NULL, or
//!          index *entry* is out of range.
//!
DISIR_EXPORT
enum disir_status
disir_import_entry_status (struct disir_import *import, int entry,
                           const char **entry_id, const char **group_id,
                           const char **version, const char **info);

//! \brief Choose how to resolve import of a config.
//!
//! \param[in] import The disir_import struct
//! \param[in] entry The index of which entry to resolve.
//! \param[in] opt Options for how to resolve import.
//!     for valid options see disir_import_option.
//!
//! \return DISIR_STATUS_OK if import can be resolved.
//! \return DISIR_STATUS_NO_CAN_DO if config cannot be imported,
//      or if the import option is disallowed based on the entry's import state.
//! \return DISIR_STATUS_RESTRICTION_VIOLATED if option is UPDATE and
//!          a keyval set on a newer config version violates a restriction
//!          introduced on a higher version than the imported config.
//!
DISIR_EXPORT
enum disir_status
disir_import_resolve_entry (struct disir_import *import,
                            int entry, enum disir_import_option opt);

//! \brief Import resolved archive entries.
//!
//! Either discard or commit resolved entries
//!
//! \param[in] instance The disir instance.
//! \param[in] opt Finalize options.
//!     Valid options are:
//!         * DISIR_IMPORT_DO      - commit resolved configs
//!         * DISIR_IMPORT_DISCARD - abort import
//! \param[in] import The structure holding disir_import
//! \param[out] report Import summary - not populated if NULL.
//!
//! \return DISIR_STATUS_OK on success.
//! \return DISIR_STATUS_INVALID_ARGUMENT if either of the inputs are NULL or invalid options.
//!
DISIR_EXPORT
enum disir_status
disir_import_finalize (struct disir_instance *instance, enum disir_import_option opt,
                       struct disir_import **import, struct import_report **report);

//! \brief Destroy import summary structure.
DISIR_EXPORT
enum disir_status
disir_import_report_destroy (struct import_report **report);

#ifdef __cplusplus
}
#endif // _cplusplus

#endif // _LIBDISIR_ARCHIVE_H
