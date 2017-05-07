#ifndef _LIBDISIR_CONTEXT_VALUE_H
#define _LIBDISIR_CONTEXT_VALUE_H

#ifdef __cplusplus
extern "C"{
#endif // __cplusplus

#include <disir/disir.h>

//! \brief Get the value type enumeration represented by the passed context.
//!
//! Retrieve the disir_value_type embedded in the passed context.
//! If the passed context is NULL, or does not contain a value,
//! DISIR_VALUE_TYPE_UNKNOWN is returned.
//!
//! \return DISIR_VALUE_TYPE_UNKNOWN if the passed context is NULL or the context
//!     does not contain a value type. Otherwise return the value type of the context.
//! \return disir_value_type represented by context if context is valid and contains
//!     a valid disir_value_type
//!
enum disir_value_type
dc_value_type (struct disir_context *context);

//! \brief Return a string representation of the value this context represents.
//!
//! Retrieve the disir_value_type embedded in the passed context, and
//! return a string representation of this enumeration.
//! If the passed context is NULL or does not contain a value, UNKNOWN is returned.
//!
//! \param[in] context Input context to infere value from.
//!
//! \return string representation of the disir_value_type held in context
//!
const char *
dc_value_type_string (struct disir_context *context);

//! \brief Set the value type associated with input context.
//!
//! Set the value type of the input context.
//! The available input contexts are
//!   * DISIR_CONTEXT_KEYVAL
//!
//! There are a number of restrictions on when you can set a value type on a context.
//! DISIR_CONTEXT_KEYVAL: Cannot have any default entries on it.
//!     Can only set type if the toplevel context is DISIR_CONTEXT_MOLD.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if `context` is NULL or `type` is out-of-bounds.
//! \return DISIR_STATUS_WRONG_CONTEXT if `context`'s toplevel context is CONFIG.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_set_value_type (struct disir_context *context, enum disir_value_type type);

//! \brief Retrieve the value type stored in the input context.
//!
//! Retrieve the value type of the input context, populated in the output argument 'type'
//!
//! \param[in] context The input context object to return the value type of.
//! \param[out] type Output value populated with the type found in context.
//!     DISIR_VALUE_TYPE_UNKNOWN is populated if any errors on the input context are found.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if either context or type are NULL pointer.
//! \return DISIR_STATUS_WRONG_CONTEXT if it does not contain a value type.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_get_value_type (struct disir_context *context, enum disir_value_type *type);

//! \brief Set a value to the context. Type is extracted from string where applicable.
//!
//! QUESTION: Remove this function? What utility does it provide?
//!
//! Parses the input string and extract the type specific value from it.
//! Boolean will only take the fist character in input value buffer into consideration.
//! Boolean false values accepted: '0', 'f', 'F'
//! Boolean true values accepted:  '1', 't', 'T'
//!
//! Only applicable to DISIR_CONTEXT_KEYVAL whose top-level is CONFIG.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if input value cannot be properly extracted.
//! \return DISIR_STATUS_WRONG_CONTEXT if context is of unsupported type.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_set_value (struct disir_context *context, const char *value, int32_t value_size);

//! \brief Set a string value to the context.
//!
//! Applicable contexts are
//!   * DISIR_CONTEXT_DOCUMENTATION
//!   * DISIR_CONTEXT_KEYVAL whose top-level is CONFIG
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if context or value are NULL,
//!     or if value_size is less or equal to zero.
//! \return DISIR_STATUS_WRONG_CONTEXT if root context is not CONFIG.
//! \return DISIR_STATUS_WRONG_VALUE_TYPE if value type in context is not string.
//! \return DISIR_STATUS_INVALID_CONTEXT if the entry does not have a mold equivalent.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_set_value_string (struct disir_context *context, const char *value, int32_t value_size);

//! \brief Set a enum value to the context.
//!
//! Applicable contexts are
//!   * DISIR_CONTEXT_DOCUMENTATION
//!   * DISIR_CONTEXT_KEYVAL whose top-level is CONFIG
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if context or value are NULL,
//!     or if value_size is less or equal to zero.
//! \return DISIR_STATUS_WRONG_CONTEXT if root context is not CONFIG.
//! \return DISIR_STATUS_WRONG_VALUE_TYPE if value type in context is not enum.
//! \return DISIR_STATUS_INVALID_CONTEXT if the entry does not have a mold equivalent.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_set_value_enum (struct disir_context *context, const char *value, int32_t value_size);

//! \brief Set a integer value to the context.
//!
//! Applicable contexts are:
//!   * DISIR_CONTEXT_KEYVAL whose top-level is CONFIG
//!   * DISIR_CONTEXT_DEFAULT
//!
//! \return DISIR_STATUS_WRONG_CONTEXT if input context is not among the applicable contexts.
//! \return DISIR_STATUS_WRONG_VALUE_TYPE if value type in context is not string.
//! \return DISIR_STATUS_INVALID_CONTEXT if context is under construction, and this operation
//!     caused it to enter an invalid state.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_set_value_integer (struct disir_context *context, int64_t value);

//! \brief Set a float value to the context.
//!
//! Applicable contexts are:
//!   * DISIR_CONTEXT_KEYVAL whose top-level is CONFIG
//!   * DISIR_CONTEXT_DEFAULT
//!
//! \return DISIR_STATUS_WRONG_CONTEXT if input context is not among the applicable contexts.
//! \return DISIR_STATUS_WRONG_VALUE_TYPE if value type in context is not string.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_set_value_float (struct disir_context *context, double value);

//! \brief Set a boolean value to the context.
//!
//! Applicable contexts are:
//!   * DISIR_CONTEXT_KEYVAL whose top-level is CONFIG
//!   * DISIR_CONTEXT_DEFAULT
//!
//! \return DISIR_STATUS_WRONG_CONTEXT if input context is not among the applicable contexts.
//! \return DISIR_STATUS_WRONG_VALUE_TYPE if value type in context is not string.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_set_value_boolean (struct disir_context *context, uint8_t value);

//! \brief Get the value stored in context in a string representation.
//!
//! The value, regardless of type, is stringified in the output buffer provided.
//! if the output_buffer_size is insufficient to hold the full value of context,
//! then only output_buffer_size - 1 bytes are copied to output and the output_size
//! will be equal or greater than output_buffer_size.
//! A terminating NULL character is always added to the end of the output buffer,
//! but is not part of the outout_size count returned
//!
//! Supported contexts are
//!   * DISIR_CONTEXT_KEYVAL whose root is CONFIG
//!
//! \param[in] context Input context to query for value.
//! \param[in] output_buffer_size Size of the output buffer. Cannot be <= 0.
//! \param[in] output Buffer with at least output_buffer_size capacity.
//! \param[out] output_size Size in bytes of the stringified value of context,
//!     not including the terminating NULL character.
//!
//! \return DISIR_STATUS_WRONG_CONTEXT if an unsupported context is supplied.
//! \return DISIR_STATUS_WRONG_CONTEXT if KEYVALs context is not CONFIG.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_get_value (struct disir_context *context, int32_t output_buffer_size,
              char *output, int32_t *output_size);

//! \brief Retrieve the string value stored on the context.
//!
//! Only applicable on the following contexts:
//!   * DISIR_CONTEXT_KEYVAL whose root is CONFIG
//!   * DISIR_CONTEXT_DOCUMENTATION
//!
//! \param context Input context to retrieve string value from.
//! \param[out] output Pointer to redirect the string value stored in context to.
//! \param[out] size Optional. Populated with the size of the output string.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if context or output are NULL.
//! \return DISIR_STATUS_WRONG_VALUE_TYPE if value type in context is not string.
//! \return DISIR_STATUS_WRONG_CONTEXT if the context is of wrong type.
//! \return DISIR_STATUS_WRONG_CONTEXT if KEYVALs root context is not CONFIG.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_get_value_string (struct disir_context *context, const char **output, int32_t *size);

//! \brief Retrieve the enum value stored on the context.
//!
//! Only applicable on the following contexts:
//!   * DISIR_CONTEXT_KEYVAL whose root is CONFIG
//!   * DISIR_CONTEXT_DEFAULT
//!
//! \param context Input context to retrieve enum string from.
//! \param[out] output Pointer to redirect the enum string stored in context to.
//! \param[out] size Optional. Populated with the size of the output string.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if context or output are NULL
//! \return DISIR_STATUS_WRONG_VALUE_TYPE if value type in context is not string.
//! \return DISIR_STATUS_WRONG_CONTEXT if the context is of wrong type.
//! \return DISIR_STATUS_WRONG_CONTEXT if KEYVALs root context is not CONFIG.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_get_value_enum (struct disir_context *context, const char **output, int32_t *size);

//! \brief Retrieve the integer value stored on the context.
//!
//! Supported contexts:
//!   * DISIR_CONTEXT_KEYVAL whose top-level is CONFIG.
//!
//! \param[in] context The context to get integer from.
//! \param[out] value The output variable to populate.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if `context` or `value` is NULL.
//! \return DISIR_STATUS_MOLD_MIDDING if MOLD is not associated with `context`.
//! \return DISIR_STATUS_WRONG_CONTEXT if `context` is not of supported type.
//! \return DISIR_STATUS_WRONG_VALUE_TYPE if value type in context is not DISIR_VALUE_TYPE_INTEGER.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_get_value_integer (struct disir_context *context, int64_t *value);

//! \brief Retrieve the float value stored on the context.
//!
//! Supported contexts:
//!   * DISIR_CONTEXT_KEYVAL whose top-level is CONFIG.
//!
//! \param[in] context The context to get float from.
//! \param[out] value The output variable to populate.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if `context` or `value` is NULL.
//! \return DISIR_STATUS_MOLD_MIDDING if MOLD is not associated with `context`.
//! \return DISIR_STATUS_WRONG_CONTEXT if `context` is not of supported type.
//! \return DISIR_STATUS_WRONG_VALUE_TYPE if value type in context is not DISIR_VALUE_TYPE_FLOAT.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_get_value_float (struct disir_context *context, double *value);

//! \brief Retrieve the boolean value stored on the context.
//!
//! Supported contexts:
//!   * DISIR_CONTEXT_KEYVAL whose top-level is CONFIG.
//!
//! \param[in] context The context to get boolean from..
//! \param[out] value The output variable to populate.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if `context` or `value` is NULL.
//! \return DISIR_STATUS_MOLD_MIDDING if MOLD is not associated with `context`.
//! \return DISIR_STATUS_WRONG_CONTEXT if `context` is not of supported type.
//! \return DISIR_STATUS_WRONG_VALUE_TYPE if value type in context is not DISIR_VALUE_TYPE_BOOLEAN.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_get_value_boolean (struct disir_context *context, uint8_t *value);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _LIBDISIR_CONTEXT_VALUE_H

