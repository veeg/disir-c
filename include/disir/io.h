#ifndef _LIBDISIR_IO_H
#define _LIBDISIR_IO_H

#ifdef __cplusplus
extern "C"{
#endif // _cplusplus


//! Disir Entry - holds information about single config/mold entries.
struct disir_entry
{
    //! Name of this entry.
    char                *de_entry_name;
    union
    {
        uint64_t        de_attributes;
        struct
        {
                        //! Entry is readbale by the user who queried this entry.
            uint64_t    DE_READABLE                 : 1,
                        //! Entry is writable by the user who queried this entry.
                        DE_WRITABLE                 : 1,
                        //! Entry is valid for all subentries of this namespace.
                        DE_NAMESPACE_ENTRY          : 1,
                                                    : 0;
        };
    };

    //! Double-linked list pointers.
    struct disir_entry *next, *prev;
};


//! \brief Free a entry structure returned from a I/O API.
//!
//! \return DISIR_STATUS_OK
enum disir_status
disir_entry_finished (struct disir_entry **entry);

//! \brief Input a config entry from the disir instance.
//!
//! Read a Config object from `disir` identified by ID `entry_id`.
//! The `mold` argument may optionally override the internal mold associated
//! wth `entry_id`. If supplied, it is the callers responsibility to invoke disir_mold_finished()
//! on this instance. Normally, this argument should be NULL.
//!
//! \param[in] instance Library instance.
//! \param[in] group_id String identifier for the which group to look for entry.
//! \param[in] entry_id String identifier for the config entry to read.
//! \param[in] mold Optional mold that describes the config to parse. If NULL,
//!     I/O plugin must locate a mold instead.
//! \param[out] config Object to populate with the state read from 'id'
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if either of arguments `disir`,
//      `entry_id` or `config` are NULL.
//! \return DISIR_STATUS_NO_CAN_DO if the matching plugin
//!     for some reason does not implement config_write operation.
//! \return DISIR_STATUS_NOT_EXIST if the plugin associated with `config`
//!     is not registered with `disir`.
//! \return status of the plugin config_read operation.
//!
enum disir_status
disir_config_read (struct disir_instance *instance, const char *group_id, const char *entry_id,
                   struct disir_mold *mold, struct disir_config **config);

//! \brief Output the config object to the disir instance.
//!
//! \param[in] instance Library instance.
//! \param[in] group_id String identifier for the which group to look for entry.
//! \param[in] entry_id String identifier for output location.
//! \param[in] config The config object to output.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if either of the input arguments are NULL.
//! \return DISIR_STATUS_NO_CAN_DO if the `config` object is not associated with a plugin.
//! \return DISIR_STATUS_NO_CAN_DO if the matching plugin
//!     for some reason does not implement config_write operation.
//! \return DISIR_STATUS_NOT_EXIST if the plugin associated with `config`
//!     is not registered with `disir`.
//! \return status of the plugin config_write operation.
//!
enum disir_status
disir_config_write (struct disir_instance *instance, const char *group_id,
                    const char *entry_id, struct disir_config *config);

//! \brief List all the available config entries, from all plugins.
//!
//! \param[in] group_id String identifier for the which group to look for entry.
//!
//! Duplicate entries from plugins who is superseeded by another plugin
//! is not returned.
//! Populates the `entries` pointer with the first returned entry in a double-linked
//! list of disir_entry's. The caller is responsible to invoke disir_entry_finished ()
//! on (every) entry returned when he is finished with the queried result.
//!
//! Call does not fail if any of the plugins registered with `disir` fail.
//! The aggregated sucessful calls to plugins is gahtered in the output `entries`
//! argument and DISIR_STATUS_OK is returned.
//! All calls that exit with a non-OK status code from a plugin is logged as a warning.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if either `disir` or `entries` are NULL.
//! \return DISRI_STATUS_NO_MEMORY if internal memory allocations fail.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
disir_config_entries (struct disir_instance *instance,
                      const char *group_id, struct disir_entry **entries);

//! \brief Query for the existence of a config entry within a group.
//!
//! \param[out] entry Optional output structure containing entry details. If supplied,
//!     caller must free the entry struct with disir_entry_finished().
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if either `instance`, `group_id` or `entry_id` are NULL.
//! \return DISIR_STATUS_NOT_EXIST if the entry does not exist.
//! \return DISIR_STATUS_EXISTS if the entry exists.
//!
enum disir_status
disir_config_query (struct disir_instance *instance, const char *group_id,
                    const char *entry_id, struct disir_entry **entry);

//! \brief Mark yourself finished with the configuration object
//!
//! \param[in,out] config Object to mark as finished. Turns the pointer to NULL
//!     on success.
//!
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
disir_config_finished (struct disir_config **config);

//! \brief Input a mold entry from the Disir instance.
//!
//! Read a Mold object from `disir` identified by ID `entry_id`.
//!
//! \param[in] instance Library instance.
//! \param[in] group_id String identifier for the which group to look for entry.
//! \param[in] entry_id String identifier for the config entry to read.
//! \param[out] mold Object to populate with the state read from `entry_id`.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if either of arguments `disir`,
//      `entry_id` or `mold` are NULL.
//! \return DISIR_STATUS_NO_CAN_DO if the matching plugin
//!     for some reason does not implement mold_write operation.
//! \return DISIR_STATUS_NOT_EXIST if the plugin associated with `mold`
//!     is not registered with `disir`.
//! \return status of the plugin mold_read operation.
//!
enum disir_status
disir_mold_read (struct disir_instance *instance, const char *group_id,
                 const char *entry_id, struct disir_mold **mold);

//! \brief Output the mold object to the Disir instance.
//!
//! \param[in] instance Library instance.
//! \param[in] group_id String identifier for the which group to look for entry.
//! \param[in] entry_id String identifier for output location.
//! \param[in] mold The mold object to output.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if either of the input arguments are NULL.
//! \return DISIR_STATUS_NO_CAN_DO if the `mold` object is not associated with a plugin.
//! \return DISIR_STATUS_NO_CAN_DO if the matching plugin
//!     for some reason does not implement mold_write operation.
//! \return DISIR_STATUS_NOT_EXIST if the plugin associated with `mold`
//!     is not registered with `disir`.
//! \return status of the plugin mold_write operation.
//!
enum disir_status
disir_mold_write (struct disir_instance *instance, const char *groupd_id,
                  const char *entry_id, struct disir_mold *mold);

//! \brief List all the available mold entries, from all plugins.
//!
//! \param[in] group_id String identifier for the which group to look for entry.
//!
//! Duplicate entries from plugins who is superseeded by another plugin
//! is not returned.
//! Populates the `entries` pointer with the first returned entry in a double-linked
//! list of disir_entry's. The caller is responsible to invoke disir_entry_finished ()
//! on (every) entry returned when he is finished with the queried result.
//!
//! Call does not fail if any of the plugins registered with `disir` fail.
//! The aggregated sucessful calls to plugins is gahtered in the output `entries`
//! argument and DISIR_STATUS_OK is returned.
//! All calls that exit with a non-OK status code from a plugin is logged as a warning.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if either `disir` or `entries` are NULL.
//! \return DISRI_STATUS_NO_MEMORY if internal memory allocations fail.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
disir_mold_entries (struct disir_instance *instance,
                    const char *group_id, struct disir_entry **entries);

//! \brief Query for the existence of a mold entry within a group.
//!
//! \param[out] entry Optional output structure containing entry details. If supplied,
//!     caller must free the entry struct with disir_entry_finished().
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if either `instance`, `group_id` or `entry_id` are NULL.
//! \return DISIR_STATUS_NOT_EXIST if the entry does not exist.
//! \return DISIR_STATUS_EXISTS if the entry exists.
//!
enum disir_status
disir_mold_query (struct disir_instance *instance, const char *group_id,
                  const char *entry_id, struct disir_entry **entry);

//! \brief Mark yourself finished with the mold object.
//!
//! \\param[in,out] mold Object to mark as finished. Turns the pointer to NULL
//!     on success.
//!
//! \return DISIR_STATUS_OK on success
//!
enum disir_status
disir_mold_finished (struct disir_mold **mold);


#ifdef __cplusplus
}
#endif // _cplusplus

#endif // _LIBDISIR_IO_H

