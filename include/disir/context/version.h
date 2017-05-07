#ifndef _LIBDISIR_CONTEXT_VERSION_H
#define _LIBDISIR_CONTEXT_VERSION_H

#ifdef __cplusplus
extern "C"{
#endif // __cplusplus


//! \brief Add an introduced semantic version number to a context.
//!
//! \return DISIR_STATUS_EXISTS is returned if an introduced entry already exists.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_add_introduced (struct disir_context *context, struct semantic_version *semver);

//! \brief Add a deprecrated semantic version number to a context.
//!
//! Supported input contexts are:
//!   * DISIR_CONTEXT_KEYVAL
//!   * DISIR_CONTEXT_SECTION
//!   * DISIR_CONTEXT_DEFAULT
//!   * DISIR_CONTEXT_RESTRICTION
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT `context` and `semver` is NULL.
//! \return DISIR_STATUS_WRONG_CONTEXT if context is not of supported type.
//! \return DISIR_STATUS_WRONG_CONTEXT if top-level is not MOLD.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_add_deprecated (struct disir_context *context, struct semantic_version *semver);

//! \brief Query the context for the introduced version it holds (if any).
//!
//! Supported input contexts are:
//!   * DISIR_CONTEXT_DEFAULT
//!   * DISIR_CONTEXT_DOCUMENTATION
//!   * DISIR_CONTEXT_SECTION
//!   * DISIR_CONTEXT_RESTRICTION
//!   * DISIR_CONTEXT_MOLD
//!
//! \param[in] context Input contect of supported type.
//! \param[out] semver Semantic version structure to populate with the output value on success.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if `context` or `semver` are NULL.
//! \return DISIR_STATUS_WRONG_CONTEXT if `context` is of unsupported type.
//! \return DISIR_STATUS_WRONG_CONTEXT if `context` whose top-level is not MOLD.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_get_introduced (struct disir_context *context, struct semantic_version *semver);

//! \brief Query the context for the deprecated version it holds (if any)
//!
//! Supported input contexts are:
//!     * DISIR_CONTEXT_KEYVAL
//!     * DISIR_CONTEXT_SECTION
//!     * DISIR_CONTEXT_RESTRICTION
//!
//! \param[in] context Input contect of supported type.
//! \param[out] semver Semantic version structure to populate with the output value on success.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if `context` or `semver` are NULL.
//! \return DISIR_STATUS_WRONG_CONTEXT if `context` is of unsupported type.
//! \return DISIR_STATUS_WRONG_CONTEXT if `context` whose top-level is not MOLD.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_get_deprecated (struct disir_context *context, struct semantic_version *semver);

//! \brief Retrieve the version number of the input context.
//!
//! Supported contexts are:
//!     * DISIR_CONTEXT_CONFIG
//!     * DISIR_CONTEXT_MOLD
//!
//! \param[in] context To retrieve version from.
//! \param[out] semver Semantic version structure to populate the version of `context`.
//!
//! \return DISIR_STATUS_INVALID_ARUGMENT if `context` or `semver` are NULL.
//! \return DISIR_STATUS_WRONG_CONTEXT if `context` is not of supported type.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_get_version (struct disir_context *context, struct semantic_version *semver);

//! \brief Set the version number of the input context.
//!
//! Supported contexts are:
//!   * DISIR_CONTEXT_CONFIG
//!   * DISIR_CONTEXT_MOLD
//!
//! \param[in] context The context to set version on.
//! \param[in] semver Semantic version structure to get version from.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if context or semver are NULL.
//! \return DISIR_STATUS_WRONG_CONTEXT if context is not of supported type.
//! \return DISIR_STATUS_CONFLICTING_SEMVER if semver is higher than mold semver when
//!     applied to a DISIR_CONTEXT_CONFIG context.
//! \return DISRI_STATUS_OK on success.
//!
enum disir_status
dc_set_version (struct disir_context *context, struct semantic_version *semver);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _LIBDISIR_CONTEXT_VERSION_H

