#ifndef _LIBDISIR_ARCHIVE_H
#define _LIBDISIR_ARCHIVE_H

// Forward decleration
struct disir_archive;
struct disir_import;

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
//!         extract data, failure when writing to new archive or writing config entries to disk.
//! \return DISIR_STATUS_INVALID_ARGUMENT if either of the inputs are NULL or
//!         invalid options.
//! \return DISIR_STATUS_NO_MEMORY if unable to allocate enough memory for disir archive structure.
//!
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
//!         invalid options.
//! \return DISIR_STATUS_EXISTS if appending group already exist in archive.
//! \return DISIR_STATUS_NOT_EXIST if no config entries exists for given group id.
//! \return DISIR_STATUS_FS_ERROR if unable to write to disk or clean up temporary files.
//! \return DISIR_STATUS_NO_CAN_DO if plugin does not support serializing configuration entries.
//!
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
//!         invalid options.
//! \return DISIR_STATUS_EXISTS if appending entry id already exist in archive.
//! \return DISIR_STATUS_FS_ERROR if unable to write to disk or clean up temporary files.
//! \return DISIR_STATUS_NO_CAN_DO if plugin does not support serializing configuartion entries.
//!
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
//      the archive entirely.
//! \param[in] archive The disir archive to be finalized.
//!
//! \return DISIR_STATUS_OK on success.
//! \return DISIR_STATUS_FS_ERROR if unable to properly clean up temp files, or remove
//!         or rename existing archives.
//! \return DISIR_STATUS_PERMISSION_ERROR if insufficient permissions to
//!         files/folders on disk.
//! \return DISIR_STATUS_NOT_EXIST if folders or files involving exported archive
//!         does not exist.
//!
enum disir_status
disir_archive_finalize (struct disir_instance *instance, const char *archive_path,
                        struct disir_archive **archive);

#endif // _LIBDISIR_ARCHIVE_H
