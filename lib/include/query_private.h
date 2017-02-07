#ifndef _LIBDISIR_PRIVATE_QUERY_H
#define _LIBDISIR_PRIVATE_QUERY_H

//! \brief Resolve a heirarchical name structure by extracting first name and index
//!
//! \param[in] parent The context errors on this query should be logged to
//! \param[in,out] name The buffer containing the name and index to extract.
//!     The buffer is modified so that the same pointer can be safely used to reference only
//!     the name portion extracted (on success).
//! \param[in,out] resolved The buffer containing the already resolved heirarchical names.
//!     As the parser works through resolving the name, portions of the resolved entry is added
//!     to this buffer. The parameters name and resolved will end up equal on a successful query.
//!     This buffer is only used to give precise feedback on any error conditions in parsing
//!     and nested context resolution.
//! \param[out] next The pointer is populated with the next name to resolve, if any.
//! \param[out] index The variable is populated with the index present in the current resolved name.
//!     If non is present, this is populated with index 0.
//!
//! The naming scheme supported by this resolve method is the following:
//!     first@3.second.third@1.forth@0.fifth
//!
//! Leading dot key seperators are not allowed: e.g., .second.third@3
//! Leading index indicators are not allowed: e.g., first@2.@4.third or @2.second
//! Blank keys are not allowed: e.g., first@5..third
//! Trailing index indicators are not allowed: e.g., first@3.second@
//! Trailing key seperators are not allowed: e.g., first@2.second.
//!
//! Modifies name and resolved.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT on parsing failure.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dx_query_resolve_name (struct disir_context *parent, char *name, char *resolved,
                       char **next, int *index);


#endif // _LIBDISIR_PRIVATE_QUERY_H

