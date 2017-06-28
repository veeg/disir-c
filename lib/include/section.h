#ifndef _LIBDISIR_PRIVATE_SECTION_H
#define _LIBDISIR_PRIVATE_SECTION_H

#include "value.h"

struct disir_section
{
    //! Context element this section element belongs to.
    struct disir_context                *se_context;

    //! For top-level context CONFIG, this points to the mold equivalent section.
    struct disir_context                *se_mold_equiv;

    //! Version this section entry was introduced.
    struct semantic_version             se_introduced;

    //! Version this section entry was introduced.
    struct semantic_version             se_deprecated;

    //! Name of this section.
    struct disir_value                  se_name;

    struct disir_documentation          *se_documentation_queue;

    //! Element storage for this section.
    struct disir_element_storage        *se_elements;

    struct disir_restriction            *se_restrictions_queue;
};

//! Construct a DISIR_CONTEXT_SECTION as a child of parent.
//! NOTE: Should ever only be called from dc_begin()
enum disir_status dx_section_begin (struct disir_context *parent, struct disir_context **section);

//! Finalize the construction of a DISIR_CONTEXT_SECTION
enum disir_status dx_section_finalize (struct disir_context *section);

//! Allocate a disir_section structure
struct disir_section *dx_section_create (struct disir_context *parent);

//! Destroy a disir_section structure, freeing all associated
//! memory and unhooking from linked list storage.
enum disir_status dx_section_destroy (struct disir_section **section);


#endif // _LIBDISIR_PRIVATE_SECTION_H

