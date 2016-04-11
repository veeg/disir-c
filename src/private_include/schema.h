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

    //! Semantic version of this schema.
    //! This value is determined by the highest semantic version number found
    //! in any of the child contexts contained within the schema.
    struct semantic_version         sc_version;

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

//! \brief Conditionally update the version number of the schema if input semver is greater.
//!
//! \param schema Input schema to update the version number of
//! \param semver Input semver to update schema with, if greater
//!
//! \return DISIR__STATUS_INVALID_ARGUMENT if schema or semver are NULL
//! \return DISIR_STATUS_OK on success
//!
enum disir_status dx_schema_update_version (struct disir_schema *schema,
                                            struct semantic_version *semver);

#endif // _LIBDISIR_SCHEMA_H

