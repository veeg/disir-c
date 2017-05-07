#ifndef _LIBDISIR_CONTEXT_H
#define _LIBDISIR_CONTEXT_H

#ifdef __cplusplus
extern "C"{
#endif // _cplusplus


//! The namespace employed for all low-level context operations
//! is dc_*

//! The disir_context is the main interface used to manipulate or
//! query various components within disir programatically.
//! It offers a detailed level of abstraction.
//! Each context is structured heirarchly, with the root
//! context being either DISIR_CONTEXT_CONFIG, DISIR_CONTEXT_MOLD
//! The root context will determine the
//! effect and allowed operations on child contexts.
struct disir_context;

//! Different types of disir_contexts that are available.
enum disir_context_type
{
    //! Top-level context - product of a MOLD
    DISIR_CONTEXT_CONFIG = 1,
    //! Top-level context - describes a CONFIG
    DISIR_CONTEXT_MOLD,
    DISIR_CONTEXT_SECTION,
    DISIR_CONTEXT_KEYVAL,
    DISIR_CONTEXT_DOCUMENTATION,
    DISIR_CONTEXT_DEFAULT,
    DISIR_CONTEXT_RESTRICTION,

    //! Sentinel context - not a valid context.
    DISIR_CONTEXT_UNKNOWN, // Must be last entry in enumeration
};

#include <stdarg.h>
#include <stdint.h>
#include <disir/disir.h>
#include <disir/collection.h>
#include <disir/context/config.h>
#include <disir/context/convenience.h>
#include <disir/context/mold.h>
#include <disir/context/restriction.h>
#include <disir/context/value.h>
#include <disir/context/version.h>
#include <disir/context/query.h>

//
// Utility context API
//

//! Return the disir_context_type associated with the passed
//! DISIR_CONTEXT.
enum disir_context_type dc_context_type (struct disir_context *context);

//! Return a string representation of the passed context.
//! type_string_size is populated with the size in octets for
//! the returned string. If this pointer is NULL, this output
//! parameter is ignored.
//! If context is NULL, the returned string is equal to
//! that if the input context were unknown.
const char * dc_context_type_string (struct disir_context *context);


//! \brief Return the error message on input context.
//!
//! \return NULL if no error message is associated with input context
//! \return const char pointer to error message.
//!
const char *dc_context_error (struct disir_context *context);

//
// Base context API
//

//! \brief Start the construction of a new context as a child of parent.
//!
//! Top-level contexts cannot be added as children of other contexts.
//! No state is altered in the parent until dc_finalize() is called.
//!
//! \param parent is the parent context to which a child shall be added.
//! \param context_type The type of context for the child.
//! \param child Output pointer is populated with the allocated context.
//!
//! \return DISIR_STATUS_WRONG_CONTEXT if an unsupported context type is submitted.
//! \return DISIR_STATUS_OK when everything is OK!
//!
enum disir_status dc_begin (struct disir_context *parent, enum disir_context_type context_type,
                            struct disir_context **child);

//! \brief Destroy the object pointed to by this context.
//!
//! This will delete all children objects. If any of the contexts, children or this one,
//! is referenced by any other pointers, the context will turn INVALID
//! and every operation attempted will result in a DISIR_STATUS_INVALID_CONTEXT status code.
//! If a context pointer is INVALID, dc_destroy() must be invoked on it to
//! decrement the reference count.
//!
//! NOTE: For DISIR_CONTEXT_MOLD and DISIR_CONTEXT_CONFIG,
//! if you have already finalized them and retrieved a context object to query with
//! (through dc_*_getcontext()), you CANNOT pass this context to dc_destroy().
//! This will lead to invalid memory access when accessing the config/mold structure
//! since you are effectivly free'ing this structure through this call.
//! Simply dc_putcontext() instead and call the appropriate disir_*_finished() instead,
//!
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status dc_destroy (struct disir_context **context);

//! \brief Submit the context to the parent.
//!
//! If any invalid state or missing elements that are required is not present
//! in the context, an appropriate status code is returned.
//! Upon success, the input context pointer is set to NULL.
//!
//! The return code depends on the state of the parent. If there is any problem with the input
//! context, but enough state exists to submit an invalid context to parent, it will still
//! be submitted to a parent who is still constructing (not finalized.)
//! The return code will then be INVALID_CONTEXT.
//! When the parent is finalized, such invalid context entries
//! will be rejected and the appropriate status code is returned.
//!
//! \return DISIR_STATUS_OK on success, context pointer is invalidated and set to NULL.
//!
enum disir_status dc_finalize (struct disir_context **context);

//! \brief Put away a context obtained while querying a parent context.
//!
//! Contexts, who have yet to be finalized and are made available through any
//! querying interface, are referenced counted when made available.
//! To balance the reference count, the caller is required to put the context back
//! to the disir library after he is finished operating on it.
//!
//! \return DISIR_STATUS_CONTEXT_IN_WRONG_STATE if context is not in constructing mode
//! \return DISIR_STATUS_OK when successful. Passed context pointer is set tp NULL.
//!
enum disir_status dc_putcontext (struct disir_context **context);

//! \brief Query the context whether or not it is valid
//!
//! \return DISIR_STATUS_OK if valid
//! \return DISIR_STATUS_INVALID_CONTEXT if invalid
//! \return DISIR_STATUS_INVALID_ARGUMENT if input is NULL
//!
enum disir_status dc_context_valid (struct disir_context *context);

//! \brief Add a name to a context entry.
//!
//! This is required on supported contexts:
//!     * DISIR_CONTEXT_KEYVAL
//!     * DISIR_CONTEXT_SECTION
//!
//! When adding a name to a context whose root context is CONFIG,
//! the name must match a KEYVAL entry found in the associated MOLD to CONFIG.
//! If no such association is found, DISIR_STATUS_WRONG_CONTEXT is returned.
//!
//! \param context Context to set the name attribute on.
//! \param name The input name to associate with the context.
//! \param name_size Size in bytes of the input `name`. Does not include null terminator.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if name or name_size are zero
//! \return DISIR_STATUS_NO_CAN_DO if an unsupported context type
//! \return DISIR_STATUS_NOT_EXIST if the input context' root is CONFIG, and there are no
//!     mold equivalent entry for name in the parent context mold.
//!     May also be returned if the parent of context is also missing a mold equivalent.
//! \return DISIR_STATUS_WRONG_CONTEXT if the located context' mold entry by name is not
//!     the same context type as the input context.
//! \return DISIR_STATUS_OK on successful insertion of name to context.
//!
enum disir_status dc_set_name (struct disir_context *context, const char *name, int32_t name_size);

//! \brief Get a name attribute associated with the context entry
//!
//! One can only retrieve a name from one of these supported contexts:
//!     * DISIR_CONTEXT_KEYVAL
//!     * DISIR_CONTEXT_SECTION
//!
//! \param[in] context Context to get name attribute from
//! \param[out] name Pointer will be re-directed to the `name` constant pointer.
//! \param[out] name_size Address is populated wth the output `name` size in bytes. May be NULL.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if context or name are NULL pointers.
//! \return DISIR_STATUS_WRONG_CONTEXT if input context is not of the supported types.
//! \return DISIR_STATUS_OK if name is successfully populated with the name attribute of context.
//!
enum disir_status dc_get_name (struct disir_context *context,
                               const char **name, int32_t *name_size);

//! \brief Rescursively resolve the name from the context to the root.
//!
//! This allocates a buffer that is populated in the output parameter.
//!
//! \param[in] context The context to resolve.
//! \param[out] output Allocated buffer with the resolved name.
//!
//! \return DISIR_STATUS_NO_MEMORY on allocation failure.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_resolve_root_name (struct disir_context *context, char **output);

// TODO: Find a proper place for this structure.
//! XXX: Very simple container. Only a list of allocated strings.
struct disir_diff_report
{
    int     dr_entries;
    char    **dr_diff_string;

    int     dr_internal_allocated;
};

//! \brief Compare two context objects for equality.
//!
//! \param[in] lhs First context argument.
//! \param[in] rhs Second context argument.
//! \param[out] report Optional difference report.
//!
//! NOTE: Only implemented for CONFIG toplevel contexts
//!
//! \return DISIR_STATUS_CONFLICT when objects differ.
//! \return DISIR_STATUS_NO_MEMORY on memory allocation failure.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_compare (struct disir_context *lhs, struct disir_context *rhs,
            struct disir_diff_report **report);

//! \brief Mark the context as fatally invalid with an associated error message.
//!
//! The context must be in constructing state.
//! If invoked multiple times on the same context, the message is simply
//! overwritten.
//!
//! \param[in] context context to mark as fatally invalid
//! \param[in] msg Error message to set on the context.
//! \param[in] ... Varadic arguments
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if context or msg is NULL.
//! \return DISIR_STATUS_CONTEXT_IN_WRONG_STATE if context is finalized.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_fatal_error (struct disir_context *context, const char *msg, ...);

//! \see dc_fatal_error
//! Varadic argument list version of dc_fatal_error()
//!
enum disir_status
dc_fatal_error_va (struct disir_context *context, const char *msg, va_list args);


#ifdef __cplusplus
}
#endif // _cplusplus

#endif // _LIBDISIR_CONTEXT_H

