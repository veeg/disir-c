#ifndef _LIBDISIR_CONFIG_H
#define _LIBDISIR_CONFIG_H

#include "context_private.h"
#include "documentation.h"

//! Represents a complete config instance.
struct disir_config
{
    //! Context object for this config
    dc_t    *cf_context;

    struct semantic_version         cf_version;

    //! The disir_schema associated with this config instance
    //! Every disir_config needs a valid schema associated
    //! with it before it can gain most capabilities.
    struct disir_schema     *cf_schema;

    //! Storage of element entries, either DISIR_CONTEXT_KEYVAL or DISIR_CONTEXT_SECTION.
    struct disir_element_storage    *cf_elements;

    //! Documentation associated with the disir_config.
    //! Only one entry is allowed. - REMOVE THIS SUPPRT - ONLY APPLICABLE TO SCHEMA
    //! FOR TOPLEVEL CONTEXT
    struct disir_documentation      *cf_documentation_queue;
};

//! INTERNAL API
//! Allocate a struct disir_config
struct disir_config *dx_config_create (dc_t *context);

//! INTERNAL API
//! Destroy the passed struct disir_config
enum disir_status dx_config_destroy (struct disir_config **config);

#endif // _LIBDISIR_CONFIG_H

