#ifndef _LIBDISIR_COLLECTION_H
#define _LIBDISIR_COLLECTION_H

#ifdef __cplusplus
extern "C"{
#endif // _cplusplus

#include <disir/disir.h>
#include <disir/context.h>

//! \brief Allocate a new context collection instance
struct disir_collection * dc_collection_create (void);

//! \brief Append the context to the end of the collection.
//!
//! Will increment the context reference count.
//!
//! \return DISIR_STATUS_OK on success
//!
enum disir_status dc_collection_push_context (struct disir_collection *collection, dc_t *context);

//! PUBLIC API
enum disir_status dc_collection_next (struct disir_collection *collection, dc_t **context);

//! PUBLC API
enum disir_status dc_collection_reset (struct disir_collection *collection);

//! PUBLIC API
int32_t dc_collection_size (struct disir_collection *collection);

//! PUBLIC API
//! Finished using the context collection.
enum disir_status dc_collection_finished (struct disir_collection **collection);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _LIBDISIR_COLLECTION_H

