#ifndef _LIBDISIR_CONTEXT_QUERY_H
#define _LIBDISIR_CONTEXT_QUERY_H

#ifdef __cplusplus
extern "C"{
#endif // __cplusplus


//! \brief Return the default value as a string representation from context.
//!
//! Retrieve the default value of input context as a string representation.
//! The supported contexts are:
//!     * DISIR_CONTEXT_DEFAULT: Plainly retrieve the appointed to default value
//!     * DISIR_CONTEXT_KEYVAL: Search for the matching default entry in keyval.
//! The matching default version when searching KEYVAL is picked. If semver is NULL,
//! the highest version is chosen.
//!
//! The output buffer is populated with the string representation of the default value held
//! by context. If the output_buffer_size is inssuficient in size, the output_string_size
//! will be equal or greater than output_buffer_size, and the output buffer populated
//! with a output_buffer_size - 1 bytes of data.
//! The output buffer is always NULL terminated.
//! On success, the output_string_size will always hold the exact number of bytes populated
//! in the buffer, not counting the terminating NULL character.
//!
//! \return DISIR_STATUS_INVALID_ARGUENT if context or output are NULL, or output_buffer_size
//!     is less than or equal to zero.
//! \return DISIR_STATUS_WRONG_CONTEXT if context is not of type DISIR_CONTEXT_DEFAULT or
//!     DISIR_CONTEXT_KEYVAL
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_get_default (struct disir_context *context, struct semantic_version *semver,
                int32_t output_buffer_size,
                char *output, int32_t *output_string_size);

//! \brief Gather all default entries on the context into a collection.
//!
//! The supported context for this function is the DISIR_CONTEXT_KEYVAL,
//! whose root context must be a DISIR_CONTEXT_MOLD.
//!
//! \param[in] context Input KEYVAL context to retrieve all default contexts from.
//! \param[out] collection Output collection populated with default contexts of input context.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if any of the input arguments are NULL.
//! \return DISIR_STATUS_WRONG_CONTEXT if either the input context or root context are wrong.
//! \return DISIR_STATUS_NO_MEMORY if collection allocation failed.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_get_default_contexts (struct disir_context *context, struct disir_collection **collection);

//!  \brief Collect all direct child elements of the passed context.
//!
//! \param[in] context Parent context to collect child elements from.
//!     Must be of context type:
//!         * DISIR_CONTEXT_CONFIG
//!         * DISIR_CONTEXT_MOLD
//!         * DISIR_CONTEXT_SECTION
//! \param[out] collection Output collection, if return status is DISIR_STATUS_OK
//!
//! \return DISIR_STATUS_OK if the output collection contains all
//!     child elements of a valid input context.
//! \return DISIR_STATUS_INVALID_ARGUMENT if input parameters are NULL
//! \return DISRI_STATUS_WRONG_CONTEXT if the input context is not of correct type.
//!
enum disir_status
dc_get_elements (struct disir_context *context, struct disir_collection **collection);

//! \brief Collect all children of the passed context matching name.
//!
//! \param[in] parent Parent context to collect child elements from.
//!     Must be of context type
//!         * DISIR_CONTEXT_CONFIG
//!         * DISIR_CONTEXT_MOLD
//!         * DISIR_CONTEXT_SECTION
//! \param[in] name Name of key to match wanted keyval contexts
//! \param[in] index Index of the keyval if multiple entries. Use 0 if only one entry.
//! \param[out] output Context to retrieve. Caller must use dc_putcontext when finished.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if input parameters are NULL
//! \return DISIR_STATUS_NOT_EXIST if the requested element/or index does not exist.
//! \return DISIR_STATUS_WRONG_CONTEXT if the input context is not of correct type.
//! \return DISIR_STATUS_OK if the output context is populated with the requested entry.
//!
enum disir_status
dc_find_element (struct disir_context *parent, const char *name, unsigned int index,
                 struct disir_context **output);

//! \brief Collect all children of the passed context matching name.
//!
//! \param[in] context Parent context to collect child elements from.
//!     Must be of context type
//!         * DISIR_CONTEXT_CONFIG
//!         * DISIR_CONTEXT_MOLD
//!         * DISIR_CONTEXT_SECTION
//! \param[in] name Name of key to match wanted keyval contexts
//!
//! \return DISIR_STATUS_OK if the output collection contains all
//!     child elements of a valid input context.
//! \return DISIR_STATUS_INVALID_ARGUMENT if input parameters are NULL
//! \return DISIR_STATUS_WRONG_CONTEXT if the input context is not of correct type.
//!
enum disir_status
dc_find_elements (struct disir_context *context, const char *name,
                  struct disir_collection **collection);

//! \brief Query for a context relative to parent.
//!
//! \param[in] parent The context to query from.
//! \param[in] name Query format to resolve.
//! \param[out] out Output context to query for and return.
//! \param[in] ... varadic arguments used for name argument.
//!
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_query_resolve_context (struct disir_context *parent, const char *name,
                          struct disir_context **out, ...);

//! \see dc_query_resolve_context
//!
//! Varatic argument version of dc_query_resolve_context.
enum disir_status
dc_query_resolve_context_va (struct disir_context *parent, const char *name,
                             struct disir_context **out, va_list args);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _LIBDISIR_CONTEXT_QUERY_H

