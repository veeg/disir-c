#ifndef _LIBDISIR_COLLECTION_H
#define _LIBDISIR_COLLECTION_H

#include <disir/disir.h>

//! Abstract Data Type for a collection of Disir Contexts.
typedef struct disir_context_collection dcc_t;

//! PUBLIC API
enum disir_status dc_collection_next(dcc_t *collection, dc_t **context);

//! PUBLC API
enum disir_status dc_collection_reset(dcc_t *collection);

//! PUBLIC API
int32_t dc_collection_size(dcc_t *collection);

//! PUBLIC API
//! Finished using the context collection. 
enum disir_status dc_collection_finished(dcc_t **collection);

#endif // _LIBDISIR_COLLECTION_H

