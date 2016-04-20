#ifndef _LIBDISIR_DEFAULT_H
#define _LIBDISIR_DEFAULT_H

struct disir_default
{
    //! Context element this default element belongs to.
    dc_t                        *de_context;

    //! Version this default entry was introduced.
    struct semantic_version     de_introduced;

    //! The value associated with this default entry.
    struct disir_value          de_value;

    struct disir_default        *next;
    struct disir_default        *prev;

};

//! Construct a DISIR_CONTEXT_DEFAULT as a child of parent.
enum disir_status dx_default_begin (dc_t *parent, dc_t **def);

//! Finalize the construction of a DISIR_CONTEXT_DEFAULT
enum disir_status dx_default_finalize (dc_t **def);

//! Allocate a disir_default structure
struct disir_default *dx_default_create (dc_t *parent);

//! Destroy a disir_default structure, freeing all associated
//! memory and unhooking from linked list storage.
enum disir_status dx_default_destroy (struct disir_default **def);


//! \brief Query a keyval context, whose root is schema, for the active default entry for semver.
//!
//! Internal function - No input validation is performed.
//!
//! \param[in] keyval Context KEYVAL whose root must be SCHEMA.
//! \param[in] semver Version to retrieve active default entry for. NULL indicates the greatest.
//! \param[out] Ouput structure pointer populated with matching entry. NULL if no default entries
//!     on keyval context.
//!
void
dx_default_get_active (dc_t *keyval, struct semantic_version *semver, struct disir_default **def);


#endif // _LIBDISIR_DEFAULT_H

