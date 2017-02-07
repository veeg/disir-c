#ifndef _LIBDISIR_PRIVATE_DOCUMENTATION_H
#define _LIBDISIR_PRIVATE_DOCUMENTATION_H

#include "value.h"

struct disir_documentation
{
    //! Context element this documentation element belongs to.
    struct disir_context                        *dd_context;

    //! Version this documentation entry was introduced.
    struct semantic_version     dd_introduced;

    //! Container for the documentation value
    struct disir_value          dd_value;

    // Simple double linked list
    struct disir_documentation *next;
    struct disir_documentation *prev;
};

//! Construct a DISIR_CONTEXT_DOCUMENTATION as a child of parent.
enum disir_status dx_documentation_begin (struct disir_context *parent,
                                          struct disir_context **doc);

//! Finalize the construction of a DISIR_CONTEXT_DOCUMENTATION
enum disir_status dx_documentation_finalize (struct disir_context *doc);

//! Allocate a disir_documentation structure
struct disir_documentation *dx_documentation_create (struct disir_context *parent);

//! Destroy a disir_documentation structure, freeing all associated
//! memory and unhooking from linked list storage.
enum disir_status dx_documentation_destroy (struct disir_documentation **documentation);

//! Return the number of documentation contexts associated with the passed context.
//! \return -1 if invalid context
int32_t dx_documentation_numentries (struct disir_context *context);

//! Add a disir_documentation to the parent context
enum disir_status dx_documentation_add (struct disir_context *parent,
                                        struct disir_documentation *doc);

#endif // _LIBDISIR_PRIVATE_DOCUMENTATION_H

