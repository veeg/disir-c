#ifndef _LIBDISIR_RESTRICTION_H
#define _LIBDISIR_RESTRICTION_H

#ifdef __cplusplus
extern "C"{
#endif // __cplusplus

#include <disir/disir.h>


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


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _LIBDISIR_RESTRICTION_H

