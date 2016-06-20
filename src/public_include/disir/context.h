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

#include <stdint.h>
#include <disir/disir.h>
#include <disir/collection.h>

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

//! \brief Get the value type enumeration represented by the passed context.
//!
//! Retrieve the disir_value_type embedded in the passed context.
//! If the passed context is NULL, or does not contain a value,
//! DISIR_VALUE_TYPE_UNKNOWN is returned.
//!
//! \return DISIR_VALUE_TYPE_UNKNOWN if the passed context is NULL or the context
//!     does not contain a value type. Otherwise return the value type of the context.
//! \return disir_value_type represented by context if context is valid and contains
//!     a valid disir_value_type
//!
enum disir_value_type dc_value_type (struct disir_context *context);


//! \brief Return a string representation of the value this context represents.
//!
//! Retrieve the disir_value_type embedded in the passed context, and
//! return a string representation of this enumeration.
//! If the passed context is NULL or does not contain a value, UNKNOWN is returned.
//!
//! \param context Input context to infere value from.
//!
//! \return string representation of the disir_value_type held in context
//!
const char * dc_value_type_string (struct disir_context *context);


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
//! \return DISIR_STATUS_WRONG_CONTEXT if the name attempted to add on a context whose root
//!     is CONFIG, is not found in the associated MOLD.
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

//! \brief Add an introduced semantic version number to a context.
//!
//! \return DISIR_STATUS_EXISTS is returned if an introduced entry already exists.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status dc_add_introduced (struct disir_context *context,
                                     struct semantic_version semver);

//! \brief Add a deprecrated semantic version number to a context.
//!
//! TODO: Implement me
//!
//! \return DISIR_STATUS_EXISTS is returned if an deprecrated entry already exists.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status dc_add_deprecrated (struct disir_context *context,
                                      struct semantic_version smever);


//! \brief Set the value type associated with input context
//!
//! Set the value type of the input context.
//! The available input contexts are as follows:
//!     * DISIR_CONTEXT_KEYVAL
//!
//! There are a number of restrictions on when you can set a value type on a context.
//! DISIR_CONTEXT_KEYVAL: Cannot have any default entries on it.
//!     Can only set type if the toplevel context is DISIR_CONTEXT_MOLD
//!
//! \return DISIR_STATUS_OK the input `context` was succesfuly populated with value type `type`
//! \return DISIR_STATUS_WRONG_CONTEXT if `context`'s toplevel context is CONFIG.
//! \return DISIR_STATUS_INVALID_ARGUMENT if `context` is NULL or `type` is out-of-bounds
//!
enum disir_status dc_set_value_type (struct disir_context *context, enum disir_value_type type);

//! \brief Retrieve the value type stored in the input context
//!
//! Retrieve the value type of the input context, populated in the output argument 'type'
//!
//! \param[in] context The input context object to return the value type of.
//! \param[out] type Output value populated with the type found in context.
//!     DISIR_VALUE_TYPE_UNKNOWN is populated if any errors on the input context are found.
//!
//! \return DISIR_STATUS_OK if the context contatins a value type
//! \return DISIR_STATUS_WRONG_CONTEXT if it does not contain a value type.
//! \return DISIR_STATUS_INVALID_ARGUMENT if either context or type are NULL pointer.
//!
enum disir_status dc_get_value_type (struct disir_context *context, enum disir_value_type *type);

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


//! \brief Set a value to the context. Type is extracted from string where applicable
//!
//! TODO: implement me
//!
//! \return DISIR_STATUS_INTERNAL_ERROR - Not Implemeneted
//!
enum disir_status dc_set_value (struct disir_context *context,
                                const char *value, int32_t value_size);

//! \brief Set a string value to the context.
//!
//! Applicable contexts are:
//!     * DISIR_CONTEXT_DOCUMENTATION
//!     * DISIR_CONTEXT_KEYVAL whose root is CONFIG
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if context or value are NULL,
//!     or if value_size is less or equal to zero.
//! \return DISIR_STATUS_WRONG_CONTEXT if root context is not CONFIG.
//! \return DISIR_STATUS_WRONG_VALUE_TYPE if value type in context is not string.
//! \return DISIR_STATUS_INVALID_CONTEXT if the entry does not have a mold equivalent.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status dc_set_value_string (struct disir_context *context,
                                       const char *value, int32_t value_size);

//! \brief Set a integer value to the context.
//!
//! Applicable contexts are:
//!     * DISIR_CONTEXT_KEYVAL whose top-level is CONFIG
//!     * DISIR_CONTEXT_DEFAULT
//!
//! \return DISIR_STATUS_WRONG_CONTEXT if input context is not among the applicable contexts.
//! \return DISIR_STATUS_WRONG_VALUE_TYPE if value type in context is not string.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status dc_set_value_integer (struct disir_context *context, int64_t value);

//! \brief Set a float value to the context.
//!
//! Applicable contexts are:
//!     * DISIR_CONTEXT_KEYVAL whose top-level is CONFIG
//!     * DISIR_CONTEXT_DEFAULT
//!
//! \return DISIR_STATUS_WRONG_CONTEXT if input context is not among the applicable contexts.
//! \return DISIR_STATUS_WRONG_VALUE_TYPE if value type in context is not string.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status dc_set_value_float (struct disir_context *context, double value);

//! \brief Set a boolean value to the context.
//!
//! Applicable contexts are:
//!     * DISIR_CONTEXT_KEYVAL whose top-level is CONFIG
//!     * DISIR_CONTEXT_DEFAULT
//!
//! \return DISIR_STATUS_WRONG_CONTEXT if input context is not among the applicable contexts.
//! \return DISIR_STATUS_WRONG_VALUE_TYPE if value type in context is not string.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status dc_set_value_boolean (struct disir_context *context, uint8_t value);


//! \brief Get the value stored in context in a string representation
//!
//! The value, regardless of type, is stringified in the output buffer provided.
//! if the output_buffer_size is insufficient to hold the full value of context,
//! then only output_buffer_size - 1 bytes are copied to output and the output_size
//! will be equal or greater than output_buffer_size.
//! A terminating NULL character is always added to the end of the output buffer,
//! but is not part of the outout_size count returned
//!
//! Supported contexts are:
//!     * DISIR_CONTEXT_KEYVAL whose root is CONFIG
//!
//! \param[in] context Input context to query for value
//! \param[in] output_buffer_size Size of the output buffer. Cannot be <= 0
//! \param[in] output Buffer with at least output_buffer_size capacity.
//! \param[out] output_size Size in bytes of the stringified value of context,
//!     not including the terminating NULL character.
//!
//! \return DISIR_STATUS_WRONG_CONTEXT if an unsupported context is supplied.
//! \return DISIR_STATUS_WRONG_CONTEXT if KEYVALs context is not CONFIG.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status dc_get_value (struct disir_context *context, int32_t output_buffer_size,
                                char *output, int32_t *output_size);

//! \brief Retrieve the string value stored on the context.
//!
//! Only applicable on the following contexts:
//! * DISIR_CONTEXT_KEYVAL whose root is CONFIG
//! * DISIR_CONTEXT_DOCUMENTATION
//!
//! \param context Input context to retrieve string value from.
//! \param[out] output Pointer to redirect the string value stored in context to.
//! \param[out] size Optional. Populated with the size of the output string.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if context or output are NULL
//! \return DISIR_STATUS_WRONG_VALUE_TYPE if value type in context is not string.
//! \return DISIR_STATUS_WRONG_CONTEXT if the context is of wrong type.
//! \return DISIR_STATUS_WRONG_CONTEXT if KEYVALs root context is not CONFIG.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status dc_get_value_string (struct disir_context *context,
                                       const char **output, int32_t *size);

//! \brief Retrieve the integer value stored on the context.
//!
//! Supported contexts:
//!     * DISIR_CONTEXT_KEYVAL whose top-level is CONFIG.
//!
//! \param[in] context The context to get integer from.
//! \param[out] value The output variable to populate.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if `context` or `value` is NULL.
//! \return DISIR_STATUS_MOLD_MIDDING if MOLD is not associated with `context`.
//! \return DISIR_STATUS_WRONG_CONTEXT if `context` is not of supported type.
//! \return DISIR_STATUS_WRONG_VALUE_TYPE if value type in context is not DISIR_VALUE_TYPE_INTEGER.
//! \return DISIR_STATUS_OK on success
//!
enum disir_status dc_get_value_integer (struct disir_context *context, int64_t *value);

//! \brief Retrieve the float value stored on the context.
//!
//! Supported contexts:
//!     * DISIR_CONTEXT_KEYVAL whose top-level is CONFIG.
//!
//! \param[in] context The context to get float from.
//! \param[out] value The output variable to populate.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if `context` or `value` is NULL.
//! \return DISIR_STATUS_MOLD_MIDDING if MOLD is not associated with `context`.
//! \return DISIR_STATUS_WRONG_CONTEXT if `context` is not of supported type.
//! \return DISIR_STATUS_WRONG_VALUE_TYPE if value type in context is not DISIR_VALUE_TYPE_FLOAT.
//! \return DISIR_STATUS_OK on success
//!
enum disir_status dc_get_value_float (struct disir_context *context, double *value);

//! \brief Retrieve the boolean value stored on the context.
//!
//! Supported contexts:
//!     * DISIR_CONTEXT_KEYVAL whose top-level is CONFIG.
//!
//! \param[in] context The context to get boolean from.
//! \param[out] value The output variable to populate.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if `context` or `value` is NULL.
//! \return DISIR_STATUS_MOLD_MIDDING if MOLD is not associated with `context`.
//! \return DISIR_STATUS_WRONG_CONTEXT if `context` is not of supported type.
//! \return DISIR_STATUS_WRONG_VALUE_TYPE if value type in context is not DISIR_VALUE_TYPE_BOOLEAN.
//! \return DISIR_STATUS_OK on success
//!
enum disir_status dc_get_value_boolean (struct disir_context *context, uint8_t *value);

//! Query the context for its intrduced member.
//! If no such member exists on the context,
//! DISIR_STATUS_NO_CAN_DO is returned.
enum disir_status dc_get_introduced (struct disir_context *context,
                                     struct semantic_version *semver);

//! Query the context for its deprecrated member.
//! If no such member exists on the context,
//! DISIR_STATUS_NO_CAN_DO is returned.
enum disir_status dc_get_deprecrated (struct disir_context *context,
                                      struct semantic_version *semver);

//!  \brief Collect all direct child elements of the passed context.
//!
//! \param[in] context Parent context to collect child elements from.
//!     Must be of context ype:
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
//! \return DISIR_STATUS_OK if the parent accepted the input keyval with 'name', at
//!     the input semver 'semver'.
//! \return DISIR_STATUS_INVALID_ARGUMENT if either 'name', 'def' or 'doc' are NULL.
//! \return DISIR_STATUS_WRONG_CONTEXT if input 'parent' is of wrong context type.
//!
enum disir_status dc_add_keyval_string (struct disir_context *parent,
                                        const char *name,
                                        const char *def,
                                        const char *doc,
                                        struct semantic_version *semver);

//! \brief Shortcut to add a KEYVAL integer entry to a parent context.
//!
//! Instead of beginning a context on a parent, setting the required name, type,
//! default, documentation fields, this function wraps all that logic into one
//! simple function call. If you require special restrictions and additional defaults,
//! you will need to go the long route through beginning and finalizing a context.
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
                                         struct semantic_version *semver);

//! \brief Shortcut to add a KEYVAL float entry to a parent context.
//!
//! Instead of beginning a context on a parent, setting the required name, type,
//! default, documentation fields, this function wraps all that logic into one
//! simple function call. If you require special restrictions and additional defaults,
//! you will need to go the long route through beginning and finalizing a context.
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
                                       struct semantic_version *semver);

//! \brief Shortcut to add a KEYVAL boolean entry to a parent context.
//!
//! Instead of beginning a context on a parent, setting the required name, type,
//! default, documentation fields, this function wraps all that logic into one
//! simple function call. If you require special restrictions and additional defaults,
//! you will need to go the long route through beginning and finalizing a context.
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

#ifdef __cplusplus
}
#endif // _cplusplus

#endif // _LIBDISIR_CONTEXT_H

