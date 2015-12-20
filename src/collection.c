#include <stdlib.h>

#include <disir/disir.h>

#include "log.h"
#include "collection.h"
#include "context_private.h"

//! PUBLIC API
int32_t
dc_collection_size(dcc_t *collection)
{
    // QUESTION: Should this return -1 instead?
    if (collection == NULL)
        return 0;

    // Collesce to get an accurate count
    dx_collection_coalesce(collection);

    return collection->cc_numentries;
}

//! PUBLIC API
enum disir_status
dc_collection_next(dcc_t *collection, dc_t **context)
{
    enum disir_status status;

    if (collection == NULL)
    {
        log_debug("Invoked with NULL collection pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }
    if (context == NULL)
    {
        log_debug("invoked with NULL context pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    // Initialize the output with NULL
    *context = NULL;

    // Coalesce array to get it in sync with reality
    status = dx_collection_coalesce(collection);
    if (status != DISIR_STATUS_OK)
    {
        return status;
    }

    // Exausted?
    if (collection->cc_iterator_index > collection->cc_numentries - 1)
    {
        log_debug("Collection iterator( %d ) exhausted (numentires: %d)", 
                collection->cc_iterator_index, collection->cc_numentries - 1);
        return DISIR_STATUS_EXHAUSTED;
    }

    // Grab the iterator index item, increment index.
    *context = collection->cc_collection[collection->cc_iterator_index];

    log_debug("Next context( %p ) at index( %d )", *context, collection->cc_iterator_index);

    collection->cc_iterator_index++;

    return DISIR_STATUS_OK;
}

//! PUBLIC API
enum disir_status
dc_collection_reset(dcc_t *collection)
{
    if (collection == NULL)
    {
        log_debug("invoked with collection NULL pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    // Simply reset the index
    collection->cc_iterator_index = 0;

    return DISIR_STATUS_OK;
}

//! INTERNAL API
enum disir_status
dx_collection_coalesce(dcc_t *collection)
{
    dc_t *context;
    int32_t index;
    int32_t probe;
    int32_t invalid_entries_count;
    int32_t iterator_moveback;

    if (collection == NULL)
    {
        log_debug("invoked with collection NULL pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    context = NULL;
    index = 0;
    probe = 1;
    invalid_entries_count = 0;
    iterator_moveback = 0;
    while(index < collection->cc_numentries)
    {
        context = collection->cc_collection[index];
        index++;

        if (context == NULL || context->cx_state == CONTEXT_STATE_DESTROYED)
        {
            // 
            if (context != NULL)
            {
                dx_context_decref(context);
                invalid_entries_count += 1;
                context = collection->cc_collection[index - 1] = NULL;
            }

            if (probe < index)
                probe = index;

            // Find next non-valid entry in the collection, move it here.
            while (probe < collection->cc_numentries)
            {
                if (collection->cc_collection[probe]->cx_state != CONTEXT_STATE_DESTROYED)
                {
                    context = collection->cc_collection[probe];
                    collection->cc_collection[probe] = NULL;
                    collection->cc_collection[index -1] = context;

                    // Move iterator index if its affected.
                    if (probe >= collection->cc_iterator_index &&
                        (index -1) < collection->cc_iterator_index)
                    {
                        iterator_moveback  += 1;
                    }
                }

                probe++;

                if (context != NULL)
                    break;
            }
        }
    }

    collection->cc_numentries -= invalid_entries_count;

    // Take care of the iterator count post-iterate
    collection->cc_iterator_index -= iterator_moveback;
    if (collection->cc_iterator_index < 0)
        collection->cc_iterator_index = 0;

    return DISIR_STATUS_OK;
}

//! INTERNAL API
dcc_t *
dx_collection_create(void)
{
    dcc_t *collection;

    collection = calloc(1, sizeof(dcc_t));
    if (collection == NULL)
        return NULL;

    collection->cc_capacity = 10;

    collection->cc_collection = calloc(collection->cc_capacity, sizeof(dc_t *));
    if (collection->cc_collection == NULL)
    {
        free(collection);
        return NULL;
    }

    return collection;
}

//! PUBLIC API
enum disir_status
dc_collection_finished(dcc_t **collection)
{
    uint32_t index;
    dc_t *context;

    if (collection == NULL || *collection == NULL)
    {
        log_debug("invoked with collection NULL pointer");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    index = 0;
    // Iterate through the entire collection - decref all non-null entires
    while (index < (*collection)->cc_numentries)
    {
        context = (*collection)->cc_collection[index];
        if (context != NULL)
        {
            dx_context_decref(context);
        }
        index++;
    }

    free((*collection)->cc_collection);
    free(*collection);

    *collection = NULL;
    return DISIR_STATUS_OK;
}

//! INTERNAL API
enum disir_status
dx_collection_push_context(dcc_t *collection, dc_t *context)
{
    enum disir_status status;
    void *reallocated_collection;
    size_t reallocated_size;
    uint32_t reallocated_capacity;

    if (collection == NULL)
    {
        log_debug("Invoked with NULL collection pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }
    if (context == NULL)
    {
        log_debug("invoked with NULL context pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    // Coalesce array to get it in sync with reality
    status = dx_collection_coalesce(collection);
    if (status != DISIR_STATUS_OK)
    {
        return status;
    }

    // Expand capacity if needed
    if (collection->cc_numentries == collection->cc_capacity)
    {
        reallocated_capacity = collection->cc_capacity + 10;
        reallocated_size = reallocated_capacity * sizeof(dc_t*);
        log_debug("Reallocating collection to new size( %d )", reallocated_size);
        reallocated_collection = realloc(collection->cc_collection, reallocated_size);
        if (reallocated_collection == NULL)
        {
            log_warn("context collection reallocation of size( %d ) failed.", reallocated_size);
            return DISIR_STATUS_NO_MEMORY;
        }
        collection->cc_collection = reallocated_collection;
        collection->cc_capacity = reallocated_capacity;
    }

    dx_context_incref(context);
    collection->cc_collection[collection->cc_numentries] = context;
    collection->cc_numentries++;

    return DISIR_STATUS_OK;
}

