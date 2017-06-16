#ifndef _LIBDISIR_PRIVATE_ELEMENT_STORAGE_H
#define _LIBDISIR_PRIVATE_ELEMENT_STORAGE_H

#include <disir/context.h>

#include "collection.h"

//! Forward declare Disir Element Storage structure.
//! This is a private structure, even to the internals of Disir.
struct disir_element_storage;

//! \brief Allocate a new instance of the Disir Element Storage
//!
//! \return Pointer to the newly allocated instance. NULL if the allocation failed.
struct disir_element_storage *
dx_element_storage_create (void);

//! \brief Destroy a previously allocated instance of Disir Element Storage
//!
//! \param[in,out] storage Double-pointer to the allocated instance that shall be destroyed.
//!                 The pointer is set to NULL when the element storage is free'd.
//! \return DISIR_STATUS_OK when the storage was successfully destroyed.
//!
enum disir_status
dx_element_storage_destroy (struct disir_element_storage **storage);

//! \brief Return the number of elements stored in the Disir Element Storage
//!
//! \param[in] storage Pointer to the element storage to query entries from.
//!
//! \return number of entries stored in the Disir Element Storage. -1 is returned
//!     if the input parameter is NULL.
//!
int32_t
dx_element_storage_numentries (struct disir_element_storage *storage);

//! \brief Add a context with the given name to the storage.
//!
//! No validation/business logic is performed. This is a raw context storage.
//! The input name is used to store the context such that it may be retrieved
//! (queried) by the same name.
//!
//! \param[in] storage The input storage to store the context
//! \param[in] name Input name used as key to store the context by.
//! \param[in] context The context to store into the storage.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if either storage, name or context are NULL.
//! \return DISIR_STATUS_NO_MEMORY if no memory could be allocated for internal storage mechanism
//! \return DISIR_STATUS_EXISTS if the context is already stored in storage (by the input name)
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dx_element_storage_add (struct disir_element_storage *storage,
                        const char *name,
                        struct disir_context *context);

//! \brief Remove a context from the element storage
//!
//! TODO: Not implemented
//!
//! \return DISIR_STATUS_INTERNAL_ERROR
enum disir_status
dx_element_storage_remove (struct disir_element_storage *storage,
                           const char * const name,
                           struct disir_context *context);

//! \brief Query the element storage for its elmenets matching name
//!
//! \param storage Element storage to query entries from
//! \param name String name of the element(s) to retrieve
//! \param[out] collection output of the matching entries, if any.
//!
//! \return DISIR_STATUS_NOT_EXIST if no entries were found.
//! \return DISIR_STATUS_OK on success
//!
enum disir_status
dx_element_storage_get (struct disir_element_storage *storage,
                        const char * const name,
                        struct disir_collection **collection);

//! \brief Get all context in storage in insertion order
//!
//! Populate a collection with all contexts in storage in the insertion order
//! they were submitted to storage. Oupput collection must be
//! finalized with dc_collection_finishe() when operations are completed
//! on the collection.
//!
//! \param[in] storage Query storage to retrieve all contexts from.
//! \param[out] collection Output collection of contexts.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if storage or collection are NULL.
//! \return DISIR_STATUS_NO_MEMORY if collection allocation failed.
//! \return DISIR_STATUS_OK on success
//!
enum disir_status
dx_element_storage_get_all (struct disir_element_storage *storage,
                            struct disir_collection **collection);


//! \brief Convenience method to get the first context with input name from storage.
//!
//! \param[in] storage Query storage to retrieve context from.
//! \param[in] name Query parameter to locate context by in storage
//! \param[out] Populated context on success (if found in storage)
//!
//! \return DISIR_STATUS_NOT_EXIST if no entries were found.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dx_element_storage_get_first (struct disir_element_storage *storage,
                              const char *name,
                              struct disir_context **context);

#endif // _LIBDISIR_PRIVATE_ELEMENT_STORAGE_H

