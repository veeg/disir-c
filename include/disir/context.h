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
    DISIR_CONTEXT_FREE_TEXT,

    //! Sentinel context - not a valid context.
    DISIR_CONTEXT_UNKNOWN, // Must be last entry in enumeration
};

#include <stdarg.h>
#include <stdint.h>
#include <disir/disir.h>
#include <disir/collection.h>
#include <disir/context/config.h>
#include <disir/context/restriction.h>
#include <disir/context/value.h>

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

//
// Documentation context API
//

//! \brief Add a documentation string to an entry.
//!
//! This will have the default introduced semver.
//! This is a shortcut between opening a new context,
//! adding value before finalizing it.
//!
//! Supported contexts:
//!     * KEYVAL
//!     * GROUP
//!     * CONFIG
//!     * MOLD
//!     * RESTRICTION
//!
//! \param context The input context to add documentation to.
//! \param doc The documentation string
//! \param doc_size The size of the `doc` string.
//!
//! \return DISIR_STATUS_WRONG_CONTEXT if an unsupported context is provided.
//! \return DISIR_STATUS_EXISTS  if a documentation entry already exists.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status dc_add_documentation (struct disir_context *context,
                                        const char *doc, int32_t doc_size);

//! \brief Get the documentation entry for a given semver on the context.
//!
//! Retrieve a specific documentation entry valid for the input semantic version number
//! given. If semver is NULL, the highest version is picked.
//! Supported context types are:
//!     * DISIR_CONTEXT_SECTION
//!     * DISIR_CONTEXT_KEYVAL
//!     * DISIR_CONTEXT_CONFIG
//!     * DISIR_CONTEXT_MOLD
//!     * DISIR_CONTEXT_RESTRICTION
//!
//! \param[in] context Input context to retrieve documentation for.
//! \param[in] semver Matching documentation entry covered by this semantic verison number.
//!     NULL indicates the greatest (semver) documentation entry.
//! \param[out] doc Output pointer the documentation constant will be populated to.
//! \param[out] doc_size Size of the documentation string, in bytes. Optional; May be NULL
//!
//! \return DISIR_STATUS_INVALID_ARGUMENTS if context or doc are NULL
//! \return DISIR_STATUS_WRONG_CONTEXT if context is not of supported type.
//! \return DISIR_STATUS_OK if doc is popualted with documentation string of context.
//!
enum disir_status dc_get_documentation (struct disir_context *context,
                                        struct semantic_version *semver,
                                        const char **doc, int32_t *doc_size);

//
// Add related context API
//

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

//! \brief Add an introduced semantic version number to a context.
//!
//! \return DISIR_STATUS_EXISTS is returned if an introduced entry already exists.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status dc_add_introduced (struct disir_context *context,
                                     struct semantic_version *semver);

//! \brief Add a deprecrated semantic version number to a context.
//!
//! Supported input contexts are:
//!     * DISIR_CONTEXT_KEYVAL
//!     * DISIR_CONTEXT_SECTION
//!     * DISIR_CONTEXT_DEFAULT
//!     * DISIR_CONTEXT_RESTRICTION
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT `context` and `semver` is NULL.
//! \return DISIR_STATUS_WRONG_CONTEXT if context is not of supported type.
//! \return DISIR_STATUS_WRONG_CONTEXT if top-level is not MOLD.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status dc_add_deprecated (struct disir_context *context,
                                     struct semantic_version *semver);

//! \brief  Add a default value to an entry, type inferred from the parent context
//!
//! Parse the input string to retrieve the value whose type is inferred from the
//! parent context. This function is merely a convenient wrapper around all
//! other dc_add_default_* functions.
//! The `value_size` parameter is only applicable for default entries whose
//! KEYVAL is STRING.
//!
//! \param context Parent context of which to add a new default entry
//! \param value String to parse the relevant value information from
//! \param value_size: Only applicable to string manipulating value types.
//!     Size in bytes of the input string to copy.A
//! \param semver Semantic version number to tag this default with. NULL indicates
//!     a semantic version number of 1.0.0
//!
//! \return DISIR_STATUS_OK if a new default object was associated with the parent context.t
enum disir_status dc_add_default (struct disir_context *context, const char *value,
                                  int32_t value_size, struct semantic_version *semver);

//! \brief Add a default string value to the parent context.
//!
//! Type of parent must be DISIR_VALUE_TYPE_STRING. There must not be a default context
//! in parent with an equal semantic version number.
//!
//! \param parent A DISIR_CONTEXT_KEYVAL context, whose toplevel context is a DISIR_CONTEXT_MOLD.
//! \param value Default string value to add.
//! \param value_size Size in bytes of the string to copy.
//! \param semver Semantic version number this default entry is valid from. NULL indicates
//!     the semantic verssion number 1.0.0
//!
//! \return DISIR_STATUS_OK if the default string 'value' entry succesfully added
//!     to the parent context.
//! \return DISIR_STATUS_CONFLICTING_SEMVER if there exists a default entry with equal 'semver'.
//!
enum disir_status dc_add_default_string (struct disir_context *parent, const char *value,
                                         int32_t value_size, struct semantic_version *semver);

//! \brief Add a default integer value to the parent context.
//!
//! Type of parent must be DISIR_VALUE_TYPE_INTEGER. There must not be a default context
//! in parent with an equal semantic version number.
//!
//! \param parent A DISIR_CONTEXT_KEYVAL context, whose toplevel context is a DISIR_CONTEXT_MOLD.
//! \param value Default integer value to add.
//! \param semver Semantic version number this default entry is valid from. NULL indicates
//!     the semantic verssion number 1.0.0
//!
//! \return DISIR_STATUS_OK if the default integer 'value' entry succesfully added
//!     to the parent context.
//! \return DISIR_STATUS_CONFLICTING_SEMVER if there exists a default entry with equal 'semver'.
//!
enum disir_status
dc_add_default_integer (struct disir_context *parent, int64_t value,
                        struct semantic_version *semver);

//! \brief Add a default float value to the parent context.
//!
//! Type of parent must be DISIR_VALUE_TYPE_FLOAT. There must not be a default context
//! in parent with an equal semantic version number.
//!
//! \param parent A DISIR_CONTEXT_KEYVAL context, whose toplevel context is a DISIR_CONTEXT_MOLD.
//! \param value Default float value to add.
//! \param semver Semantic version number this default entry is valid from. NULL indicates
//!     the semantic verssion number 1.0.0
//!
//! \return DISIR_STATUS_OK if the default float 'value' entry succesfully added
//!     to the parent context.
//! \return DISIR_STATUS_CONFLICTING_SEMVER if there exists a default entry with equal 'semver'.
//!
enum disir_status
dc_add_default_float (struct disir_context *parent, double value, struct semantic_version *semver);

//! \brief Add a default boolean value to the parent context.
//!
//! Type of parent must be DISIR_VALUE_TYPE_BOOLEAN. There must not be a default context
//! in parent with an equal semantic version number.
//!
//! \param parent A DISIR_CONTEXT_KEYVAL context, whose toplevel context is a DISIR_CONTEXT_MOLD.
//! \param booelan Default bool value to add.
//! \param semver Semantic version number this default entry is valid from. NULL indicates
//!     the semantic verssion number 1.0.0
//!
//! \return DISIR_STATUS_OK if the default 'boolean' entry succesfully added
//!     to the parent context.
//! \return DISIR_STATUS_CONFLICTING_SEMVER if there exists a default entry with equal 'semver'.
//!
enum disir_status
dc_add_default_boolean (struct disir_context *parent, uint8_t boolean,
                        struct semantic_version *semver);


//! \brief Return the default value as a string representation from context.
//!
//! Retrieve the default value of input context as a string representation.
//! The supported contexts are:
//!     * DISIR_CONTEXT_DEFAULT: Plainly retrieve the appointed to default value
//!     * DISIR_CONTEXT_KEYVAL: Search for the matching default entry in keyval.
//! The matching default version when searching KEYVAL is picked. If semver is NULL,
//! the highest version is chosen.
//!
//! The output buffer is populated with the string representation of the default value held
//! by context. If the output_buffer_size is inssuficient in size, the output_string_size
//! will be equal or greater than output_buffer_size, and the output buffer populated
//! with a output_buffer_size - 1 bytes of data.
//! The output buffer is always NULL terminated.
//! On success, the output_string_size will always hold the exact number of bytes populated
//! in the buffer, not counting the terminating NULL character.
//!
//! \return DISIR_STATUS_INVALID_ARGUENT if context or output are NULL, or output_buffer_size
//!     is less than or equal to zero.
//! \return DISIR_STATUS_WRONG_CONTEXT if context is not of type DISIR_CONTEXT_DEFAULT or
//!     DISIR_CONTEXT_KEYVAL
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_get_default (struct disir_context *context, struct semantic_version *semver,
                int32_t output_buffer_size,
                char *output, int32_t *output_string_size);


//! \brief Gather all default entries on the context into a collection.
//!
//! The supported context for this function is the DISIR_CONTEXT_KEYVAL,
//! whose root context must be a DISIR_CONTEXT_MOLD.
//!
//! \param[in] context Input KEYVAL context to retrieve all default contexts from.
//! \param[out] collection Output collection populated with default contexts of input context.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if any of the input arguments are NULL.
//! \return DISIR_STATUS_WRONG_CONTEXT if either the input context or root context are wrong.
//! \return DISIR_STATUS_NO_MEMORY if collection allocation failed.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status dc_get_default_contexts (struct disir_context *context,
                                           struct disir_collection **collection);

//! \brief Query the context for the introduced version it holds (if any)
//!
//! Supported input contexts are:
//!     * DISIR_CONTEXT_DEFAULT
//!     * DISIR_CONTEXT_DOCUMENTATION
//!     * DISIR_CONTEXT_SECTION
//!     * DISIR_CONTEXT_RESTRICTION
//!     * DISIR_CONTEXT_MOLD
//!
//! \param[in] context Input contect of supported type.
//! \param[out] semver Semantic version structure to populate with the output value on success.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if `context` or `semver` are NULL.
//! \return DISIR_STATUS_WRONG_CONTEXT if `context` is of unsupported type.
//! \return DISIR_STATUS_WRONG_CONTEXT if `context` whose top-level is not MOLD.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status dc_get_introduced (struct disir_context *context,
                                     struct semantic_version *semver);

//! \brief Query the context for the deprecated version it holds (if any)
//!
//! Supported input contexts are:
//!     * DISIR_CONTEXT_KEYVAL
//!     * DISIR_CONTEXT_SECTION
//!     * DISIR_CONTEXT_RESTRICTION
//!
//! \param[in] context Input contect of supported type.
//! \param[out] semver Semantic version structure to populate with the output value on success.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if `context` or `semver` are NULL.
//! \return DISIR_STATUS_WRONG_CONTEXT if `context` is of unsupported type.
//! \return DISIR_STATUS_WRONG_CONTEXT if `context` whose top-level is not MOLD.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status dc_get_deprecated (struct disir_context *context,
                                     struct semantic_version *semver);

//!  \brief Collect all direct child elements of the passed context.
//!
//! \param[in] context Parent context to collect child elements from.
//!     Must be of context type:
//!         * DISIR_CONTEXT_CONFIG
//!         * DISIR_CONTEXT_MOLD
//!         * DISIR_CONTEXT_SECTION
//! \param[out] collection Output collection, if return status is DISIR_STATUS_OK
//!
//! \return DISIR_STATUS_OK if the output collection contains all
//!     child elements of a valid input context.
//! \return DISIR_STATUS_INVALID_ARGUMENT if input parameters are NULL
//! \return DISRI_STATUS_WRONG_CONTEXT if the input context is not of correct type.
//!
enum disir_status dc_get_elements (struct disir_context *context,
                                   struct disir_collection **collection);

//! \brief Collect all children of the passed context matching name.
//!
//! \param[in] parent Parent context to collect child elements from.
//!     Must be of context type
//!         * DISIR_CONTEXT_CONFIG
//!         * DISIR_CONTEXT_MOLD
//!         * DISIR_CONTEXT_SECTION
//! \param[in] name Name of key to match wanted keyval contexts
//! \param[in] index Index of the keyval if multiple entries. Use 0 if only one entry.
//! \param[out] output Context to retrieve. Caller must use dc_putcontext when finished.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if input parameters are NULL
//! \return DISIR_STATUS_NOT_EXIST if the requested element/or index does not exist.
//! \return DISIR_STATUS_WRONG_CONTEXT if the input context is not of correct type.
//! \return DISIR_STATUS_OK if the output context is populated with the requested entry.
//!
enum disir_status
dc_find_element (struct disir_context *parent, const char *name, unsigned int index,
                 struct disir_context **output);

//! \brief Collect all children of the passed context matching name.
//!
//! \param[in] context Parent context to collect child elements from.
//!     Must be of context type
//!         * DISIR_CONTEXT_CONFIG
//!         * DISIR_CONTEXT_MOLD
//!         * DISIR_CONTEXT_SECTION
//! \param[in] name Name of key to match wanted keyval contexts
//!
//! \return DISIR_STATUS_OK if the output collection contains all
//!     child elements of a valid input context.
//! \return DISIR_STATUS_INVALID_ARGUMENT if input parameters are NULL
//! \return DISIR_STATUS_WRONG_CONTEXT if the input context is not of correct type.
//!
enum disir_status
dc_find_elements (struct disir_context *context, const char *name,
                  struct disir_collection **collection);

//! \brief Query for a context relative to parent
//!
//! \param[in] parent The context to query from.
//! \param[in] name Query format to resolve.
//! \param[out] out Output context to query for and return.
//! \param[in] ... varadic arguments used for name argument.
//!
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_query_resolve_context (struct disir_context *parent, const char *name,
                          struct disir_context **out, ...);

//! \see dc_query_resolve_context
//!
//! Varatic argument version of dc_query_resolve_context
enum disir_status
dc_query_resolve_context_va (struct disir_context *parent, const char *name,
                             struct disir_context **out, va_list args);

//
// KEYVAL related context API
//

//! \brief Shortcut to add a KEYVAL string entry to a parent context.
//!
//! Instead of beginning a context on a parent, setting the required name, type,
//! default, documentation fields, this function wraps all that logic into one
//! simple function call. If you require special restrictions and additional defaults,
//! you will need to go the long route through beginning and finalizing a context.
//!
//! \param[out] output Optional output context storage. If address is passed,
//!             the output KEYVAL context is populated and reference increased.
//!             Caller must take care to use dc_putcontext () when he is done.
//!
//! \return DISIR_STATUS_OK if the parent accepted the input keyval with 'name', at
//!     the input semver 'semver'.
//! \return DISIR_STATUS_INVALID_ARGUMENT if either 'name', 'def' or 'doc' are NULL.
//! \return DISIR_STATUS_WRONG_CONTEXT if input 'parent' is of wrong context type.
//!
enum disir_status dc_add_keyval_string (struct disir_context *parent,
                                        const char *name,
                                        const char *def,
                                        const char *doc,
                                        struct semantic_version *semver,
                                        struct disir_context **output);


//! \brief Shortcut to add a KEYVAL enum entry to a parent context.
//!
//! Instead of beginning a context on a parent, setting the required name, type,
//! default, documentation fields, this function wraps all that logic into one
//! simple function call. If you require special restrictions and additional defaults,
//! you will need to go the long route through beginning and finalizing a context.
//!
//! NOTE: The enum requires additional restrictions on it to mark valid enum values.
//! The context will be invalid after this call. One SHOULD catch the output context
//! and add the required restrictions
//!
//! \param[out] output Optional output context storage. If address is passed,
//!             the output KEYVAL context is populated and reference increased.
//!             Caller must take care to use dc_putcontext () when he is done.
//!
//! \return DISIR_STATUS_OK if the parent accepted the input keyval with 'name', at
//!     the input semver 'semver'.
//! \return DISIR_STATUS_INVALID_ARGUMENT if either 'name', 'def' or 'doc' are NULL.
//! \return DISIR_STATUS_WRONG_CONTEXT if input 'parent' is of wrong context type.
//!
enum disir_status
dc_add_keyval_enum (struct disir_context *parent, const char *name, const char *def,
                    const char *doc, struct semantic_version *semver,
                    struct disir_context **output);

//! \brief Shortcut to add a KEYVAL integer entry to a parent context.
//!
//! Instead of beginning a context on a parent, setting the required name, type,
//! default, documentation fields, this function wraps all that logic into one
//! simple function call. If you require special restrictions and additional defaults,
//! you will need to go the long route through beginning and finalizing a context.
//!
//! \param[out] output Optional output context storage. If address is passed,
//!             the output KEYVAL context is populated and reference increased.
//!             Caller must take care to use dc_putcontext () when he is done.
//!
//! \return DISIR_STATUS_OK if the parent accepted the input keyval with 'name', at
//!     the input semver 'semver'.
//! \return DISIR_STATUS_INVALID_ARGUMENT if either 'name', 'def' or 'doc' are NULL.
//! \return DISIR_STATUS_WRONG_CONTEXT if input 'parent' is of wrong context type.
//!
enum disir_status dc_add_keyval_integer (struct disir_context *parent,
                                         const char *name,
                                         int64_t def,
                                         const char *doc,
                                         struct semantic_version *semver,
                                         struct disir_context **output);

//! \brief Shortcut to add a KEYVAL float entry to a parent context.
//!
//! Instead of beginning a context on a parent, setting the required name, type,
//! default, documentation fields, this function wraps all that logic into one
//! simple function call. If you require special restrictions and additional defaults,
//! you will need to go the long route through beginning and finalizing a context.
//!
//! \param[out] output Optional output context storage. If address is passed,
//!             the output KEYVAL context is populated and reference increased.
//!             Caller must take care to use dc_putcontext () when he is done.
//!
//! \return DISIR_STATUS_OK if the parent accepted the input keyval with 'name', at
//!     the input semver 'semver'.
//! \return DISIR_STATUS_INVALID_ARGUMENT if either 'name', 'def' or 'doc' are NULL.
//! \return DISIR_STATUS_WRONG_CONTEXT if input 'parent' is of wrong context type.
//!
enum disir_status dc_add_keyval_float (struct disir_context *parent,
                                       const char *name,
                                       double def,
                                       const char *doc,
                                       struct semantic_version *semver,
                                       struct disir_context **output);

//! \brief Shortcut to add a KEYVAL boolean entry to a parent context.
//!
//! Instead of beginning a context on a parent, setting the required name, type,
//! default, documentation fields, this function wraps all that logic into one
//! simple function call. If you require special restrictions and additional defaults,
//! you will need to go the long route through beginning and finalizing a context.
//!
//! \param[out] output Optional output context storage. If address is passed,
//!             the output KEYVAL context is populated and reference increased.
//!             Caller must take care to use dc_putcontext () when he is done.
//!
//! \return DISIR_STATUS_OK if the parent accepted the input keyval with 'name', at
//!     the input semver 'semver'.
//! \return DISIR_STATUS_INVALID_ARGUMENT if either 'name', 'def' or 'doc' are NULL.
//! \return DISIR_STATUS_WRONG_CONTEXT if input 'parent' is of wrong context type.
//!
enum disir_status dc_add_keyval_boolean (struct disir_context *parent,
                                         const char *name,
                                         uint8_t def,
                                         const char *doc,
                                         struct semantic_version *semver,
                                         struct disir_context **output);

//! \brief Return a string representation of the restriction enumeration type.
const char * dc_restriction_enum_string (enum disir_restriction_type restriction);

//! \brief Return a string representation of the restriction type of the restriction context.
//!
//! If the input context is not of type DISIR_CONTEXT_RESTRICTION or
//! context is NULL, "INVALID" is returned.
//!
const char * dc_restriction_context_string (struct disir_context *context);

//! \brief Query the RESTRICTION context for its restriction type.
//!
//! Must be called on a context of type DISIR_CONTEXT_RESTRICTION.
//! Populate the `type` argument with the restriction type of the `context`.
//!
//! \param[in] context Input DISRI_CONTEXT_RESTRICTION context to query.
//! \param[out] type Output pointer to populate restriction_type with.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if `context` or `type` are NULL.
//! \return DISIR_STATUS_WRONG_CONTEXT if `context` is not of correct type.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_get_restriction_type (struct disir_context *context, enum disir_restriction_type *type);

//! \brief Set the restricction type of a restriction context.
//!
//! Input context must be DISIR_CONTEXT_RESTRICTION.
//! You may only set restriction type when the context is not yet finalized.
//!
//! Inclusive restrictions may be sat on a context belonging to the following parent context types:
//!     * DISIR_CONTEXT_KEYVAL.
//!     * DISIR_CONTEXT_SECTION.
//!
//! Exclusive restrictions may only be sat on a restriction context
//! belonging to parent context DISIR_CONTEXT_KEYVAL.
//! You cannot add exclusive restrictions who has
//! no value type is sat on the parent KEYVAL context.
//!
//! The following value type restrictions apply to parent KEYVAL value types:
//! DISIR_RESTRICTION_EXL_VALUE_ENUM:
//!     * DISIR_VALUE_TYPE_ENUM
//! DISIR_RESTRICTION_EXC_VALUE_RANGE:
//!     * DISIR_VALUE_TYPE_INTEGER
//!     * DISIR_VALUE_TYPE_FLOAT
//! DISIR_RESTRICTION_EXC_VALUE_NUMERIC:
//!     * DISIR_VALUE_TYPE_INTEGER
//!     * DISIR_VALUE_TYPE_FLOAT
//!
//! Attempting to set restriction type who violates the above rules results in
//! DISIR_STATUS_WRONG_CONTEXT return status.
//!
//! \param context DISIR_CONTEXT_RESTRICTION context to set restriction on.
//! \param type Type of restriction to assign.
//!
//! \return DISIR_STATUS_OK on success.
//! \return DISIR_STATUS_CONTEXT_IN_WRONG_STATE if context is finalized.
//! \return DISIR_STATUS_CONTEXT_WRONG_CONTEXT if the input parent context is of wrong type or
//!         the restriction type is not applicable to the input parent context.
//! return DISIR_STATUS_WRONG_VALUE_TYPE if the restriction type is not applicable to
//!         the value type of the input parent context.
//!
enum disir_status
dc_set_restriction_type (struct disir_context *context, enum disir_restriction_type type);

//! \brief Get a string `value` from input DISIR_CONTEXT_RESTRICTION `context`.
//!
//! Only applicable to `context` of type DISIR_CONTEXT_RESTRICTION.
//! Only applicable to context whose restriction type is one of:
//!     * DISIR_RESTRICTION_EXC_VALUE_ENUM
//!
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status dc_restriction_get_string (struct disir_context *context, const char **value);

//! \brief Set a string `value` to the input DISIR_CONTEXT_RESTRICTION `context`
//!
//! Only applicable to `context` of type DISIR_CONTEXT_RESTRICTION.
//! Only applicable to context whose restriction type is one of:
//!     * DISIR_RESTRICTION_EXC_VALUE_ENUM
//!
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status dc_restriction_set_string (struct disir_context *context, const char *value);

//! \brief Get the `min` and `max` values of input DISR_CONTEXT_RESTRICTION of type RANGE.
//!
//! Only applicable to `context` of type DISIR_CONTEXT_RESTRICTION.
//! Only applicable to context whose restriction type is one of:
//!     * DISIR_RESTRICTION_EXC_VALUE_RANGE
//!
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_restriction_get_range (struct disir_context *context, double *min, double *max);

//! \brief Set a range value to the input DISIR_CONTEXT_RESTRICTION `context`
//!
//! Only applicable to `context` of type DISIR_CONTEXT_RESTRICTION.
//! Only applicable to context whose restriction type is one of:
//!     * DISIR_RESTRICTION_EXC_VALUE_RANGE
//!
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status dc_restriction_set_range (struct disir_context *context,
                                            double min,
                                            double max);

//! \brief Get the numeric `value` stored on this DISIR_CONTEXT_RESTRICTION of type NUMERIC.
//!
//!
//! Only applicable to `context` of type DISIR_CONTEXT_RESTRICTION.
//! Only applicable to context whose restriction type is one of:
//!     * DISIR_RESTRICTION_EXC_VALUE_RANGE
//!
//! \return DISIR_STATUS_OK on success
//!
enum disir_status dc_restriction_get_numeric (struct disir_context *context, double *value);

//! \brief Set a numeric `value` to the input DISIR_CONTEXT_RESTRICTION `context`
//!
//! Only applicable to `context` of type DISIR_CONTEXT_RESTRICTION.
//! Only applicable to context whose restriction type is one of:
//!     * DISIR_RESTRICTION_EXC_VALUE_RANGE
//!     * DISIR_RESTRICTION_EXC_VALUE_NUMERIC
//!     * DISIR_RESTRICTION_INC_ENTRY_MIN
//!     * DISIR_RESTRICTION_INC_ENTRY_MAX
//!
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status dc_restriction_set_numeric (struct disir_context *context, double value);

//! \brief Add a DISIR_RESTRICTION_EXC_VALUE_NUMERIC restriction to parent.
//!
//! Only applicable to the following `parent` contexts whose top-level is MOLD:
//!     * DISIR_CONTET_KEYVAL
//
//! This is a shortcut  method instead of running through dc_begin, dc_set_restriction_type,
//! dc_restriction_set_numeric and dc_finalize, with optional dc_add_documenation and
//! dc_add_introduced.
//!
//! \param[out] output Optional output context storage. If address is passed,
//!             the output RESTRICTION context is populated and reference increased.
//!             Caller must take care to use dc_putcontext () when he is done.
//!
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_add_restriction_value_numeric (struct disir_context *parent, double value, const char *doc,
                                  struct semantic_version *semver,
                                  struct disir_context **output);

//! \brief Add a DISIR_RESTRICTION_EXC_VALUE_RANGE restriction to parent.
//!
//! Only applicable to the following `parent` contexts whose top-level is MOLD:
//!     * DISIR_CONTET_KEYVAL
//
//! This is a shortcut  method instead of running through dc_begin, dc_set_restriction_type,
//! dc_restriction_set_numeric and dc_finalize, with optional dc_add_documenation and
//! dc_add_introduced.
//!
//! \param[out] output Optional output context storage. If address is passed,
//!             the output RESTRICTION context is populated and reference increased.
//!             Caller must take care to use dc_putcontext () when he is done.
//!
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_add_restriction_value_range (struct disir_context *parent, double min, double max,
                                const char * doc, struct semantic_version *semver,
                                struct disir_context **output);

//! \brief Add a DISIR_RESTRICTION_EXC_VALUE_ENUM restriction to parent.
//!
//! Only applicable to the following `parent` contexts whose top-level is MOLD:
//!     * DISIR_CONTET_KEYVAL
//
//! This is a shortcut  method instead of running through dc_begin, dc_set_restriction_type,
//! dc_restriction_set_string and dc_finalize, with optional dc_add_documenation and
//! dc_add_introduced.
//!
//! \param[out] output Optional output context storage. If address is passed,
//!             the output RESTRICTION context is populated and reference increased.
//!             Caller must take care to use dc_putcontext () when he is done.
//!
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_add_restriction_value_enum (struct disir_context *parent, const char *value, const char *doc,
                               struct semantic_version *semver,
                               struct disir_context **output);

//! \brief Add a DISIR_RESTRICTION_INC_ENTRY_MIN restriction to parent.
//!
//! The input parent much have root context MOLD, and must be of type:
//!     * DISIR_CONTEXT_KEYVAL
//!     * DISIR_CONTEXT_SECTION
//!
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_add_restriction_entries_min (struct disir_context *parent, int64_t min,
                                struct semantic_version *semver);

//! \brief Add a DISIR_RESTRICTION_INC_ENTRY_MIN restriction to parent.
//!
//! The input parent much have root context MOLD, and must be of type:
//!     * DISIR_CONTEXT_KEYVAL
//!     * DISIR_CONTEXT_SECTION
//!
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_add_restriction_entries_max (struct disir_context *parent, int64_t max,
                                struct semantic_version *semver);

//! \brief Retrieve the version number of the input context.
//!
//! Supported contexts are:
//!     * DISIR_CONTEXT_CONFIG
//!     * DISIR_CONTEXT_MOLD
//!
//! \param[in] context To retrieve version from.
//! \param[out] semver Semantic version structure to populate the version of `context`.
//!
//! \return DISIR_STATUS_INVALID_ARUGMENT if `context` or `semver` are NULL.
//! \return DISIR_STATUS_WRONG_CONTEXT if `context` is not of supported type.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status dc_get_version (struct disir_context *context, struct semantic_version *semver);

//! \brief Set the version number of the input context
//!
//! Supported contexts are:
//!     * DISIR_CONTEXT_CONFIG
//!     * DISIR_CONTEXT_MOLD
//!
//! \param[in] context The context to set version on
//! \param[in] semver Semantic version structure to get version from
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if context or semver are NULL.
//! \return DISIR_STATUS_WRONG_CONTEXT if context is not of supported type.
//! \return DISIR_STATUS_CONFLICTING_SEMVER if semver is higher than mold semver when
//!     applied to a DISIR_CONTEXT_CONFIG context.
//! \return DISRI_STATUS_OK on success.
//!
enum disir_status dc_set_version (struct disir_context *context, struct semantic_version *semver);


//
// Config related context API
//

//! \brief Retrieve the context associated with an already constructed disir_config.
//!
//! This context may be used to manipulate or query the config object.
//!
//! \return context of type DISIR_CONTEXT_CONFIG
//!
struct disir_context * dc_config_getcontext (struct disir_config *config);

//! \brief Get the version number of this config.
//!
//! \param[in] config Input mold to retrieve semver for
//! \param[out] semver Output structure populated with the semver of config.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if config or semver are NULL
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_config_get_version (struct disir_config *config, struct semantic_version *semver);


//! \brief Begin construction of a CONFIG context based on the passed mold.
//!
//! \param[in] mold Input mold that this CONFIG object shall represent.
//! \param[out] config Output CONFIG context object.
//!
//! \return DISIR_STATUS_OK on success
//!
enum disir_status dc_config_begin (struct disir_mold *mold, struct disir_context **config);

//! \brief Finalize the construction of a DISIR_CONTEXT_CONFIG
//!
//! \param[in,out] context A CONFIG context to finalize. Will be sat to NULL on success.
//! \param[out] config The CONFIG object to populated on success.
//!
//! \return DISIRSTATUS_WRONG_CONTEXT if input context is not of type CONFIG.
//! \return DISIR_STATUS_OK on success
//!
enum disir_status dc_config_finalize (struct disir_context **context,
                                      struct disir_config **config);

//
// Schema related context API
//

//! Retrieve the context associated with an already constructed disir_mold.
//! This context may be used to manipulate or query the mold object.
struct disir_context * dc_mold_getcontext (struct disir_mold *mold);

//! \brief Get the version number of this mold.
//!
//! \param[in] mold Input mold to retrieve semver for
//! \param[out] semver Output structure populated with the semver of mold.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if mold or semver are NULL
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_mold_get_version (struct disir_mold *mold, struct semantic_version *semver);

//! Construct the DISIR_CONTEXT_MOLD
enum disir_status dc_mold_begin (struct disir_context **mold);

//! Finalize the construction of a DISIR_CONTEXT_MOLD, returning
//! an allocated disir_mold object in the output parameter.
//! If any unfinalized descendant contexts exists,
//! DISIR_STATUS_CONTEXT_IN_WRONG_STATE will be returned.
//! If the context supplied is not of type DISIR_CONTEXT_MOLD,
//! status DISIR_STATUS_WRONG_CONTEXT will be returned.
//! On success, DISIR_STATUS_OK is returned.
enum disir_status dc_mold_finalize (struct disir_context **context, struct disir_mold **mold);

//! \brief Construct a FREE_TEXT context to store a string
//!
//! Creates a free-standing context which holds a string value.
//! The context is freed by the first dc_putcontext() invoked on it.
//!
//! \param[in] text String value to store in the allocated context
//! \param[out] context Allocated context to store the string.
//!
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status dc_free_text_create (const char *text, struct disir_context **context);

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

