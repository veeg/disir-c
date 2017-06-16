#ifndef _LIBDISIR_RESTRICTION_H
#define _LIBDISIR_RESTRICTION_H

#ifdef __cplusplus
extern "C"{
#endif // __cplusplus


//! \brief Retrieve a collection of all DISIR_CONTEXT_RESTRICTION available on the context.
//!
//! Only contexts of type DISIR_CONTEXT_KEYVAL and DISIR_CONTEXT_SECTION may have restrictions
//! associated with them. This function retrieves all of these associated DISIR_CONTEXT_RESTRICTION
//! contexts available, both inclusive and exclusive. The input context' top-level may be
//! either DISIR_CONTEXT_CONFIG or DISIR_CONTEXT_MOLD.
//!
//! \param[in] context The context to retrieve all restrictions from.
//! \param[out] collection Allocated collection containing all DISIR_CONTEXT_RESTRICTION contexts
//!     associated with the input context. If no such restriction contexts exist, this collection
//!     is not allocated and is set to NULL instead.
//!
//! \return DISIR_STATUS_WRONG_CONTEXT if context is not one of expected type.
//! \return DISIR_STATUS_NO_MEMORY on allocation failure.
//! \return DISIR_STATUS_NOT_EXIST if there where no restrictions to retrieve
//! \return DISIR_STATUS_OK on success. The collection is allocated and populated.
//!
enum disir_status
dc_restriction_collection (struct disir_context *context, struct disir_collection **collection);

//! \brief Return a string representation of the restriction enumeration type.
//!
//! \return Stringified restriction type.
//!
const char *
dc_restriction_enum_string (enum disir_restriction_type restriction);

//! \brief Return enum type of the matched input string.
//!
//! \return DISIR_RESTRICTION_UNKNOWN if string does not match a known type.
//! \return matched restriction enum on success.
//!
enum disir_restriction_type
dc_restriction_string_to_enum (const char *string);

//! \brief Return a string representation of the group this restriction belongs to.
//!
//! \return Stringified group type of restriction.
//!
const char *
dc_restriction_group_type (enum disir_restriction_type restriction);

//! \brief Return a string representation of the restriction type of the restriction context.
//!
//! \return 'INVALID' if context is NULL or not of type DISIR_CONTEXT_RESTRICTION
//! \return Stringified restriction type.
//!
const char *
dc_restriction_context_string (struct disir_context *context);

//! \brief Query the RESTRICTION context for its restriction type.
//!
//! Must be called on a context of type DISIR_CONTEXT_RESTRICTION.
//! Populate the `type` argument with the restriction type of the `context`.
//!
//! \param[in] context Input DISRI_CONTEXT_RESTRICTION context to query.
//! \param[out] type Output pointer to populate restriction_type with.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if `context` or `type` are NULL.
//! \return DISIR_STATUS_WRONG_CONTEXT if `context` is not of correct type.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_get_restriction_type (struct disir_context *context, enum disir_restriction_type *type);

//! \brief Set the restricction type of a restriction context.
//!
//! Input context must be DISIR_CONTEXT_RESTRICTION.
//! You may only set restriction type when the context is not yet finalized.
//!
//! Inclusive restrictions may be sat on a context belonging to the following parent context types:
//!     * DISIR_CONTEXT_KEYVAL.
//!     * DISIR_CONTEXT_SECTION.
//!
//! Exclusive restrictions may only be sat on a restriction context
//! belonging to parent context DISIR_CONTEXT_KEYVAL.
//! You cannot add exclusive restrictions who has
//! no value type is sat on the parent KEYVAL context.
//!
//! The following value type restrictions apply to parent KEYVAL value types:
//! DISIR_RESTRICTION_EXL_VALUE_ENUM:
//!     * DISIR_VALUE_TYPE_ENUM
//! DISIR_RESTRICTION_EXC_VALUE_RANGE:
//!     * DISIR_VALUE_TYPE_INTEGER
//!     * DISIR_VALUE_TYPE_FLOAT
//! DISIR_RESTRICTION_EXC_VALUE_NUMERIC:
//!     * DISIR_VALUE_TYPE_INTEGER
//!     * DISIR_VALUE_TYPE_FLOAT
//!
//! Attempting to set restriction type who violates the above rules results in
//! DISIR_STATUS_WRONG_CONTEXT return status.
//!
//! \param context DISIR_CONTEXT_RESTRICTION context to set restriction on.
//! \param type Type of restriction to assign.
//!
//! \return DISIR_STATUS_OK on success.
//! \return DISIR_STATUS_CONTEXT_IN_WRONG_STATE if context is finalized.
//! \return DISIR_STATUS_CONTEXT_WRONG_CONTEXT if the input parent context is of wrong type or
//!         the restriction type is not applicable to the input parent context.
//! return DISIR_STATUS_WRONG_VALUE_TYPE if the restriction type is not applicable to
//!         the value type of the input parent context.
//!
enum disir_status
dc_set_restriction_type (struct disir_context *context, enum disir_restriction_type type);

//! \brief Get a string `value` from input DISIR_CONTEXT_RESTRICTION `context`.
//!
//! Only applicable to `context` of type DISIR_CONTEXT_RESTRICTION.
//! Only applicable to context whose restriction type is one of:
//!     * DISIR_RESTRICTION_EXC_VALUE_ENUM
//!
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_restriction_get_string (struct disir_context *context, const char **value);

//! \brief Set a string `value` to the input DISIR_CONTEXT_RESTRICTION `context`
//!
//! Only applicable to `context` of type DISIR_CONTEXT_RESTRICTION.
//! Only applicable to context whose restriction type is one of:
//!     * DISIR_RESTRICTION_EXC_VALUE_ENUM
//!
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_restriction_set_string (struct disir_context *context, const char *value);

//! \brief Get the `min` and `max` values of input DISR_CONTEXT_RESTRICTION of type RANGE.
//!
//! Only applicable to `context` of type DISIR_CONTEXT_RESTRICTION.
//! Only applicable to context whose restriction type is one of:
//!     * DISIR_RESTRICTION_EXC_VALUE_RANGE
//!
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_restriction_get_range (struct disir_context *context, double *min, double *max);

//! \brief Set a range value to the input DISIR_CONTEXT_RESTRICTION `context`.
//!
//! Only applicable to `context` of type DISIR_CONTEXT_RESTRICTION.
//! Only applicable to context whose restriction type is one of:
//!     * DISIR_RESTRICTION_EXC_VALUE_RANGE
//!
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_restriction_set_range (struct disir_context *context, double min, double max);

//! \brief Get the numeric `value` stored on this DISIR_CONTEXT_RESTRICTION of type NUMERIC.
//!
//! Only applicable to `context` of type DISIR_CONTEXT_RESTRICTION.
//! Only applicable to context whose restriction type is one of:
//!     * DISIR_RESTRICTION_EXC_VALUE_RANGE
//!
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_restriction_get_numeric (struct disir_context *context, double *value);

//! \brief Set a numeric `value` to the input DISIR_CONTEXT_RESTRICTION `context`.
//!
//! Only applicable to `context` of type DISIR_CONTEXT_RESTRICTION.
//! Only applicable to context whose restriction type is one of:
//!   * DISIR_RESTRICTION_EXC_VALUE_RANGE
//!   * DISIR_RESTRICTION_EXC_VALUE_NUMERIC
//!   * DISIR_RESTRICTION_INC_ENTRY_MIN
//!   * DISIR_RESTRICTION_INC_ENTRY_MAX
//!
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_restriction_set_numeric (struct disir_context *context, double value);

//! \brief Add a DISIR_RESTRICTION_EXC_VALUE_NUMERIC restriction to parent.
//!
//! Only applicable to the following `parent` contexts whose top-level is MOLD:
//!   * DISIR_CONTET_KEYVAL
//
//! This is a shortcut  method instead of running through dc_begin, dc_set_restriction_type,
//! dc_restriction_set_numeric and dc_finalize, with optional dc_add_documenation and
//! dc_add_introduced.
//!
//! \param[out] output Optional output context storage. If address is passed,
//!             the output RESTRICTION context is populated and reference increased.
//!             Caller must take care to use dc_putcontext () when he is done.
//!
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_add_restriction_value_numeric (struct disir_context *parent, double value, const char *doc,
                                  struct semantic_version *semver,
                                  struct disir_context **output);

//! \brief Add a DISIR_RESTRICTION_EXC_VALUE_RANGE restriction to parent.
//!
//! Only applicable to the following `parent` contexts whose top-level is MOLD:
//!   * DISIR_CONTET_KEYVAL
//
//! This is a shortcut  method instead of running through dc_begin, dc_set_restriction_type,
//! dc_restriction_set_numeric and dc_finalize, with optional dc_add_documenation and
//! dc_add_introduced.
//!
//! \param[out] output Optional output context storage. If address is passed,
//!             the output RESTRICTION context is populated and reference increased.
//!             Caller must take care to use dc_putcontext () when he is done.
//!
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_add_restriction_value_range (struct disir_context *parent, double min, double max,
                                const char * doc, struct semantic_version *semver,
                                struct disir_context **output);

//! \brief Add a DISIR_RESTRICTION_EXC_VALUE_ENUM restriction to parent.
//!
//! Only applicable to the following `parent` contexts whose top-level is MOLD:
//!   * DISIR_CONTET_KEYVAL
//
//! This is a shortcut  method instead of running through dc_begin, dc_set_restriction_type,
//! dc_restriction_set_string and dc_finalize, with optional dc_add_documenation and
//! dc_add_introduced.
//!
//! \param[out] output Optional output context storage. If address is passed,
//!             the output RESTRICTION context is populated and reference increased.
//!             Caller must take care to use dc_putcontext () when he is done.
//!
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_add_restriction_value_enum (struct disir_context *parent, const char *value, const char *doc,
                               struct semantic_version *semver,
                               struct disir_context **output);

//! \brief Add a DISIR_RESTRICTION_INC_ENTRY_MIN restriction to parent.
//!
//! The input parent much have root context MOLD, and must be of type:
//!     * DISIR_CONTEXT_KEYVAL
//!     * DISIR_CONTEXT_SECTION
//!
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_add_restriction_entries_min (struct disir_context *parent, int64_t min,
                                struct semantic_version *semver);

//! \brief Add a DISIR_RESTRICTION_INC_ENTRY_MIN restriction to parent.
//!
//! The input parent much have root context MOLD, and must be of type:
//!     * DISIR_CONTEXT_KEYVAL
//!     * DISIR_CONTEXT_SECTION
//!
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_add_restriction_entries_max (struct disir_context *parent, int64_t max,
                                struct semantic_version *semver);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _LIBDISIR_RESTRICTION_H

