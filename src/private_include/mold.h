#ifndef _LIBDISIR_MOLD_H
#define _LIBDISIR_MOLD_H

#include "context_private.h"
#include "documentation.h"
#include "element_storage.h"

//! Represents a complete mold instance.
struct disir_mold
{
    //! Context object for this mold
    struct disir_context                            *mo_context;

    //! Count of how many ADT structure pointers the user posesses.
    int                             mo_reference_count;

    //! Name of the plugin that loaded this mold (if applicable)
    char                            *mo_plugin_name;

    //! Semantic version of this mold.
    //! This value is determined by the highest semantic version number found
    //! in any of the child contexts contained within the mold.
    struct semantic_version         mo_version;

    //! Storage of element entries, either DISIR_CONTEXT_KEYVAL or DISIR_CONTEXT_SECTION.
    struct disir_element_storage    *mo_elements;

    //! Documentation associated with the disir_mold.
    struct disir_documentation      *mo_documentation_queue;
};

//! INTERNAL API
//! Allocate a struct disir_mold
struct disir_mold *dx_mold_create (struct disir_context *context);

//! INTERNAL API
//! Destroy the passed struct disir_mold
enum disir_status dx_mold_destroy (struct disir_mold **mold);

//! \brief Conditionally update the version number of the mold if input semver is greater.
//!
//! \param mold Input mold to update the version number of
//! \param semver Input semver to update mold with, if greater
//!
//! \return DISIR__STATUS_INVALID_ARGUMENT if mold or semver are NULL
//! \return DISIR_STATUS_OK on success
//!
enum disir_status dx_mold_update_version (struct disir_mold *mold,
                                            struct semantic_version *semver);

#endif // _LIBDISIR_MOLD_H

