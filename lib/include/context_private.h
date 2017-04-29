#ifndef _LIBDISIR_PRIVATE_CONTEXT_H
#define _LIBDISIR_PRIVATE_CONTEXT_H

#include <disir/context.h>

//
// Definitions
//


//! The almighty disir_context
//! This is the unifying element between every object within libdisir
struct disir_context
{
    //! Type of object this context describes.
    enum disir_context_type     cx_type;

    //! States this context can be in.
    union {
        uint64_t        cx_state;
        struct {
            uint64_t    CONTEXT_STATE_CONSTRUCTING              : 1,
                        CONTEXT_STATE_FINALIZED                 : 2,
                        CONTEXT_STATE_INVALID                   : 3,
                        CONTEXT_STATE_FATAL                     : 4,
                        CONTEXT_STATE_DESTROYED                 : 5,
                        CONTEXT_STATE_IN_PARENT                 : 6,
                                                                : 0;
        };
    };

    //! The actual object pointed to by this context.
    union
    {
        struct disir_config         *cx_config;
        struct disir_default        *cx_default;
        struct disir_section        *cx_section;
        struct disir_mold           *cx_mold;
        struct disir_group          *cx_group;
        struct disir_keyval         *cx_keyval;
        struct disir_documentation  *cx_documentation;
        struct disir_value          *cx_value;
        struct disir_restriction    *cx_restriction;
    };

    //! Parent context of this context.
    struct disir_context                        *cx_parent_context;

    //! Root context to this context.
    //! A root context may only be one of:
    //!     * DISIR_CONTEXT_CONFIG
    //!     * DISIR_CONTEXT_MOLD
    struct disir_context                        *cx_root_context;


    //! Reference count on how many context pointers the user
    //! is in possession of.
    int64_t                     cx_refcount;

    //! Allocated and populated if an error message occurs.
    //! Should probably be a stack of messages, with a counter.
    //! and a state counter!
    char                        *cx_error_message;
    int32_t                     cx_error_message_size;
};

//
// Macro prototypes
//

//! Check if the passed context is of the same type
//! as any of the passed DISIR_CONTEXT_* in the variadic list.
//! Logs an error to the context and returns DISIR_STATUS_WRONG_CONTEXT
//! upon mismatching types.
#define CONTEXT_TYPE_CHECK(context, ...) \
    dx_context_type_check_log_error (context, __VA_ARGS__, 0)

//! Check if the passed disir_context pointer is non-NULL,
//! is not of type DISIR_CONTEXT_UNKNOWN and its state is not INVALID.
#define CONTEXT_NULL_INVALID_TYPE_CHECK(context) \
    dx_context_sp_full_check_log_error (context, __FUNCTION__)

//! \see CONTEXT_NULL_INVALID_TYPE_CHECK
//! Input a disir_context double pointers instead.
#define CONTEXT_DOUBLE_NULL_INVALID_TYPE_CHECK(context) \
    dx_context_dp_full_check_log_error (context, __FUNCTION__)


//
// Utility context prototypes
//

//! \brief Increment the reference count for the passed context.
//!
//!
void dx_context_incref (struct disir_context *context);

//! \brief Decrement the reference count for the passed context.
//!
//! If the reference count reaches zero, the context is de-allocated.
//! Nothing is done for object which is pointed to by this context.
//! It is the callers responsibility to make sure it is properly free'd
//!
//! If the context ends up destroyed, since its reference count reaches zero,
//! the context pointer is sat to NULL.
//!
void dx_context_decref (struct disir_context **context);

//! Attach 'parent' as parent context to 'context'
void dx_context_attach (struct disir_context *parent, struct disir_context *context);

//! Return the string representation of the disir_context_type enumeration
const char * dx_context_type_string (enum disir_context_type type);

//! Return a string description of the passed capability.
//! \return string description of passed capability
//!     if 'capability' is unknown, a standard 'unknown capability' string
//!     is returned.
const char *dx_context_capability_string (uint64_t capability);

//! Return 1 if the passed disir_context type is a Top-Level context type
//! \return 1 if 'type' is an enum value representing a top-level context.
//! \return 0 if 'type' is not an enum value representing a top-level context.
uint32_t dx_context_type_is_toplevel (enum disir_context_type type);

//! Sanify the passed enum value, making sure its within DISIR_CONTEXT_TYPE
//! bounds. If its out of bounds, DISIR_CONTEXT_UNKNOWN is returned.
//! \return DISIR_CONTEXT_UNKNOWN is returned if input type is out-of-bounds
//! \return 'type' if within bounds.
enum disir_context_type dx_context_type_sanify (enum disir_context_type type);

//! Allocate a disir_context
struct disir_context * dx_context_create (enum disir_context_type type);

//! Free an allocated disir_context
void dx_context_destroy (struct disir_context **context);

//! \brief Associate the input config related context with its equiv mold related context
//!
//! The input context must have root CONFIG context, where valid contexts are:
//!     * DISIR_CONTEXT_KEYVAL
//!     * DISIR_CONTEXT_SECTION
//!
//! \param context The input context whose root is CONFIG
//! \param value The value used to locate the mold equivalent
//! \param value_size Bytes inputted in value
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if context or value are NULL, if value_size is
//!     less or equal to zero, or if the input value is not associated in the mold.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status dx_set_mold_equiv (struct disir_context *context,
                                     const char *value, int32_t value_size);

//! \brief Identify context type of a child context matching name
//!
//! The input context must have root CONFIG context, where valid contexts are:
//!     * DISIR_CONTEXT_KEYVAL
//!     * DISIR_CONTEXT_SECTION
//!
//! \param context The input context whose root is CONFIG
//! \param name The value used to locate the mathcing child mold equivalent
//! \param type The given context type of the queried context
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if context or value are NULL, if value_size is
//!     less or equal to zero, or if the input value is not associated in the mold.
//! \return DISIR_STATUS_NOT_EXIST if a mold equivalent does not exist
//! \return DISIR_STATUS_WRONG_CONTEXT if either root context is not CONFIG
//! or parent context is neither section nor config.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status dx_get_mold_equiv_type (struct disir_context *parent,
                                          const char *name, enum disir_context_type *type);

//! \brief Validate the input context for any erroneous state
//!
//! TODO: Implement error report.
//!
//! This will clear the CONTEXT_STATE_INVALID state and re-calculate if
//! the context is deemed invalid. If it contains any child contexts, they will
//! also be recursively invoked.
//!
//! If the parent context is constructing, any invalid indicating error will result in
//! status DISIR_STATUS_INVALID_CONTEXT instead of its descriptive status.
//! (Reasoning: to ease implementation of code using this method. The return code of
//! any method finaliznig or checking constructing elements shall be allowed, and
//! the DISIR_STATUS_INVALID_CONTEXT shall indicate that the state was persistet, albeit
//! not valid.)
//!
//! \return DISIR_STATUS_MOLD_MISSING if the input context is not
//!         affiliated with a mold equivalent.
//! \return DISIR_STATUS_WRONG_VALUE_TYPE if the wrong value type is assigned to keyval.
//! \return DISIR_STATUS_RESTRICTION_VIOLATED if the context does not fulfill its restriction
//!         requirements.
//! \return DISIR_STATUS_INVALID_CONTEXT if this context is considered invalid.
//!         If invoked on a top-level context, an updated error report is available.
//! \return DISIR_STATUS_ELEMENTS_INVALID if any of its children are deemed invalid.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status dx_validate_context (struct disir_context *context);

//! \brief Set the input message to the error string of context
void
dx_context_error_set (struct disir_context *context, const char *fmt_message, ...);
//! \brief Set the input message to the error string of context from va_list args
void
dx_context_error_set_va (struct disir_context *context, const char *fmt_message, va_list args);


//! Check that the passed context is either of the passed
//! DISIR_CONTEXT_* following the variadic list.
//! This function requires a trailing sentinel value of
//! 0. Use CONTEXT_TYPE_CHECK() to automatically append this.
//! If this function is called without the trailinng zero,
//! unknown and horrible things may happen.
//! An error message is populated in the context upon
//! an error status.
//!
//! \return DISIR_STATUS_INVALID_CONTEXT is returned if the input_iterator
//!     context is of type DISIR_CONTEXT_UNKNOWN.
//! \return DISIR_STATYS_TOO_FEW_ARGUMENTS is returned if no entries
//!     exists in the variadic argument list.
//! \return DISIR_STATUS_WRONG_CONTEXT is returned if the input context
//!     is not in the set of variadic DISIR_CONTEXT_* arguments.
//! \return DISIR_STATUS_INSUFFICIENT_RESOURCES is returned if internal
//!     buffer cannot fit any message.
//! \return DISIR_STATUS_INTERNAL_ERROR is returned upon encoding error.
//! \return DISIR_STATUS_OK is returned when passed context exists in set
//!     of variadic arguments of DISIR_CONTEXT_*
enum disir_status dx_context_type_check_log_error (struct disir_context *context, ...);

//! Check if the passed context single pointer is INVALID.
//! \return DISIR_STATUS_INVALID_ARGUMENT if null pointer is supplied.
//! \return DISIR_STATUS_INVALID_CONTEXT if the passed context is INVALID.
//! \return DISIR_STATUS_OK if everything is great!
enum disir_status dx_context_sp_full_check_log_error (struct disir_context *context,
                                                      const char *function_name);

//! Check if the passed context double pointer is INVALID.
//! \return DISIR_STATUS_INVALID_ARGUMENT if null pointer is supplied.
//! \return DISIR_STATUS_INVALID_CONTEXT if the passed context is INVALID.
//! \return DISIR_STATUS_OK if everything is great!
enum disir_status dx_context_dp_full_check_log_error (struct disir_context **context,
                                                      const char *function_name);

// Transfer the logwarn entry from source to destination
void dx_context_transfer_logwarn (struct disir_context *destination, struct disir_context *source);

//! \brief Retrieve a name for this context.
//!
//! If the context is of type KEYVAL or SECTION, return the actual name given.
//! Otherwise return the string representation of its context type.
const char *dx_context_name (struct disir_context *context);


#endif // _LIBDISIR_PRIVATE_CONTEXT_H

