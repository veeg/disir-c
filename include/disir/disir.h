#ifndef _LIBDISIR_H
#define _LIBDISIR_H

#ifdef __cplusplus
extern "C"{
#endif // _cplusplus


#include <stdint.h>


//! \brief Status return codes returned by most libdisir API functions.
enum disir_status
{
    //! Ooperation successful
    DISIR_STATUS_OK = 0,
    //! The input context does not have the capability to peform the requested operation
    DISIR_STATUS_NO_CAN_DO,
    //! The operation was invoked with invalid arguments
    DISIR_STATUS_INVALID_ARGUMENT,
    //! The operation was invoked with too few arguments
    DISIR_STATUS_TOO_FEW_ARGUMENTS,
    //!
    //! TODO: Determine if this status can be removed/replaced
    DISIR_STATUS_CONTEXT_IN_WRONG_STATE,
    //! The operation does not support context of this type.
    DISIR_STATUS_WRONG_CONTEXT,
    //! The context is in an invalid state.
    DISIR_STATUS_INVALID_CONTEXT,
    //! The context object has been destroyed.
    DISIR_STATUS_DESTROYED_CONTEXT,
    //! The context object has internal error state. Unrecoverable.
    DISIR_STATUS_BAD_CONTEXT_OBJECT,
    //! An allocation operation failed. Unrecoverable.
    DISIR_STATUS_NO_MEMORY,
    //!
    //! TODO: determine if this status can be removed/replaced
    DISIR_STATUS_NO_ERROR,
    //! An internal, unrecoverable error occured.
    DISIR_STATUS_INTERNAL_ERROR,
    //! Unable to allocate internal resources for the requested operation.
    DISIR_STATUS_INSUFFICIENT_RESOURCES,
    //! The resource already exists
    DISIR_STATUS_EXISTS,
    //! The operations results in conflicting semantic version numbers.
    DISIR_STATUS_CONFLICTING_SEMVER,
    //! Conflicting result from operation
    DISIR_STATUS_CONFLICT,
    //! The requested resource is exhausted.
    DISIR_STATUS_EXHAUSTED,
    //! Input to operation on a top-level CONFIG is missing its MOLD equivalent.
    DISIR_STATUS_MOLD_MISSING,
    //! Wrong Value type.
    DISIR_STATUS_WRONG_VALUE_TYPE,
    //! Requested resource does not exist.
    DISIR_STATUS_NOT_EXIST,
    //! Operation resulted in a restriction violation.
    DISIR_STATUS_RESTRICTION_VIOLATED,
    //! One ore more children of context is invalid
    DISIR_STATUS_ELEMENTS_INVALID,
    //! The operation is not supported by the API endpoint.
    DISIR_STATUS_NOT_SUPPORTED,

    //! Sentinel status - not returned by any API
    DISIR_STATUS_UNKNOWN // Must be the last status in enumeration

};

//! Forward declare the main disir object
struct disir_instance;
//! Forward declare the disir_update object
struct disir_update;
//! Forward declaration of the top-level context disir_config
struct disir_config;
//! Forward declaration of the top-level context disir_mold
struct disir_mold;

struct disir_collection;

//! The different value types that may be held by differrent contexts.
//! This enumeration also defines the explicit type of a DISIR_CONTEXT_KEYVAL.
enum disir_value_type
{
    DISIR_VALUE_TYPE_STRING = 1,
    DISIR_VALUE_TYPE_INTEGER,
    DISIR_VALUE_TYPE_FLOAT,
    DISIR_VALUE_TYPE_BOOLEAN,
    DISIR_VALUE_TYPE_ENUM,

    DISIR_VALUE_TYPE_UNKNOWN, // Must be the last element
};

//! The different restrictions that can be held by a DISIR_CONTEXT_RESTRICTION
//! Restrictions are catagorized into two classes:
//!     Inclusive: All inclusive restrictions must be fufilled for the context to be valid.
//!     Exclusive: One of the restrictions must be fufilled for the context ot be valid.
enum disir_restriction_type
{
    //    INCLUSIVE Restrictions
    //! Minimum number of elements of the same key that must be present.
    DISIR_RESTRICTION_INC_ENTRY_MIN = 1,
    //! Maximum number of elements of the same key that must be present.
    DISIR_RESTRICTION_INC_ENTRY_MAX,

    //    EXCLUSIVE Restrictions
    //! Value of key must match one of multiple string enum restrictions.
    //! Applicable to value type ENUM
    DISIR_RESTRICTION_EXC_VALUE_ENUM,
    //! Value of key must be within the floating point range.
    //! Applicable to value tye INTEGER and FLOAT
    DISIR_RESTRICTION_EXC_VALUE_RANGE,
    //! Value of key must be exact numeric value.
    //! Applicable to value type INTEGER and FLOAT
    DISIR_RESTRICTION_EXC_VALUE_NUMERIC,
    //! TODO: Support REGEX
    //DISIR_RESTRICTION_EXC_VALUE_REGEX,

    DISIR_RESTRICTION_UNKNOWN // Sentinel value - do not add any entries below this.
};


#include <disir/util.h>
#include <disir/context.h>
#include <disir/collection.h>
#include <disir/io.h>

//! \brief Create a mold for the libdisir configuration entry.
//!
//! \param[out] mold Populated address with the created mold.
//!
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status disir_libdisir_mold (struct disir_mold **mold);

//! \brief Output the libdisir config entry to disk.
//!
//! \param config Valid configuration object based of libdisir_mold
//! \param filepath Path to disk location to write the INI-formatted config file.
//!
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status disir_libdisir_config_to_disk (struct disir_instance *instance,
                                                 struct disir_config *config,
                                                 const char *filepath);


//! \brief Return a string representation of the disir status.
const char * disir_status_string (enum disir_status status);

//! \brief Allocate a new libdisir instance.
//!
//! If a `config` parameter is present, this configuration is used within
//! this libdisir instance. If NULL, it is ignored.
//! If `config_filepath` parameter is non-NULL, the INI-formatted file
//! is parsed and validated against the libdisir mold. If NULL, it is ignored.
//! If both `config` and `config_filepath` are NULL, the defaults from libdisir_mold
//! are used instead.
//! On validation error for either of these arguments, the disir instance creation
//! is aborted and this function fails.
//!
//! \param[in] config Valid configuration entry based on the libdisir_mold.
//!     Takes precedense over the `config_filepath` argument.
//!     Instance takes ownership of input config and associated mold.
//!     They will be destroyed by disir_instance_destroy.
//!     May be NULL.
//! \param[in] config_filepath Location on disk to read libdisir configuration file from.
//!     May be NULL, in case the internal defaults will be used.
//!     `config` takes precedense over filepath option.
//! \param[out] disir Pointer will be populated with the address of the allocated instance.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if the input address is NULL
//! \return DISIR_STATUS_NO_MEMORY if memory allocation failed.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
disir_instance_create (const char *config_filepath, struct disir_config *config,
                       struct disir_instance **disir);

//! \brief Destroy a previously allocated libdisir instance
//!
//! Destroy a libdisir instance previously allocated with disir_instance_create()
//!
//! \param[in,out] instance Instance to destroy. Pointer is sat to NULL upon successful destroy
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if input paramter address and its pointed to value
//!     are NULL:
//! \return DISIR_STATUS_OK on success.
enum disir_status
disir_instance_destroy (struct disir_instance **instance);

//! \brief Log a USER level log entry to the disir log.
//!
void disir_log_user (struct disir_instance *instance, const char *message, ...);

//! \brief Set an error message to the disir instance.
//!
//! This will also issue a ERROR level log event to the log stream.
//!
void disir_error_set (struct disir_instance *instance, const char *message, ...);

//! \brief Clear any error message previously sat on the disir instance.
void disir_error_clear (struct disir_instance *instance);

//! \brief Copy the contents of the error message
//!
//! If the buffer is of an insufficient size, and over 4 bytes large,
//! a partial error message will be populated. Status return code
//! DISIR_STATUS_INSSUFICIENT_RESOURCES indicates insufficient buffer.
//! Output parameter `bytes_written` will be filled with the total size of the error message.
//! The buffer will automatically be appended with a NUL terminator,
//! which means the input buffer must be atleast 1 byte greater than
//! the total size of the error message.
//!
//! \param[in] instance Libdisir instance to copy error message from.
//! \param[in,out] buffer Output buffer to write error message to.
//! \param[in] buffer_size Size of the output `buffer`.
//! \param[out] bytes_written Number of bytes written to `buffer`.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if `disir` or `buffer` are NULL.
//! \return DISIR_STATUS_INSSUFICIENT_RESOURCES if `buffer` is of an insufficient size.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
disir_error_copy (struct disir_instance *instance,
                  char *buffer, int32_t buffer_size, int32_t *bytes_written);

//! \brief Return the error message from the instance.
//!
//!If no error message exists, NULL is returned.
//!
const char * disir_error (struct disir_instance *instance);

//! \brief Generate a config at a given version from the finished mold.
//!
//! \param[in] mold The completed mold of which to generate a config object of.
//! \param[in] semver Version number of mold to generate config of. If NULL, highest
//!     mold version is used for generation.
//! \param[out] config Output config object allocated with generated config object.
//!
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
disir_generate_config_from_mold (struct disir_mold *mold, struct semantic_version *semver,
                                   struct disir_config **config);

//! \brief Validate the config, checking for any contexts that are invalid.
//!
//! \param[in] config Input config to validate
//! \param[out] collection Populated collection of invalid contexts, if any.
//!                        This parameter is optional. If NULL, no collection is returned.
//!
//! \return DISIR_STATUS_OK on success.
//! \return DISIR_STATUS_INVALID_ARGUMENT if config is NULL.
//! \return DISIR_STATUS_INVALID_CONTEXT if config contains invalid contexts. Collection is
//!         (optionally) populated.
//!
enum disir_status
disir_config_valid (struct disir_config *config, struct disir_collection **collection);

//! \brief Update the config 0bject to a new target version
//!
//! Update a config to a new semver version. The update state and progress is
//! recorded in its own disir_update structure which is an output parameter of this function.
//! If either of the return codes are DISIR_STATUS_OK or DISIR_STATUS_CONFLICT,
//! then the update operation is successful or underway. If an conflict occurs,
//! the disir_update_conflict() function is used to retrieve the conflicting keyval.
//! A resolution must be made with disir_update_resolve()
//! After a resolution has been successfully determined, the update may continue
//! using disir_update_continue()
//!
//! When either this function or disir_update_continue returns DISIR_STATUS_OK,
//! the update is successful and the config object is updated to the targeted semver version.
//! The update structure must be finalized through disir_update_finished() when
//! the operation is done.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if config or update are NULL.
//! \return DISIR_STATUS_CONFLICTING_SEMVER if the config version is higher than mold version.
//! \return DISIR_STATUS_NO_CAN_DO if the config and target are of equal version.
//! \return DISIR_STATUS_NO_MEMORY if memory allocation failed internally.
//! \return DISIR_STATUS_CONFLICT if there exists a conflicting keyval that requires
//!     manual resolution through disir_update_resolve.
//! \return DISIR_STATUS_OK if the update operation went through without any conflicts
//!
enum disir_status
disir_update_config (struct disir_config *config,
                     struct semantic_version *target, struct disir_update **update);

//! \brief Get the conflicting keyval and altenative values for a conflict in update
//!
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
disir_update_conflict (struct disir_update *update, const char **name,
                       const char **keyval, const char **mold);

//! \brief Resolve a conflict in an update with the new value
//!
//! \return DISIR_STATUS_OK on success.
//1
enum disir_status
disir_update_resolve (struct disir_update *update, const char *resolve);

//! \brief Continue update after a conflict resolution
//!
//! \return DISIR_STATUS_OK on success. The config object is now up-to-date with its target
//!     version. Use disir_update_finished() on the update object to dispose of it.
//!
enum disir_status
disir_update_continue (struct disir_update *update);

//! \brief Finished updating the config structure - Free all resources used to perform the update.
//!
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
disir_update_finished (struct disir_update **update);


#ifdef __cplusplus
}
#endif // _cplusplus

#endif // _LIBDISIR_H

