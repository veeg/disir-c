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
struct disir_collection
{
    // Dynamically allocated array of disir_context objects.
    // cc_capacity holds the total size of this array.
    struct disir_context            **cc_collection;

    //! Number of entries allocated to fit in cc_collection.
    int32_t         cc_capacity;

    //! Number of entries present in cc_collection.
    int32_t         cc_numentries;

    //! Index into cc_collection the iterator is presently at.
    int32_t         cc_iterator_index;
};

//! INTERNAL API
//! Make sure every entry in collection is valid and stored sequentially.
//! Modify iterator index and numentries if context(s) are found to be invalid.
//! Decref and lose invalid context pointers.
enum disir_status dx_collection_coalesce (struct disir_collection *collection);

//! \brief Return the next entry in the collection without coalescing.
//!
//! This function is used to potentially retrieve destroyed contexts
//! from a collection. This is used when destroying an element storage, to
//! avoid accessing and freeing contexts in the wrong order.
//!
//! \return DISIR_STATUS_EXHAUSTED when iterator is exhausted.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dx_collection_next_noncoalesce (struct disir_collection *collection,
                                struct disir_context **context);


#endif // _LIBDISIR_PRIVATE_CONTEXT_COLLECTION_H

