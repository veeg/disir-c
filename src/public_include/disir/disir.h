#ifndef _LIBDISIR_H
#define _LIBDISIR_H

#include <stdint.h>

//! The public interface for libdisir

//! libdisir status return codes
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
    //! The requested resource is exhausted.
    DISIR_STATUS_EXHAUSTED,

};

//! Forward declare the disir main object
struct disir;

//! Forward declaration of the top-level context disir_config
struct disir_config;
//! Forward declaration of the top-level context disir_schema
struct disir_schema;

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
enum disir_restriction
{
    DISIR_RESTRICTION_VALUE = 1,
    DISIR_RESTRICTION_RANGE,
};

//! \brief Allocate a new libdisir instance
//!
//! \param[in,out] Pointer will be populated with the address of the allocated instance.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if the input address is NULL
//! \return DISIR_STATUS_NO_MEMORY if memory allocation failed.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
disir_instance_create (struct disir **disir);

//! \brief Destroy a previously allocated libdisir instance
//!
//! Destroy a libdisir instance previously allocated with disir_instance_create()
//!
//! \param[in,out] disir Instance to destroy. Pointer is sat to NULL upon successful destroy
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if input paramter address and its pointed to value
//!     are NULL:
//! \return DISIR_STATUS_OK on success.
enum disir_status
disir_instance_destroy (struct disir **disir);


#include <disir/util.h>


#endif // _LIBDISIR_H

