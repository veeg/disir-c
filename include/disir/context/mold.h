#ifndef _LIBDISIR_CONTEXT_MOLD_H
#define _LIBDISIR_CONTEXT_MOLD_H

#ifdef __cplusplus
extern "C"{
#endif // __cplusplus

#include <disir/disir.h>

//!
//! This file exposes the low level Disir Mold Context API.
//!


//! \brief Retrieve the context of this mold object.
//!
//! This context may be used to manipulate or query the mold object
//! using the low-level API.
//!
//! \param mold Mold object.
//!
//! \return context.
//!
DISIR_EXPORT
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
DISIR_EXPORT
enum disir_status
dc_mold_get_version (struct disir_mold *mold, struct disir_version *version);

//! \brief Begin construction of a new mold object from scratch.
//!
//! Use the returned context to add definitions using the low level context api.
//!
//! \param[out] mold Context object allocated for this mold.
//!
//! \return DISIR_STATUS_OK on success.
//!
DISIR_EXPORT
enum disir_status
dc_mold_begin (struct disir_context **mold);

//! \brief Finalize the construction of a DISIR_CONTEXT_MOLD.
//!
//! \param[in,out] context Mold context to finalize. Parameter is NULLed on success.
//! \param[out] mold Mold object returned. Is only populated on status
//!     OK and INVALID_CONTEXT.
//!
//! \return DISIR_STATUS_INVALID_CONTEXT if any of the child elements
//!     of this mold violates the mold definition rules.
//! \return DISIR_CONTEXT_WRONG_CONTEXT if context is not of type MOLD.
//! \return DISIR_STATUS_OK on success.
//!
DISIR_EXPORT
enum disir_status
dc_mold_finalize (struct disir_context **context, struct disir_mold **mold);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _LIBDISIR_CONTEXT_MOLD_H

