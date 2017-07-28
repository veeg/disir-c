#ifndef _LIBDISIR_IMPORT_H
#define _LIBDISIR_IMPORT_H

//! Defines the state of a single archive entry
struct disir_import_entry
{
    //! Name of imported config
    char *ie_entry_id;
    //! To which group the entry belongs
    char *ie_group_id;
    //! From which backend this entry was serialized
    char *ie_backend_id;
    //! Archive entry info
    char *ie_info;
    //! Version of archive entry
    char *ie_version;
    //! current conflict status of config - can be one of the following:
    //!   * DISIR_STATUS_OK if equivalent config does not exist on system already.
    //!   * DISIR_STATUS_CONFLICT if config has conflicting entries with one on the system.
    //!   * DISIR_STATUS_NO_CAN_DO if in any way config cannot be imported.
    //!   * DISIR_STATUS_CONFIG_INVALID if the serialized archive entry is invalid.
    //!   * DISIR_STATUS_CONFLICTING_SEMVER if either config versions mismatch or, config
    //!         version mismatches with mold version, if config does not exist on system.
    enum disir_status ie_status;
    //! Whether this config will be imported or not
    unsigned short ie_import;
    //! Set if config conflicts with  an existing equivalent
    unsigned short ie_conflict;
    //! In-memory reference to archive entry config
    struct disir_config *ie_config;
};

//! Holds all config entries from an import
struct disir_import
{
    //! Number of entries
    int di_num_entries;
    //! Array of import entries
    struct disir_import_entry **di_entries;
};

//! Destroy all import entries and entry
enum disir_status
dx_import_destroy (struct disir_import *entry);

//! Destroy a single entry
enum disir_status
dx_import_entry_destroy (struct disir_import_entry *entry);

//! \brief Determine whether an archive entry conflicts or
//!     simply cannot be imported.
enum disir_status
dx_resolve_config_import_status (struct disir_instance *instance,
                                 const char *config_path,
                                 struct disir_import_entry *import);

#endif // _LIBDISIR_IMPORT_H
