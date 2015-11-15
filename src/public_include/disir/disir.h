#ifndef _LIBDISIR_H
#define _LIBDISIR_H

#include <stdint.h>

//! The public interface for libdisir

//! libdisir status return codes
enum disir_status
{
    DISIR_STATUS_OK = 0,
    DISIR_STATUS_NO_CAN_DO,
    DISIR_STATUS_INVALID_ARGUMENT,
    DISIR_STATUS_TOO_FEW_ARGUMENTS,
    DISIR_STATUS_CONTEXT_IN_WRONG_STATE,
    DISIR_STATUS_WRONG_CONTEXT,
    DISIR_STATUS_INVALID_CONTEXT,
    DISIR_STATUS_BAD_CONTEXT_OBJECT,
    DISIR_STATUS_NO_MEMORY,
    DISIR_STATUS_NO_ERROR,
    DISIR_STATUS_INTERNAL_ERROR,
    DISIR_STATUS_INSUFFICIENT_RESOURCES,
    DISIR_STATUS_EXISTS,
    DISIR_STATUS_CONFLICTING_SEMVER,
    
};

//! Forward declaration of the top-level context disir_config
struct disir_config;
//! Forward declaration of the top-level context disir_schema
struct disir_schema;
//! Forward declaration of the top-level context disir_template
struct disir_template;

//! The different types that can be held by a DISIR_CONTEXT_TYPE
enum disir_type
{
    DISIR_TYPE_STRING = 1,
    DISIR_TYPE_INTEGER,
    DISIR_TYPE_FLOAT,
    DISIR_TYPE_BOOLEAN,
};

//! The different restrictions that can be held by a DISIR_CONTEXT_RESTRICTION
enum disir_restriction
{
    DISIR_RESTRICTION_VALUE = 1,
    DISIR_RESTRICTION_RANGE,
};

#include <disir/util.h>




#endif // _LIBDISIR_H
