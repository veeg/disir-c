#ifndef _LIBDISIR_PRIVATE_CONTEXT_COLLECTION_H
#define _LIBDISIR_PRIVATE_CONTEXT_COLLECTION_H

#include <disir/collection.h>

//!
//! The Context Collection can hold an arbitrary large set of
//! Disir Contexts. Every context stored in the collection will
//! have an incremented reference count - only when the collection
//! is freed or a coalesce operation is required will invalid contexts
//! be decref'ed and removed from the collection.
//! The collection has an in-built iterator, which returns the context
//! as they were inputted to the collection.
struct disir_context_collection
{
    // Dynamically allocated array of disir_context objects.
    // cc_capacity holds the total size of this array.
    dc_t            **cc_collection;

    //! Number of entries allocated to fit in cc_collection.
    int32_t         cc_capacity;

    //! Number of entries present in cc_collection.
    int32_t         cc_numentries;

    //! Index into cc_collection the iterator is presently at.
    int32_t         cc_iterator_index;
};

//! INTERNAL API
dcc_t * dx_collection_create(void);

//! INTERNAL API
//! Make sure every entry in collection is valid and stored sequentially.
//! Modify iterator index and numentries if context(s) are found to be invalid.
//! Decref and lose invalid context pointers.
enum disir_status dx_collection_coalesce(dcc_t *collection);

//! INTERNAL API
//! Append the context to the end of the collection.
//! Will increment the context reference count. 
enum disir_status dx_collection_push_context(dcc_t *collection, dc_t *context);


#endif // _LIBDISIR_PRIVATE_CONTEXT_COLLECTION_H

