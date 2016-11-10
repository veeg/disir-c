#ifndef _LIBDISIR_CONFIG_H
#define _LIBDISIR_CONFIG_H

#include "context_private.h"
#include "element_storage.h"

//! Represents a complete config instance.
struct disir_config
{
    //! Context object for this config
    struct disir_context                            *cf_context;

    //! Config has an explicit reference to a mold entry.
    //! The config acquires a cf_mold->mo_reference_count when created.
    struct disir_mold               *cf_mold;

    //! Populated with the plugin this config was loaded from (if applicable)
    //! This information is used internally to determine the plugin source
    //! when attempting a write-back.
    char                            *cf_plugin_name;

    //! Version of this config instance.
    //! The version of a config shall always represent a snapshot
    //! of its mold, with appropriate modifications. Version cannot
    //! be greater than its mold version.
    //! The version is by default 1.0.0.
    struct semantic_version         cf_version;

    //! Storage of element entries, either:
    //!     * DISIR_CONTEXT_KEYVAL
    //!     * DISIR_CONTEXT_SECTION.
    struct disir_element_storage    *cf_elements;
};

//! \brief Create a new disir_config structure with the input as its context representation
//!
//! \para[in] The DISR_CONTEXT_CONFIG context object which represents this config structure.
//!
//! \return NULL if memory allocation failed.
//! \return newly allocated and initialized config structure
//!
struct disir_config *dx_config_create (struct disir_context *context);

//! \brief Destroy a previously allocated disir_config structure. Destroys all children as well.
//!
//! \param[in,out] config Structure to destroy. Sat to NULL on success.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if config or *config are NULL.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status dx_config_destroy (struct disir_config **config);


#endif // _LIBDISIR_CONFIG_H

