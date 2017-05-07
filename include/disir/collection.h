#ifndef _LIBDISIR_COLLECTION_H
#define _LIBDISIR_COLLECTION_H

#ifdef __cplusplus
extern "C"{
#endif // _cplusplus

#include <disir/disir.h>


//! \brief Allocate a new context collection instance.
//!
//! A `disir_collection` is the only container of multiple
//! `disir_context` objects. An query API endpoint operating
//! on the context level will either return a single
//! `disir_context` or a `disir_collection`.
//! The collection is iterable and safe to access all
//! context that lie within, even if the context has otherwise
//! been destroyed.
//! The caller must call dc_collection_finished() when all
//! operations associated with the collection are completed.
//!
//! \return NULL on allocation failure.
//! \return Pointer to alloated disir_collection memory.
//!
struct disir_collection *
dc_collection_create (void);

//! \brief Append the context to the end of the collection.
//!
//! Will increment the context reference count.
//!
//! \param[in] collection The iterator to append context to.
//! \param[out] context The context to append with.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if collection or context are NULL.
//! \return DISIR_STATUS_NO_MEMORY on memory re-allocation error.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_collection_push_context (struct disir_collection *collection, struct disir_context *context);

//! \brief Retrieve the next context from the collection.
//!
//! A reference is incremented on the yielded context.
//! The caller is responsible for calling dc_putcontext()
//! when he is finished with it.
//! When the iterator is exhausted, dc_collection_reset() will set
//! the iterator to the first context in the collection.
//!
//! The collection is coalesced before retrieving the next context.
//!
//! \param[in] collection The iterator to query the next context from.
//! \param[out] context Populated pointer with the context to retrieve.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if collection or context are NULL.
//! \return DISIR_STATUS_EXHAUSTED when the collection iterator is empty.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_collection_next (struct disir_collection *collection, struct disir_context **context);

//! \brief Reset the iterator back to the first context in collection.
//!
//! \param[in] collection The iterator to reset.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if collection is NULL.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_collection_reset (struct disir_collection *collection);

//! \brief Retrieve the number of context' in the collection.
//!
//! The collection is coalesced before retrieving the size.
//!
//! \param[in] collection The iterator to reset.
//!
//! \return 0 if collection is NULL.
//! \return >= 0
//!
int32_t
dc_collection_size (struct disir_collection *collection);

//! \brief Free up the collection and all references within it.
//!
//! Releases all contexts within it (reducing its reference count)
//! and free the memory associated with the dc_collection_create()
//! operation.
//!
//! The pointer is set ot NULL upon successful freeing of collection.
//!
//! \param[in,out] collection Address of the pointer holding the collection.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if collection or *collection is NULL.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_collection_finished (struct disir_collection **collection);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _LIBDISIR_COLLECTION_H

