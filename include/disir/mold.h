#ifndef _LIBDISIR_MOLD_H
#define _LIBDISIR_MOLD_H

#ifdef __cplusplus
extern "C"{
#endif // __cplusplus

#include <disir/disir.h>

//!
//! This file exposes the high level Disir mold API.
//!


//! \brief Input a mold entry from the Disir instance.
//!
//! Read a Mold object from `disir` identified by `entry_id`.
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
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
disir_mold_finished (struct disir_mold **mold);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _LIBDISIR_MOLD_H

