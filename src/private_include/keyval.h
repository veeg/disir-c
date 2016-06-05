#ifndef _LIBDISIR_KEYVAL_H
#define _LIBDISIR_KEYVAL_H

#include <disir/disir.h>
#include <disir/context.h>

#include "value.h"
#include "default.h"

struct disir_keyval
{
    //! Context element this keyval element belongs to.
    struct disir_context                        *kv_context;

    //! Context mold element this keyval element represents
    //! Only applicable to parent toplevel context DISIR_CONTEXT_CONFIG
    struct disir_context                        *kv_mold_equiv;

    //! Version this keyval entry was introduced.
    struct semantic_version     kv_introduced;

    //! Version this keyval entry was deprecrated  (0.0.0 means its NOT deprecrated)
    //! XXX: Introduce a deprecrated context - to tie a message with semver together.?
    struct semantic_version     kv_deprecrated;

    //! Default entry queue
    struct disir_default        *kv_default_queue;

    //! Queue of documentation entries
    struct disir_documentation  *kv_documentation_queue;

    //! Name of this keyval.
    struct disir_value          kv_name;

    //! Value held by this KEYVAL, given its root is CONFIG.
    //! The value type is infered from this structure
    struct disir_value          kv_value;

    //! Whether or not this keyval entry is disabled.
    uint32_t                    kv_disabled;
};

//! Construct a DISIR_CONTEXT_KEYVAL as a child of parent.
enum disir_status dx_keyval_begin (struct disir_context *parent, struct disir_context **doc);

//! Finalize the construction of a DISIR_CONTEXT_KEYVAL
enum disir_status dx_keyval_finalize (struct disir_context **doc);

//! Allocate a disir_keyval structure
struct disir_keyval *dx_keyval_create (struct disir_context *parent);

//! Destroy a disir_keyval structure, freeing all associated
//! memory and unhooking from linked list storage.
enum disir_status dx_keyval_destroy (struct disir_keyval **keyval);


#endif // _LIBDISIR_KEYVAL_H

