#ifndef _LIBDISIR_DOCUMENTATION_H
#define _LIBDISIR_DOCUMENTATION_H

struct disir_documentation
{
    //! Context element this documentation element belongs to.
    dc_t                        *dd_context;
    
    //! Version this documentation entry was introduced.
    struct semantic_version     dd_introduced;
    
    //! Allocated if dd_string_size is greater than zero.
    char                        *dd_string;
    
    //! Number of bytes that make up the documentation string.
    //! Excluding NULL terminator
    int32_t                     dd_string_size;
    
    //! Number of bytes allocated for dd_string
    int32_t                     dd_string_allocated;
    
    // Simple double linked list
    struct disir_documentation *dd_next;
    struct disir_documentation *dd_prev;

};

//! Construct a DISIR_CONTEXT_DOCUMENTATION as a child of parent.
enum disir_status dx_documentation_begin(dc_t *parent, dc_t **doc);

//! Finalize the construction of a DISIR_CONTEXT_DOCUMENTATION
enum disir_status dx_documentation_finalize(dc_t **doc);

//! Allocate a disir_documentation structure
struct disir_documentation *dx_documentation_create(dc_t *parent);

//! Destroy a disir_documentation structure, freeing all associated
//! memory and unhooking from linked list storage.
enum disir_status dx_documentation_destroy(struct disir_documentation **documentation);

//! Allocate and populate the documentation string entry
enum disir_status dx_documentation_add_value_string(struct disir_documentation *doc, const char *value, int32_t value_size);

//! Fetch the associated documentation structure from the passed context.
//! NULL is returned if no such documentation exists
//! Function asserts if it receives an unhandled context type
struct disir_documentation *dx_documentation_fetch(dc_t *context);

//! Add a disir_documentation to the parent context
enum disir_status dx_documentation_add(dc_t *parent, struct disir_documentation *doc);

#endif // _LIBDISIR_DOCUMENTATION_H
