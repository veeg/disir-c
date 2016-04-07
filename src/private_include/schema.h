#ifndef _LIBDISIR_SCHEMA_H
#define _LIBDISIR_SCHEMA_H

#include "context_private.h"
#include "documentation.h"
#include "element_storage.h"

//! Represents a complete schema instance.
struct disir_schema
{
    //! Context object for this schema
    dc_t    *sc_context;

    //! Storage of element entries, either DISIR_CONTEXT_KEYVAL or DISIR_CONTEXT_SECTION.
    struct disir_element_storage    *sc_elements;

    //! Documentation associated with the disir_schema.
    struct disir_documentation      *sc_documentation_queue;
};

//! INTERNAL API
//! Allocate a struct disir_schema
struct disir_schema *dx_schema_create (dc_t *context);

//! INTERNAL API
//! Destroy the passed struct disir_schema
enum disir_status dx_schema_destroy (struct disir_schema **schema);

#endif // _LIBDISIR_SCHEMA_H

