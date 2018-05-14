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

//! \brief Resolve the parent context of the leaf node in the query.
//!
//! This function assumes that the leaf name query did not resolve to a valid context already.
//! example:
//!     "first@2.second@4.third@2"
//!
//!     If "first@2.second@4" exists, we are assumaing that the leaf context "third@2" does not
//!     exist. But for us to be able to create this leaf context, "third@1" must exist.
//!
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dx_query_resolve_parent_context (struct disir_context *parent, struct disir_context **out,
                                 char *keyval_name, const char *query, va_list args);

//! \brief Ensure that all sections exist in query.
//!
//! If any of the sections in the query does not exist,
//! it will attempt to create one in its place if its eligdable.
//! It will not allow to create a section whose index exceed
//! that of which already exists.
//! Example:
//!     "section@0" exists.
//!     "section@1" does not exist.
//!     "section@1.keyval" will succeed, with section index 1 created.
//!     query "section@2.keyval" will fail, since index 1 does not exist.
//!
//! If the ancestor parameter is supplied, the section that must be created,
//! closest to the root of the query, will NOT be finalized and returned to
//! the caller. The parent will always be a reference to the section occurring
//! directly before the keyval specification.
//! Example:
//!     "section@0" exists.
//!     "section@0.nested@1" exists.
//!     "section@0.nested@2" does not exist.
//!     query: "section@0.nested@2.super@1.keyval"
//!      - ancestor will be "section@0.nested@2"
//!      - parent will be "section@0.nested@.super@1"
//!
//! \return DISIR_STATUS_NO_CAN_DO if we're attempting to access a non-existent
//!     index to an element we cannot create an instance of. We can only create instances
//!     of the index that is one greater than the number current instances.
//! \return DISIR_STATUS_MOL_MISSING if any name referenced in query does not exist.
//! \return DISIR_STATUS_CONFLICT if name in query resolved as a keyval, when section was expected.
//! \return DISIR_STATUS_RESTRICTION_VIOLATED if we exceed maximum allowed instances.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dx_query_ensure_ancestors (struct disir_context *config,
                           const char *query, va_list args,
                           struct disir_context **ancestor,
                           struct disir_context **parent,
                           char *element_child_name, int *element_child_index);


#endif // _LIBDISIR_PRIVATE_QUERY_H

