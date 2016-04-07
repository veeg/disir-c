#ifndef _LIBDISIR_SECTION_H
#define _LIBDISIR_SECTION_H

struct disir_section
{
    //! Context element this section element belongs to.
    dc_t                            *se_context;

    //! Version this section entry was introduced.
    struct semantic_version         se_introduced;

    struct disir_element_storage    *se_elements;

};

//! Construct a DISIR_CONTEXT_SECTION as a child of parent.
enum disir_status dx_section_begin (dc_t *parent, dc_t **section);

//! Finalize the construction of a DISIR_CONTEXT_SECTION
enum disir_status dx_section_finalize (dc_t **section);

//! Allocate a disir_section structure
struct disir_section *dx_section_create (dc_t *parent);

//! Destroy a disir_section structure, freeing all associated
//! memory and unhooking from linked list storage.
enum disir_status dx_section_destroy (struct disir_section **section);


#endif // _LIBDISIR_SECTION_H

