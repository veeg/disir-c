#ifndef _LIBDISIR_CONFIG_H
#define _LIBDISIR_CONFIG_H

#include "context_private.h"
#include "documentation.h"

//! Represents a complete config instance.
struct disir_config
{
    //! Context object for this config
    dc_t    *cf_context;

    //! The disir_schema associated with this config instance
    //! Every disir_config needs a valid schema associated
    //! with it before it can gain most capabilities.
    struct disir_schema     *cf_schema;

    // TODO: Hashmap of entries, either keyval or group.
    
    //! Documentation associated with the disir_config.
    //! Only one entry is allowed.
    struct disir_documentation      *cf_documentation;
};

//! INTERNAL API
//! Allocate a struct disir_config
struct disir_config *dx_config_create(dc_t *context);

//! INTERNAL API
//! Destroy the passed struct disir_config
enum disir_status dx_config_destroy(struct disir_config **config);

#endif // _LIBDISIR_CONFIG_H
