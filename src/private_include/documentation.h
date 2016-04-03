#ifndef _LIBDISIR_DOCUMENTATION_H
#define _LIBDISIR_DOCUMENTATION_H

#include "value.h"

struct disir_documentation
{
    //! Context element this documentation element belongs to.
    dc_t                        *dd_context;

    //! Version this documentation entry was introduced.
    struct semantic_version     dd_introduced;

    //! Container for the documentation value
    struct disir_value          dd_value;

    // Simple double linked list
    struct disir_documentation *next;
    struct disir_documentation *prev;
};

//! Construct a DISIR_CONTEXT_DOCUMENTATION as a child of parent.
enum disir_status dx_documentation_begin (dc_t *parent, dc_t **doc);

//! Finalize the construction of a DISIR_CONTEXT_DOCUMENTATION
enum disir_status dx_documentation_finalize (dc_t **doc);

//! Allocate a disir_documentation structure
struct disir_documentation *dx_documentation_create (dc_t *parent);

//! Destroy a disir_documentation structure, freeing all associated
//! memory and unhooking from linked list storage.
enum disir_status dx_documentation_destroy (struct disir_documentation **documentation);

//! Allocate and populate the documentation string entry
enum disir_status dx_documentation_add_value_string (struct disir_documentation *doc,
                                                     const char *value,
                                                     int32_t value_size);

//! Return the number of documentation contexts associated with the passed context.
//! \return -1 if invalid context
int32_t dx_documentation_numentries (dc_t *context);

//! Add a disir_documentation to the parent context
enum disir_status dx_documentation_add (dc_t *parent, struct disir_documentation *doc);

#endif // _LIBDISIR_DOCUMENTATION_H

