#ifndef _LIBDISIR_CONTEXT_MOLD_H
#define _LIBDISIR_CONTEXT_MOLD_H

#ifdef __cplusplus
extern "C"{
#endif // __cplusplus


//!
//! This file exposes the low level Disir Mold Context API.
//!


//! TODO: Rework api doc
//! Retrieve the context associated with an already constructed disir_mold.
//! This context may be used to manipulate or query the mold object.
struct disir_context *
dc_mold_getcontext (struct disir_mold *mold);

//! \brief Get the version number of this mold.
//!
//! \param[in] mold Input mold to retrieve version for
//! \param[out] version Output structure populated with the version of mold.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if mold or version are NULL
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_mold_get_version (struct disir_mold *mold, struct disir_version *version);

//! TODO: Rework api doc
//! Construct the DISIR_CONTEXT_MOLD.
enum disir_status
dc_mold_begin (struct disir_context **mold);

//! TODO: Rework api doc
//! Finalize the construction of a DISIR_CONTEXT_MOLD, returning
//! an allocated disir_mold object in the output parameter.
//! If any unfinalized descendant contexts exists,
//! DISIR_STATUS_CONTEXT_IN_WRONG_STATE will be returned.
//! If the context supplied is not of type DISIR_CONTEXT_MOLD,
//! status DISIR_STATUS_WRONG_CONTEXT will be returned.
//! On success, DISIR_STATUS_OK is returned.
enum disir_status
dc_mold_finalize (struct disir_context **context, struct disir_mold **mold);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _LIBDISIR_CONTEXT_MOLD_H

