#ifndef _LIBDISIR_CONFIG_H
#define _LIBDISIR_CONFIG_H

#ifdef __cplusplus
extern "C"{
#endif // __cplusplus

#include <disir/disir.h>

//!
//! This file exposes the high level Disir Config API.
//!


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
//! \return DISIR_STATUS_INVALID_CONTEXT is returned when the config retrieved is invalid
//!     according to its mold definition. disir_error is not set on the instance, since the
//!     invalid state lies in the individual context elements of the config. Iterate all child
//!     contexts and test state retrieve the erroneous state.
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

//! \brief Remove a configuration entry from the group.
//!
//! \param[in] instance Library instance.
//! \param[in] group_id String identifier for the which group to look for entry.
//! \param[in] entry_id String identifier to remove.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if either of the input arguments are NULL.
//! \return DISIR_STATUS_NO_CAN_DO if the matching plugin
//!     for some reason does not implement config_write operation.
//! \return DISIR_STATUS_NOT_EXIST if the plugin associated with `group`
//!     is not registered with `instance`.
//! \return status of the plugin config_remove operation.
//!
enum disir_status
disir_config_remove (struct disir_instance *instance, const char *group_id,
                     const char *entry_id);

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
//! \return DISIR_STATUS_MOLD_MISSING if no mold coveres the requested `entry_id`.
//! \return DISIR_STATUS_NOT_EXIST if the entry does not exist.
//! \return DISIR_STATUS_EXISTS if the entry exists.
//!
enum disir_status
disir_config_query (struct disir_instance *instance, const char *group_id,
                    const char *entry_id, struct disir_entry **entry);

//! \brief Generate a config at a given version from the finished mold.
//!
//! \param[in] mold The completed mold of which to generate a config object of.
//! \param[in] version Version number of mold to generate config of. If NULL, highest
//!     mold version is used for generation.
//! \param[out] config Output config object allocated with generated config object.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if mold argument is NULL.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
disir_generate_config_from_mold (struct disir_mold *mold, struct disir_version *version,
                                 struct disir_config **config);

//! \brief Validate the config, checking for any contexts that are invalid.
//!
//! \param[in] config Input config to validate
//! \param[out] collection Populated collection of invalid contexts, if any.
//!     This parameter is optional. If NULL, no collection is returned.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if config is NULL.
//! \return DISIR_STATUS_INVALID_CONTEXT if config contains invalid contexts.
//!     Collection is (optionally) populated.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
disir_config_valid (struct disir_config *config, struct disir_collection **collection);

//! \brief Mark yourself finished with the configuration object.
//!
//! NOTE: Destroys the config object outright - not usable anywhere after this operation
//! QUESTION: Should this perhaps only decref the reference counter on the context?
//!
//! \param[in,out] config Object to mark as finished. Turns the pointer to NULL
//!     on success.
//!
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
disir_config_finished (struct disir_config **config);

//! \brief Query the config for a string valued keyval.
//!
//! For an exhaustive explaination of the query syntax, see XXX_QUERY_XXX
//!
//! \param[in] config The config object to query from.
//! \param[out] value The pointer to populate a reference to the string value to retrieve.
//! \param[in] query The varadic template and arguments to construct the query. See XXX_QUERY_XXX.
//!
//! \return DISIR_STATUS_OK on success.
//! \return DISIR_STATUS_WRONG_VALUE_TYPE if the resolved query is not of type string.
//!
enum disir_status
disir_config_get_keyval_string (struct disir_config *config, const char **value,
                                const char *query, ...);

//! \brief Set a string keyval in config.
//!
//! \see Detailed Disir query resolution semantic for setters.
//!
//! \param[in] config The config to query from.
//! \param[in] value The value to set the queried string keyval to.
//! \param[in] query A string suitable for query name resolution. See relevant documentation.
//! \param[in] ... Varadic argument list to populate query with, when applicable.
//!
//! \return DISIR_STATUS_OK on success.
//! \return DISIR_STATUS_WRONG_VALUE_TYPE if the resolved query is not of type string.
//!
enum disir_status
disir_config_set_keyval_string (struct disir_config *config, const char *value,
                                const char *query, ...);

//! \brief Query the config for an enum valued keyval.
//!
//! For an exhaustive explaination of the query syntax, see XXX_QUERY_XXX
//!
//! \param[in] config The config object to query from.
//! \param[out] value The pointer to populate a reference to the enum value to retrieve.
//! \param[in] query The varadic template and arguments to construct the query. See XXX_QUERY_XXX.
//!
//! \return DISIR_STATUS_OK on success.
//! \return DISIR_STATUS_WRONG_VALUE_TYPE if the resolved query is not of type enum.
//!
enum disir_status
disir_config_get_keyval_enum (struct disir_config *config, const char **value,
                              const char *query, ...);

//! \brief Set a enum keyval in config.
//!
//! \see Detailed Disir query resolution semantic for setters.
//!
//! \param[in] config The config to query from.
//! \param[in] value The value to set the queried enum keyval to.
//! \param[in] query A string suitable for query name resolution. See relevant documentation.
//! \param[in] ... Varadic argument list to populate query with, when applicable.
//!
//! \return DISIR_STATUS_OK on success.
//! \return DISIR_STATUS_WRONG_VALUE_TYPE if the resolved query is not of type enum.
//!
enum disir_status
disir_config_set_keyval_enum (struct disir_config *config, const char *value,
                              const char *query, ...);

//! \brief Query the config for a boolean valued keyval.
//!
//! For an exhaustive explaination of the query syntax, see XXX_QUERY_XXX
//!
//! \param[in] config The config object to query from.
//! \param[out] value The pointer to populate a reference to the boolean value to retrieve.
//! \param[in] query The varadic template and arguments to construct the query. See XXX_QUERY_XXX.
//!
//! \return DISIR_STATUS_OK on success.
//! \return DISIR_STATUS_WRONG_VALUE_TYPE if the resolved query is not of type boolean.
//!
enum disir_status
disir_config_get_keyval_boolean (struct disir_config *config, uint8_t *value,
                                 const char *query, ...);

//! \brief Set a boolean keyval in config.
//!
//! \see Detailed Disir query resolution semantic for setters.
//!
//! \param[in] config The config to query from.
//! \param[in] value The value to set the queried boolean keyval to.
//! \param[in] query A string suitable for query name resolution. See relevant documentation.
//! \param[in] ... Varadic argument list to populate query with, when applicable.
//!
//! \return DISIR_STATUS_OK on success.
//! \return DISIR_STATUS_WRONG_VALUE_TYPE if the resolved query is not of type boolean.
//!
enum disir_status
disir_config_set_keyval_boolean (struct disir_config *config, uint8_t value,
                                 const char *query, ...);

//! \brief Query the config for a float valued keyval.
//!
//! For an exhaustive explaination of the query syntax, see XXX_QUERY_XXX
//!
//! \param[in] config The config object to query from.
//! \param[out] value The pointer to populate a reference to the float value to retrieve.
//! \param[in] query The varadic template and arguments to construct the query. See XXX_QUERY_XXX.
//!
//! \return DISIR_STATUS_OK on success.
//! \return DISIR_STATUS_WRONG_VALUE_TYPE if the resolved query is not of type float.
//!
enum disir_status
disir_config_get_keyval_float (struct disir_config *config, double *value,
                               const char *query, ...);

//! \brief Set a float keyval in config.
//!
//! \see Detailed Disir query resolution semantic for setters.
//!
//! \param[in] config The config to query from.
//! \param[in] value The value to set the queried float keyval to.
//! \param[in] query A string suitable for query name resolution. See relevant documentation.
//! \param[in] ... Varadic argument list to populate query with, when applicable.
//!
//! \return DISIR_STATUS_OK on success.
//! \return DISIR_STATUS_WRONG_VALUE_TYPE if the resolved query is not of type float.
//!
enum disir_status
disir_config_set_keyval_float (struct disir_config *config, double value,
                                 const char *query, ...);

//! \brief Query the config for a integer valued keyval.
//!
//! For an exhaustive explaination of the query syntax, see XXX_QUERY_XXX
//!
//! \param[in] config The config object to query from.
//! \param[out] value The pointer to populate a reference to the integer value to retrieve.
//! \param[in] query The varadic template and arguments to construct the query. See XXX_QUERY_XXX.
//!
//! \return DISIR_STATUS_OK on success.
//! \return DISIR_STATUS_WRONG_VALUE_TYPE if the resolved query is not of type integer.
//!
enum disir_status
disir_config_get_keyval_integer (struct disir_config *config, int64_t *value,
                                 const char *query, ...);

//! \brief Set a integer keyval in config.
//!
//! \see Detailed Disir query resolution semantic for setters.
//!
//! \param[in] config The config to query from.
//! \param[in] value The value to set the queried integer keyval to.
//! \param[in] query A string suitable for query name resolution. See relevant documentation.
//! \param[in] ... Varadic argument list to populate query with, when applicable.
//!
//! \return DISIR_STATUS_OK on success.
//! \return DISIR_STATUS_WRONG_VALUE_TYPE if the resolved query is not of type integer.
//!
enum disir_status
disir_config_set_keyval_integer (struct disir_config *config, int64_t value,
                                 const char *query, ...);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _LIBDISIR_CONFIG_H

